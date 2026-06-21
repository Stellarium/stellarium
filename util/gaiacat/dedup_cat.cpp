// Deduplicate a Stellarium .cat file against a reference catalog.
// Stars whose Gaia ID appears in the reference are removed.
// Single pass over input: unordered_set O(1) lookup, sequential output write.

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>

static int nr_of_zones(int level) {
	return 20 * (1 << (level * 2)) + 1;
}

static void dedup(const std::string& ref_path, const std::string& in_path, const std::string& out_path) {
	// ── Read reference: build unordered_set ──
	std::unordered_set<uint64_t> ref_ids;

	{
		FILE* f = std::fopen(ref_path.c_str(), "rb");
		if (!f) { std::cerr << "ERROR: cannot open " << ref_path << "\n"; return; }

		uint32_t hdr[6]; float ep;
		std::fread(hdr, sizeof(uint32_t), 6, f);
		std::fread(&ep, sizeof(float), 1, f);
		int level_ref = static_cast<int>(hdr[4]);
		int nz = nr_of_zones(level_ref);
		auto counts = std::vector<uint32_t>(nz);
		fseeko(f, 28, SEEK_SET);
		std::fread(counts.data(), sizeof(uint32_t), nz, f);

		uint64_t total_ref = 0;
		for (auto c : counts) total_ref += c;
		std::cout << "Reference: level=" << level_ref << ", " << total_ref << " stars\n";

		int64_t base = 28 + static_cast<int64_t>(nz) * 4;
		int64_t off = base;
		uint8_t buf[32];
		ref_ids.reserve(static_cast<size_t>(total_ref));
		for (int z = 0; z < nz; ++z) {
			uint32_t cnt = counts[z];
			if (cnt == 0) continue;
			fseeko(f, off, SEEK_SET);
			for (uint32_t i = 0; i < cnt; ++i) {
				std::fread(buf, 32, 1, f);
				uint64_t gid;
				std::memcpy(&gid, buf, 8);
				ref_ids.insert(gid);
			}
			off += static_cast<int64_t>(cnt) * 32;
		}
		std::fclose(f);
		std::cout << "  " << ref_ids.size() << " unique IDs loaded\n";
	}

	// ── Process input: zone-by-zone write to output ──
	FILE* fin = std::fopen(in_path.c_str(), "rb");
	if (!fin) { std::cerr << "ERROR: cannot open " << in_path << "\n"; return; }

	uint32_t hdr[6]; float epoch;
	std::fread(hdr, sizeof(uint32_t), 6, fin);
	std::fread(&epoch, sizeof(float), 1, fin);
	int level_in = static_cast<int>(hdr[4]);
	int nz = nr_of_zones(level_in);

	auto in_counts = std::vector<uint32_t>(nz);
	fseeko(fin, 28, SEEK_SET);
	std::fread(in_counts.data(), sizeof(uint32_t), nz, fin);

	uint64_t total_in = 0;
	for (auto c : in_counts) total_in += c;
	std::cout << "Input:    level=" << level_in << ", " << total_in << " stars\n";

	// Output: write header + placeholder zone table, then stream stars
	FILE* fout = std::fopen(out_path.c_str(), "wb");
	if (!fout) { std::cerr << "ERROR: cannot create " << out_path << "\n"; std::fclose(fin); return; }

	std::fwrite(hdr, sizeof(uint32_t), 6, fout);
	std::fwrite(&epoch, sizeof(float), 1, fout);

	// Placeholder zone table (will be overwritten at the end)
	auto out_counts = std::vector<uint32_t>(nz, 0);
	std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);

	int64_t in_base = 28 + static_cast<int64_t>(nz) * 4;
	int64_t in_off = in_base;
	uint64_t total_out = 0;
	uint64_t removed = 0;
	uint8_t buf[32];

	for (int z = 0; z < nz; ++z) {
		uint32_t cnt = in_counts[z];
		if (cnt == 0) continue;

		fseeko(fin, in_off, SEEK_SET);
		for (uint32_t i = 0; i < cnt; ++i) {
			std::fread(buf, 32, 1, fin);
			uint64_t gid;
			std::memcpy(&gid, buf, 8);
			if (ref_ids.find(gid) != ref_ids.end()) {
				removed++;
			} else {
				std::fwrite(buf, 32, 1, fout);
				out_counts[z]++;
				total_out++;
			}
		}
		in_off += static_cast<int64_t>(cnt) * 32;
	}

	// Write corrected zone table
	fseeko(fout, 28, SEEK_SET);
	std::fwrite(out_counts.data(), sizeof(uint32_t), nz, fout);

	std::fclose(fin);
	std::fclose(fout);

	std::cout << "  Removed: " << removed << " (" << (100.0*removed/(total_in?total_in:1)) << "%)\n";
	std::cout << "  Output:  " << total_out << " stars\n";

	auto size_mb = static_cast<double>(28 + nz*4 + total_out*32) / 1048576.0;
	std::cout << "  Written: " << out_path << " (" << size_mb << " MB)\n";
}

int main(int argc, char** argv) {
	if (argc != 4) {
		std::cerr << "Usage: dedup_cat <reference.cat> <input.cat> <output.cat>\n";
		return 1;
	}
	dedup(argv[1], argv[2], argv[3]);
	return 0;
}
