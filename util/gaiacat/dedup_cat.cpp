// Deduplicate a Stellarium .cat file against a reference catalog.
// Stars whose Gaia ID appears in the reference are removed.
// Auto-detects Star2 (type 1, 32 bytes) and Star3 (type 2, 16 bytes) from headers.
// Reference and input may have different record sizes; output preserves input format.
// Single pass over input. Supports verbose duplicate printing and dry-run.

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <cmath>
#include <array>

static int nr_of_zones(int level) { return 20 * (1 << (level * 2)) + 1; }

// Read catalog type and level from a .cat header; returns record size (0 on error).
static size_t read_header(FILE* f, uint32_t hdr[6], float& epoch, int& level, uint32_t& cat_type)
{
	std::fread(hdr, sizeof(uint32_t), 6, f);
	std::fread(&epoch, sizeof(float), 1, f);
	cat_type = hdr[1];
	level    = static_cast<int>(hdr[4]);
	if (cat_type == 1) return 32;
	if (cat_type == 2) return 16;
	return 0;
}

struct StarInfo {
	double ra_deg = 0, dec_deg = 0, vmag = 0, bv = 0, plx_mas = 0, pmra = 0, pmdec = 0;
	int64_t gaia_id = 0;
	bool has_pm_plx = false;

	static StarInfo decode(const uint8_t* buf, uint32_t cat_type) {
		StarInfo s;
		std::memcpy(&s.gaia_id, buf, 8);
		if (cat_type == 2) {
			uint32_t x0 = buf[8] | (buf[9] << 8) | (buf[10] << 16);
			uint32_t x1 = buf[11] | (buf[12] << 8) | (buf[13] << 16);
			s.ra_deg  = x0 / 36000.0;
			s.dec_deg = x1 / 36000.0 - 90.0;
			s.bv      = 0.025 * buf[14] - 1.0;
			s.vmag    = 16.0 + 0.02 * buf[15];
			s.has_pm_plx = false;
		} else {
			int32_t x0, x1, dx0, dx1; int16_t bv_i, vmag_i; uint16_t plx_u;
			std::memcpy(&x0,buf+8,4); std::memcpy(&x1,buf+12,4); std::memcpy(&dx0,buf+16,4); std::memcpy(&dx1,buf+20,4);
			std::memcpy(&bv_i,buf+24,2); std::memcpy(&vmag_i,buf+26,2); std::memcpy(&plx_u,buf+28,2);
			s.ra_deg=x0/3600000.0; s.dec_deg=x1/3600000.0; s.vmag=vmag_i/1000.0;
			s.bv=bv_i/1000.0; s.plx_mas=plx_u/100.0; s.pmra=dx0/1000.0; s.pmdec=dx1/1000.0;
			s.has_pm_plx = true;
		}
		return s;
	}
	void print() const {
		printf("Gaia %lld  RA=%.6f  DEC=%.6f  V=%.3f  B-V=%.3f",
			(long long)gaia_id, ra_deg, dec_deg, vmag, bv);
		if (has_pm_plx)
			printf("  plx=%.2f mas  pm=(%.2f,%.2f) mas/yr", plx_mas, pmra, pmdec);
	}
};

using RefMap = std::unordered_map<uint64_t, std::array<uint8_t, 32>>;

static void load_reference(const std::string& path, RefMap& ref_map, int& level_ref,
			   uint32_t& cat_type_ref, bool verbose)
{
	FILE* f = std::fopen(path.c_str(), "rb");
	if (!f) { std::cerr << "ERROR: cannot open " << path << "\n"; std::exit(1); }
	uint32_t hdr[6]; float ep;
	size_t rec_size = read_header(f, hdr, ep, level_ref, cat_type_ref);
	if (rec_size == 0) { std::cerr << "ERROR: " << path << " unsupported catalog type " << cat_type_ref << "\n"; std::exit(1); }
	int nz = nr_of_zones(level_ref);
	auto counts = std::vector<uint32_t>(nz);
	fseeko(f, 28, SEEK_SET); std::fread(counts.data(), sizeof(uint32_t), nz, f);
	uint64_t total = 0; for (auto c : counts) total += c;
	std::cout << "Reference: level=" << level_ref << " type=Star" << (cat_type_ref == 2 ? 3 : 2)
		  << ", " << total << " stars\n";

	if (verbose) ref_map.reserve(static_cast<size_t>(total));
	int64_t base = 28 + static_cast<int64_t>(nz) * 4, off = base;
	std::vector<uint8_t> buf(rec_size);
	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = counts[z]; if (cnt == 0) continue;
		fseeko(f, off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf.data(), rec_size, 1, f);
			uint64_t gid; std::memcpy(&gid, buf.data(), 8);
			if (verbose) { std::array<uint8_t, 32> rec{}; std::memcpy(rec.data(), buf.data(), rec_size); ref_map[gid] = rec; }
			else ref_map[gid] = {};
		}
		off += static_cast<int64_t>(cnt) * rec_size;
	}
	std::fclose(f);
	std::cout << "  " << ref_map.size() << " unique IDs loaded\n";
}

struct DedupResult { uint64_t removed = 0, total_out = 0; };

