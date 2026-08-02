// apply_bv: overwrite the B-V field of generated .cat files with values from
// the external B-V store (bucket files produced by bvextract, see bv_store.hpp).
// Stars missing from the store keep their original (polynomial) B-V.
// Only the b_v field is modified; header, zone table and all other record
// bytes are copied verbatim, so the output remains a valid .cat file.
//
// Usage: apply_bv <bv_dir> <out_dir> <cat1> [cat2 ...] [--buckets N]

#include "bv_store.hpp"
#include "types.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#endif

namespace
{
struct FileStats
{
	uint64_t records = 0;
	uint64_t hits    = 0;
	uint64_t misses  = 0;
};

int16_t to_star3_raw(int16_t bv_milli)
{
	int raw = static_cast<int>(std::lround((bv_milli / 1000.0 + 1.0) * 40.0));
	if (raw < 0)   raw = 0;
	if (raw > 254) raw = 254;   // 255 = missing sentinel
	return static_cast<int16_t>(raw);
}

// Copy one .cat file, replacing b_v from the store where present.
void apply_cat(const char* in_path, const char* out_path, const BvStore& store, FileStats& st)
{
	FILE* fin = std::fopen(in_path, "rb");
	if (!fin) {
		std::fprintf(stderr, "ERROR: cannot open %s\n", in_path);
		std::exit(1);
	}
	FILE* fout = std::fopen(out_path, "wb");
	if (!fout) {
		std::fprintf(stderr, "ERROR: cannot create %s\n", out_path);
		std::fclose(fin);
		std::exit(1);
	}

	uint32_t hdr[6];
	float epoch;
	if (std::fread(hdr, sizeof(uint32_t), 6, fin) != 6 || std::fread(&epoch, sizeof(float), 1, fin) != 1) {
		std::fprintf(stderr, "ERROR: %s too short\n", in_path);
		std::fclose(fin);
		std::fclose(fout);
		std::exit(1);
	}
	if (hdr[0] != FILE_MAGIC) {
		std::fprintf(stderr, "ERROR: %s bad magic 0x%08X\n", in_path, hdr[0]);
		std::fclose(fin);
		std::fclose(fout);
		std::exit(1);
	}

	const uint32_t type  = hdr[1];
	const uint32_t level = hdr[4];
	const size_t rec_size = catalog_record_size(type);
	if (rec_size == 0) {
		std::fprintf(stderr, "ERROR: %s unknown type %u\n", in_path, type);
		std::fclose(fin);
		std::fclose(fout);
		std::exit(1);
	}
	const uint64_t nzones = 20ull * (1ull << (2 * level)) + 1;

	std::fwrite(hdr, sizeof(uint32_t), 6, fout);
	std::fwrite(&epoch, sizeof(float), 1, fout);

	std::vector<uint8_t> zonetab(nzones * 4);
	if (std::fread(zonetab.data(), 1, zonetab.size(), fin) != zonetab.size()) {
		std::fprintf(stderr, "ERROR: %s zone table truncated\n", in_path);
		std::fclose(fin);
		std::fclose(fout);
		std::exit(1);
	}
	std::fwrite(zonetab.data(), 1, zonetab.size(), fout);

	std::vector<uint8_t> buf(rec_size);
	for (;;) {
		size_t got = std::fread(buf.data(), 1, rec_size, fin);
		if (got == 0)
			break;
		if (got != rec_size) {
			std::fprintf(stderr, "WARNING: %s short read %zu/%zu at record %llu\n",
			             in_path, got, rec_size, (unsigned long long)st.records);
			break;
		}
		st.records++;

		uint64_t sid;
		std::memcpy(&sid, buf.data(), 8);

		int16_t bv;
		if (store.query(sid, bv)) {
			if (type == CATALOG_TYPE_STAR1) {
				std::memcpy(buf.data() + 32, &bv, 2);
			} else if (type == CATALOG_TYPE_STAR2) {
				std::memcpy(buf.data() + 24, &bv, 2);
			} else {
				buf[14] = static_cast<uint8_t>(to_star3_raw(bv));
			}
			st.hits++;
		} else {
			st.misses++;
		}

		std::fwrite(buf.data(), 1, rec_size, fout);
	}

	std::fclose(fin);
	std::fclose(fout);
	const char* tname = type == CATALOG_TYPE_STAR1 ? "Star1" : type == CATALOG_TYPE_STAR3 ? "Star3" : "Star2";
	std::printf("  %s -> %s : %llu records, %llu hit (%.2f%%), type %s, level %u\n",
	            in_path, out_path, (unsigned long long)st.records,
	            (unsigned long long)st.hits, st.records ? 100.0 * st.hits / st.records : 0.0,
	            tname, level);
}
}  // namespace

int main(int argc, char** argv)
{
	if (argc < 4) {
		std::fprintf(stderr, "Usage: apply_bv <bv_dir> <out_dir> <cat1> [cat2 ...] [--buckets N]\n");
		return 1;
	}

	const std::string bv_dir = argv[1];
	const std::string out_dir = argv[2];
	uint32_t n_buckets = 256;
	std::vector<std::string> cats;
	for (int i = 3; i < argc; ++i) {
		if (std::strcmp(argv[i], "--buckets") == 0 && i + 1 < argc) {
			n_buckets = static_cast<uint32_t>(std::atoi(argv[++i]));
			if (n_buckets == 0 || (n_buckets & (n_buckets - 1)) != 0) {
				std::fprintf(stderr, "ERROR: --buckets must be a power of two\n");
				return 1;
			}
		} else {
			cats.push_back(argv[i]);
		}
	}
	if (cats.empty()) {
		std::fprintf(stderr, "ERROR: no input .cat files\n");
		return 1;
	}

#ifdef _WIN32
	_setmaxstdio(2048);
#endif

	BvStore store;
	store.load(bv_dir, n_buckets);
	std::printf("B-V store: %llu entries in %u buckets\n",
	            (unsigned long long)store.total(), n_buckets);

	uint64_t all_hits = 0, all_records = 0;
	for (const auto& c : cats) {
		std::string base = c;
		const size_t slash = base.find_last_of("/\\");
		if (slash != std::string::npos)
			base = base.substr(slash + 1);
		FileStats st;
		apply_cat(c.c_str(), (out_dir + "/" + base).c_str(), store, st);
		all_hits += st.hits;
		all_records += st.records;
	}

	std::printf("Total: %llu/%llu records got B-V from the store (%.2f%%)\n",
	            (unsigned long long)all_hits, (unsigned long long)all_records,
	            all_records ? 100.0 * all_hits / all_records : 0.0);
	return 0;
}
