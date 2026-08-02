// bvextract: extract (source_id, B-V) from official Stellarium .cat files into
// the pipeline's external B-V store (bucket files).
//
// Bucket layout (per bucket file, little-endian):
//   [0] magic u32 = 0x42564231
//   [4] count u32
//   [8] records: u64 source_id, i16 b_v*1000   (10 bytes, sorted by source_id)
// Bucket index = splitmix64(source_id) & (n_buckets - 1), n_buckets a power of two.
// Consumers locate a bucket the same way, then binary-search within it.
//
// Usage: bvextract <out_dir> <cat1> [cat2 ...] [--buckets N]

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
struct Stats
{
	uint64_t records  = 0;
	uint64_t missing  = 0;   // B-V sentinel (32767 / 255)
	uint64_t dups     = 0;   // same source_id seen more than once
	uint64_t skip_magic = 0;
};

// Append one (sid, bv) to its bucket file. bv may be any int16; callers skip
// missing sentinels before calling.
void append_record(FILE* f, uint64_t sid, int16_t bv)
{
	BvEntry e;
	e.sid = sid;
	e.bv  = bv;
	std::fwrite(&e, sizeof(BvEntry), 1, f);
}

// Extract B-V from one .cat file, appending to the open bucket files.
// Extract the B-V of one record; returns false when it is a missing sentinel.
bool extract_record_bv(const uint8_t* buf, uint32_t type, uint64_t& sid, int16_t& bv)
{
	std::memcpy(&sid, buf, 8);
	if (type == CATALOG_TYPE_STAR1) {
		std::memcpy(&bv, buf + 32, 2);
		return bv != STAR2_BV_MISSING;
	}
	if (type == CATALOG_TYPE_STAR2) {
		std::memcpy(&bv, buf + 24, 2);
		return bv != STAR2_BV_MISSING;
	}
	const uint8_t bvr = buf[14];
	if (bvr == STAR3_BV_MISSING)
		return false;
	bv = static_cast<int16_t>(std::lround((bvr / 40.0 - 1.0) * 1000.0));
	return true;
}

// Extract B-V from one .cat file, appending to the open bucket files.
void extract_cat(const char* path, std::vector<FILE*>& buckets, uint32_t n_buckets, Stats& st)
{
	FILE* f = std::fopen(path, "rb");
	if (!f) {
		std::fprintf(stderr, "ERROR: cannot open %s\n", path);
		std::exit(1);
	}

	uint32_t hdr[6];
	float epoch;
	if (std::fread(hdr, sizeof(uint32_t), 6, f) != 6 || std::fread(&epoch, sizeof(float), 1, f) != 1) {
		std::fprintf(stderr, "ERROR: %s too short\n", path);
		std::fclose(f);
		std::exit(1);
	}
	if (hdr[0] != FILE_MAGIC) {
		std::fprintf(stderr, "WARNING: %s bad magic 0x%08X (skipping)\n", path, hdr[0]);
		st.skip_magic++;
		std::fclose(f);
		return;
	}

	const uint32_t type  = hdr[1];
	const uint32_t level = hdr[4];
	const size_t rec_size = catalog_record_size(type);
	if (rec_size == 0) {
		std::fprintf(stderr, "WARNING: %s unknown type %u (skipping)\n", path, type);
		st.skip_magic++;
		std::fclose(f);
		return;
	}

	const uint64_t nzones = 20ull * (1ull << (2 * level)) + 1;
	if (std::fseek(f, static_cast<long>(nzones) * 4, SEEK_CUR) != 0) {
		std::fprintf(stderr, "ERROR: %s seek failed\n", path);
		std::fclose(f);
		std::exit(1);
	}

	std::vector<uint8_t> buf(rec_size);
	uint64_t n = 0;
	for (;;) {
		size_t got = std::fread(buf.data(), 1, rec_size, f);
		if (got == 0) break;
		if (got != rec_size) {
			std::fprintf(stderr, "WARNING: %s short read %zu/%zu at record %llu\n",
			             path, got, rec_size, (unsigned long long)n);
			break;
		}
		++n;

		uint64_t sid;
		int16_t bv;
		if (!extract_record_bv(buf.data(), type, sid, bv)) {
			st.missing++;
			continue;
		}
		uint32_t b = static_cast<uint32_t>(splitmix64(sid) & (n_buckets - 1));
		append_record(buckets[b], sid, bv);
		st.records++;
	}

	std::fclose(f);
	const char* tname = type == CATALOG_TYPE_STAR1 ? "Star1" : type == CATALOG_TYPE_STAR3 ? "Star3" : "Star2";
	std::printf("  %s: %llu records (type %s, level %u)\n", path, (unsigned long long)n, tname, level);
}

