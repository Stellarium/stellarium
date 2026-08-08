// Compare two Stellarium .cat files �?streaming zone-by-zone.
// Auto-detects Star2 (type 1, 32 bytes) and Star3 (type 2, 16 bytes) from the header.
// Matches by Gaia ID and compares decoded physical values with type-aware tolerances:
//   - Star2↔Star2: ±3 mas position, ±3 mmag V, ±3 uas/yr pm, ±30 uas plx (same as before)
//   - Star3 involved: ±0.15 arcsec position, ±30 mmag V (Star3 quantization is coarser)
// B-V is never a match criterion (pipelines may convert it differently), only reported.
// O(n) per zone (one hash-map build + one linear scan).

#include "cat_reader.hpp"
#include "cat_record.hpp"

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

// Decoded physical representation of one star from either format
using PhysStar = DecodedStar;

static inline bool isNoBV(double bv)
{
	// Missing BP-RP decodes to the format-specific sentinel value
	return bv > 5.0;   // 32.767 (Star1/2) or 5.375 (Star3); real B-V < 5.0
}

// --compare-bv: report B-V differences as dBV without affecting match verdicts.
// Off by default because pipelines legitimately differ (synthetic photometry
// vs BP-RP polynomial).
static bool   g_compare_bv = false;
static double g_bv_tol     = 0.05;   // mag

static PhysStar decode(const CatReader& cf, const uint8_t* buf)
{
	return decode_record(buf, cf.cat_type);
}

struct CompareResult
{
	uint64_t only_a = 0, only_b = 0;
	uint64_t only_a_no_bv = 0, only_b_no_bv = 0;
	uint64_t matched    = 0;
	uint64_t mismatched = 0;
	uint64_t bad_x0 = 0, bad_x1 = 0, bad_dx0 = 0, bad_dx1 = 0;
	uint64_t bad_vmag = 0, bad_plx = 0, bad_rv = 0, bad_bv = 0;
	double sum_x0 = 0, sum_x1 = 0, sum_dx0 = 0, sum_dx1 = 0;
	double sum_vmag = 0, sum_plx = 0, sum_rv = 0, sum_bv = 0;
};

// Type-aware tolerances (physical units)
struct Tolerances
{
	double pos_deg;    // RA/DEC
	double vmag;       // mag
	double pm_masyr;   // proper motion (only when both have it)
	double plx_mas;    // parallax (only when both have it)
	double rv_kms;     // radial velocity (only when both are Star1)
};

static Tolerances tolerances_for(const CatReader& a, const CatReader& b)
{
	Tolerances t;
	bool coarse = (a.cat_type == 2 || b.cat_type == 2);
	bool star1  = (a.cat_type == 0 || b.cat_type == 0);
	// Star2 legacy: ±3 mas / ±3 mmag. Star3 quantization: 0.1 arcsec / 0.02 mag.
	t.pos_deg  = coarse ? 0.15 / 3600.0 : star1 ? 2.0 / 3600000.0 : 3.0 / 3600000.0;
	t.vmag     = coarse ? 0.030 : star1 ? 0.005 : 0.003;
	t.pm_masyr = star1 ? 0.02 : 0.003;   // Star1 pm vector: 1 uas/yr quantization, model diffs
	t.plx_mas  = star1 ? 0.05 : 0.03;    // Star1 plx: 20 uas units
	t.rv_kms   = 0.5;
	return t;
}

// Circular RA difference in degrees, folded into (-180, 180]
static inline double ra_diff_deg(double a, double b)
{
	double d = a - b;
	if (d > 180.0)  d -= 360.0;
	if (d < -180.0) d += 360.0;
	return d;
}

