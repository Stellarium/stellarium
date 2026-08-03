// Shared on-disk B-V store: bucket files produced by bvextract, consumed by
// apply_bv (and potentially by the 2cat converters later).
//
// Bucket layout (per bucket file, little-endian):
//   [0] magic u32 = 0x42564231
//   [4] count u32
//   [8] records: u64 source_id, i16 b_v*1000  (10 bytes, sorted by source_id)
// Bucket index = splitmix64(source_id) & (n_buckets - 1), n_buckets a power of two.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

inline constexpr uint32_t BV_MAGIC = 0x42564231;

// Fast mixing hash used for bucketing. The consumer must use the exact same
// function (and bucket count) as bvextract.
inline uint64_t splitmix64(uint64_t x)
{
	x += 0x9E3779B97F4A7C15ULL;
	x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
	x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
	return x ^ (x >> 31);
}

#pragma pack(push, 1)
struct BvEntry
{
	uint64_t sid;
	int16_t  bv;
};
#pragma pack(pop)
static_assert(sizeof(BvEntry) == 10, "BvEntry must be 10 bytes");

// Whole-store in-memory B-V lookup. Loads every bucket up front; each bucket
// is kept sorted so lookups are binary searches. Peak memory = 10 B/record.
class BvStore
{
public:
	// Load all n_buckets bucket files from dir (names bv_00.bvbin ...).
	// Missing buckets are tolerated (warned, treated as empty).
	bool load(const std::string& dir, uint32_t n_buckets)
	{
		buckets_.clear();
		buckets_.resize(n_buckets);
		for (uint32_t b = 0; b < n_buckets; ++b) {
			char name[64];
			std::snprintf(name, sizeof(name), "%s%02u.bvbin", "bv_", b);
			std::string path = dir + "/" + name;
			FILE* f = std::fopen(path.c_str(), "rb");
			if (!f) {
				std::fprintf(stderr, "WARNING: missing bucket %s (treated as empty)\n", path.c_str());
				continue;
			}
			uint32_t hdr[2];
			if (std::fread(hdr, sizeof(uint32_t), 2, f) != 2 || hdr[0] != BV_MAGIC) {
				std::fprintf(stderr, "WARNING: %s bad header (skipped)\n", path.c_str());
				std::fclose(f);
				continue;
			}
			buckets_[b].resize(hdr[1]);
			size_t got = std::fread(buckets_[b].data(), sizeof(BvEntry), hdr[1], f);
			std::fclose(f);
			if (got != hdr[1])
				buckets_[b].resize(got);
		}
		return true;
	}

	uint64_t total() const
	{
		uint64_t n = 0;
		for (const auto& b : buckets_)
			n += b.size();
		return n;
	}

	// Returns false when the source_id has no B-V in the store.
	bool query(uint64_t sid, int16_t& bv) const
	{
		const auto& entries = buckets_[splitmix64(sid) & (buckets_.size() - 1)];
		auto it = std::lower_bound(entries.begin(), entries.end(), sid,
		                           [](const BvEntry& e, uint64_t s) { return e.sid < s; });
		if (it == entries.end() || it->sid != sid)
			return false;
		bv = it->bv;
		return true;
	}

private:
	std::vector<std::vector<BvEntry>> buckets_;
};
