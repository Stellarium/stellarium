// Deduplicate a Stellarium .cat file against a reference catalog.
// Stars whose Gaia ID appears in the reference are removed.
// Auto-detects Star1 (type 0, 48 bytes), Star2 (type 1, 32 bytes) and
// Star3 (type 2, 16 bytes) from headers.
// Reference and input may have different record sizes; output preserves input format.
// Single pass over input. Supports verbose duplicate printing and dry-run.

#include "cat_reader.hpp"

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

static const char* star_type_name(uint32_t cat_type)
{
	return cat_type == 0 ? "1" : cat_type == 2 ? "3" : "2";
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
		} else if (cat_type == 0) {
			// Star1: 3D position/PM vectors scaled by 2e9 / 1000.
			int32_t x0, x1, x2, dx0, dx1, dx2;
			int16_t bv_i, vmag_i, rv_i;
			uint16_t plx_u, plxe_u;
			std::memcpy(&x0, buf+8, 4);   std::memcpy(&x1, buf+12, 4);  std::memcpy(&x2, buf+16, 4);
			std::memcpy(&dx0, buf+20, 4); std::memcpy(&dx1, buf+24, 4); std::memcpy(&dx2, buf+28, 4);
			std::memcpy(&bv_i, buf+32, 2);   std::memcpy(&vmag_i, buf+34, 2);
			std::memcpy(&plx_u, buf+36, 2);  std::memcpy(&plxe_u, buf+38, 2);
			std::memcpy(&rv_i, buf+40, 2);
			double nx = x0 / 2e9, ny = x1 / 2e9, nz = x2 / 2e9;
			double r = std::sqrt(nx*nx + ny*ny + nz*nz);
			s.ra_deg = std::atan2(ny, nx) * 180.0 / M_PI;
			if (s.ra_deg < 0) s.ra_deg += 360.0;
			s.dec_deg = (r > 0 ? std::asin(nz / r) : 0.0) * 180.0 / M_PI;
			s.bv = bv_i / 1000.0;  s.vmag = vmag_i / 1000.0;
			s.plx_mas = plx_u / 50.0;
			// dx* are PM components in the same 3D basis (mas/yr), without cos(dec).
			double ra = s.ra_deg * M_PI / 180.0, dec = s.dec_deg * M_PI / 180.0;
			s.pmra  = (dx0 * -std::sin(ra) + dx1 * std::cos(ra)) / 1000.0;
			s.pmdec = (dx0 * -std::sin(dec) * std::cos(ra) + dx1 * -std::sin(dec) * std::sin(ra)
			           + dx2 * std::cos(dec)) / 1000.0;
			s.has_pm_plx = true;
			(void)plxe_u; (void)rv_i;
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
	CatReader rd;
	std::string err;
	if (!rd.open(path, err)) { std::cerr << "ERROR: " << err << "\n"; std::exit(1); }
	level_ref = rd.level;
	cat_type_ref = rd.cat_type;
	const size_t rec_size = rd.rec_size;
	const int nz = rd.nzones;
	uint64_t total = rd.total_stars();
	std::cout << "Reference: level=" << level_ref << " type=Star" << star_type_name(cat_type_ref)
		  << ", " << total << " stars\n";

	if (verbose) ref_map.reserve(static_cast<size_t>(total));
	int64_t off = rd.star_base;
	std::vector<uint8_t> buf(rec_size);
	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = rd.counts[z]; if (cnt == 0) continue;
		fseeko(rd.f, off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf.data(), rec_size, 1, rd.f);
			uint64_t gid; std::memcpy(&gid, buf.data(), 8);
			if (verbose) { std::array<uint8_t, 32> rec{}; std::memcpy(rec.data(), buf.data(), rec_size); ref_map[gid] = rec; }
			else ref_map[gid] = {};
		}
		off += static_cast<int64_t>(cnt) * rec_size;
	}
	std::fclose(rd.f);
	std::cout << "  " << ref_map.size() << " unique IDs loaded\n";
}

struct DedupResult { uint64_t removed = 0, total_out = 0; };

static void dedup_stream(CatReader& in, FILE* fout,
			 int level_ref, uint32_t cat_type_ref,
			 const RefMap& ref_map, bool verbose, bool dry_run)
{
	const int nz = in.nzones;
	const int level_in = in.level;
	const uint32_t cat_type_in = in.cat_type;
	const size_t rec_size = in.rec_size;
	uint64_t total_in = in.total_stars();
	std::cout << "Input:    level=" << level_in << " type=Star" << star_type_name(cat_type_in)
		  << ", " << total_in << " stars\n";

	std::vector<uint32_t> out_counts; DedupResult r;
	if (!dry_run) {
		out_counts.resize(nz, 0);
		std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);
	}

	int64_t in_off = in.star_base;
	std::vector<uint8_t> buf(rec_size);

	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = in.counts[z]; if (cnt == 0) continue;
		fseeko(in.f, in_off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf.data(), rec_size, 1, in.f);
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

	CatReader in;
	std::string err;
	if (!in.open(in_path, err)) { std::cerr << "ERROR: " << err << "\n"; return; }

	FILE* fout = nullptr;
	if (!dry_run) {
		fout = std::fopen(out_path.c_str(), "wb");
		if (!fout) { std::cerr << "ERROR: cannot create " << out_path << "\n"; return; }
		std::fwrite(in.hdr, sizeof(uint32_t), 6, fout);
		std::fwrite(&in.epoch, sizeof(float), 1, fout);
	}

	dedup_stream(in, fout, level_ref, cat_type_ref, ref_map, verbose, dry_run);

	if (fout) std::fclose(fout);
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