static void write_mismatch_line(uint64_t gaia_id, int zone,
                                double dra, double dde, double dpmra, double dpmde,
                                double dvm, double dplx, double drv, double dbv,
                                bool astrom, bool rv_ok, bool bv_line,
                                std::ofstream* file)
{
	char line[320];
	int n;
	if (rv_ok || bv_line)
		n = std::snprintf(line, sizeof(line),
		                  "%llu z=%d  dRA=%.3fmas dDE=%.3fmas dpmra=%.3f dpmde=%.3f dV=%.3fmag dPlx=%.3fmas dRV=%.2fkm/s dBV=%.3fmag\n",
		                  (unsigned long long)gaia_id, zone, dra, dde, dpmra, dpmde, dvm, dplx, drv, dbv);
	else if (astrom)
		n = std::snprintf(line, sizeof(line),
		                  "%llu z=%d  dRA=%.3fmas dDE=%.3fmas dpmra=%.3f dpmde=%.3f dV=%.3fmag dPlx=%.3fmas\n",
		                  (unsigned long long)gaia_id, zone, dra, dde, dpmra, dpmde, dvm, dplx);
	else
		n = std::snprintf(line, sizeof(line),
		                  "%llu z=%d  dRA=%.3fmas dDE=%.3fmas dV=%.3fmag [star3]\n",
		                  (unsigned long long)gaia_id, zone, dra, dde, dvm);
	if (n > 0)
	{
		std::cout.write(line, n);
		if (file && file->is_open()) file->write(line, n);
	}
}

static void record_mismatch(const PhysStar& a, const PhysStar& b, int zone,
                            const Tolerances& tol, CompareResult& r, std::ofstream* file)
{
	r.mismatched++;
	double dra_deg = ra_diff_deg(a.ra_deg, b.ra_deg);
	double dra  = dra_deg * 3600000.0;    // mas
	double dde  = (a.dec_deg - b.dec_deg) * 3600000.0;
	double dvm  = a.vmag - b.vmag;
	if (std::fabs(dra_deg) > tol.pos_deg)             { r.bad_x0++; r.sum_x0 += std::fabs(dra); }
	if (std::fabs(a.dec_deg - b.dec_deg) > tol.pos_deg) { r.bad_x1++; r.sum_x1 += std::fabs(dde); }
	if (std::fabs(dvm) > tol.vmag)                      { r.bad_vmag++; r.sum_vmag += std::fabs(dvm); }

	bool astrom = a.has_pm_plx && b.has_pm_plx;
	double dpmra = 0, dpmde = 0, dplx = 0, drv = 0;
	if (astrom)
	{
		dpmra = a.pmra_masyr - b.pmra_masyr;
		dpmde = a.pmdec_masyr - b.pmdec_masyr;
		dplx  = a.plx_mas - b.plx_mas;
		if (std::fabs(dpmra) > tol.pm_masyr) { r.bad_dx0++; r.sum_dx0 += std::fabs(dpmra); }
		if (std::fabs(dpmde) > tol.pm_masyr) { r.bad_dx1++; r.sum_dx1 += std::fabs(dpmde); }
		if (std::fabs(dplx) > tol.plx_mas)   { r.bad_plx++; r.sum_plx += std::fabs(dplx); }
	}
	bool rv_ok = a.has_rv && b.has_rv;
	if (rv_ok)
	{
		drv = a.rv_kms - b.rv_kms;
		if (std::fabs(drv) > tol.rv_kms) { r.bad_rv++; r.sum_rv += std::fabs(drv); }
	}
	double dbv = 0;
	bool bv_line = g_compare_bv && !isNoBV(a.bv) && !isNoBV(b.bv);
	if (bv_line) dbv = a.bv - b.bv;

	write_mismatch_line(a.gaia_id, zone, dra, dde, dpmra, dpmde, dvm, dplx, drv, dbv,
	                    astrom, rv_ok, bv_line, file);
}

static std::unordered_map<uint64_t, PhysStar> loadZoneMap(CatReader& cf, int z, int64_t off)
{
	std::unordered_map<uint64_t, PhysStar> map;
	uint32_t cnt = (z < cf.nzones) ? cf.counts[z] : 0;
	if (cnt == 0) return map;
	fseeko(cf.f, off, SEEK_SET);
	std::vector<uint8_t> buf(cf.rec_size);
	for (uint32_t i = 0; i < cnt; ++i)
	{
		std::fread(buf.data(), cf.rec_size, 1, cf.f);
		auto s = decode(cf, buf.data());
		map[s.match_key] = s;
	}
	return map;
}

