// Gaia DR3 .bin → Stellarium .cat converter.
// Usage:
//   gaia2cat --gaia-bin <dir> --out-dir <dir> [--workers <n>] [--dry-run]
//            [--star3-from <level>] [--levels <lo>[-<hi>]]

#include "types.hpp"
#include "convert.hpp"
#include "geodesic.hpp"
#include "bucket.hpp"
#include "cat_writer.hpp"
#include "star_provider.hpp"
#include "catalog_naming.hpp"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;
using namespace std::chrono;
using Clock = steady_clock;

struct LevelConfig {
	std::string name;
	int         level;
	double      mag_lo;
	double      mag_hi;
	int         n_buckets;
	uint32_t    cat_type;   // CATALOG_TYPE_STAR2 or CATALOG_TYPE_STAR3
};

using ZoneCounts = std::vector<std::vector<uint32_t>>;

static ZoneCounts make_zone_counts(const std::vector<LevelConfig>& levels) {
	ZoneCounts out;
	for (const auto& lv : levels)
		out.emplace_back(nr_of_zones(lv.level), 0);
	return out;
}

static void save_counts(const std::string& work_dir, const LevelConfig& lv, const std::vector<uint32_t>& counts) {
	std::string path = work_dir + "/" + lv.name + "_counts.bin";
	FILE* f = std::fopen(path.c_str(), "wb");
	if (!f) { std::cerr << "ERROR: cannot write " << path << "\n"; return; }
	uint32_t nz = static_cast<uint32_t>(counts.size());
	std::fwrite(&nz, sizeof(uint32_t), 1, f);
	std::fwrite(counts.data(), sizeof(uint32_t), nz, f);
	std::fclose(f);
}

static bool load_counts(const std::string& work_dir, const LevelConfig& lv, std::vector<uint32_t>& counts) {
	std::string path = work_dir + "/" + lv.name + "_counts.bin";
	FILE* f = std::fopen(path.c_str(), "rb");
	if (!f) return false;
	uint32_t nz = 0;
	if (std::fread(&nz, sizeof(uint32_t), 1, f) != 1) { std::fclose(f); return false; }
	if (nz != counts.size()) { std::fclose(f); return false; }
	if (std::fread(counts.data(), sizeof(uint32_t), nz, f) != nz) { std::fclose(f); return false; }
	std::fclose(f);
	return true;
}

static bool pass1_complete(const std::string& work_dir, const std::vector<LevelConfig>& levels) {
	for (const auto& lv : levels) {
		std::string path = work_dir + "/" + lv.name + "_counts.bin";
		if (!fs::exists(path)) return false;
	}
	return true;
}

// Catalog file naming: stars_<level>_<type>v<major>_<minor>.cat (matches ZoneArray debug string)
// Minor is the official minor from starsConfig.json + 1 (1 if no official entry).
static std::string cat_filename(const LevelConfig& lv, const std::string& stars_config) {
	return next_cat_filename(lv.level, lv.cat_type, official_cat_filename(stars_config, lv.level));
}

static void cleanup_incomplete(const std::string& work_dir, const std::vector<LevelConfig>& levels) {
	for (const auto& lv : levels) {
		std::string bucket_dir = work_dir + "/" + lv.name + "_buckets";
		std::error_code ec;
		fs::remove_all(bucket_dir, ec);
	}
}

static void cleanup_pass2(const std::string& out_dir, const std::vector<LevelConfig>& levels,
                          const std::string& stars_config) {
	for (const auto& lv : levels) {
		std::string cat_path = out_dir + "/" + cat_filename(lv, stars_config);
		std::error_code ec;
		fs::remove(cat_path, ec);
	}
}

