#include "cat_writer.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <cmath>
#include <cstdint>

// Clamp counters (reset per write_cat call) to report quantization losses
static std::atomic<uint64_t> g_clamp_vmag_lo{0};   // V < 16.0 in Star3
static std::atomic<uint64_t> g_clamp_vmag_hi{0};   // V > 21.1 in Star3
static std::atomic<uint64_t> g_clamp_bv{0};        // B-V out of [-1.0, 5.35] in Star3

static inline void store_u24le(uint8_t out[3], uint32_t v)
{
	out[0] = static_cast<uint8_t>(v);
	out[1] = static_cast<uint8_t>(v >> 8);
	out[2] = static_cast<uint8_t>(v >> 16);
}

// Convert a BucketRecord to Star1 on-disk form.
// Position becomes a 3D unit vector × 2e9; proper motion becomes the 3D vector
// in the local triad (p = east, q = north) in uas/yr. Bucket pmra is stored
// without the cos(dec) factor, so it is restored here before projection.
static CatRecord1 to_cat_record1(const BucketRecord& r)
{
	CatRecord1 cr{};
	cr.gaia_id = r.gaia_id;

	const double ra   = (r.ra_i  / 3600000.0) * M_PI / 180.0;
	const double dec  = (r.dec_i / 3600000.0) * M_PI / 180.0;
	const double sra  = std::sin(ra),  cra  = std::cos(ra);
	const double sdec = std::sin(dec), cdec = std::cos(dec);

	cr.x0 = static_cast<int32_t>(std::lround(cdec * cra * 2e9));
	cr.x1 = static_cast<int32_t>(std::lround(cdec * sra * 2e9));
	cr.x2 = static_cast<int32_t>(std::lround(sdec        * 2e9));

	const double pmra  = (r.pmra_i / 1000.0) * cdec;   // mas/yr, Gaia-native (with cos(dec))
	const double pmdec =  r.pmdec_i / 1000.0;          // mas/yr
	cr.dx0 = static_cast<int32_t>(std::lround((pmra * -sra + pmdec * -sdec * cra) * 1000.0));
	cr.dx1 = static_cast<int32_t>(std::lround((pmra *  cra + pmdec * -sdec * sra) * 1000.0));
	cr.dx2 = static_cast<int32_t>(std::lround((               pmdec *  cdec      ) * 1000.0));

	cr.b_v  = r.bv;
	cr.vmag = r.vmag;
	// Star1 parallax unit is 20 uas (bucket carries 10 uas units)
	cr.plx     = static_cast<uint16_t>(std::max(0, std::min((r.plx_i + 1) / 2, 65535)));
	cr.plx_err = static_cast<uint16_t>(std::max(0, std::min(r.plx_err_i, 65535)));
	cr.rv      = static_cast<int16_t>(std::max(-32768, std::min(r.rv_i, 32767)));
	cr.spInt   = 0;   // spectral type requires SIMBAD data, not available from Gaia bin
	cr.objtype = 0;
	cr.hip[0] = cr.hip[1] = cr.hip[2] = 0;
	return cr;
}

// Convert a BucketRecord to Star3 on-disk form.
// BucketRecord carries mas/millimag units; Star3 uses 0.1 arcsec, 0.02 mag, 0.025 mag.
static CatRecord3 to_cat_record3(const BucketRecord& r)
{
	CatRecord3 cr{};
	cr.gaia_id = r.gaia_id;

	// RA: ra_i is in 1/3,600,000 deg (= mas); Star3 wants 0.1 arcsec = 100 mas units
	uint32_t ra100 = static_cast<uint32_t>((static_cast<int64_t>(r.ra_i) + 50) / 100);
	if (ra100 >= 12960000u) ra100 -= 12960000u;   // wrap RA=360 deg → 0
	store_u24le(cr.x0, ra100);

	// DEC: dec_i is signed mas; Star3 stores (dec + 90 deg) in 0.1 arcsec units
	int64_t dec100 = (static_cast<int64_t>(r.dec_i) + 50) / 100 + 3240000;
	if (dec100 < 0) dec100 = 0;
	if (dec100 > 6480000) dec100 = 6480000;
	store_u24le(cr.x1, static_cast<uint32_t>(dec100));

	// B-V: millimag → raw = (B-V + 1.0) / 0.025; sentinel maps to sentinel
	if (r.bv == STAR2_BV_MISSING) {
		cr.b_v = STAR3_BV_MISSING;
	} else {
		int bvr = static_cast<int>(std::lround((r.bv / 1000.0 + 1.0) * 40.0));
		if (bvr < 0)   { bvr = 0;   g_clamp_bv.fetch_add(1); }
		if (bvr > 254) { bvr = 254; g_clamp_bv.fetch_add(1); }  // 255 = missing sentinel
		cr.b_v = static_cast<uint8_t>(bvr);
	}

	// Vmag: millimag → raw = (V - 16.0) / 0.02
	int vr = static_cast<int>(std::lround((r.vmag / 1000.0 - 16.0) * 50.0));
	if (vr < 0)   { vr = 0;   g_clamp_vmag_lo.fetch_add(1); }
	if (vr > 255) { vr = 255; g_clamp_vmag_hi.fetch_add(1); }
	cr.vmag = static_cast<uint8_t>(vr);

	return cr;
}

