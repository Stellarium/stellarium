// Catalog file naming: reads the Stellarium user starsConfig.json to derive
// our output file names as "official minor version + 1".
//
// Official names follow  stars_<level>_<type>v<major>_<minor>.cat
// where <type> is the star format (0=Star1, 1=Star2, 2=Star3, same as the
// file header's type field), <major>/<minor> are the catalog version
// (e.g. stars_0_0v0_21.cat: Star1, major 0, minor 21). We keep the star
// format chosen by the generator script (which overrides starsConfig), take
// the official <major> when present, and bump the official <minor> by 1 so
// that a Stellarium upgrade to our catalogs is an unambiguous version bump
// (minor 1 when the official entry is missing).
#pragma once

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Windows: %APPDATA%\Stellarium\stars\hip_gaia3\starsConfig.json
// POSIX:   $HOME/.stellarium/stars/hip_gaia3/starsConfig.json
static inline std::string default_stars_config_path()
{
#ifdef _WIN32
	const char* ap = std::getenv("APPDATA");
	if (ap && *ap)
		return std::string(ap) + "\\Stellarium\\stars\\hip_gaia3\\starsConfig.json";
#endif
	const char* home = std::getenv("HOME");
	if (home && *home)
		return std::string(home) + "/.stellarium/stars/hip_gaia3/starsConfig.json";
	return {};
}

// Read the whole file into a string; empty on error.
static inline std::string read_file_text(const std::string& path)
{
	FILE* f = std::fopen(path.c_str(), "rb");
	if (!f) return {};
	std::fseek(f, 0, SEEK_END);
	long n = std::ftell(f);
	std::fseek(f, 0, SEEK_SET);
	std::string text;
	if (n > 0) {
		text.resize(static_cast<size_t>(n));
		if (std::fread(&text[0], 1, static_cast<size_t>(n), f) != static_cast<size_t>(n))
			text.clear();
	}
	std::fclose(f);
	return text;
}

// Official fileName for the given level from starsConfig.json
// (a catalog entry is identified by its fileName "stars_<level>_...";
// key order inside entries is not assumed), or "" if not found.
static inline std::string official_cat_filename(const std::string& json_path, int level)
{
	const std::string text = read_file_text(json_path);
	if (text.empty()) return {};
	size_t p = 0;
	while (true) {
		p = text.find("\"fileName\"", p);
		if (p == std::string::npos) break;
		const size_t a = text.find('"', p + 10);
		if (a == std::string::npos) break;
		const size_t b = text.find('"', a + 1);
		if (b == std::string::npos) break;
		const std::string name = text.substr(a + 1, b - a - 1);
		int lv = -1;
		if (std::sscanf(name.c_str(), "stars_%d_", &lv) == 1 && lv == level)
			return name;
		p = b + 1;
	}
	return {};
}

// Parse "stars_<lv>_<type>v<major>_<minor>.cat" -> type/major/minor.
// Returns true on success. Pointers may be null to skip a field.
static inline bool parse_cat_version(const std::string& fname, int* type_out,
                                     int* major_out, int* minor_out)
{
	int type = -1, major = -1, minor = -1;
	if (std::sscanf(fname.c_str(), "stars_%*d_%dv%d_%d.cat", &type, &major, &minor) == 3) {
		if (type_out) *type_out = type;
		if (major_out) *major_out = major;
		if (minor_out) *minor_out = minor;
		return true;
	}
	return false;
}

// Build our output name: our own <level> and <type> (generator's choice
// overrides starsConfig), official <major> (0 if none), official <minor> + 1
// (minor 1 when there is no official entry).
static inline std::string next_cat_filename(int level, uint32_t cat_type,
                                            const std::string& official_name)
{
	int major = 0, minor = 1;
	if (!official_name.empty()) {
		int otype = -1, omajor = -1, ominor = -1;
		if (parse_cat_version(official_name, &otype, &omajor, &ominor)) {
			if (omajor >= 0) major = omajor;
			if (ominor >= 0) minor = ominor + 1;
		}
	}
	char buf[64];
	std::snprintf(buf, sizeof(buf), "stars_%d_%dv%d_%d.cat", level, cat_type, major, minor);
	return buf;
}

// ---- spectral type table ("stars_hip_sp_<type>v<major>_<minor>.cat") ----

static inline std::string json_string_value(const std::string& text, const char* key)
{
	const std::string k = "\"" + std::string(key) + "\"";
	const size_t p = text.find(k);
	if (p == std::string::npos) return {};
	const size_t a = text.find('"', p + k.size());
	if (a == std::string::npos) return {};
	const size_t b = text.find('"', a + 1);
	if (b == std::string::npos) return {};
	return text.substr(a + 1, b - a - 1);
}

// Official spectral table fileName from starsConfig.json ("hipSpectralFile"),
// or "" if not found.
static inline std::string official_sp_filename(const std::string& json_path)
{
	return json_string_value(read_file_text(json_path), "hipSpectralFile");
}

// Parse "stars_hip_sp_<type>v<major>_<minor>.cat" -> type/major/minor.
static inline bool parse_sp_version(const std::string& fname, int* type_out,
                                    int* major_out, int* minor_out)
{
	int type = -1, major = -1, minor = -1;
	if (std::sscanf(fname.c_str(), "stars_hip_sp_%dv%d_%d.cat", &type, &major, &minor) == 3) {
		if (type_out) *type_out = type;
		if (major_out) *major_out = major;
		if (minor_out) *minor_out = minor;
		return true;
	}
	return false;
}

// Build our output spectral-table name: official <type>/<major>, official
// <minor> + 1 (type 0 / major 0 / minor 1 when there is no official entry).
static inline std::string next_sp_filename(const std::string& official_name)
{
	int type = 0, major = 0, minor = 1;
	if (!official_name.empty()) {
		int otype = -1, omajor = -1, ominor = -1;
		if (parse_sp_version(official_name, &otype, &omajor, &ominor)) {
			if (otype >= 0) type = otype;
			if (omajor >= 0) major = omajor;
			if (ominor >= 0) minor = ominor + 1;
		}
	}
	char buf[64];
	std::snprintf(buf, sizeof(buf), "stars_hip_sp_%dv%d_%d.cat", type, major, minor);
	return buf;
}
