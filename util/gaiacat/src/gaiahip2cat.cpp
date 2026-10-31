// gaiahip2cat: SIMBAD (hip_processed_with_binary.dat) + Gaia .bin -> Stellarium lv0-6 .cat
// Replicates henrysky's Star_Catalog_lv0_6.ipynb offline.
// Usage:
//   gaiahip2cat --simbad <hip_processed_with_binary.dat> --gaia-bin <dir> --out-dir <dir>
//           --otype <otype.dat> [--sp-table <stars_hip_sp.cat>] [--workers n] [--max-files n]
//
// All inputs are local; no network access is needed.

#include "types.hpp"
#include "convert.hpp"
#include "geodesic.hpp"
#include "catalog_naming.hpp"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

// Run a command line without a shell (avoids CWE-78 shell injection).
// Windows: CreateProcessA (no cmd.exe); POSIX: fork + execvp with quoted split.
static std::string quote_arg(const std::string& s)
{
	if (s.find(' ') == std::string::npos && s.find('\t') == std::string::npos)
		return s;
	return "\"" + s + "\"";
}

static int run_command_line(const std::string& cmdline)
{
#ifdef _WIN32
	STARTUPINFOA si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	std::vector<char> buf(cmdline.begin(), cmdline.end());
	buf.push_back('\0');
	if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr, FALSE, 0,
	                    nullptr, nullptr, &si, &pi))
		return -1;
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD code = 1;
	GetExitCodeProcess(pi.hProcess, &code);
	CloseHandle(pi.hProcess);
	return (int)code;
#else
	std::vector<std::string> parts;
	std::string cur;
	bool inq = false;
	for (char ch : cmdline) {
		if (ch == '"') inq = !inq;
		else if (ch == ' ' && !inq) {
			if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
		} else cur += ch;
	}
	if (!cur.empty()) parts.push_back(cur);
	if (parts.empty()) return -1;
	std::vector<char*> argv;
	for (auto& p : parts) argv.push_back(&p[0]);
	argv.push_back(nullptr);
	pid_t pid = fork();
	if (pid < 0) return -1;
	if (pid == 0) { execvp(argv[0], argv.data()); _exit(127); } // flawfinder: ignore -- execvp without a shell; argv is passed directly
	int st = 0;
	waitpid(pid, &st, 0);
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
#endif
}

namespace fs = std::filesystem;

static constexpr double D2R = M_PI / 180.0;
static constexpr double R2D = 180.0 / M_PI;
static constexpr double MAS2RAD = 4.8481368110953594e-9;  // mas to radians
static constexpr double CATALOG_EPOCH_JD = 2457389.0;   // J2016.0

// J-band values for the SIMBAD V fallback chain (indexed by row)
static std::vector<double> g_J;

// ------------------------------------------------------------------ photometry
// g_to_v comes from convert.cpp (henrysky's ADQL v_mag polynomial)

// henrysky py/gaia.py gbprp_to_bv(gmag, bprp, red_correction=True) -> (V, B-V)
static inline void gbprp_to_bv(double g, double bprp, double& v, double& bv)
{
	if (std::isnan(bprp)) { v = g; bv = 0.65; return; }
	v = g - (0.01426 * bprp * bprp * bprp - 0.2156 * bprp * bprp + 0.01424 * bprp - 0.02704);
	double bmag;
	if (bprp > 2.2)   // red end correction for dwarfs and red giants
		bmag = g - (-0.5 * bprp - 1.6);
	else
		bmag = g - (-0.006061 * std::pow(bprp, 4) + 0.06718 * std::pow(bprp, 3)
		            - 0.3604 * bprp * bprp - 0.6874 * bprp + 0.01448);
	bv = bmag - v;
}

// ------------------------------------------------------------------ bin reading
#pragma pack(push, 1)
struct alignas(1) BinRec {
	int64_t sid;
	double  ra, dec, plx;
	float   plxe;
	double  pmra, pmdec;
	float   g, bprp, rv, rve;
};
#pragma pack(pop)
static_assert(sizeof(BinRec) == 68, "bin record must be 68 bytes");

// ------------------------------------------------------------------ star row
struct StarRow {
	int64_t source_id = 0;
	int     hip = 0;
	int     comp = 0;
	double  ra = 0, dec = 0;          // degrees, J2016 after propagation
	double  pmra = 0, pmdec = 0;      // mas/yr, with cos(dec) factor
	double  plx = 0, plxe = 0;        // mas
	double  rv = 0;                   // km/s
	double  vmag = 0, bv = 0.65;
	int     epoch_x10 = 20000;        // 2000.0 or 2016.0
	std::string sp, otype = "*";
};

// Slim row for Gaia-only stars (no HIP/component/epoch/sp/otype: always
// 0/0/J2016/""/"*"). fillna defaults are applied at collection time.
// 64 bytes vs ~160 for StarRow; ~40M of these dominate the memory footprint.
struct GaiaRow {
	int64_t source_id;
	double  ra, dec, pmra, pmdec;     // doubles: mas/uas output precision needed
	float   plx, plxe, rv, vmag, bv;
};

// Reference into either container, used by the per-level selection.
struct RowRef { const StarRow* s; const GaiaRow* g; };
static inline double ref_vmag(const RowRef& r) { return r.s ? r.s->vmag : r.g->vmag; }

// Uniform field view materialized on stack where full access is needed.
struct RowView {
	int64_t source_id; int hip, comp;
	double ra, dec, pmra, pmdec, plx, plxe, rv, vmag, bv;
	const std::string* sp;            // nullptr = empty
	const std::string* otype;
};
static const std::string GAIA_ONLY_OTYPE = "*";
static RowView load_view(const RowRef& r)
{
	if (r.s) {
		const StarRow& s = *r.s;
		return {s.source_id, s.hip, s.comp, s.ra, s.dec, s.pmra, s.pmdec,
		        s.plx, s.plxe, s.rv, s.vmag, s.bv, &s.sp, &s.otype};
	}
	const GaiaRow& g = *r.g;
	return {g.source_id, 0, 0, g.ra, g.dec, g.pmra, g.pmdec,
	        g.plx, g.plxe, g.rv, g.vmag, g.bv, nullptr, &GAIA_ONLY_OTYPE};
}