// Write the records of one zone run [ri, rj) at the given .cat file offset.
static void write_zone_records(FILE* fcat, const std::vector<BucketRecord>& records,
                               size_t ri, size_t rj, uint32_t cat_type)
{
	for (size_t k = ri; k < rj; ++k) {
		const auto& r = records[k];
		if (cat_type == CATALOG_TYPE_STAR1) {
			CatRecord1 cr = to_cat_record1(r);
			std::fwrite(&cr, sizeof(CatRecord1), 1, fcat);
		} else if (cat_type == CATALOG_TYPE_STAR3) {
			CatRecord3 cr = to_cat_record3(r);
			std::fwrite(&cr, sizeof(CatRecord3), 1, fcat);
		} else {
			CatRecord cr{};
			cr.gaia_id = r.gaia_id;
			cr.x0      = r.ra_i;
			cr.x1      = r.dec_i;
			cr.dx0     = r.pmra_i;
			cr.dx1     = r.pmdec_i;
			cr.b_v     = r.bv;
			cr.vmag    = r.vmag;
			cr.plx     = static_cast<uint16_t>(std::max(0, std::min(r.plx_i, 65535)));
			cr.plx_err = static_cast<uint16_t>(std::max(0, std::min(r.plx_err_i, 65535)));
			std::fwrite(&cr, sizeof(CatRecord), 1, fcat);
		}
	}
}

// Sort one bucket and write its records into the .cat file.
// Multiple buckets can process in parallel because they write to disjoint zone ranges.
static void sort_and_write_bucket(
	const std::string& bucket_path,
	const std::vector<uint64_t>& offsets,  // byte offset for each zone in .cat
	const std::vector<uint32_t>& counts,
	const std::string& cat_path,
	uint32_t cat_type,
	int bucket_idx,
	int n_buckets)
{
	// Read entire bucket into memory
	std::vector<BucketRecord> records;
	{
		FILE* f = std::fopen(bucket_path.c_str(), "rb");
		if (!f) {
			std::cerr << "ERROR: cannot open bucket " << bucket_path << "\n";
			return;
		}
		fseeko(f, 0, SEEK_END);
		off_t fsize = ftello(f);
		fseeko(f, 0, SEEK_SET);
		size_t n_rec = static_cast<size_t>(fsize) / sizeof(BucketRecord);
		records.resize(n_rec);
		size_t nread = std::fread(records.data(), sizeof(BucketRecord), n_rec, f);
		std::fclose(f);
		if (nread != n_rec) {
			std::cerr << "WARNING: bucket " << bucket_path << " short read " << nread << "/" << n_rec << "\n";
			records.resize(nread);
		}
	}

	if (records.empty()) {
		std::cout << "  bucket " << (bucket_idx+1) << "/" << n_buckets << ": 0 records\n";
		return;
	}

	// Sort by (zone, vmag)
	std::sort(records.begin(), records.end(),
		[](const BucketRecord& a, const BucketRecord& b) {
			if (a.zone != b.zone) return a.zone < b.zone;
			return a.vmag < b.vmag;
		});

	// Open .cat file for random-access writing
	FILE* fcat = std::fopen(cat_path.c_str(), "r+b");
	if (!fcat) {
		std::cerr << "ERROR: cannot open .cat " << cat_path << "\n";
		return;
	}

	// Write records grouped by zone
	size_t n_zones = offsets.size() - 1;
	size_t zi = 0;
	for (size_t ri = 0; ri < records.size(); ) {
		uint32_t zone = records[ri].zone;
		if (zone >= n_zones) {
			ri++;
			continue;
		}

		// Find end of this zone's records
		size_t rj = ri + 1;
		while (rj < records.size() && records[rj].zone == zone) rj++;
		size_t zone_n = rj - ri;

		// Verify count matches
		if (static_cast<size_t>(counts[zone]) != zone_n) {
			// May be off by 1 due to rounding; warn on large discrepancies
			long long diff = static_cast<long long>(counts[zone]) - static_cast<long long>(zone_n);
			if (std::abs(diff) > 10) {
				std::cerr << "WARNING: zone " << zone << " count mismatch: expected "
					  << counts[zone] << " got " << zone_n << "\n";
			}
		}

		// Seek to zone position and write
		fseeko(fcat, static_cast<off_t>(offsets[zone]), SEEK_SET);
		write_zone_records(fcat, records, ri, rj, cat_type);

		zi++;
		ri = rj;
	}

	std::fclose(fcat);
	std::cout << "  bucket " << (bucket_idx+1) << "/" << n_buckets
		  << ": " << records.size() << " records, " << zi << " zones\n";
}

