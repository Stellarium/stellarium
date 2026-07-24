// Compare two Stellarium Star2 .cat files — streaming zone-by-zone.
// Matches by Gaia ID and compares all binary fields with per-field tolerance.
// O(n) per zone (one hash-map build + one linear scan).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

static int nr_of_zones(int level)
{
	return 20 * (1 << (level * 2)) + 1;
}

// Star2 on-disk layout (32 bytes, little-endian)
struct RawStar2
{
	int64_t gaia_id;
	int32_t x0, x1;   // RA, DEC in mas
	int32_t dx0, dx1; // pmra, pmdec in uas/yr
	int16_t bv, vmag; // milli-mag
	uint16_t plx;     // parallax in 10 uas
	uint16_t plx_err; // parallax error in 10 uas

	static RawStar2 decode(const uint8_t buf[32])
	{
		RawStar2 s;
		std::memcpy(&s, buf, 32);
		return s;
	}
};

static bool near(int32_t a, int32_t b)
{
	return std::abs((int64_t)a - (int64_t)b) <= 3;
}
static bool near(int16_t a, int16_t b)
{
	return std::abs((int)a - (int)b) <= 3;
}
static bool near(uint16_t a, uint16_t b)
{
	uint16_t d = (a > b) ? (a - b) : (b - a);
	return d <= 3;
}

static inline bool isNoBV(int16_t bv)
{
	// Star2: missing BP-RP is encoded as the int16_t maximum value.
	return bv == 32767;
}

struct CatFile
{
	FILE* f   = nullptr;
	int level = 0;
	int nz    = 0;
	std::vector<uint32_t> counts;
	int64_t star_base = 0;
	uint64_t total    = 0;

	bool open(const std::string& path)
	{
		f = std::fopen(path.c_str(), "rb");
		if (!f)
		{
			std::cerr << "ERROR: cannot open " << path << "\n";
			return false;
		}
		uint32_t hdr[6];
		float ep;
		std::fread(hdr, sizeof(uint32_t), 6, f);
		std::fread(&ep, sizeof(float), 1, f);
		level = static_cast<int>(hdr[4]);
		nz    = nr_of_zones(level);
		counts.resize(nz);
		std::fseek(f, 28, SEEK_SET);
		std::fread(counts.data(), sizeof(uint32_t), nz, f);
		total = 0;
		for (auto c : counts)
			total += c;
		star_base = 28 + static_cast<int64_t>(nz) * sizeof(uint32_t);
		return true;
	}
	void close()
	{
		if (f)
		{
			std::fclose(f);
			f = nullptr;
		}
	}
};

struct CompareResult
{
	uint64_t only_a = 0, only_b = 0;
	uint64_t only_a_no_bv = 0, only_b_no_bv = 0;
	uint64_t matched    = 0;
	uint64_t mismatched = 0;
	uint64_t bad_x0 = 0, bad_x1 = 0, bad_dx0 = 0, bad_dx1 = 0;
	uint64_t bad_vmag = 0, bad_plx = 0;
	int64_t sum_x0 = 0, sum_x1 = 0, sum_dx0 = 0, sum_dx1 = 0;
	int64_t sum_vmag = 0, sum_plx = 0;
};

static void record_mismatch(const RawStar2& a, const RawStar2& b, int zone, CompareResult& r, std::ofstream* file)
{
	r.mismatched++;
#define CHK(f)                                                              \
	do                                                                  \
	{                                                                   \
		if (!near(a.f, b.f))                                        \
		{                                                           \
			r.bad_##f++;                                        \
			r.sum_##f += std::abs((int64_t)a.f - (int64_t)b.f); \
		}                                                           \
	}                                                                   \
	while (0)
	CHK(x0);
	CHK(x1);
	CHK(dx0);
	CHK(dx1);
	CHK(vmag);
	CHK(plx);
#undef CHK

	char line[256];
	int n = std::snprintf(line, sizeof(line),
	                      "%llu z=%d  dRA=%.3fmas dDE=%.3fmas dpmra=%.2f dpmde=%.2f dV=%.3fmag dPlx=%.2fmas\n",
	                      (unsigned long long)a.gaia_id, zone, (a.x0 - b.x0) / 1000.0, (a.x1 - b.x1) / 1000.0,
	                      (a.dx0 - b.dx0) / 1000.0, (a.dx1 - b.dx1) / 1000.0, (a.vmag - b.vmag) / 1000.0,
	                      (a.plx - b.plx) / 10.0);
	if (n > 0)
	{
		std::cout.write(line, n);
		if (file && file->is_open()) file->write(line, n);
	}
}

