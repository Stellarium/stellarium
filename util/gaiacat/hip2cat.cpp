// hip2cat: SIMBAD (hip_processed_with_binary.dat) + Gaia .bin -> Stellarium lv0-6 .cat
// Replicates henrysky's Star_Catalog_lv0_6.ipynb offline.
// Usage:
//   hip2cat --simbad <hip_processed_with_binary.dat> --gaia-bin <dir> --out-dir <dir>
//           --otype <otype.dat> [--sp-table <stars_hip_sp.cat>] [--workers n] [--max-files n]
//
// All inputs are local; no network access is needed.

#include "types.hpp"
#include "geodesic.hpp"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <string>
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

namespace fs = std::filesystem;

static constexpr double D2R = M_PI / 180.0;
static constexpr double R2D = 180.0 / M_PI;
static constexpr double CATALOG_EPOCH_JD = 2457389.0;   // J2016.0

// J-band values for the SIMBAD V fallback chain (indexed by row)
static std::vector<double> g_J;

// ------------------------------------------------------------------ photometry
// V from G and BP-RP (same polynomial as convert.cpp / henrysky's ADQL v_mag)
static inline double g_to_v(double g, double c)
{
	if (std::isnan(c)) return g;
	return g + 0.02704 - 0.01424 * c + 0.2156 * c * c - 0.01426 * c * c * c;
}

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
	CsvCols c;
	for (size_t i = 0; i < hdr.size(); ++i) {
		const std::string& h = hdr[i];
		if      (h == "hip") c.hip = (int)i;
		else if (h == "componentid") c.comp = (int)i;
		else if (h == "source_id") c.sid = (int)i;
		else if (h == "ra") c.ra = (int)i;
		else if (h == "dec") c.dec = (int)i;
		else if (h == "plx_value") c.plx = (int)i;
		else if (h == "plx_err") c.plxe = (int)i;
		else if (h == "pmra") c.pmra = (int)i;
		else if (h == "pmdec") c.pmdec = (int)i;
		else if (h == "B") c.B = (int)i;
		else if (h == "V") c.V = (int)i;
		else if (h == "J") c.J = (int)i;
		else if (h == "sp_type") c.sp = (int)i;
		else if (h == "otype") c.otype = (int)i;
		else if (h == "rvz_radvel") c.rv = (int)i;
		else if (h == "rvz_err") c.rve = (int)i;
	}
	return c;
}

// ------------------------------------------------------------------ space motion
// Linear 6D propagation (equivalent to astropy apply_space_motion for these purposes)
struct Vel { double v[3]; double dist_pc; };

static Vel make_velocity(double ra, double dec, double pmra, double pmdec,
                         double plx_mas, double rv_kms,
                         double& r0x, double& r0y, double& r0z)
{
	double sra = std::sin(ra), cra = std::cos(ra);
	double sdec = std::sin(dec), cdec = std::cos(dec);
	r0x = cdec * cra; r0y = cdec * sra; r0z = sdec;
	double p[3] = {-sra, cra, 0.0};
	double q[3] = {-sdec * cra, -sdec * sra, cdec};
	const double K = 4.740470446;                 // km/s per (AU/yr)
	double dist_pc = plx_mas > 1e-9 ? 1000.0 / plx_mas : 1e12;  // ~infinite
	double vt_p = K * (pmra / 1000.0) * dist_pc;  // AU/yr
	double vt_q = K * (pmdec / 1000.0) * dist_pc;
	double vr   = rv_kms / K;
	Vel vel;
	vel.v[0] = vt_p * p[0] + vt_q * q[0] + vr * r0x;
	vel.v[1] = vt_p * p[1] + vt_q * q[1] + vr * r0y;
	vel.v[2] = vt_p * p[2] + vt_q * q[2] + vr * r0z;
	vel.dist_pc = dist_pc;
	return vel;
}

