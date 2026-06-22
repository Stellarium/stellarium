// Deduplicate a Stellarium .cat file against a reference catalog.
// Stars whose Gaia ID appears in the reference are removed.
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

struct Star2Info {
	double ra_deg, dec_deg, vmag, bv, plx_mas, pmra, pmdec; int64_t gaia_id;

	static Star2Info decode(const uint8_t buf[32]) {
		Star2Info s;
		std::memcpy(&s.gaia_id, buf, 8);
		int32_t x0, x1, dx0, dx1; int16_t bv_i, vmag_i; uint16_t plx_u;
		std::memcpy(&x0,buf+8,4); std::memcpy(&x1,buf+12,4); std::memcpy(&dx0,buf+16,4); std::memcpy(&dx1,buf+20,4);
		std::memcpy(&bv_i,buf+24,2); std::memcpy(&vmag_i,buf+26,2); std::memcpy(&plx_u,buf+28,2);
		s.ra_deg=x0/3600000.0; s.dec_deg=x1/3600000.0; s.vmag=vmag_i/1000.0;
		s.bv=bv_i/1000.0; s.plx_mas=plx_u/100.0; s.pmra=dx0/1000.0; s.pmdec=dx1/1000.0;
		return s;
	}
	void print() const {
		printf("Gaia %lld  RA=%.6f  DEC=%.6f  V=%.3f  B-V=%.3f  plx=%.2f mas  pm=(%.2f,%.2f) mas/yr",
			(long long)gaia_id, ra_deg, dec_deg, vmag, bv, plx_mas, pmra, pmdec);
	}
};

using RefMap = std::unordered_map<uint64_t, std::array<uint8_t, 32>>;

static void load_reference(const std::string& path, RefMap& ref_map, int& level_ref, bool verbose)
{
	FILE* f = std::fopen(path.c_str(), "rb");
	if (!f) { std::cerr << "ERROR: cannot open " << path << "\n"; std::exit(1); }
	uint32_t hdr[6]; float ep;
	std::fread(hdr, sizeof(uint32_t), 6, f); std::fread(&ep, sizeof(float), 1, f);
	level_ref = static_cast<int>(hdr[4]);
	int nz = nr_of_zones(level_ref);
	auto counts = std::vector<uint32_t>(nz);
	fseeko(f, 28, SEEK_SET); std::fread(counts.data(), sizeof(uint32_t), nz, f);
	uint64_t total = 0; for (auto c : counts) total += c;
	std::cout << "Reference: level=" << level_ref << ", " << total << " stars\n";

	if (verbose) ref_map.reserve(static_cast<size_t>(total));
	int64_t base = 28 + static_cast<int64_t>(nz) * 4, off = base;
	uint8_t buf[32];
	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = counts[z]; if (cnt == 0) continue;
		fseeko(f, off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf, 32, 1, f);
			uint64_t gid; std::memcpy(&gid, buf, 8);
			if (verbose) { std::array<uint8_t, 32> rec; std::memcpy(rec.data(), buf, 32); ref_map[gid] = rec; }
			else ref_map[gid] = {};
		}
		off += static_cast<int64_t>(cnt) * 32;
	}
	std::fclose(f);
	std::cout << "  " << ref_map.size() << " unique IDs loaded\n";
}

struct DedupResult { uint64_t removed = 0, total_out = 0; };

static void dedup_stream(FILE* fin, FILE* fout, int nz, int level_in, int level_ref,
			 const RefMap& ref_map, bool verbose, bool dry_run)
{
	auto in_counts = std::vector<uint32_t>(nz);
	fseeko(fin, 28, SEEK_SET); std::fread(in_counts.data(), sizeof(uint32_t), nz, fin);
	uint64_t total_in = 0; for (auto c : in_counts) total_in += c;
	std::cout << "Input:    level=" << level_in << ", " << total_in << " stars\n";

	std::vector<uint32_t> out_counts; DedupResult r;
	if (!dry_run) {
		out_counts.resize(nz, 0);
		std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);
	}

	int64_t in_base = 28 + static_cast<int64_t>(nz) * 4, in_off = in_base;
	uint8_t buf[32];

	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = in_counts[z]; if (cnt == 0) continue;
		fseeko(fin, in_off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf, 32, 1, fin);
			uint64_t gid; std::memcpy(&gid, buf, 8);
			auto it = ref_map.find(gid);
			if (it != ref_map.end()) {
				r.removed++;
				if (verbose) {
					auto in_info  = Star2Info::decode(buf);
					auto ref_info = Star2Info::decode(it->second.data());
					printf("\n--- Duplicate #%llu (zone %d) ---\n", (unsigned long long)r.removed, z);
					printf("Level %d (reference): ", level_ref); ref_info.print(); printf("\n");
					printf("Level %d (input):     ", level_in);  in_info.print();  printf("\n");
					printf("  Vmag diff=%.3f  B-V diff=%.4f  RA diff=%.6f deg  DEC diff=%.6f deg  plx diff=%.3f mas\n\n",
						in_info.vmag - ref_info.vmag, in_info.bv - ref_info.bv,
						in_info.ra_deg - ref_info.ra_deg, in_info.dec_deg - ref_info.dec_deg,
						in_info.plx_mas - ref_info.plx_mas);
				}
			} else {
				r.total_out++;
				if (!dry_run) { std::fwrite(buf, 32, 1, fout); out_counts[z]++; }
			}
		}
		in_off += static_cast<int64_t>(cnt) * 32;
	}

	if (!dry_run) {
		fseeko(fout, 28, SEEK_SET);
		std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);
		auto size_mb = static_cast<double>(28 + nz*4 + r.total_out*32) / 1048576.0;
		std::cout << "  Written: " << size_mb << " MB\n";
	}
	std::cout << "  Removed: " << r.removed << " (" << (100.0*r.removed/(total_in?total_in:1)) << "%)\n";
	std::cout << "  Output:  " << r.total_out << " stars\n";
}

static void dedup(const std::string& ref_path, const std::string& in_path,
		  const std::string& out_path, bool verbose, bool dry_run)
{
	RefMap ref_map; int level_ref;
	load_reference(ref_path, ref_map, level_ref, verbose);

	FILE* fin = std::fopen(in_path.c_str(), "rb");
	if (!fin) { std::cerr << "ERROR: cannot open " << in_path << "\n"; return; }
	uint32_t hdr[6]; float epoch;
	std::fread(hdr, sizeof(uint32_t), 6, fin); std::fread(&epoch, sizeof(float), 1, fin);
	int level_in = static_cast<int>(hdr[4]);
	int nz = nr_of_zones(level_in);

	FILE* fout = nullptr;
	if (!dry_run) {
		fout = std::fopen(out_path.c_str(), "wb");
		if (!fout) { std::cerr << "ERROR: cannot create " << out_path << "\n"; std::fclose(fin); return; }
		std::fwrite(hdr, sizeof(uint32_t), 6, fout);
		std::fwrite(&epoch, sizeof(float), 1, fout);
	}

	dedup_stream(fin, fout, nz, level_in, level_ref, ref_map, verbose, dry_run);

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