static std::unordered_map<uint64_t, RawStar2> loadZoneMap(CatFile& cf, int z, int64_t off)
{
	std::unordered_map<uint64_t, RawStar2> map;
	uint32_t cnt = (z < cf.nz) ? cf.counts[z] : 0;
	if (cnt == 0) return map;
	std::fseek(cf.f, off, SEEK_SET);
	uint8_t buf[32];
	for (uint32_t i = 0; i < cnt; ++i)
	{
		std::fread(buf, 32, 1, cf.f);
		auto s = RawStar2::decode(buf);
		map[s.gaia_id] = s;
	}
	return map;
}

static void recordOnlyB(const RawStar2& s, int zone, CompareResult& result,
                        const std::string& out_path, std::ofstream& out_file)
{
	result.only_b++;
	if (isNoBV(s.bv)) result.only_b_no_bv++;
	if (!out_path.empty() && out_file.is_open())
	{
		char line[128];
		int n = std::snprintf(
			line, sizeof(line),
			"%llu z=%d RA=%.6f DEC=%+.6f V=%.3f BV=%.3f [only in B]\n",
			(unsigned long long)s.gaia_id, zone, s.x0 / 3600000.0,
			s.x1 / 3600000.0, s.vmag / 1000.0, s.bv / 1000.0);
		if (n > 0) out_file.write(line, n);
	}
}

static void matchOrRecordB(std::unordered_map<uint64_t, RawStar2>& map_a,
                            const RawStar2& s, int zone, CompareResult& result,
                            const std::string& out_path, std::ofstream& out_file)
{
	auto it = map_a.find(s.gaia_id);
	if (it == map_a.end())
	{
		recordOnlyB(s, zone, result, out_path, out_file);
		return;
	}
	const auto& sa = it->second;
	if (near(sa.x0, s.x0) && near(sa.x1, s.x1) && near(sa.dx0, s.dx0) &&
	    near(sa.dx1, s.dx1) && near(sa.vmag, s.vmag) && near(sa.plx, s.plx))
		result.matched++;
	else
		record_mismatch(sa, s, zone, result,
		                out_path.empty() ? nullptr : &out_file);
	map_a.erase(it);
}

static void processZoneB(std::unordered_map<uint64_t, RawStar2>& map_a, CatFile& b,
                         int z, int64_t off_b, CompareResult& result,
                         const std::string& out_path, std::ofstream& out_file)
{
	uint32_t cnt_b = (z < b.nz) ? b.counts[z] : 0;
	if (cnt_b == 0) return;
	std::fseek(b.f, off_b, SEEK_SET);
	uint8_t buf[32];
	for (uint32_t i = 0; i < cnt_b; ++i)
	{
		std::fread(buf, 32, 1, b.f);
		auto s = RawStar2::decode(buf);
		matchOrRecordB(map_a, s, z, result, out_path, out_file);
	}
}

static void writeOnlyA(const std::unordered_map<uint64_t, RawStar2>& map_a,
                       int z, CompareResult& result,
                       const std::string& out_path, std::ofstream& out_file)
{
	for (auto& [gid, s] : map_a)
	{
		result.only_a++;
		if (isNoBV(s.bv)) result.only_a_no_bv++;
		if (out_path.empty()) continue;
		char line[128];
		int n = std::snprintf(line, sizeof(line),
		                      "%llu z=%d RA=%.6f DEC=%+.6f V=%.3f BV=%.3f [only in A]\n",
		                      (unsigned long long)gid, z, s.x0 / 3600000.0, s.x1 / 3600000.0,
		                      s.vmag / 1000.0, s.bv / 1000.0);
		if (n > 0 && out_file.is_open()) out_file.write(line, n);
	}
}

