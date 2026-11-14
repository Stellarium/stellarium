// Shared reader for Stellarium .cat star catalogs: validates the header
// (FILE_MAGIC, catalog type) and loads or skips the zone table.
#pragma once

#include "types.hpp"
#include "geodesic.hpp"

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

struct CatReader
{
	FILE* f = nullptr;
	uint32_t hdr[6] = {};
	float epoch = 0.f;
	uint32_t cat_type = 0;
	int level = 0;
	size_t rec_size = 0;
	int nzones = 0;
	std::vector<uint32_t> counts;
	int64_t star_base = 0;

	// Opens path, validates the header and leaves the file positioned at the
	// first star record. With read_counts the zone table is loaded into
	// counts, otherwise it is only seeked past. On failure returns false,
	// fills err and leaves f null.
	bool open(const std::string& path, std::string& err, bool read_counts = true)
	{
		f = std::fopen(path.c_str(), "rb");
		if (!f) {
			err = "cannot open " + path;
			return false;
		}
		bool ok = read_header(path, err) && read_zone_table(err, read_counts);
		if (!ok) {
			std::fclose(f);
			f = nullptr;
		}
		return ok;
	}

	uint64_t total_stars() const
	{
		uint64_t t = 0;
		for (uint32_t c : counts)
			t += c;
		return t;
	}

	void close()
	{
		if (f) {
			std::fclose(f);
			f = nullptr;
		}
	}

private:
	bool read_header(const std::string& path, std::string& err)
	{
		if (std::fread(hdr, sizeof(uint32_t), 6, f) != 6 ||
		    std::fread(&epoch, sizeof(float), 1, f) != 1) {
			err = path + " too short";
			return false;
		}
		if (hdr[0] != FILE_MAGIC) {
			char msg[160];
			std::snprintf(msg, sizeof(msg), "%s bad magic 0x%08X", path.c_str(), hdr[0]);
			err = msg;
			return false;
		}
		cat_type = hdr[1];
		level = static_cast<int>(hdr[4]);
		rec_size = catalog_record_size(cat_type);
		if (rec_size == 0) {
			err = path + " has unsupported catalog type " + std::to_string(cat_type);
			return false;
		}
		nzones = nr_of_zones(level);
		star_base = 28 + static_cast<int64_t>(nzones) * 4;
		return true;
	}

	bool read_zone_table(std::string& err, bool read_counts)
	{
		if (read_counts) {
			counts.resize(nzones);
			if (std::fread(counts.data(), sizeof(uint32_t), nzones, f) != static_cast<size_t>(nzones)) {
				err = "zone table truncated";
				return false;
			}
			return true;
		}
		if (std::fseek(f, static_cast<long>(star_base), SEEK_SET) != 0) {
			err = "seek past zone table failed";
			return false;
		}
		return true;
	}
};
