// apply_bv: overwrite the B-V field of generated .cat files with values from
// the external B-V store (bucket files produced by bvextract, see bv_store.hpp).
// Stars missing from the store keep their original (polynomial) B-V.
// Only the b_v field is modified; header, zone table and all other record
// bytes are copied verbatim, so the output remains a valid .cat file.
//
// Usage: apply_bv <bv_dir> <out_dir> <cat1> [cat2 ...] [--buckets N]

#include "bv_store.hpp"
#include "cat_reader.hpp"
#include "cat_record.hpp"
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

// Copy one .cat file, replacing b_v from the store where present.
void apply_cat(const char* in_path, const char* out_path, const BvStore& store, FileStats& st)
{
	CatReader ch;
	std::string err;
	if (!ch.open(in_path, err)) {
		std::fprintf(stderr, "ERROR: %s\n", err.c_str());
		std::exit(1);
	}

	FILE* fout = std::fopen(out_path, "wb");
	if (!fout) {
		std::fprintf(stderr, "ERROR: cannot create %s\n", out_path);
		std::exit(1);
	}
	std::fwrite(ch.hdr, sizeof(uint32_t), 6, fout);
	std::fwrite(&ch.epoch, sizeof(float), 1, fout);
	std::fwrite(ch.counts.data(), sizeof(uint32_t), ch.counts.size(), fout);

	std::vector<uint8_t> buf(ch.rec_size);
	for (;;) {
		size_t got = std::fread(buf.data(), 1, ch.rec_size, ch.f);
		if (got == 0)
			break;
		if (got != ch.rec_size) {
			std::fprintf(stderr, "WARNING: %s short read %zu/%zu at record %llu\n",
			             in_path, got, ch.rec_size, (unsigned long long)st.records);
			break;
		}
		st.records++;

		uint64_t sid;
		std::memcpy(&sid, buf.data(), 8);

		int16_t bv;
		if (store.query(sid, bv)) {
			overwrite_bv_field(ch.cat_type, buf.data(), bv);
			st.hits++;
		} else {
			st.misses++;
		}

		std::fwrite(buf.data(), 1, ch.rec_size, fout);
	}

	std::fclose(ch.f);
	std::fclose(fout);
	const char* tname = ch.cat_type == CATALOG_TYPE_STAR1 ? "Star1" : ch.cat_type == CATALOG_TYPE_STAR3 ? "Star3" : "Star2";
	std::printf("  %s -> %s : %llu records, %llu hit (%.2f%%), type %s, level %u\n",
	            in_path, out_path, (unsigned long long)st.records,
	            (unsigned long long)st.hits, st.records ? 100.0 * st.hits / st.records : 0.0,
	            tname, ch.hdr[4]);
}
}  // namespace

// Parse <bv_dir> <out_dir> <cat...> [--buckets N]; returns false on bad usage.
bool parse_args(int argc, char** argv, std::string& bv_dir, std::string& out_dir,
                uint32_t& n_buckets, std::vector<std::string>& cats)
{
	if (argc < 4) {
		std::fprintf(stderr, "Usage: apply_bv <bv_dir> <out_dir> <cat1> [cat2 ...] [--buckets N]\n");
		return false;
	}
	bv_dir = argv[1];
	out_dir = argv[2];
	for (int i = 3; i < argc; ++i) {
		if (std::strcmp(argv[i], "--buckets") == 0 && i + 1 < argc) {
			n_buckets = static_cast<uint32_t>(std::atoi(argv[++i]));
			if (n_buckets == 0 || (n_buckets & (n_buckets - 1)) != 0) {
				std::fprintf(stderr, "ERROR: --buckets must be a power of two\n");
				return false;
			}
		} else {
			cats.push_back(argv[i]);
		}
	}
	if (cats.empty()) {
		std::fprintf(stderr, "ERROR: no input .cat files\n");
		return false;
	}
	return true;
}

int main(int argc, char** argv)
{
	std::string bv_dir, out_dir;
	uint32_t n_buckets = 256;
	std::vector<std::string> cats;
	if (!parse_args(argc, argv, bv_dir, out_dir, n_buckets, cats))
		return 1;

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
