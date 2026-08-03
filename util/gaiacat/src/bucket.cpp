#include "bucket.hpp"
#include <cstring>
#include <cerrno>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

BucketWriter::BucketWriter(int n_buckets, int zones_per_bucket, const std::string& bucket_dir)
	: n_buckets_(n_buckets), zones_per_bucket_(zones_per_bucket), bucket_dir_(bucket_dir)
{
	buckets_.reserve(n_buckets);
	for (int b = 0; b < n_buckets; ++b) {
		auto bk = std::make_unique<Bucket>();
		std::ostringstream oss;
		oss << bucket_dir << "/bucket_" << std::setw(4) << std::setfill('0') << b << ".dat";
		bk->path = oss.str();
		buckets_.push_back(std::move(bk));
	}
}

BucketWriter::~BucketWriter() {
	finish();
}

void BucketWriter::push(const BucketRecord& rec) {
	int b = static_cast<int>(rec.zone) / zones_per_bucket_;
	if (b >= n_buckets_) b = n_buckets_ - 1;

	// Try fast path: cache hit
	{
		std::lock_guard<std::mutex> lru_lock(lru_mtx_);
		auto it = lru_map_.find(b);
		if (it != lru_map_.end()) {
			FILE* f = it->second;
			// Move b to front of LRU list
			auto lit = std::find(lru_list_.begin(), lru_list_.end(), b);
			if (lit != lru_list_.end()) {
				lru_list_.erase(lit);
				lru_list_.push_front(b);
			}
			std::fwrite(&rec, sizeof(BucketRecord), 1, f);
			return;
		}
		// Cache miss — open file, maybe evict
		if (static_cast<int>(lru_map_.size()) >= MAX_OPEN) {
			int victim = lru_list_.back();
			lru_list_.pop_back();
			FILE* vf = lru_map_[victim];
			lru_map_.erase(victim);
			std::fclose(vf);
		}
		FILE* nf = std::fopen(buckets_[b]->path.c_str(), "ab");
		if (!nf) {
			std::cerr << "ERROR: cannot open " << buckets_[b]->path << ": " << std::strerror(errno) << "\n";
			std::exit(1);
		}
		std::fwrite(&rec, sizeof(BucketRecord), 1, nf);
		lru_list_.push_front(b);
		lru_map_[b] = nf;
	}
}

void BucketWriter::finish() {
	std::lock_guard<std::mutex> lru_lock(lru_mtx_);
	for (auto& [bucket, file] : lru_map_)
		std::fclose(file);
	lru_map_.clear();
	lru_list_.clear();
}

std::vector<std::string> BucketWriter::bucket_paths() const {
	std::vector<std::string> paths;
	for (const auto& bk : buckets_)
		paths.push_back(bk->path);
	return paths;
}
