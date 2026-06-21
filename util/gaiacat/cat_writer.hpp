// .cat file writer: reads sorted bucket files, writes final Star2 catalog
#pragma once
#include "types.hpp"
#include "geodesic.hpp"
#include <string>
#include <vector>
#include <cstdint>

// Write a complete .cat file from bucket files
// bucket_paths: paths to all bucket_NNNN.dat files
// counts: per-zone star count (size = nr_of_zones)
// level: geodesic level
// mag_min: minimum V magnitude for this catalog (millimag)
// out_path: output .cat file path
// n_sort_threads: number of parallel sort workers
void write_cat(const std::vector<std::string>& bucket_paths,
	       const std::vector<uint32_t>& counts,
	       int level, int mag_min,
	       const std::string& out_path,
	       int n_sort_threads);