std::string bucket_path(const std::string& out_dir, uint32_t idx, const char* ext)
{
	char name[64];
	std::snprintf(name, sizeof(name), "%s%02u.%s", "bv_", idx, ext);
	return out_dir + "/" + name;
}

// Sort one bucket file by source_id and write the final .bvbin; returns its record count.
uint64_t finalize_bucket(const std::string& tmp_name, const std::string& final_name, uint64_t& dups)
{
	FILE* f = std::fopen(tmp_name.c_str(), "rb");
	if (!f) { std::fprintf(stderr, "ERROR: cannot reopen %s\n", tmp_name.c_str()); std::exit(1); }
	std::fseek(f, 0, SEEK_END);
	long sz = std::ftell(f);
	std::fseek(f, 0, SEEK_SET);
	size_t n = static_cast<size_t>(sz) / sizeof(BvEntry);

	std::vector<BvEntry> entries(n);
	size_t got = std::fread(entries.data(), sizeof(BvEntry), n, f);
	std::fclose(f);
	if (got != n) { std::fprintf(stderr, "ERROR: short read on %s\n", tmp_name.c_str()); std::exit(1); }

	std::sort(entries.begin(), entries.end(),
	          [](const BvEntry& a, const BvEntry& c) { return a.sid < c.sid; });
	for (size_t i = 1; i < entries.size(); ++i)
		if (entries[i].sid == entries[i - 1].sid) dups++;

	FILE* fo = std::fopen(final_name.c_str(), "wb");
	if (!fo) { std::fprintf(stderr, "ERROR: cannot create %s\n", final_name.c_str()); std::exit(1); }
	uint32_t hdr[2] = {BV_MAGIC, static_cast<uint32_t>(entries.size())};
	std::fwrite(hdr, sizeof(uint32_t), 2, fo);
	std::fwrite(entries.data(), sizeof(BvEntry), entries.size(), fo);
	std::fclose(fo);
	std::remove(tmp_name.c_str());
	return entries.size();
}
}  // namespace

// Parse <out_dir> <cat...> [--buckets N]; returns false on bad usage.
bool parse_args(int argc, char** argv, std::string& out_dir,
                uint32_t& n_buckets, std::vector<std::string>& cats)
{
	if (argc < 3) {
		std::fprintf(stderr, "Usage: bvextract <out_dir> <cat1> [cat2 ...] [--buckets N]\n");
		return false;
	}
	out_dir = argv[1];
	for (int i = 2; i < argc; ++i) {
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
	std::string out_dir;
	uint32_t n_buckets = 256;
	std::vector<std::string> cats;
	if (!parse_args(argc, argv, out_dir, n_buckets, cats))
		return 1;

#ifdef _WIN32
	_setmaxstdio(2048);
#endif

	std::vector<std::string> tmp_names(n_buckets);
	std::vector<FILE*> tmp(n_buckets, nullptr);
	for (uint32_t b = 0; b < n_buckets; ++b) {
		tmp_names[b] = bucket_path(out_dir, b, "tmp");
		tmp[b] = std::fopen(tmp_names[b].c_str(), "wb");
		if (!tmp[b]) {
			std::fprintf(stderr, "ERROR: cannot create %s\n", tmp_names[b].c_str());
			return 1;
		}
	}

	Stats st;
	std::printf("Extracting %zu catalog(s) into %u buckets...\n", cats.size(), n_buckets);
	for (const auto& c : cats)
		extract_cat(c.c_str(), tmp, n_buckets, st);

	for (uint32_t b = 0; b < n_buckets; ++b)
		std::fclose(tmp[b]);

	std::printf("Total: %llu B-V records, %llu missing sentinels, %zu skipped files\n",
	            (unsigned long long)st.records, (unsigned long long)st.missing, st.skip_magic);

	// Sort each bucket by source_id and write the final file.
	uint64_t total = 0;
	for (uint32_t b = 0; b < n_buckets; ++b)
		total += finalize_bucket(tmp_names[b], bucket_path(out_dir, b, "bvbin"), st.dups);

	std::printf("Wrote %llu B-V records in %u buckets (%llu duplicate source_ids)\n",
	            (unsigned long long)total, n_buckets, (unsigned long long)st.dups);
	return 0;
}