static void dedup_stream(FILE* fin, FILE* fout, int nz, int level_in, uint32_t cat_type_in, size_t rec_size,
			 int level_ref, uint32_t cat_type_ref,
			 const RefMap& ref_map, bool verbose, bool dry_run)
{
	auto in_counts = std::vector<uint32_t>(nz);
	fseeko(fin, 28, SEEK_SET); std::fread(in_counts.data(), sizeof(uint32_t), nz, fin);
	uint64_t total_in = 0; for (auto c : in_counts) total_in += c;
	std::cout << "Input:    level=" << level_in << " type=Star" << (cat_type_in == 2 ? 3 : 2)
		  << ", " << total_in << " stars\n";

	std::vector<uint32_t> out_counts; DedupResult r;
	if (!dry_run) {
		out_counts.resize(nz, 0);
		std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);
	}

	int64_t in_base = 28 + static_cast<int64_t>(nz) * 4, in_off = in_base;
	std::vector<uint8_t> buf(rec_size);

	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = in_counts[z]; if (cnt == 0) continue;
		fseeko(fin, in_off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf.data(), rec_size, 1, fin);
			uint64_t gid; std::memcpy(&gid, buf.data(), 8);
			auto it = ref_map.find(gid);
			if (it != ref_map.end()) {
				r.removed++;
				if (verbose) {
					auto in_info  = StarInfo::decode(buf.data(), cat_type_in);
					auto ref_info = StarInfo::decode(it->second.data(), cat_type_ref);
					printf("\n--- Duplicate #%llu (zone %d) ---\n", (unsigned long long)r.removed, z);
					printf("Level %d (reference): ", level_ref); ref_info.print(); printf("\n");
					printf("Level %d (input):     ", level_in);  in_info.print();  printf("\n");
					printf("  Vmag diff=%.3f  B-V diff=%.4f  RA diff=%.6f deg  DEC diff=%.6f deg",
						in_info.vmag - ref_info.vmag, in_info.bv - ref_info.bv,
						in_info.ra_deg - ref_info.ra_deg, in_info.dec_deg - ref_info.dec_deg);
					if (in_info.has_pm_plx && ref_info.has_pm_plx)
						printf("  plx diff=%.3f mas", in_info.plx_mas - ref_info.plx_mas);
					printf("\n\n");
				}
			} else {
				r.total_out++;
				if (!dry_run) { std::fwrite(buf.data(), rec_size, 1, fout); out_counts[z]++; }
			}
		}
		in_off += static_cast<int64_t>(cnt) * rec_size;
	}

	if (!dry_run) {
		fseeko(fout, 28, SEEK_SET);
		std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);
		auto size_mb = static_cast<double>(28 + nz*4 + r.total_out*rec_size) / 1048576.0;
		std::cout << "  Written: " << size_mb << " MB\n";
	}
	std::cout << "  Removed: " << r.removed << " (" << (100.0*r.removed/(total_in?total_in:1)) << "%)\n";
	std::cout << "  Output:  " << r.total_out << " stars\n";
}

static void dedup(const std::string& ref_path, const std::string& in_path,
		  const std::string& out_path, bool verbose, bool dry_run)
{
	RefMap ref_map; int level_ref; uint32_t cat_type_ref;
	load_reference(ref_path, ref_map, level_ref, cat_type_ref, verbose);

	FILE* fin = std::fopen(in_path.c_str(), "rb");
	if (!fin) { std::cerr << "ERROR: cannot open " << in_path << "\n"; return; }
	uint32_t hdr[6]; float epoch; int level_in; uint32_t cat_type_in;
	size_t rec_size = read_header(fin, hdr, epoch, level_in, cat_type_in);
	if (rec_size == 0) { std::cerr << "ERROR: " << in_path << " unsupported catalog type " << cat_type_in << "\n"; std::fclose(fin); return; }
	int nz = nr_of_zones(level_in);

	FILE* fout = nullptr;
	if (!dry_run) {
		fout = std::fopen(out_path.c_str(), "wb");
		if (!fout) { std::cerr << "ERROR: cannot create " << out_path << "\n"; std::fclose(fin); return; }
		std::fwrite(hdr, sizeof(uint32_t), 6, fout);
		std::fwrite(&epoch, sizeof(float), 1, fout);
	}

	dedup_stream(fin, fout, nz, level_in, cat_type_in, rec_size, level_ref, cat_type_ref,
		     ref_map, verbose, dry_run);

	if (fout) std::fclose(fout);
	std::fclose(fin);
}

int main(int argc, char** argv) {
	bool verbose = false, dry_run = false;
	std::string ref, input, output;
	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if (a == "--verbose") verbose = true;
		else if (a == "--dry-run") dry_run = true;
		else if (ref.empty()) ref = a;
		else if (input.empty()) input = a;
		else output = a;
	}
	if (ref.empty() || input.empty() || (!dry_run && output.empty())) {
		std::cerr << "Usage: dedupcat [--verbose] [--dry-run] <reference.cat> <input.cat> [<output.cat>]\n";
		return 1;
	}
	dedup(ref, input, output, verbose, dry_run);
	return 0;
}