// ------------------------------------------------------------------ CSV parsing (pandas-style, quote aware)
static std::vector<std::string> csv_split(const std::string& line)
{
	std::vector<std::string> out;
	std::string cur;
	bool in_q = false;
	for (size_t i = 0; i < line.size(); ++i) {
		char c = line[i];
		if (in_q) {
			if (c == '"') {
				if (i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
				else in_q = false;
			} else cur += c;
		} else if (c == '"') {
			in_q = true;
		} else if (c == ',') {
			out.push_back(cur);
			cur.clear();
		} else {
			cur += c;
		}
	}
	out.push_back(cur);
	return out;
}

static inline double to_d(const std::string& s)
{
	if (s.empty()) return NAN;
	return std::strtod(s.c_str(), nullptr);
}

static inline int64_t to_i64(const std::string& s)
{
	if (s.empty()) return 0;
	return std::strtoll(s.c_str(), nullptr, 10);
}

struct CsvCols {
	int hip = -1, comp = -1, sid = -1, ra = -1, dec = -1, plx = -1, plxe = -1;
	int pmra = -1, pmdec = -1, B = -1, V = -1, J = -1, sp = -1, otype = -1;
	int rv = -1, rve = -1;
	bool ok() const { return hip >= 0 && sid >= 0 && ra >= 0 && V >= 0; }
};

static CsvCols find_cols(const std::vector<std::string>& hdr)
{
	static const std::pair<const char*, int CsvCols::*> kColMap[] = {
		{"hip", &CsvCols::hip},
		{"componentid", &CsvCols::comp},
		{"source_id", &CsvCols::sid},
		{"ra", &CsvCols::ra},
		{"dec", &CsvCols::dec},
		{"plx_value", &CsvCols::plx},
		{"plx_err", &CsvCols::plxe},
		{"pmra", &CsvCols::pmra},
		{"pmdec", &CsvCols::pmdec},
		{"B", &CsvCols::B},
		{"V", &CsvCols::V},
		{"J", &CsvCols::J},
		{"sp_type", &CsvCols::sp},
		{"otype", &CsvCols::otype},
		{"rvz_radvel", &CsvCols::rv},
		{"rvz_err", &CsvCols::rve},
	};
	CsvCols c;
	for (size_t i = 0; i < hdr.size(); ++i) {
		for (const auto& kv : kColMap) {
			if (hdr[i] == kv.first) {
				c.*(kv.second) = (int)i;
				break;
			}
		}
	}
	return c;
}

// ------------------------------------------------------------------ space motion
// IAU SOFA starpm: rigid 6D pv-vector propagation matching astropy's
// apply_space_motion (which uses ERFA, a SOFA derivative).
// Constants from IAU SOFA standards (2012).
// Reference: www.iausofa.org / ERFA starpm.c

static constexpr double SOFA_PARSEC_AU = 648000.0 / M_PI;  // AU per parsec (≈206264.806247)
static constexpr double SOFA_KM_AU     = 149597870.7;      // km per AU
static constexpr double SOFA_DY        = 365.25;           // days per Julian year

// Convert Gaia/SIMBAD astrometry at epoch1 (Julian years) to epoch2.
// pmra, pmdec are in mas/yr (Gaia: pmra includes cos(dec) factor).
// plx in mas, rv in km/s. Returns new (ra_deg, dec_deg, pmra_mas, pmdec_mas, plx_mas).
static void iau_starpm(double ra_deg, double dec_deg,
                       double pmra, double pmdec,
                       double plx_mas, double rv_kms,
                       double ep1_jyr, double ep2_jyr,
                       double& ra2, double& dec2,
                       double& pmra2, double& pmdec2, double& plx2)
{
	double dt = ep2_jyr - ep1_jyr;
	double ra = ra_deg * D2R, dec = dec_deg * D2R;
	double pmr = pmra * MAS2RAD;   // rad/yr
	double pmd = pmdec * MAS2RAD;

	// unit position vector
	double sra = std::sin(ra), cra = std::cos(ra);
	double sdec = std::sin(dec), cdec = std::cos(dec);
	double r[3] = {cdec * cra, cdec * sra, sdec};

	// distance and position in AU
	double px_arcsec = plx_mas / 1000.0;
	double dist_pc = (px_arcsec > 1e-12) ? 1.0 / px_arcsec : 1e12;
	double dist_au = dist_pc * SOFA_PARSEC_AU;
	double pos[3], vel[3];
	for (int i = 0; i < 3; ++i) pos[i] = r[i] * dist_au;

	// spatial velocity in AU/yr
	double v_r_yr = rv_kms * (86400.0 * SOFA_DY / SOFA_KM_AU);  // km/s → AU/yr
	for (int i = 0; i < 3; ++i) vel[i] = v_r_yr * r[i];

	// tangential velocity: the pm vector on the sphere is perpendicular to r;
	// its magnitude in rad/yr times distance gives AU/yr in the tangent plane.
	double p_east[3]  = {-sra, cra, 0.0};
	double p_north[3] = {-sdec * cra, -sdec * sra, cdec};
	for (int i = 0; i < 3; ++i) vel[i] += (pmr * p_east[i] + pmd * p_north[i]) * dist_au;

	// propagate
	for (int i = 0; i < 3; ++i) pos[i] += vel[i] * dt;

	// back to spherical
	double d2 = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
	double r2[3]; for (int i = 0; i < 3; ++i) r2[i] = pos[i] / d2;
	double ra_r  = std::atan2(r2[1], r2[0]);
	if (ra_r < 0) ra_r += 2.0 * M_PI;
	ra2    = ra_r * R2D;
	dec2   = std::asin(r2[2]) * R2D;
	plx2   = (px_arcsec > 1e-12) ? SOFA_PARSEC_AU / (d2 / dist_pc) * 1000.0 : 0;
	// (d2/dist_pc gives AU per pc; px2 = 1/(d2/dist_pc) arcsec = dist_pc/d2 arcsec)
	// In mas: plx2_mas = 1000*dist_pc/d2... hmm let me recalculate:
	// plx2 [arcsec] = SOFA_PARSEC_AU / d2
	// plx2_mas = plx2 * 1000 = SOFA_PARSEC_AU * 1000 / d2
	// But for px=0 stars: set to 0.
	plx2   = (px_arcsec > 1e-12) ? SOFA_PARSEC_AU * 1000.0 / d2 : 0;
	(void)plx2; // parallax change over a human lifetime is negligible; don't update

	// new proper motion: reproject velocity minus radial component
	double sra2 = std::sin(ra_r), cra2 = std::cos(ra_r);
	double sdec2 = std::sin(dec2 * D2R), cdec2 = std::cos(dec2 * D2R);
	double p2[3]  = {-sra2, cra2, 0.0};
	double q2[3]  = {-sdec2 * cra2, -sdec2 * sra2, cdec2};
	double vr2    = (vel[0] * r2[0] + vel[1] * r2[1] + vel[2] * r2[2]);     // AU/yr
	double vt_p2  = vel[0] * p2[0] + vel[1] * p2[1];                         // AU/yr (east)
	double vt_q2  = vel[0] * q2[0] + vel[1] * q2[1] + vel[2] * q2[2];       // AU/yr (north)
	pmra2  = vt_p2 / d2 * R2D * 3600.0 * 1000.0;   // mas/yr (with cos(dec2))
	pmdec2 = vt_q2 / d2 * R2D * 3600.0 * 1000.0;
}

// ------------------------------------------------------------------ patches (henrysky lv0_6 cell 2)
static void apply_pre_fusion_patches(std::vector<StarRow>& rows)
{
	// delete HIP 55987B (way too dim)
	rows.erase(std::remove_if(rows.begin(), rows.end(),
	                          [](const StarRow& r) { return r.hip == 55987 && r.comp == 2; }),
	           rows.end());

	auto has_sid = [&](int64_t sid) {
		for (const auto& r : rows) if (r.source_id == sid) return true;
		return false;
	};
	// add HIP 81693 B
	if (!has_sid(1312665361414730624))
		rows.push_back({1312665361414730624, 81693, 2, 0, 0, 0, 0, 93.32, 0.47, 0, NAN, NAN, 20000, "", "*"});
	// add HIP 84012 B
	if (!has_sid(4136366183978856960))
		rows.push_back({4136366183978856960, 84012, 2, 0, 0, 0, 0, 36.91, 0.80, 0, NAN, NAN, 20000, "", "*"});
	// add HIP 36850 B
	if (!has_sid(892348694913501952))
		rows.push_back({892348694913501952, 36850, 2, 0, 0, 0, 0, 66.356, 0.041, 0, NAN, NAN, 20000, "", "*"});
	// HD 183559B (formerly "HIP 95759B" in the official-era SIMBAD snapshot):
	// SIMBAD dropped the HIP alias for this component, and its Gaia DR3 source
	// has no G-band photometry (close-double blend), so neither the SIMBAD path
	// (renamed away) nor the Gaia-only path (requires G) can reach it. Re-add
	// manually: astrometry from Gaia DR3, photometry from the official record.
	if (!has_sid(2051732716553182336))
		rows.push_back({2051732716553182336, 95759, 2,
		                292.15690538984126, 37.18856842370232,
		                4.491698373297273, -21.449247959590082,
		                4.0750884647101415, 0.08124616742134094,
		                NAN, 9.620, 0.340, 20160, "", "*"});

	// HIP 84345: existing single becomes B component; add A component manually
	for (auto& r : rows)
		if (r.hip == 84345 && (r.comp == 0 || r.comp == 1)) r.comp = 2;
	bool has84345A = false;
	for (const auto& r : rows) if (r.hip == 84345 && r.comp == 1) has84345A = true;
	if (!has84345A) {
		StarRow a;
		a.source_id = 0; a.hip = 84345; a.comp = 1;
		a.ra = 258.661886421; a.dec = 14.390371047;
		a.plx = 9.08; a.plxe = 1.32;
		a.pmra = -17.0; a.pmdec = 47.0;
		a.rv = -33.1;
		a.vmag = 3.33; a.bv = 4.67 - 3.33;
		a.sp = "M5Ib-II"; a.otype = "*";
		a.epoch_x10 = 20000;
		rows.push_back(a);
	}
}

static void apply_post_fusion_patches(std::vector<StarRow>& rows)
{
	// HIP 55987A: SIMBAD has no source_id; fill with Gaia DR3 5236791746461774080 data.
	// (henrysky sets "pndec" instead of "pmdec" by typo: pmdec stays unset; reproduced.)
	for (auto& r : rows) {
		if (r.hip == 55987 && r.comp == 1) {
			r.source_id = 5236791746461774080;
			r.vmag = 15.843829;
			r.bv = 16.615616 - 15.843829;
			r.ra = 172.11765151979628; r.dec = -66.48608788896621;
			r.pmra = -8.85141903479891;
			r.plx = 0.6125993002975423; r.plxe = 0.0275;
			r.rv = 0.0;
			r.epoch_x10 = 20160;
		}
	}
	// HIP 81693A source_id + component A
	for (auto& r : rows)
		if (r.hip == 81693 && r.comp == 0) { r.source_id = 1312665361415345920; r.comp = 1; }
	// HIP 84012 / 36850 -> A components
	for (auto& r : rows) {
		if (r.hip == 84012 && r.comp == 0) r.comp = 1;
		if (r.hip == 36850 && r.comp == 0) r.comp = 1;
	}
}

static void apply_final_patches(std::vector<StarRow>& rows)
{
	// Ross 248 is of interest in the future solar neighborhood: force into first 3 catalogs
	for (auto& r : rows) {
		if (r.source_id == 1926461164913660160) {
			r.hip = 9999999; r.otype = "BY*"; r.sp = "M5.0V";
		}
	}
	auto fix_plx = [&](int hip, double plx, double plxe) {
		for (auto& r : rows) if (r.hip == hip) { r.plx = plx; r.plxe = plxe; }
	};
	fix_plx(36850, 64.12, 3.75);
	fix_plx(84345, 9.07, 1.32);
	fix_plx(110900, 14.5943, 0.1562);
	fix_plx(54844, 18.35, 0.96);
	fix_plx(55203, 114.4867, 0.4316);
	// two Gaia stars missing astrometry in the ADQL extract
	for (auto& r : rows) {
		if (r.source_id == 756853643637996160) {
			r.plx = 114.4867; r.plxe = 0.4316;
			r.pmra = -339.398; r.pmdec = -607.892;
			r.rv = -15.9; r.sp = "G2V"; r.bv = 5.41 - 4.77;
		}
		if (r.source_id == 3683687763520080256) {
			r.plx = 78.5233; r.plxe = 1.3879;
			r.pmra = -534.318; r.pmdec = -64.270;
			r.rv = -19.8; r.sp = "F0mF2V"; r.bv = 3.85 - 3.49;
		}
	}
}

// ------------------------------------------------------------------ table IO helpers
static std::vector<std::string> read_text_lines(const std::string& path)
{
	std::vector<std::string> lines;
	std::ifstream f(path);
	std::string line;
	while (std::getline(f, line)) {
		if (!line.empty() && line.back() == '\r') line.pop_back();
		lines.push_back(line);
	}
	return lines;
}

// ------------------------------------------------------------------ main
int main(int argc, char** argv)
{
	std::string simbad_path, bin_dir, out_dir, otype_path, sp_path;
	std::string stars_config = default_stars_config_path();
	int n_workers = (int)std::min(4u, std::thread::hardware_concurrency());
	long max_files = -1;
	std::string erfa_python;
	int level_lo = 0, level_hi = 3;   // default: lv0-3 (Star1), gaiahip2cat's primary purpose
	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if      (a == "--simbad"    && i + 1 < argc) simbad_path = argv[++i];
		else if (a == "--gaia-bin"  && i + 1 < argc) bin_dir = argv[++i];
		else if (a == "--out-dir"   && i + 1 < argc) out_dir = argv[++i];
		else if (a == "--otype"     && i + 1 < argc) otype_path = argv[++i];
		else if (a == "--sp-table"  && i + 1 < argc) sp_path = argv[++i];
		else if (a == "--stars-config" && i + 1 < argc) stars_config = argv[++i];
		else if (a == "--workers"   && i + 1 < argc) n_workers = std::atoi(argv[++i]);
		else if (a == "--max-files" && i + 1 < argc) max_files = std::atol(argv[++i]);
		else if (a == "--erfa-python" && i + 1 < argc) erfa_python = argv[++i];
		else if (a == "--levels"    && i + 1 < argc) {
			std::string spec = argv[++i];
			if (std::sscanf(spec.c_str(), "%d-%d", &level_lo, &level_hi) < 2) {
				if (std::sscanf(spec.c_str(), "%d", &level_lo) != 1) {
					std::cerr << "ERROR: bad --levels spec\n"; return 1;
				}
				level_hi = level_lo;
			}
		}
		else {
			std::cerr << "Usage: gaiahip2cat --simbad <dat> --gaia-bin <dir> --out-dir <dir> --otype <dat>"
				" [--sp-table f] [--workers n] [--levels 0-3] [--max-files n] [--erfa-python <path>]\n";
			return 1;
		}
	}
	if (simbad_path.empty() || bin_dir.empty() || out_dir.empty() || otype_path.empty()) {
		std::cerr << "Usage: gaiahip2cat --simbad <dat> --gaia-bin <dir> --out-dir <dir> --otype <dat>"
			" [--sp-table f] [--workers n] [--levels 0-3] [--max-files n] [--erfa-python <path>]\n";
		return 1;
	}
	if (level_lo < 0 || level_hi > 6 || level_lo > level_hi) {
		std::cerr << "ERROR: --levels must satisfy 0 <= lo <= hi <= 6\n"; return 1;
	}
	if (!stars_config.empty() && !fs::exists(stars_config))
		std::cerr << "WARNING: starsConfig.json not found at " << stars_config
		          << " -- using default catalog naming\n";
	fs::create_directories(out_dir);

	// ---- level table
	struct LvCfg { int level; double lo, hi; uint32_t type; uint32_t minor; };
	const LvCfg lvcs[7] = {
		{0, -2.0, 6.0,  0, 21},
		{1,  6.0, 7.5,  0, 16},
		{2,  7.5, 9.0,  0, 17},
		{3,  9.0, 10.5, 0, 10},
		{4, 10.5, 12.0, 1, 6},
		{5, 12.0, 13.75,1, 6},
		{6, 13.75,15.5, 1, 4},
	};
	// Gaia-only stars are only collected up to the brightest hi of the requested
	// levels (default lv0-3: 10.5, not the ADQL 15.5) — this is what keeps the
	// memory footprint small (~0.4M rows for lv0-3 instead of ~40M).
	double collect_hi = 0;
	for (const auto& lc : lvcs)
		if (lc.level >= level_lo && lc.level <= level_hi)
			collect_hi = std::max(collect_hi, lc.hi);
	collect_hi += 1e-4;   // guard against double->float vmag rounding at the boundary
	if (level_lo <= 2 && erfa_python.empty())
		std::cerr << "WARNING: --erfa-python not given: lv0-2 will use direction-only zones"
		          " and will NOT match the official catalogs' global zone.\n";

	// ---- object type table (line index = objtype, line 0 = "")
	// Entries in the official table are whitespace-padded (e.g. "*  ", "PN "):
	// trim for lookup or every padded type would be re-appended at the end.
	auto trim = [](std::string& s) {
		size_t e = s.find_last_not_of(" \t");
		if (e != std::string::npos) s.resize(e + 1);
	};
	std::vector<std::string> otype_ls = read_text_lines(otype_path);
	for (auto& s : otype_ls) trim(s);
	std::unordered_map<std::string, int> otype_idx;
	for (size_t i = 0; i < otype_ls.size(); ++i) otype_idx[otype_ls[i]] = (int)i;
	std::cout << "otype table: " << otype_ls.size() << " entries\n";

	// ---- spectral type table: reuse if given, else build fresh
	std::vector<std::string> sp_ls;
	bool sp_reuse = false;
	int sp_appended = 0;
	if (!sp_path.empty() && fs::exists(sp_path)) {
		sp_ls = read_text_lines(sp_path);
		for (auto& s : sp_ls) trim(s);
		sp_reuse = true;
		std::cout << "sp table (reused): " << sp_ls.size() << " entries\n";
	} else {
		sp_ls.push_back("");   // index 0 = no information
	}
	std::unordered_map<std::string, int> sp_idx;
	for (size_t i = 0; i < sp_ls.size(); ++i) sp_idx[sp_ls[i]] = (int)i;

	// ---- read SIMBAD table
	std::vector<StarRow> rows;
	{
		std::ifstream f(simbad_path);
		if (!f) { std::cerr << "cannot open " << simbad_path << "\n"; return 1; }
		std::string line;
		std::getline(f, line);
		CsvCols cc = find_cols(csv_split(line));
		if (!cc.ok()) { std::cerr << "missing columns in " << simbad_path << "\n"; return 1; }
		while (std::getline(f, line)) {
			if (line.empty()) continue;
			auto fld = csv_split(line);
			StarRow r;
			r.hip       = (int)to_i64(fld[cc.hip]);
			r.comp      = cc.comp >= 0 ? (int)to_i64(fld[cc.comp]) : 0;
			r.source_id = to_i64(fld[cc.sid]);
			r.ra        = to_d(fld[cc.ra]);
			r.dec       = to_d(fld[cc.dec]);
			r.plx       = cc.plx  >= 0 ? to_d(fld[cc.plx])  : NAN;
			r.plxe      = cc.plxe >= 0 ? to_d(fld[cc.plxe]) : NAN;
			r.pmra      = cc.pmra >= 0 ? to_d(fld[cc.pmra]) : NAN;
			r.pmdec     = cc.pmdec>= 0 ? to_d(fld[cc.pmdec]): NAN;
			double B    = cc.B >= 0 ? to_d(fld[cc.B]) : NAN;
			r.vmag      = to_d(fld[cc.V]);
			double J    = cc.J >= 0 ? to_d(fld[cc.J]) : NAN;
			r.bv        = (!std::isnan(B) && !std::isnan(r.vmag)) ? B - r.vmag : NAN;
			if (cc.sp >= 0)     r.sp = fld[cc.sp];
			if (cc.otype >= 0 && !fld[cc.otype].empty()) r.otype = fld[cc.otype];
			r.rv        = cc.rv >= 0 ? to_d(fld[cc.rv]) : NAN;
			// stash B_J for the V fallback chain in vmag's place-holder? no: keep J in plxe slot? use static map
			// We need J later: store in a side vector keyed by row index
			r.epoch_x10 = 20000;
			rows.push_back(std::move(r));
			g_J.push_back(J);
		}
	}
	std::cout << "SIMBAD rows: " << rows.size() << "\n";

	apply_pre_fusion_patches(rows);
	while (g_J.size() < rows.size()) g_J.push_back(NAN);   // align after row additions

	// ---- scan Gaia bin: join SIMBAD ids + collect Gaia-only V<15.5 stars
	std::unordered_set<int64_t> simbad_ids;
	for (const auto& r : rows) if (r.source_id > 0) simbad_ids.insert(r.source_id);

	std::vector<std::string> bin_files;
	for (const auto& e : fs::directory_iterator(bin_dir))
		if (e.path().extension() == ".bin") bin_files.push_back(e.path().string());
	std::sort(bin_files.begin(), bin_files.end());
	if (max_files > 0 && (long)bin_files.size() > max_files) bin_files.resize(max_files);
	std::cout << "bin files: " << bin_files.size() << " (" << n_workers << " workers)\n";

	struct GaiaHit { double ra, dec, plx, plxe, pmra, pmdec, g, bprp, rv; };
	std::vector<std::unordered_map<int64_t, GaiaHit>> hits(n_workers);
	std::vector<std::vector<GaiaRow>> gaia_only(n_workers);
	std::cout << "Gaia-only collection cut: V <= " << collect_hi << "\n";

	{
		std::atomic<size_t> next{0};
		std::atomic<int> done{0};
		auto t0 = std::chrono::steady_clock::now();
		std::vector<std::thread> pool;
		for (int t = 0; t < n_workers; ++t) {
			pool.emplace_back([&, t]() {
				while (true) {
					size_t fi = next.fetch_add(1);
					if (fi >= bin_files.size()) break;
					FILE* f = std::fopen(bin_files[fi].c_str(), "rb");
					if (!f) { done++; continue; }
					BinRec rec;
					while (std::fread(&rec, 68, 1, f) == 1) {
						if (rec.sid == 0 || std::isnan(rec.g)) continue;
						double bprp = std::isnan(rec.bprp) ? NAN : (double)rec.bprp;
						double v = g_to_v(rec.g, bprp);
						// ADQL selection applies to every row: (bp_rp NOT NULL AND v<=15.5)
						// OR (bp_rp NULL AND G<=15.5). Stars beyond 15.5 have no Gaia row in
						// henrysky's table, so they must not be fused either (a SIMBAD xmatch
						// may point at a faint/wrong neighbour, e.g. 8 Peg).
						bool sel = std::isnan(bprp) ? (rec.g <= 15.5) : (v <= 15.5);
						if (simbad_ids.count(rec.sid)) {
							if (!sel) continue;
							GaiaHit h{rec.ra, rec.dec, rec.plx, (double)rec.plxe,
							          rec.pmra, rec.pmdec, (double)rec.g, bprp, (double)rec.rv};
							hits[t][rec.sid] = h;
							continue;
						}
						if (!sel) continue;
						// Only collect up to the brightest requested level's hi
						// (v == final vmag for Gaia-only rows). Exception: Ross 248
						// (V=12.4) is forced into lv0-2 by henrysky with a fake HIP,
						// so it must survive even the lv0-3 collection cut.
						if (v > collect_hi && rec.sid != 1926461164913660160) continue;
						GaiaRow gr;
						gr.source_id = rec.sid;
						gr.ra = rec.ra; gr.dec = rec.dec;
						gr.pmra = std::isnan(rec.pmra) ? 0.0 : rec.pmra;
						gr.pmdec = std::isnan(rec.pmdec) ? 0.0 : rec.pmdec;
						double plx = std::isnan(rec.plx) ? 0.0 : rec.plx;
						double plxe = std::isnan(rec.plxe) ? 0.0 : (double)rec.plxe;
						if (plx < 0) { plx = 0; plxe = 0; }
						gr.plx = (float)plx; gr.plxe = (float)plxe;
						gr.rv = std::isnan(rec.rv) ? 0.0f : rec.rv;
						double vv, bv;
						gbprp_to_bv(rec.g, bprp, vv, bv);
						gr.vmag = (float)vv; gr.bv = (float)bv;
						gaia_only[t].push_back(gr);
					}
					std::fclose(f);
					done++;
				}
			});
		}
		// progress reporter (flushed, visible in redirected logs)
		std::thread prog([&]() {
			while (done.load() < (int)bin_files.size()) {
				std::this_thread::sleep_for(std::chrono::seconds(5));
				double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
				size_t nh = 0, ng = 0;
				for (auto& h : hits) nh += h.size();
				for (auto& g : gaia_only) ng += g.size();
				std::cout << "  [scan " << done.load() << "/" << bin_files.size() << "] "
					  << el << "s, hits=" << nh << ", gaia-only=" << ng << std::endl;
			}
		});
		for (auto& th : pool) th.join();
		prog.join();
	}
	size_t n_hits = 0, n_gaia_only = 0;
	for (auto& h : hits) n_hits += h.size();
	for (auto& g : gaia_only) n_gaia_only += g.size();
	std::cout << "Gaia join hits: " << n_hits << " / " << simbad_ids.size()
		  << ", Gaia-only stars: " << n_gaia_only << "\n";

	// ---- fusion: Gaia astrometry overrides SIMBAD for matched rows
	{
		std::unordered_map<int64_t, GaiaHit> all_hits;
		for (auto& h : hits) all_hits.merge(h);
		for (auto& r : rows) {
			auto it = all_hits.find(r.source_id);
			if (it == all_hits.end()) continue;
			const GaiaHit& h = it->second;
			r.ra = h.ra; r.dec = h.dec;
			if (!std::isnan(h.plx)) { r.plx = h.plx; r.plxe = h.plxe; }
			if (!std::isnan(h.pmra) && !std::isnan(h.pmdec)) { r.pmra = h.pmra; r.pmdec = h.pmdec; }
			r.epoch_x10 = 20160;
			// V chain: SIMBAD V -> Gaia v_mag -> Gaia G
			if (std::isnan(r.vmag)) {
				if (!std::isnan(h.bprp)) r.vmag = g_to_v(h.g, h.bprp);
				else r.vmag = h.g;
			}
			// B-V chain: SIMBAD B-V -> Gaia_B_V
			if (std::isnan(r.bv)) {
				double vv, bv;
				gbprp_to_bv(h.g, h.bprp, vv, bv);
				r.bv = bv;
			}
		}
	}

	apply_post_fusion_patches(rows);

	// V chain continues: J, then J+3
	for (size_t i = 0; i < rows.size(); ++i) {
		if (std::isnan(rows[i].vmag) && !std::isnan(g_J[i])) rows[i].vmag = g_J[i];
		if (std::isnan(rows[i].vmag) && !std::isnan(g_J[i])) rows[i].vmag = g_J[i] + 3.0;
	}
	// drop componentid >= 2 without V; drop rows still without V at all
	rows.erase(std::remove_if(rows.begin(), rows.end(),
	                          [](const StarRow& r) { return (r.comp >= 2 && std::isnan(r.vmag)) || std::isnan(r.vmag); }),
	           rows.end());
	// negative parallax -> NaN
	for (auto& r : rows) {
		if (!std::isnan(r.plx) && r.plx < 0) { r.plx = NAN; r.plxe = NAN; }
	}

	// ---- propagate SIMBAD-epoch (J2000) rows with pm to J2016.0
	{
		std::atomic<size_t> idx{0};
		std::vector<std::thread> pool;
		for (int t = 0; t < n_workers; ++t) {
			pool.emplace_back([&]() {
				while (true) {
					size_t i = idx.fetch_add(1);
					if (i >= rows.size()) break;
					StarRow& r = rows[i];
					bool have_pm = !std::isnan(r.pmra) && !std::isnan(r.pmdec);
					if (!have_pm) { r.epoch_x10 = 20160; continue; }
					if (r.epoch_x10 == 20160) continue;
					double plx_fill = std::isnan(r.plx) ? 1.0 : r.plx;   // assume 1 kpc if missing
					double rv_fill  = std::isnan(r.rv)  ? 0.0 : r.rv;
					double _ra, _dec, _pmra2, _pmdec2, _plx2;
					iau_starpm(r.ra, r.dec, r.pmra, r.pmdec, plx_fill, rv_fill,
					           r.epoch_x10 / 10.0, 2016.0, _ra, _dec, _pmra2, _pmdec2, _plx2);
					r.ra = _ra; r.dec = _dec;
					r.epoch_x10 = 20160;
				}
			});
		}
		for (auto& th : pool) th.join();
	}

	// ---- Gaia-only stars stay in their own slim container (no merge into rows):
	// per-level selection iterates both containers via RowRef.
	apply_final_patches(rows);

	// Ross 248 has no HIP number, so it normally arrives via the Gaia-only
	// container rather than the SIMBAD rows. Move it into rows so the
	// apply_final_patches renaming (fake HIP 9999999) and the lv2 all-HIP
	// inclusion rule apply to it.
	{
		bool in_rows = false;
		for (const auto& r : rows) if (r.source_id == 1926461164913660160) { in_rows = true; break; }
		if (!in_rows) {
			for (auto& w : gaia_only) {
				auto it = std::find_if(w.begin(), w.end(), [](const GaiaRow& g) {
					return g.source_id == 1926461164913660160;
				});
				if (it == w.end()) continue;
				StarRow r;
				r.source_id = it->source_id;
				r.ra = it->ra; r.dec = it->dec;
				r.pmra = it->pmra; r.pmdec = it->pmdec;
				r.plx = it->plx; r.plxe = it->plxe;
				r.rv = it->rv; r.vmag = it->vmag; r.bv = it->bv;
				r.epoch_x10 = 20160;
				r.hip = 9999999; r.otype = "BY*"; r.sp = "M5.0V";
				rows.push_back(r);
				w.erase(it);
				n_gaia_only--;
				std::cout << "Ross 248: moved from Gaia-only to rows (fake HIP)\n";
				break;
			}
		}
	}

	// Gaia star 3683687763520080256: no 5-parameter astrometry in DR3 (2-param
	// solution), and it is not a SIMBAD row either, so it arrives via gaia_only.
	// henrysky patches it on the combined table; replicate by moving it into
	// rows with the same patched values (also carries its spectral type).
	{
		bool in_rows = false;
		for (const auto& r : rows) if (r.source_id == 3683687763520080256) { in_rows = true; break; }
		if (!in_rows) {
			for (auto& w : gaia_only) {
				auto it = std::find_if(w.begin(), w.end(), [](const GaiaRow& g) {
					return g.source_id == 3683687763520080256;
				});
				if (it == w.end()) continue;
				StarRow r;
				r.source_id = it->source_id;
				r.ra = it->ra; r.dec = it->dec;
				r.plx = 78.5233; r.plxe = 1.3879;
				r.pmra = -534.318; r.pmdec = -64.270;
				r.rv = -19.8;
				r.vmag = it->vmag; r.bv = 3.85 - 3.49;
				r.sp = "F0mF2V";
				r.epoch_x10 = 20160;
				rows.push_back(r);
				w.erase(it);
				n_gaia_only--;
				std::cout << "Gaia 3683687763520080256: moved from Gaia-only to rows (astrometry patch)\n";
				break;
			}
		}
	}

	// ---- drop Gaia-only duplicates of SIMBAD rows that received their source_id
	// only after the scan (post-fusion patches, e.g. HIP 81693A): the scan had
	// already collected the genuine Gaia star, so it would appear twice.
	{
		std::unordered_set<int64_t> final_sids;
		for (const auto& r : rows) if (r.source_id > 0) final_sids.insert(r.source_id);
		size_t dropped = 0;
		for (auto& w : gaia_only) {
			auto it = std::remove_if(w.begin(), w.end(), [&](const GaiaRow& g) {
				return final_sids.count(g.source_id) != 0;
			});
			dropped += (size_t)(w.end() - it);
			w.erase(it, w.end());
		}
		n_gaia_only -= dropped;
		if (dropped) std::cout << "dropped " << dropped << " Gaia-only duplicates of patched SIMBAD rows\n";
	}

	// fillna defaults
	for (auto& r : rows) {
		if (std::isnan(r.bv)) r.bv = 0.65;
		if (std::isnan(r.plx)) r.plx = 0;
		if (std::isnan(r.plxe)) r.plxe = 0;
		if (std::isnan(r.pmra)) r.pmra = 0;
		if (std::isnan(r.pmdec)) r.pmdec = 0;
		if (std::isnan(r.rv)) r.rv = 0;
		if (r.plx < 0) { r.plx = 0; r.plxe = 0; }
	}
	std::cout << "SIMBAD rows: " << rows.size() << ", Gaia-only rows: " << n_gaia_only << "\n";

	for (const auto& lc : lvcs) {
		if (lc.level < level_lo || lc.level > level_hi) continue;
		// selection (henrysky inequalities: vmag > lo && vmag <= hi)
		// Gaia-only rows have hip == 0, so their condition reduces to the plain
		// magnitude range for every level (the lv2 hip>0 clause never matches).
		std::vector<RowRef> sel;
		for (const auto& r : rows) {
			bool in;
			if (lc.level == 2)
				in = (r.vmag > lc.lo && r.hip > 0) || (r.vmag > lc.lo && r.vmag <= lc.hi);
			else if (lc.level <= 1)
				in = (r.vmag > lc.lo && r.vmag <= lc.hi);
			else
				in = (r.vmag > lc.lo && r.vmag <= lc.hi && r.hip == 0);
			if (in) sel.push_back({&r, nullptr});
		}
		for (const auto& w : gaia_only)
			for (const auto& g : w)
				if (g.vmag > lc.lo && g.vmag <= lc.hi) sel.push_back({nullptr, &g});

		// zone assignment (+ past/future global-zone analysis for lv0-2)
		std::vector<uint32_t> zones(sel.size(), 0);
		int global_zone = 20 * (1 << (lc.level * 2));

		// Basic zone computation (direction only, fast)
		for (size_t i = 0; i < sel.size(); ++i) {
			double ra = sel[i].s ? sel[i].s->ra : sel[i].g->ra;
			double dec = sel[i].s ? sel[i].s->dec : sel[i].g->dec;
			double x = std::cos(ra * D2R) * std::cos(dec * D2R);
			double y = std::sin(ra * D2R) * std::cos(dec * D2R);
			double z = std::sin(dec * D2R);
			zones[i] = (uint32_t)zone_number(x, y, z, lc.level);
		}

		if (lc.level <= 2) {
			if (!erfa_python.empty()) {
				// ERFA zone analysis via Python (astropy, exact match to official pipeline)
				std::string script = (fs::path(argv[0]).parent_path() / ".." / "python" / "erfa_zone.py").string();
				std::string tmp_in = out_dir + "/erfa_lv" + std::to_string(lc.level) + "_in.bin";
				std::string tmp_out = out_dir + "/erfa_lv" + std::to_string(lc.level) + "_out.bin";
				{
					FILE* ef = std::fopen(tmp_in.c_str(), "wb");
					uint32_t n = (uint32_t)sel.size();
					std::fwrite(&n, 4, 1, ef);
					for (const auto& rf : sel) {
						RowView v = load_view(rf);
						double d[7] = {v.ra, v.dec, v.pmra, v.pmdec, v.plx, v.rv, v.plxe};
						std::fwrite(d, 8, 7, ef);
					}
					std::fclose(ef);
				}
				std::string cmd = quote_arg(erfa_python) + " " + quote_arg(script) + " "
				                  + quote_arg(tmp_in) + " " + quote_arg(tmp_out) + " "
				                  + std::to_string(lc.level);
				int rc = run_command_line(cmd);
				if (rc == 0) {
					FILE* rf = std::fopen(tmp_out.c_str(), "rb");
					if (rf) {
						uint64_t n_global = 0;
						for (size_t i = 0; i < sel.size(); ++i) {
							uint8_t flag; std::fread(&flag, 1, 1, rf);
							if (flag) { zones[i] = (uint32_t)global_zone; n_global++; }
						}
						std::fclose(rf);
						std::cout << "  lv" << lc.level << " (ERFA): " << n_global << " global\n";
					}
				} else {
					std::cerr << "WARNING: erfa_zone.py failed (rc=" << rc << "), zones not updated\n";
				}
				std::remove(tmp_in.c_str()); std::remove(tmp_out.c_str());
			} else {
				std::cerr << "WARNING: lv" << lc.level << " written WITHOUT --erfa-python:"
				          " no past/future global-zone analysis, zones will NOT match the"
				          " official catalog (direction-only).\n";
			}
		}

		// sort by (zone, vmag)
		std::vector<size_t> order(sel.size());
		for (size_t i = 0; i < sel.size(); ++i) order[i] = i;
		std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
			if (zones[a] != zones[b]) return zones[a] < zones[b];
			return ref_vmag(sel[a]) < ref_vmag(sel[b]);
		});

		// zone counts
		int n_zones = global_zone + 1;
		std::vector<uint32_t> counts(n_zones, 0);
		for (size_t i = 0; i < sel.size(); ++i) counts[zones[i]]++;

		// write .cat: name = official fileName from starsConfig.json, minor + 1
		const std::string official = official_cat_filename(stars_config, lc.level);
		const std::string fname = next_cat_filename(lc.level, lc.type, official);
		int h_type = -1, h_major = -1, h_minor = -1;
		parse_cat_version(fname, &h_type, &h_major, &h_minor);
		const uint32_t major = (uint32_t)std::max(0, h_major);
		const uint32_t minor = (uint32_t)std::max(0, h_minor);
		std::string out_path = out_dir + "/" + fname;
		FILE* f = std::fopen(out_path.c_str(), "wb");
		if (!f) { std::cerr << "cannot write " << out_path << "\n"; return 1; }
		uint32_t header[6] = {FILE_MAGIC, lc.type, major, minor, (uint32_t)lc.level,
		                      (uint32_t)(int)std::lround(lc.lo * 1000.0)};
		std::fwrite(header, 4, 6, f);
		float epoch = (float)CATALOG_EPOCH_JD;
		std::fwrite(&epoch, 4, 1, f);
		std::fwrite(counts.data(), 4, n_zones, f);

		for (size_t oi : order) {
			RowView r = load_view(sel[oi]);
			if (lc.type == 0) {
				CatRecord1 cr{};
				cr.gaia_id = r.source_id;
				double ra = r.ra * D2R, dec = r.dec * D2R;
				double sra = std::sin(ra), cra = std::cos(ra);
				double sdec = std::sin(dec), cdec = std::cos(dec);
				cr.x0 = (int32_t)std::lround(cdec * cra * 2e9);
				cr.x1 = (int32_t)std::lround(cdec * sra * 2e9);
				cr.x2 = (int32_t)std::lround(sdec * 2e9);
				double pmra = r.pmra, pmdec = r.pmdec;   // with cos(dec), mas/yr
				cr.dx0 = (int32_t)std::lround((pmra * -sra + pmdec * -sdec * cra) * 1000.0);
				cr.dx1 = (int32_t)std::lround((pmra *  cra + pmdec * -sdec * sra) * 1000.0);
				cr.dx2 = (int32_t)std::lround((pmdec * cdec) * 1000.0);
				cr.b_v = (int16_t)std::lround(r.bv * 1000.0);
				cr.vmag = (int16_t)std::lround(r.vmag * 1000.0);
				cr.plx = (uint16_t)std::max(0, std::min((int)std::lround(r.plx * 50.0), 65535));
				cr.plx_err = (uint16_t)std::max(0, std::min((int)std::lround(r.plxe * 100.0), 65535));
				cr.rv = (int16_t)std::max(-32768, std::min((int)std::lround(r.rv * 10.0), 32767));
				// spectral type index: unknown types are appended even when
				// reusing the official table (SIMBAD revises sp strings over
				// time, e.g. "M0" -> "M0.0III"); existing indices are unchanged.
				int sp_i = 0;
				if (r.sp && !r.sp->empty()) {
					const std::string& sp = *r.sp;
					auto it = sp_idx.find(sp);
					if (it != sp_idx.end()) {
						sp_i = it->second;
					} else {
						sp_i = (int)sp_ls.size();
						sp_idx[sp] = sp_i;
						sp_ls.push_back(sp);
						sp_appended++;
					}
				}
				cr.spInt = (uint16_t)sp_i;
				int ot_i = 0;
				const std::string& ot = *r.otype;
				auto oit = otype_idx.find(ot);
				if (oit != otype_idx.end()) ot_i = oit->second;
				else {
					ot_i = (int)otype_ls.size();
					otype_idx[ot] = ot_i;
					otype_ls.push_back(ot);
					std::cout << "  NOTE: appended otype '" << ot << "' at index " << ot_i << "\n";
				}
				cr.objtype = (uint8_t)ot_i;
				int hip_pack = (r.hip == 9999999) ? 0 : r.hip;
				uint32_t comb = ((uint32_t)hip_pack << 5) | (uint32_t)(r.comp & 31);
				cr.hip[0] = (uint8_t)comb;
				cr.hip[1] = (uint8_t)(comb >> 8);
				cr.hip[2] = (uint8_t)(comb >> 16);
				std::fwrite(&cr, sizeof(CatRecord1), 1, f);
			} else {
				CatRecord cr{};
				cr.gaia_id = r.source_id;
				cr.x0 = (int32_t)std::lround(r.ra * 3600000.0);
				cr.x1 = (int32_t)std::lround(r.dec * 3600000.0);
				double cdec = std::cos(r.dec * D2R);
				cr.dx0 = (int32_t)std::lround(r.pmra * 1000.0 / std::max(0.00001, cdec));
				cr.dx1 = (int32_t)std::lround(r.pmdec * 1000.0);
				cr.b_v = (int16_t)std::lround(r.bv * 1000.0);
				cr.vmag = (int16_t)std::lround(r.vmag * 1000.0);
				cr.plx = (uint16_t)std::max(0, std::min((int)std::lround(r.plx * 100.0), 65535));
				cr.plx_err = (uint16_t)std::max(0, std::min((int)std::lround(r.plxe * 100.0), 65535));
				std::fwrite(&cr, sizeof(CatRecord), 1, f);
			}
		}
		std::fclose(f);
		std::cout << "  wrote " << fname << ": " << sel.size() << " stars\n";
	}

	// write spectral type table if we built it fresh or appended new types
	if (!sp_reuse || sp_appended > 0) {
		const std::string sp_name = next_sp_filename(official_sp_filename(stars_config));
		std::string sp_out = out_dir + "/" + sp_name;
		std::ofstream f(sp_out, std::ios::binary);
		f << "\n";
		for (size_t i = 1; i < sp_ls.size(); ++i) f << sp_ls[i] << "\n";
		std::cout << "wrote " << sp_out << " (" << (sp_ls.size() - 1) << " types, "
		          << sp_appended << " appended)\n";
	}

	std::cout << "Done.\n";
	return 0;
}