static void recordOnlyB(const PhysStar& s, int zone, CompareResult& result,
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
 			(unsigned long long)s.match_key, zone, s.ra_deg, s.dec_deg, s.vmag, s.bv);
		if (n > 0) out_file.write(line, n);
	}
}

static bool within_tolerances(const PhysStar& sa, const PhysStar& s, const Tolerances& tol)
{
	bool ok = std::fabs(ra_diff_deg(sa.ra_deg, s.ra_deg)) <= tol.pos_deg &&
	          std::fabs(sa.dec_deg - s.dec_deg) <= tol.pos_deg &&
	          std::fabs(sa.vmag - s.vmag) <= tol.vmag;
	if (ok && sa.has_pm_plx && s.has_pm_plx)
		ok = std::fabs(sa.pmra_masyr - s.pmra_masyr) <= tol.pm_masyr &&
		     std::fabs(sa.pmdec_masyr - s.pmdec_masyr) <= tol.pm_masyr &&
		     std::fabs(sa.plx_mas - s.plx_mas) <= tol.plx_mas;
	if (ok && sa.has_rv && s.has_rv)
		ok = std::fabs(sa.rv_kms - s.rv_kms) <= tol.rv_kms;
	return ok;
}

static void track_bv_diff(const PhysStar& sa, const PhysStar& s, CompareResult& result)
{
	double dbv = sa.bv - s.bv;
	if (std::fabs(dbv) > g_bv_tol) { result.bad_bv++; result.sum_bv += std::fabs(dbv); }
}

static void matchOrRecordB(std::unordered_map<uint64_t, PhysStar>& map_a,
                            const PhysStar& s, int zone, const Tolerances& tol,
                            CompareResult& result,
                            const std::string& out_path, std::ofstream& out_file)
{
	auto it = map_a.find(s.match_key);
	if (it == map_a.end())
	{
		recordOnlyB(s, zone, result, out_path, out_file);
		return;
	}
	const auto& sa = it->second;
	// --compare-bv: track B-V differences for every compared pair (report-only)
	if (g_compare_bv && !isNoBV(sa.bv) && !isNoBV(s.bv))
		track_bv_diff(sa, s, result);
	if (within_tolerances(sa, s, tol))
		result.matched++;
	else
		record_mismatch(sa, s, zone, tol, result,
		                out_path.empty() ? nullptr : &out_file);
	map_a.erase(it);
}

static void processZoneB(std::unordered_map<uint64_t, PhysStar>& map_a, CatReader& b,
                         int z, int64_t off_b, const Tolerances& tol, CompareResult& result,
                         const std::string& out_path, std::ofstream& out_file)
{
	uint32_t cnt_b = (z < b.nzones) ? b.counts[z] : 0;
	if (cnt_b == 0) return;
	fseeko(b.f, off_b, SEEK_SET);
	std::vector<uint8_t> buf(b.rec_size);
	for (uint32_t i = 0; i < cnt_b; ++i)
	{
		std::fread(buf.data(), b.rec_size, 1, b.f);
		auto s = decode(b, buf.data());
		matchOrRecordB(map_a, s, z, tol, result, out_path, out_file);
	}
}