void write_cat(const std::vector<std::string>& bucket_paths,
	       const std::vector<uint32_t>& counts,
	       int level, int mag_min, uint32_t cat_type,
	       const std::string& out_path,
	       int n_sort_threads)
{
	int n_zones = nr_of_zones(level);
	const size_t rec_size = catalog_record_size(cat_type);
	if (rec_size == 0) {
		std::cerr << "ERROR: unknown catalog type " << cat_type << "\n";
		return;
	}

	g_clamp_vmag_lo.store(0);
	g_clamp_vmag_hi.store(0);
	g_clamp_bv.store(0);

	// Pre-compute byte offsets for each zone
	std::vector<uint64_t> offsets(n_zones + 1);
	offsets[0] = 28 + static_cast<uint64_t>(n_zones) * 4;  // header + zone table
	for (int z = 1; z <= n_zones; ++z)
		offsets[z] = offsets[z-1] + static_cast<uint64_t>(counts[z-1]) * rec_size;

	// Pre-write header + zone table + extend file to final size
	{
		FILE* fcat = std::fopen(out_path.c_str(), "wb");
		if (!fcat) { std::cerr << "ERROR: cannot create " << out_path << "\n"; return; }

		uint32_t header[6];
		header[0] = FILE_MAGIC;
		header[1] = cat_type;
		header[2] = CATALOG_MAJOR;
		header[3] = CATALOG_MINOR;
		header[4] = static_cast<uint32_t>(level);
		header[5] = static_cast<uint32_t>(mag_min);
		std::fwrite(header, sizeof(uint32_t), 6, fcat);

		float epoch = static_cast<float>(CATALOG_EPOCH);
		std::fwrite(&epoch, sizeof(float), 1, fcat);

		// Zone table
		std::fwrite(counts.data(), sizeof(uint32_t), n_zones, fcat);

		// Extend to final size (pre-allocate on disk)
		uint64_t total_size = offsets.back();
		fseeko(fcat, static_cast<off_t>(total_size) - 1, SEEK_SET);
		char zero = 0;
		std::fwrite(&zero, 1, 1, fcat);
		std::fclose(fcat);
	}

	// Diagnostic: verify header was written correctly
	{
		FILE* vf = std::fopen(out_path.c_str(), "rb");
		if (vf) {
			uint32_t vh[6]; float ve;
			std::fread(vh, sizeof(uint32_t), 6, vf);
			std::fread(&ve, sizeof(float), 1, vf);
			if (vh[0] != FILE_MAGIC || vh[1] != cat_type || vh[4] != static_cast<uint32_t>(level))
				std::cerr << "ERROR: header verification failed! magic=0x"
					  << std::hex << vh[0] << " type=" << std::dec << vh[1]
					  << " level=" << vh[4]
					  << " (expected magic=0x" << std::hex << FILE_MAGIC
					  << " type=" << std::dec << cat_type
					  << " level=" << level << ")\n";
			std::fclose(vf);
		}
	}

	int n_buckets = static_cast<int>(bucket_paths.size());
	int n_threads = std::min(n_sort_threads, n_buckets);
	std::cout << "\nPASS 2: Sorting " << n_buckets << " buckets (" << n_threads << " threads)"
		  << (cat_type == CATALOG_TYPE_STAR1 ? " [Star1]"
		     : cat_type == CATALOG_TYPE_STAR3 ? " [Star3]" : " [Star2]") << "\n";

	// Process buckets in parallel using thread pool
	std::vector<std::thread> threads;
	std::atomic<int> next_bucket{0};

	for (int t = 0; t < n_threads; ++t) {
		threads.emplace_back([&, t]() {
			while (true) {
				int bi = next_bucket.fetch_add(1);
				if (bi >= n_buckets) break;
				sort_and_write_bucket(bucket_paths[bi], offsets, counts,
						      out_path, cat_type, bi, n_buckets);
			}
		});
	}
	for (auto& t : threads) t.join();

	if (cat_type == CATALOG_TYPE_STAR3) {
		uint64_t lo = g_clamp_vmag_lo.load(), hi = g_clamp_vmag_hi.load(), bv = g_clamp_bv.load();
		if (lo || hi || bv) {
			std::cout << "  Star3 quantization clamps: V<16.0: " << lo
				  << "  V>21.1: " << hi
				  << "  B-V out of range: " << bv << "\n";
		}
	}

	std::cout << "  wrote " << out_path << "\n";
}
