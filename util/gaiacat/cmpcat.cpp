// Compare two Stellarium .cat files — streaming zone-by-zone.
// Low memory: only current zone's stars loaded at a time.
// Uses Gaia ID as the key for matching across files.

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>

static int nr_of_zones(int level) { return 20 * (1 << (level * 2)) + 1; }

struct Star2Brief {
	int64_t gaia_id; int32_t x0, x1, dx0, dx1; int16_t vmag, bv; uint16_t plx, plx_err;

	static Star2Brief decode(const uint8_t buf[32]) {
		Star2Brief s;
		std::memcpy(&s.gaia_id, buf, 8);
		std::memcpy(&s.x0,  buf+8,  4); std::memcpy(&s.x1,  buf+12, 4);
		std::memcpy(&s.dx0, buf+16, 4); std::memcpy(&s.dx1, buf+20, 4);
		std::memcpy(&s.bv,  buf+24, 2); std::memcpy(&s.vmag,buf+26, 2);
		std::memcpy(&s.plx, buf+28, 2); std::memcpy(&s.plx_err, buf+30, 2);
		return s;
	}
	void print(const char* label, int zone) const {
		printf("  %s Gaia %lld  zone=%u  V=%.3f  B-V=%.3f  RA=%.6f  DEC=%.6f  pm=(%.2f,%.2f) mas/yr  plx=%.2f mas\n",
			label, (long long)gaia_id, zone, vmag/1000.0, bv/1000.0,
			x0/3600000.0, x1/3600000.0, dx0/1000.0, dx1/1000.0, plx/100.0);
	}
};

struct CatFile {
	FILE* f; int level, nz; std::vector<uint32_t> counts; int64_t star_base; uint64_t total;

	bool open(const std::string& path) {
		f = std::fopen(path.c_str(), "rb");
		if (!f) { std::cerr << "ERROR: cannot open " << path << "\n"; return false; }
		uint32_t hdr[6]; float ep;
		std::fread(hdr, sizeof(uint32_t), 6, f); std::fread(&ep, sizeof(float), 1, f);
		level = static_cast<int>(hdr[4]); nz = nr_of_zones(level);
		counts.resize(nz);
		fseeko(f, 28, SEEK_SET); std::fread(counts.data(), sizeof(uint32_t), nz, f);
		total = 0; for (auto c : counts) total += c;
		star_base = 28 + static_cast<int64_t>(nz) * 4;
		return true;
	}
	void close() { if (f) { std::fclose(f); f = nullptr; } }
};

struct ZoneResult {
	uint64_t only_a = 0, only_b = 0; int printed = 0;
};

static void compare_zone(CatFile& a, CatFile& b, int z, int64_t& off_a, int64_t& off_b, ZoneResult& r) {
	uint32_t cnt_a = (z < a.nz) ? a.counts[z] : 0;
	uint32_t cnt_b = (z < b.nz) ? b.counts[z] : 0;
	if (cnt_a == 0 && cnt_b == 0) return;

	uint8_t buf[32];
	std::unordered_map<uint64_t, Star2Brief> map_a;
	auto print_once = [&]() -> bool { return ++r.printed <= 200; };

	if (cnt_a > 0) {
		fseeko(a.f, off_a, SEEK_SET);
		for (uint32_t i = 0; i < cnt_a; ++i) {
			std::fread(buf, 32, 1, a.f);
			auto s = Star2Brief::decode(buf);
			map_a[s.gaia_id] = s;
		}
	}
	if (cnt_b > 0) {
		fseeko(b.f, off_b, SEEK_SET);
		for (uint32_t i = 0; i < cnt_b; ++i) {
			std::fread(buf, 32, 1, b.f);
			auto s = Star2Brief::decode(buf);
			auto it = map_a.find(s.gaia_id);
			if (it == map_a.end()) { r.only_b++; if (print_once()) s.print("only in B:", z); }
			else map_a.erase(it);
		}
	}
	for (auto& [gid, s] : map_a) { r.only_a++; if (print_once()) s.print("only in A:", z); }

	off_a += static_cast<int64_t>(cnt_a) * 32;
	off_b += static_cast<int64_t>(cnt_b) * 32;
}

static void compare(const std::string& a_path, const std::string& b_path) {
	CatFile a, b;
	if (!a.open(a_path)) return;
	if (!b.open(b_path)) { a.close(); return; }

	std::cout << "A: level=" << a.level << " stars=" << a.total << "\n";
	std::cout << "B: level=" << b.level << " stars=" << b.total << "\n";

	int nz = std::max(a.nz, b.nz);
	int64_t off_a = a.star_base, off_b = b.star_base;
	ZoneResult result;

	for (int z = 0; z < nz; ++z)
		compare_zone(a, b, z, off_a, off_b, result);

	a.close(); b.close();

	std::cout << "\nOnly in A:    " << result.only_a << "\n";
	std::cout << "Only in B:    " << result.only_b << "\n";
	if (result.only_a == 0 && result.only_b == 0)
		std::cout << "Files are IDENTICAL.\n";
}

int main(int argc, char** argv) {
	std::string a, b;
	for (int i = 1; i < argc; ++i) { std::string arg = argv[i]; if (a.empty()) a = arg; else if (b.empty()) b = arg; }
	if (a.empty() || b.empty()) { std::cerr << "Usage: cmpcat <fileA.cat> <fileB.cat>\n"; return 1; }
	compare(a, b);
	return 0;
}