// Propagate catalog position by dt_yr years; returns new ra/dec (deg)
static void propagate_pos(double ra_deg, double dec_deg, double pmra, double pmdec,
                          double plx_mas, double rv_kms, double dt_yr,
                          double& out_ra, double& out_dec)
{
	double r0x, r0y, r0z;
	Vel vel = make_velocity(ra_deg * D2R, dec_deg * D2R, pmra, pmdec, plx_mas, rv_kms, r0x, r0y, r0z);
	double dist_au = vel.dist_pc * 206265.0;
	double x = r0x * dist_au + vel.v[0] * dt_yr;
	double y = r0y * dist_au + vel.v[1] * dt_yr;
	double z = r0z * dist_au + vel.v[2] * dt_yr;
	double d = std::sqrt(x * x + y * y + z * z);
	out_ra = std::atan2(y, x) * R2D;
	if (out_ra < 0) out_ra += 360.0;
	out_dec = std::asin(z / d) * R2D;
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
	int n_workers = (int)std::min(4u, std::thread::hardware_concurrency());
	long max_files = -1;
	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if      (a == "--simbad"    && i + 1 < argc) simbad_path = argv[++i];
		else if (a == "--gaia-bin"  && i + 1 < argc) bin_dir = argv[++i];
		else if (a == "--out-dir"   && i + 1 < argc) out_dir = argv[++i];
		else if (a == "--otype"     && i + 1 < argc) otype_path = argv[++i];
		else if (a == "--sp-table"  && i + 1 < argc) sp_path = argv[++i];
		else if (a == "--workers"   && i + 1 < argc) n_workers = std::atoi(argv[++i]);
		else if (a == "--max-files" && i + 1 < argc) max_files = std::atol(argv[++i]);
		else {
			std::cerr << "Usage: hip2cat --simbad <dat> --gaia-bin <dir> --out-dir <dir> --otype <dat> [--sp-table f] [--workers n] [--max-files n]\n";
			return 1;
		}
	}
	if (simbad_path.empty() || bin_dir.empty() || out_dir.empty() || otype_path.empty()) {
		std::cerr << "Usage: hip2cat --simbad <dat> --gaia-bin <dir> --out-dir <dir> --otype <dat> [--sp-table f] [--workers n] [--max-files n]\n";
		return 1;
	}
	fs::create_directories(out_dir);

	// ---- object type table (line index = objtype, line 0 = "")
	std::vector<std::string> otype_ls = read_text_lines(otype_path);
	std::unordered_map<std::string, int> otype_idx;
	for (size_t i = 0; i < otype_ls.size(); ++i) otype_idx[otype_ls[i]] = (int)i;
	std::cout << "otype table: " << otype_ls.size() << " entries\n";

	// ---- spectral type table: reuse if given, else build fresh
	std::vector<std::string> sp_ls;
	bool sp_reuse = false;
	if (!sp_path.empty() && fs::exists(sp_path)) {
		sp_ls = read_text_lines(sp_path);
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
	std::vector<std::vector<StarRow>> gaia_only(n_workers);

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
						if (simbad_ids.count(rec.sid)) {
							GaiaHit h{rec.ra, rec.dec, rec.plx, (double)rec.plxe,
							          rec.pmra, rec.pmdec, (double)rec.g, bprp, (double)rec.rv};
							hits[t][rec.sid] = h;
							continue;
						}
						// ADQL selection: (bp_rp NOT NULL AND v<=15.5) OR (bp_rp NULL AND G<=15.5)
						bool sel = std::isnan(bprp) ? (rec.g <= 15.5) : (v <= 15.5);
						if (!sel) continue;
						StarRow r;
						r.source_id = rec.sid;
						r.ra = rec.ra; r.dec = rec.dec;
						r.plx = std::isnan(rec.plx) ? NAN : rec.plx;
						r.plxe = std::isnan(rec.plxe) ? NAN : (double)rec.plxe;
						r.pmra = rec.pmra; r.pmdec = rec.pmdec;
						r.rv = std::isnan(rec.rv) ? NAN : (double)rec.rv;
						double vv, bv;
						gbprp_to_bv(rec.g, bprp, vv, bv);
						r.vmag = vv; r.bv = bv;
						r.epoch_x10 = 20160;
						gaia_only[t].push_back(r);
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
					double plx_for_prop = std::isnan(r.plx) ? 1.0 : r.plx;   // assume 1 kpc if missing
					double rv_for_prop = std::isnan(r.rv) ? 0.0 : r.rv;
					propagate_pos(r.ra, r.dec, r.pmra, r.pmdec, plx_for_prop, rv_for_prop, 16.0, r.ra, r.dec);
					r.epoch_x10 = 20160;
				}
			});
		}
		for (auto& th : pool) th.join();
	}

	// ---- merge Gaia-only stars
	apply_final_patches(rows);
	for (auto& g : gaia_only) rows.insert(rows.end(), g.begin(), g.end());
	gaia_only.clear();

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
	std::cout << "combined rows: " << rows.size() << "\n";

	// ---- levels
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

	for (const auto& lc : lvcs) {
		// selection (henrysky inequalities: vmag > lo && vmag <= hi)
		std::vector<const StarRow*> sel;
		for (const auto& r : rows) {
			bool in;
			if (lc.level == 2)
				in = (r.vmag > lc.lo && r.hip > 0) || (r.vmag > lc.lo && r.vmag <= lc.hi);
			else if (lc.level <= 1)
				in = (r.vmag > lc.lo && r.vmag <= lc.hi);
			else
				in = (r.vmag > lc.lo && r.vmag <= lc.hi && r.hip == 0);
			if (in) sel.push_back(&r);
		}

		// zone assignment (+ past/future global-zone analysis for lv0-2)
		std::vector<uint32_t> zones(sel.size(), 0);
		int global_zone = 20 * (1 << (lc.level * 2));
		{
			std::atomic<size_t> idx{0};
			std::atomic<uint64_t> n_global{0};
			std::vector<std::thread> pool;
			for (int t = 0; t < n_workers; ++t) {
				pool.emplace_back([&]() {
					while (true) {
						size_t i = idx.fetch_add(1);
						if (i >= sel.size()) break;
						const StarRow& r = *sel[i];
						double x = std::cos(r.ra * D2R) * std::cos(r.dec * D2R);
						double y = std::sin(r.ra * D2R) * std::cos(r.dec * D2R);
						double z = std::sin(r.dec * D2R);
						uint32_t zone = (uint32_t)zone_number(x, y, z, lc.level);

						if (lc.level <= 2) {
							bool good = (r.plx > 0 || r.plxe > 0) && (r.plx / r.plxe > 5);
							// velocity vector in the J2016 frame (plx==0 -> ~infinite distance)
							double r0x, r0y, r0z;
							Vel vel = make_velocity(r.ra * D2R, r.dec * D2R, r.pmra, r.pmdec,
							                        r.plx, r.rv, r0x, r0y, r0z);
							double dist_au = vel.dist_pc * 206265.0;
							int counter_past = 0, counter_future = 0;
							double min_diff = 0.0;
							int zone_past = zone, zone_future = zone;
							for (int k = 1; k <= 21; ++k) {
								double dtp = k * 10000.0;
								double px = r0x * dist_au + vel.v[0] * dtp;
								double py = r0y * dist_au + vel.v[1] * dtp;
								double pz = r0z * dist_au + vel.v[2] * dtp;
								int zz = zone_number(px, py, pz, lc.level);
								counter_future += (zz != zone_future);
								zone_future = zz;
								if (good) {
									double dk = std::sqrt(px * px + py * py + pz * pz);
									double diff = 5.0 * std::log10(dk / dist_au);
									if (diff < min_diff) min_diff = diff;
								}
								double mx = r0x * dist_au - vel.v[0] * dtp;
								double my = r0y * dist_au - vel.v[1] * dtp;
								double mz = r0z * dist_au - vel.v[2] * dtp;
								zz = zone_number(mx, my, mz, lc.level);
								counter_past += (zz != zone_past);
								zone_past = zz;
								if (good) {
									double dk = std::sqrt(mx * mx + my * my + mz * mz);
									double diff = 5.0 * std::log10(dk / dist_au);
									if (diff < min_diff) min_diff = diff;
								}
							}
							bool go_global;
							if (lc.level == 0)
								go_global = (counter_past + counter_future > 0) && good;
							else
								go_global = (counter_past > 1 || counter_future > 1) ||
								            (good && min_diff < -0.3);
							if (go_global) { zone = (uint32_t)global_zone; n_global++; }
						}
						zones[i] = zone;
					}
				});
			}
			for (auto& th : pool) th.join();
			if (lc.level <= 2)
				std::cout << "  lv" << lc.level << ": " << n_global.load() << " stars to global zone\n";
		}

		// sort by (zone, vmag)
		std::vector<size_t> order(sel.size());
		for (size_t i = 0; i < sel.size(); ++i) order[i] = i;
		std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
			if (zones[a] != zones[b]) return zones[a] < zones[b];
			return sel[a]->vmag < sel[b]->vmag;
		});

		// zone counts
		int n_zones = global_zone + 1;
		std::vector<uint32_t> counts(n_zones, 0);
		for (size_t i = 0; i < sel.size(); ++i) counts[zones[i]]++;

		// write .cat
		char fname[64];
		std::snprintf(fname, sizeof(fname), "stars_%d_%dv0_%d.cat", lc.level, lc.type, lc.minor);
		std::string out_path = out_dir + "/" + fname;
		FILE* f = std::fopen(out_path.c_str(), "wb");
		if (!f) { std::cerr << "cannot write " << out_path << "\n"; return 1; }
		uint32_t header[6] = {FILE_MAGIC, lc.type, 0, lc.minor, (uint32_t)lc.level,
		                      (uint32_t)(int)std::lround(lc.lo * 1000.0)};
		std::fwrite(header, 4, 6, f);
		float epoch = (float)CATALOG_EPOCH_JD;
		std::fwrite(&epoch, 4, 1, f);
		std::fwrite(counts.data(), 4, n_zones, f);

		for (size_t oi : order) {
			const StarRow& r = *sel[oi];
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
				// spectral type index
				int sp_i = 0;
				if (!r.sp.empty()) {
					auto it = sp_idx.find(r.sp);
					if (it != sp_idx.end()) {
						sp_i = it->second;
					} else if (!sp_reuse) {
						sp_i = (int)sp_ls.size();
						sp_idx[r.sp] = sp_i;
						sp_ls.push_back(r.sp);
					} else {
						sp_i = 0;
					}
				}
				cr.spInt = (uint16_t)sp_i;
				int ot_i = 0;
				auto oit = otype_idx.find(r.otype);
				if (oit != otype_idx.end()) ot_i = oit->second;
				else {
					ot_i = (int)otype_ls.size();
					otype_idx[r.otype] = ot_i;
					otype_ls.push_back(r.otype);
					std::cout << "  NOTE: appended otype '" << r.otype << "' at index " << ot_i << "\n";
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

	// write spectral type table if we built it fresh
	if (!sp_reuse) {
		std::string sp_out = out_dir + "/stars_hip_sp_0v0_6.cat";
		std::ofstream f(sp_out, std::ios::binary);
		f << "\n";
		for (size_t i = 1; i < sp_ls.size(); ++i) f << sp_ls[i] << "\n";
		std::cout << "wrote " << sp_out << " (" << (sp_ls.size() - 1) << " types)\n";
	}

	std::cout << "Done.\n";
	return 0;
}
