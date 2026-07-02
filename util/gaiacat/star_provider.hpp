// Star data providers: abstract SkyChart .dat vs Gaia .bin reading.
// Both deliver the same units so the conversion pipeline is identical.

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

// ---------- SkyChart .dat reader ----------

class DatReader {
public:
	bool open(const std::string& path) {
		f_ = std::fopen(path.c_str(), "rb");
		return f_ != nullptr;
	}
	bool next(StarData& out) {
		if (!f_ || feof(f_)) return false;
		uint8_t buf[38];
		if (std::fread(buf, 38, 1, f_) != 1) return false;

		SkyChartRecord r;
		std::memcpy(&r, buf, 38);

		out.gaia_id  = r.gaia_id;
		out.ra_deg   = r.ra_raw  / 3600000.0;
		out.dec_deg  = r.dec_raw / 3600000.0 - 90.0;
		out.G_mag    = r.g_mag / 1000.0;

		bool have_color = (std::abs(r.bp_mag) < 30000 && std::abs(r.rp_mag) < 30000);
		if (have_color) {
			double bp = r.bp_mag / 1000.0;
			double rp = r.rp_mag / 1000.0;
			out.bp_rp = bp - rp;
		} else {
			out.bp_rp = NAN;
		}

		// SkyChart stores pmra/pmdec in arcsec/yr, parallax in arcsec
		out.pmra     = r.pmra * 1000.0;   // as/yr → mas/yr
		out.pmdec    = r.pmdec * 1000.0;
		out.parallax = r.plx  * 1000.0;   // as → mas
		return true;
	}
	void close() { if (f_) { std::fclose(f_); f_ = nullptr; } }

private:
	FILE* f_ = nullptr;
};

// ---------- Gaia .bin reader ----------

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