static void writeOnlyA(const std::unordered_map<uint64_t, PhysStar>& map_a,
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
		                      (unsigned long long)gid, z, s.ra_deg, s.dec_deg, s.vmag, s.bv);
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
		auto avg = [](uint64_t n, double sum) -> double { return n ? sum / n : 0; };
		std::cout << "  dRA:   n=" << result.bad_x0 << "  avg|Δ|=" << avg(result.bad_x0, result.sum_x0)
			  << " mas\n";
		std::cout << "  dDE:   n=" << result.bad_x1 << "  avg|Δ|=" << avg(result.bad_x1, result.sum_x1)
			  << " mas\n";
		std::cout << "  dpmra: n=" << result.bad_dx0
			  << "  avg|Δ|=" << avg(result.bad_dx0, result.sum_dx0) << " mas/yr\n";
		std::cout << "  dpmde: n=" << result.bad_dx1
			  << "  avg|Δ|=" << avg(result.bad_dx1, result.sum_dx1) << " mas/yr\n";
		std::cout << "  dV:    n=" << result.bad_vmag
			  << "  avg|Δ|=" << avg(result.bad_vmag, result.sum_vmag) << " mag\n";
		std::cout << "  dPlx:  n=" << result.bad_plx
			  << "  avg|Δ|=" << avg(result.bad_plx, result.sum_plx) << " mas\n";
		if (result.bad_rv > 0)
			std::cout << "  dRV:   n=" << result.bad_rv
				  << "  avg|Δ|=" << avg(result.bad_rv, result.sum_rv) << " km/s\n";
		if (g_compare_bv)
			std::cout << "  dBV:   n=" << result.bad_bv
				  << "  avg|Δ|=" << avg(result.bad_bv, result.sum_bv) << " mag (report-only)\n";
	}
	if (result.only_a == 0 && result.only_b == 0 && result.mismatched == 0)
		std::cout << "\nFiles are IDENTICAL.\n";
	else if (result.mismatched == 0)
		std::cout << "\nID sets IDENTICAL (some stars in different zones).\n";
}

static void compareAllZones(CatReader& a, CatReader& b, CompareResult& result,
                            const std::string& out_path, std::ofstream& out_file)
{
	int nz        = std::max(a.nzones, b.nzones);
	int64_t off_a = a.star_base, off_b = b.star_base;
	Tolerances tol = tolerances_for(a, b);

	for (int z = 0; z < nz; ++z)
	{
		uint32_t cnt_a = (z < a.nzones) ? a.counts[z] : 0;
		uint32_t cnt_b = (z < b.nzones) ? b.counts[z] : 0;

		auto map_a = loadZoneMap(a, z, off_a);
		processZoneB(map_a, b, z, off_b, tol, result, out_path, out_file);
		writeOnlyA(map_a, z, result, out_path, out_file);

		off_a += static_cast<int64_t>(cnt_a) * a.rec_size;
		off_b += static_cast<int64_t>(cnt_b) * b.rec_size;
	}
}

static void compare(const std::string& a_path, const std::string& b_path, const std::string& out_path)
{
	CatReader a, b;
	std::string err;
	if (!a.open(a_path, err)) { std::cerr << "ERROR: " << err << "\n"; return; }
	if (!b.open(b_path, err))
	{
		std::cerr << "ERROR: " << err << "\n";
		a.close();
		return;
	}
	std::cout << "A: level=" << a.level << "  type=Star" << (a.cat_type == 2 ? 3 : 2) << "  stars=" << a.total_stars() << "\n";
	std::cout << "B: level=" << b.level << "  type=Star" << (b.cat_type == 2 ? 3 : 2) << "  stars=" << b.total_stars() << "\n";
	bool coarse = (a.cat_type == 2 || b.cat_type == 2);
	if (coarse)
		std::cout << "Tolerance: ±0.15 arcsec position, ±0.030 mag V (Star3 quantization)\n\n";
	else
		std::cout << "Tolerance: ±3 mas position, ±3 mmag V, ±3 uas/yr pm, ±30 uas plx\n\n";

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
		if (arg == "--compare-bv")
			g_compare_bv = true;
		else if (arg == "--bv-tol" && i + 1 < argc)
			g_bv_tol = std::atof(argv[++i]);
		else if (a.empty())
			a = arg;
		else if (b.empty())
			b = arg;
		else if (out.empty())
			out = arg;
	}
	if (a.empty() || b.empty())
	{
		std::cerr << "Usage: cmpcat <fileA.cat> <fileB.cat> [output.txt] [--compare-bv] [--bv-tol mag]\n";
		return 1;
	}
	compare(a, b, out);
	return 0;
}