static void printSummary(const CompareResult& result)
{
	std::cout << "\nOnly in A:        " << result.only_a << "\n";
	std::cout << "  (no BV):         " << result.only_a_no_bv << "\n";
	std::cout << "Only in B:        " << result.only_b << "\n";
	std::cout << "  (no BV):         " << result.only_b_no_bv << "\n";
	std::cout << "Matched (near):   " << result.matched << "\n";
	std::cout << "Mismatched:       " << result.mismatched << "\n";
	if (result.mismatched > 0)
	{
		auto avg = [](uint64_t n, int64_t sum) -> double { return n ? (double)sum / n : 0; };
		std::cout << "  dRA:   n=" << result.bad_x0 << "  avg|Δ|=" << avg(result.bad_x0, result.sum_x0) / 1000.0
			  << " mas\n";
		std::cout << "  dDE:   n=" << result.bad_x1 << "  avg|Δ|=" << avg(result.bad_x1, result.sum_x1) / 1000.0
			  << " mas\n";
		std::cout << "  dpmra: n=" << result.bad_dx0
			  << "  avg|Δ|=" << avg(result.bad_dx0, result.sum_dx0) / 1000.0 << " uas/yr\n";
		std::cout << "  dpmde: n=" << result.bad_dx1
			  << "  avg|Δ|=" << avg(result.bad_dx1, result.sum_dx1) / 1000.0 << " uas/yr\n";
		std::cout << "  dV:    n=" << result.bad_vmag
			  << "  avg|Δ|=" << avg(result.bad_vmag, result.sum_vmag) / 1000.0 << " mmag\n";
		std::cout << "  dPlx:  n=" << result.bad_plx
			  << "  avg|Δ|=" << avg(result.bad_plx, result.sum_plx) / 10.0 << " mas\n";
	}
	if (result.only_a == 0 && result.only_b == 0 && result.mismatched == 0)
		std::cout << "\nFiles are IDENTICAL.\n";
	else if (result.mismatched == 0)
		std::cout << "\nID sets IDENTICAL (some stars in different zones).\n";
}

static void compareAllZones(CatFile& a, CatFile& b, CompareResult& result,
                            const std::string& out_path, std::ofstream& out_file)
{
	int nz        = std::max(a.nz, b.nz);
	int64_t off_a = a.star_base, off_b = b.star_base;

	for (int z = 0; z < nz; ++z)
	{
		uint32_t cnt_a = (z < a.nz) ? a.counts[z] : 0;
		uint32_t cnt_b = (z < b.nz) ? b.counts[z] : 0;

		auto map_a = loadZoneMap(a, z, off_a);
		processZoneB(map_a, b, z, off_b, result, out_path, out_file);
		writeOnlyA(map_a, z, result, out_path, out_file);

		off_a += static_cast<int64_t>(cnt_a) * 32;
		off_b += static_cast<int64_t>(cnt_b) * 32;
	}
}

static void compare(const std::string& a_path, const std::string& b_path, const std::string& out_path)
{
	CatFile a, b;
	if (!a.open(a_path)) return;
	if (!b.open(b_path))
	{
		a.close();
		return;
	}
	std::cout << "A: level=" << a.level << "  stars=" << a.total << "\n";
	std::cout << "B: level=" << b.level << "  stars=" << b.total << "\n";
	std::cout << "Tolerance: +/-3 units in raw on-disk representation\n\n";

	std::ofstream out_file;
	if (!out_path.empty())
	{
		out_file.open(out_path);
		if (!out_file) std::cerr << "WARNING: cannot open output file " << out_path << "\n";
	}

	CompareResult result;
	compareAllZones(a, b, result, out_path, out_file);

	a.close();
	b.close();
	if (out_file.is_open()) out_file.close();

	printSummary(result);
}

int main(int argc, char** argv)
{
	std::string a, b, out;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (a.empty())
			a = arg;
		else if (b.empty())
			b = arg;
		else if (out.empty())
			out = arg;
	}
	if (a.empty() || b.empty())
	{
		std::cerr << "Usage: cmpcat <fileA.cat> <fileB.cat> [output.txt]\n";
		return 1;
	}
	compare(a, b, out);
	return 0;
}
