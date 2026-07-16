// Gaia .bin reader: delivers star data in a common format for the conversion pipeline.

#pragma once

#include "types.hpp"
#include "convert.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <algorithm>

struct StarData {
	int64_t gaia_id = 0;
	double  ra_deg  = 0;    // degrees
	double  dec_deg = 0;    // degrees
	double  G_mag   = 0;    // Gaia G magnitude (or NaN)
	double  bp_rp   = NAN;  // BP-RP color (NaN if unavailable)
	double  pmra    = 0;    // mas/yr (Gaia: includes cosδ factor)
	double  pmdec   = 0;    // mas/yr
	double  parallax= 0;    // mas
};

class BinReader {
public:
	bool open(const std::string& path) {
		f_ = std::fopen(path.c_str(), "rb");
		return f_ != nullptr;
	}
	bool next(StarData& out) {
		if (!f_ || feof(f_)) return false;
		// Record layout: q d d d f d d f f = 60 bytes (packed, no padding)
		#pragma pack(push, 1)
		struct { int64_t sid; double ra,dec,parallax; float plx_err; double pmra,pmdec; float G,bp_rp; } rec;
		#pragma pack(pop)
		if (std::fread(&rec, 60, 1, f_) != 1) return false;

		out.gaia_id  = rec.sid;
		out.ra_deg   = rec.ra;
		out.dec_deg  = rec.dec;
		out.G_mag    = rec.G;
		out.bp_rp    = std::isnan(rec.bp_rp) ? NAN : (double)rec.bp_rp;
		out.pmra     = rec.pmra;
		out.pmdec    = rec.pmdec;
		out.parallax = rec.parallax;
		return true;
	}
	void close() { if (f_) { std::fclose(f_); f_ = nullptr; } }

private:
	FILE* f_ = nullptr;
};