// Process one star file (template works with any Reader that has open/next/close)
template<typename Reader>
static void process_star_file(
	const std::string& path,
	const std::vector<LevelConfig>& levels,
	std::vector<BucketWriter*>& bucket_writers,
	ZoneCounts& local_counts,
	bool dry_run)
{
	Reader reader;
	if (!reader.open(path)) return;

	StarData star;
	while (reader.next(star)) {
		if (star.gaia_id == 0) continue;
		if (std::isnan(star.G_mag)) continue;

		double v  = g_to_v(star.G_mag, star.bp_rp);
		double bv = bp_rp_to_bv(star.bp_rp);

		for (size_t li = 0; li < levels.size(); ++li) {
			const auto& lv = levels[li];
			if (v < lv.mag_lo || v >= lv.mag_hi) continue;

			double ra_rad  = star.ra_deg  * M_PI / 180.0;
			double dec_rad = star.dec_deg * M_PI / 180.0;
			double cos_dec = std::cos(dec_rad);
			double x = std::cos(ra_rad) * cos_dec;
			double y = std::sin(ra_rad) * cos_dec;
			double z = std::sin(dec_rad);

			int zone = zone_number(x, y, z, lv.level);
			local_counts[li][zone]++;

			if (!dry_run) {
				BucketRecord brec{};
				brec.zone    = static_cast<uint32_t>(zone);
				brec.vmag    = static_cast<int16_t>(std::round(v * 1000.0));
				brec.bv      = std::isnan(star.bp_rp)
				? std::numeric_limits<int16_t>::max()   // +32767 sentinel: missing BP-RP
				: static_cast<int16_t>(std::round(bv * 1000.0));
				brec.ra_i    = static_cast<int32_t>(std::round(star.ra_deg * 3600000.0));
				brec.dec_i   = static_cast<int32_t>(std::round(star.dec_deg * 3600000.0));
				brec.gaia_id = star.gaia_id;
				// star.pmra/pmdec already in mas/yr (Gaia native), parallax in mas
				brec.pmra_i  = std::isnan(star.pmra)  ? 0 : static_cast<int32_t>(std::round(star.pmra  * 1000.0 / std::max(0.00001, cos_dec)));
				brec.pmdec_i = std::isnan(star.pmdec) ? 0 : static_cast<int32_t>(std::round(star.pmdec * 1000.0));
				brec.plx_i   = std::isnan(star.parallax) ? 0 : static_cast<int32_t>(std::round(star.parallax * 100.0));
			brec.plx_err_i = std::isnan(star.plx_err) ? 0 : static_cast<int32_t>(std::round(star.plx_err * 100.0));
			brec.rv_i    = std::isnan(star.rv) ? 0 : static_cast<int32_t>(std::round(star.rv * 10.0));

				bucket_writers[li]->push(brec);
			}
			break;
		}
	}
	reader.close();
}

