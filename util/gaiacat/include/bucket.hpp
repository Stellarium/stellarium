#pragma once
#include "types.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <cstdio>
#include <list>
#include <unordered_map>

class BucketWriter {
public:
	BucketWriter(int n_buckets, int zones_per_bucket, const std::string& bucket_dir);
	~BucketWriter();

	void push(const BucketRecord& rec);
	void finish();
	std::vector<std::string> bucket_paths() const;
	int num_buckets() const { return n_buckets_; }

private:
	static constexpr int MAX_OPEN = 96;

	struct Bucket {
		std::string path;
		std::mutex  mtx;
		FILE*       file = nullptr;  // non-null when in LRU cache
	};

	int n_buckets_;
	int zones_per_bucket_;
	std::string bucket_dir_;
	std::vector<std::unique_ptr<Bucket>> buckets_;

	// LRU file handle cache
	std::mutex           lru_mtx_;
	std::list<int>       lru_list_;                     // bucket indices, MRU at front
	std::unordered_map<int, FILE*> lru_map_;             // bucket index → FILE*
};
