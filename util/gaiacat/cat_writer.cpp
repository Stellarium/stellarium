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

// Sort one bucket and write its records into the .cat file.
// Multiple buckets can process in parallel because they write to disjoint zone ranges.
static void sort_and_write_bucket(
	const std::string& bucket_path,
	const std::vector<uint64_t>& offsets,  // byte offset for each zone in .cat
	const std::vector<uint32_t>& counts,
	const std::string& cat_path,
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
		for (size_t k = ri; k < rj; ++k) {
			const auto& r = records[k];
			CatRecord cr{};
			cr.gaia_id = r.gaia_id;
			cr.x0      = r.ra_i;
			cr.x1      = r.dec_i;
			cr.dx0     = r.pmra_i;
			cr.dx1     = r.pmdec_i;
			cr.b_v     = r.bv;
			cr.vmag    = r.vmag;
			cr.plx     = static_cast<uint16_t>(std::max(0, std::min(r.plx_i, 65535)));
			cr.plx_err = 0;
			std::fwrite(&cr, sizeof(CatRecord), 1, fcat);
		}

		zi++;
		ri = rj;
	}

	std::fclose(fcat);
	std::cout << "  bucket " << (bucket_idx+1) << "/" << n_buckets
		  << ": " << records.size() << " records, " << zi << " zones\n";
}

void write_cat(const std::vector<std::string>& bucket_paths,
	       const std::vector<uint32_t>& counts,
	       int level, int mag_min,
	       const std::string& out_path,
	       int n_sort_threads)
{
	int n_zones = nr_of_zones(level);

	// Pre-compute byte offsets for each zone
	std::vector<uint64_t> offsets(n_zones + 1);
	offsets[0] = 28 + static_cast<uint64_t>(n_zones) * 4;  // header + zone table
	for (int z = 1; z <= n_zones; ++z)
		offsets[z] = offsets[z-1] + static_cast<uint64_t>(counts[z-1]) * sizeof(CatRecord);

	// Pre-write header + zone table + extend file to final size
	{
		FILE* fcat = std::fopen(out_path.c_str(), "wb");
		if (!fcat) { std::cerr << "ERROR: cannot create " << out_path << "\n"; return; }

		uint32_t header[6];
		header[0] = FILE_MAGIC;
		header[1] = CATALOG_TYPE;
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
			if (vh[0] != FILE_MAGIC || vh[4] != static_cast<uint32_t>(level))
				std::cerr << "ERROR: header verification failed! magic=0x"
					  << std::hex << vh[0] << " level=" << std::dec << vh[4]
					  << " (expected magic=0x" << std::hex << FILE_MAGIC
					  << " level=" << std::dec << level << ")\n";
			std::fclose(vf);
		}
	}

	int n_buckets = static_cast<int>(bucket_paths.size());
	int n_threads = std::min(n_sort_threads, n_buckets);
	std::cout << "\nPASS 2: Sorting " << n_buckets << " buckets (" << n_threads << " threads)\n";

	// Process buckets in parallel using thread pool
	std::vector<std::thread> threads;
	std::atomic<int> next_bucket{0};

	for (int t = 0; t < n_threads; ++t) {
		threads.emplace_back([&, t]() {
			while (true) {
				int bi = next_bucket.fetch_add(1);
				if (bi >= n_buckets) break;
				sort_and_write_bucket(bucket_paths[bi], offsets, counts,
						      out_path, bi, n_buckets);
			}
		});
	}
	for (auto& t : threads) t.join();

	std::cout << "  wrote " << out_path << "\n";
}