int main(int argc, char** argv) {
	std::string input_dir;
	std::string out_dir;
	std::string work_dir;
	std::string stars_config = default_stars_config_path();
	int    n_workers   = std::thread::hardware_concurrency();
	bool   dry_run     = false;
	int    star3_from  = 8;   // levels >= this use Star3 (V >= 16.0 required; matches official split)
	int    level_lo    = 4;   // default: generate levels 4-10 (lv4 overlaps lv3 by 0.25 mag for dedup)
	int    level_hi    = 10;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if      (arg == "--gaia-bin"    && i+1 < argc) { input_dir  = argv[++i]; }
		else if (arg == "--out-dir"     && i+1 < argc) { out_dir    = argv[++i]; }
		else if (arg == "--work-dir"    && i+1 < argc) { work_dir   = argv[++i]; }
		else if (arg == "--stars-config" && i+1 < argc) { stars_config = argv[++i]; }
		else if (arg == "--workers"     && i+1 < argc) { n_workers  = std::stoi(argv[++i]); }
		else if (arg == "--star3-from"  && i+1 < argc) { star3_from = std::stoi(argv[++i]); }
		else if (arg == "--levels"      && i+1 < argc) {
			std::string spec = argv[++i];
			if (std::sscanf(spec.c_str(), "%d-%d", &level_lo, &level_hi) != 2) {
				if (std::sscanf(spec.c_str(), "%d", &level_lo) != 1) {
					std::cerr << "ERROR: bad --levels spec '" << spec << "' (expected e.g. 0-6 or 7)\n";
					return 1;
				}
				level_hi = level_lo;
			}
		}
		else if (arg == "--dry-run")                    { dry_run    = true; }
		else {
			std::cerr << "Usage: gaia2cat --gaia-bin <dir> --out-dir <dir> [--work-dir <dir>] [--stars-config <json>] [--workers <n>] [--star3-from <level>] [--levels <lo>[-<hi>]] [--dry-run]\n";
			return 1;
		}
	}
	if (input_dir.empty() || out_dir.empty()) {
		std::cerr << "Usage: gaia2cat --gaia-bin <dir> --out-dir <dir> [--work-dir <dir>] [--stars-config <json>] [--workers <n>] [--star3-from <level>] [--levels <lo>[-<hi>]] [--dry-run]\n";
		return 1;
	}
	if (level_lo < 0 || level_hi > 10 || level_lo > level_hi) {
		std::cerr << "ERROR: --levels must satisfy 0 <= lo <= hi <= 10\n";
		return 1;
	}
	if (star3_from < 8) {
		std::cerr << "ERROR: --star3-from must be >= 8 (Star3 cannot represent V < 16.0)\n";
		return 1;
	}
	if (!stars_config.empty() && !fs::exists(stars_config))
		std::cerr << "WARNING: starsConfig.json not found at " << stars_config
		          << " -- using default catalog naming\n";
	if (work_dir.empty()) work_dir = out_dir;

	if (!dry_run) {
		fs::create_directories(out_dir);
		fs::create_directories(work_dir);
	}

	auto type_for = [star3_from](int level) -> uint32_t {
		if (level <= 3) return CATALOG_TYPE_STAR1;
		return level >= star3_from ? CATALOG_TYPE_STAR3 : CATALOG_TYPE_STAR2;
	};
	// Official magnitude split (matches henrysky's lv0-6 pipeline and default catalogs).
	// stars_4 starts at 10.25 instead of 10.5: a deliberate 0.25 mag overlap with
	// stars_3 so that dedupcat can remove boundary duplicates without losing stars.
	struct LevelDef { const char* name; int level; double mag_lo, mag_hi; };
	static const LevelDef kAllLevels[] = {
		{"stars_0",   0, -2.00,  6.00},
		{"stars_1",   1,  6.00,  7.50},
		{"stars_2",   2,  7.50,  9.00},
		{"stars_3",   3,  9.00, 10.50},
		{"stars_4",   4, 10.25, 12.00},
		{"stars_5",   5, 12.00, 13.75},
		{"stars_6",   6, 13.75, 15.50},
		{"stars_7",   7, 15.50, 16.75},
		{"stars_8",   8, 16.75, 18.50},
		{"stars_9",   9, 18.50, 20.25},
		{"stars_10", 10, 20.25, 23.00},
	};
	std::vector<LevelConfig> levels;
	for (const auto& d : kAllLevels) {
		if (d.level < level_lo || d.level > level_hi) continue;
		levels.push_back({d.name, d.level, d.mag_lo, d.mag_hi, 256, type_for(d.level)});
	}
	for (const auto& lv : levels) {
		const char* tname = lv.cat_type == CATALOG_TYPE_STAR1 ? "Star1"
		                  : lv.cat_type == CATALOG_TYPE_STAR3 ? "Star3" : "Star2";
		std::cout << "  " << lv.name << ": V [" << lv.mag_lo << ", " << lv.mag_hi << ")  " << tname << "\n";
	}

	// Discover input files
	std::vector<std::string> input_files;
	for (const auto& entry : fs::directory_iterator(input_dir)) {
		if (entry.path().extension() == ".bin")
			input_files.push_back(entry.path().string());
	}
	std::sort(input_files.begin(), input_files.end());
	std::cout << "Found " << input_files.size() << " .bin files\n";

	if (input_files.empty()) return 1;

	ZoneCounts all_counts = make_zone_counts(levels);
	bool resume = pass1_complete(work_dir, levels);

	// ── PASS 1: scan + bucket ──
	std::vector<BucketWriter*> bucket_writers;

	if (resume) {
		std::cout << "\nPASS 1 already complete (counts.bin found), skipping.\n";
		for (size_t li = 0; li < levels.size(); ++li) {
			if (!load_counts(work_dir, levels[li], all_counts[li])) {
				std::cerr << "ERROR: failed to load counts for " << levels[li].name << "\n";
				return 1;
			}
		}
	} else {
		cleanup_incomplete(work_dir, levels);

		std::cout << "\n===== PASS 1: " << input_files.size() << " files ("
		          << n_workers << " workers)";
		if (dry_run) std::cout << " -- DRY RUN";
		std::cout << " =====\n";

		auto t0 = Clock::now();

		std::vector<ZoneCounts> worker_counts(n_workers);
		for (int t = 0; t < n_workers; ++t)
			worker_counts[t] = make_zone_counts(levels);

		if (!dry_run) {
			for (size_t li = 0; li < levels.size(); ++li) {
				const auto& lv = levels[li];
				auto bucket_dir = fs::path(work_dir) / (lv.name + "_buckets");
				std::error_code ec;
				fs::create_directories(bucket_dir, ec);
				if (ec) {
					std::cerr << "ERROR: cannot create bucket dir " << bucket_dir << "\n";
					return 1;
				}
				int zones_per_bucket = (nr_of_zones(lv.level) + lv.n_buckets - 1) / lv.n_buckets;
				bucket_writers.push_back(new BucketWriter(lv.n_buckets, zones_per_bucket, bucket_dir.string()));
			}
		}

		std::atomic<int> next_file{0};
		std::atomic<int> completed{0};
		std::vector<std::thread> workers;
		for (int t = 0; t < n_workers; ++t) {
			int worker_id = t;
			workers.emplace_back([&, worker_id]() {
				while (true) {
					int fi = next_file.fetch_add(1);
					if (fi >= static_cast<int>(input_files.size())) break;

				process_star_file<BinReader>(input_files[fi], levels, bucket_writers, worker_counts[worker_id], dry_run);
				completed.fetch_add(1);
				}
			});
		}

		std::thread progress([&]() {
			while (completed.load() < static_cast<int>(input_files.size())) {
				std::this_thread::sleep_for(std::chrono::seconds(2));
				int done = completed.load();
				auto elapsed = duration_cast<seconds>(steady_clock::now() - t0).count();
				std::cout << "  [" << done << "/" << input_files.size() << "] "
				          << (100.0*done/input_files.size()) << "%  "
				          << elapsed << "s\n";
			}
		});

		for (auto& w : workers) w.join();
		progress.join();

		for (int t = 0; t < n_workers; ++t) {
			for (size_t li = 0; li < levels.size(); ++li) {
				size_t nz = all_counts[li].size();
				for (size_t z = 0; z < nz; ++z)
					all_counts[li][z] += worker_counts[t][li][z];
			}
		}

		auto t1 = Clock::now();
		double elapsed = duration_cast<milliseconds>(t1 - t0).count() / 1000.0;
		std::cout << "PASS 1 complete in " << elapsed << "s\n";

		if (!dry_run)
			for (size_t li = 0; li < levels.size(); ++li)
				save_counts(work_dir, levels[li], all_counts[li]);

		if (!dry_run)
			for (auto* bw : bucket_writers) bw->finish();
	}

	// Print counts
	for (size_t li = 0; li < levels.size(); ++li) {
		const auto& lv = levels[li];
		uint64_t total = 0;
		int non_empty = 0;
		for (auto c : all_counts[li]) { total += c; if (c > 0) non_empty++; }
		std::cout << "  " << lv.name << ": " << total << " stars, " << non_empty << " non-empty zones\n";
	}

	if (dry_run) {
		std::cout << "DRY RUN -- skipping Pass 2\n";
		return 0;
	}

	// ── PASS 2: sort + write .cat ──
	cleanup_pass2(out_dir, levels, stars_config);

	for (size_t li = 0; li < levels.size(); ++li) {
		const auto& lv = levels[li];

		std::vector<std::string> bpaths;
		for (int b = 0; b < lv.n_buckets; ++b) {
			std::ostringstream oss;
			oss << work_dir << "/" << lv.name << "_buckets/bucket_"
			    << std::setw(4) << std::setfill('0') << b << ".dat";
			bpaths.push_back(oss.str());
		}

		int mag_min = static_cast<int>(lv.mag_lo * 1000.0);
		const std::string out_name = cat_filename(lv, stars_config);
		std::string out_path = out_dir + "/" + out_name;
		int h_type = -1, h_major = -1, h_minor = -1;
		parse_cat_version(out_name, &h_type, &h_major, &h_minor);
		const uint32_t major = (uint32_t)std::max(0, h_major);
		const uint32_t minor = (uint32_t)std::max(0, h_minor);

		write_cat(bpaths, all_counts[li], lv.level, mag_min, lv.cat_type, major, minor, out_path, n_workers);

		for (const auto& p : bpaths) fs::remove(p);
	}

	for (auto* bw : bucket_writers) { bw->finish(); delete bw; }

	std::cout << "\nDone.\n";
	return 0;
}
