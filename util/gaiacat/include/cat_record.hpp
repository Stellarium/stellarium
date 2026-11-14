// Shared record-level codec for Stellarium .cat star catalogs (Star1/2/3):
// byte-offset helpers, physical decoding, B-V extraction and the small
// encode helpers that need no caller-side unit adaptation.
#pragma once

#include "types.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

inline uint32_t load_u24le(const uint8_t* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16);
}

inline void store_u24le(uint8_t out[3], uint32_t v)
{
	out[0] = static_cast<uint8_t>(v);
	out[1] = static_cast<uint8_t>(v >> 8);
	out[2] = static_cast<uint8_t>(v >> 16);
}

// Decoded physical representation of one star from any record format.
struct DecodedStar
{
	int64_t gaia_id = 0;
	int64_t match_key = 0;            // gaia_id, or HIP<<5|component composite when gaia_id==0
	double  ra_deg  = 0;
	double  dec_deg = 0;
	double  vmag    = 0;              // mag
	double  bv      = 0;              // mag; sentinel decodes to 32.767 (Star1/2) / 5.375 (Star3)
	bool    has_pm_plx = false;       // false for Star3
	double  pmra_masyr = 0, pmdec_masyr = 0, plx_mas = 0;
	bool    has_rv = false;           // Star1 only
	double  rv_kms = 0;
};

namespace cat_record_detail
{
// Star1 (48 bytes): 3D position vector x2e9, 3D pm vector in uas/yr, plx in 20 uas
inline DecodedStar decode_star1(const uint8_t* buf)
{
	DecodedStar s;
	int32_t x0, x1, x2, dx0, dx1, dx2;
	int16_t bv, vmag, rv;
	uint16_t plx;
	std::memcpy(&s.gaia_id, buf, 8);
	std::memcpy(&x0,  buf + 8,  4);
	std::memcpy(&x1,  buf + 12, 4);
	std::memcpy(&x2,  buf + 16, 4);
	std::memcpy(&dx0, buf + 20, 4);
	std::memcpy(&dx1, buf + 24, 4);
	std::memcpy(&dx2, buf + 28, 4);
	std::memcpy(&bv,  buf + 32, 2);
	std::memcpy(&vmag, buf + 34, 2);
	std::memcpy(&plx, buf + 36, 2);
	// plx_err at buf+38 (10 uas) not decoded
	std::memcpy(&rv,  buf + 40, 2);

	double nx = x0 / 2e9, ny = x1 / 2e9, nz = x2 / 2e9;
	s.ra_deg  = std::atan2(ny, nx) * 180.0 / M_PI;
	if (s.ra_deg < 0) s.ra_deg += 360.0;
	double r = std::sqrt(nx * nx + ny * ny + nz * nz);
	s.dec_deg = r > 0 ? std::asin(nz / r) * 180.0 / M_PI : 0;

	// project pm vector back to the local triad (p = east, q = north)
	double ra = s.ra_deg * M_PI / 180.0, dec = s.dec_deg * M_PI / 180.0;
	double p0 = -std::sin(ra), p1 = std::cos(ra);
	double q0 = -std::sin(dec) * std::cos(ra), q1 = -std::sin(dec) * std::sin(ra), q2 = std::cos(dec);
	s.pmra_masyr  = (dx0 * p0 + dx1 * p1) / 1000.0;
	s.pmdec_masyr = (dx0 * q0 + dx1 * q1 + dx2 * q2) / 1000.0;
	s.vmag = vmag / 1000.0;
	s.bv   = bv / 1000.0;
	s.plx_mas = plx / 50.0;           // 20 uas units
	s.has_pm_plx = true;
	s.has_rv = true;
	s.rv_kms = rv / 10.0;

	// match key: HIP<<5|component for stars without a Gaia id (hip[3] at bytes 45-47)
	uint32_t comb = buf[45] | (buf[46] << 8) | (buf[47] << 16);
	s.match_key = s.gaia_id > 0 ? s.gaia_id : ((int64_t)1 << 62) | comb;
	return s;
}

// Star2 (32 bytes): ra/dec in mas, pm in uas/yr (dx0 carries the cos(dec) factor),
// plx in 10 uas
inline DecodedStar decode_star2(const uint8_t* buf)
{
	DecodedStar s;
	int32_t x0, x1, dx0, dx1;
	int16_t bv, vmag;
	uint16_t plx;
	std::memcpy(&s.gaia_id, buf, 8);
	std::memcpy(&x0,   buf + 8,  4);
	std::memcpy(&x1,   buf + 12, 4);
	std::memcpy(&dx0,  buf + 16, 4);
	std::memcpy(&dx1,  buf + 20, 4);
	std::memcpy(&bv,   buf + 24, 2);
	std::memcpy(&vmag, buf + 26, 2);
	std::memcpy(&plx,  buf + 28, 2);
	s.ra_deg  = x0 / 3600000.0;
	s.dec_deg = x1 / 3600000.0;
	s.vmag    = vmag / 1000.0;
	s.bv      = bv / 1000.0;          // 32.767 marks missing BP-RP
	s.has_pm_plx = true;
	s.pmra_masyr  = dx0 / 1000.0;
	s.pmdec_masyr = dx1 / 1000.0;
	s.plx_mas     = plx / 100.0;
	s.match_key = s.gaia_id;
	return s;
}

// Star3 (16 bytes): ra/(dec+90) as u24 in 0.1 arcsec, bv in 0.025 mag offset -1,
// vmag in 0.02 mag offset 16; no pm/plx
inline DecodedStar decode_star3(const uint8_t* buf)
{
	DecodedStar s;
	std::memcpy(&s.gaia_id, buf, 8);
	s.ra_deg  = load_u24le(buf + 8) / 36000.0;
	s.dec_deg = load_u24le(buf + 11) / 36000.0 - 90.0;
	s.bv      = 0.025 * buf[14] - 1.0;   // 255 -> 5.375 marks missing BP-RP
	s.vmag    = 16.0 + 0.02 * buf[15];
	s.has_pm_plx = false;
	s.match_key = s.gaia_id;
	return s;
}
}  // namespace cat_record_detail

// Decode one record of the given catalog type (CATALOG_TYPE_STAR1/2/3).
inline DecodedStar decode_record(const uint8_t* buf, uint32_t cat_type)
{
	return cat_type == CATALOG_TYPE_STAR1 ? cat_record_detail::decode_star1(buf)
	     : cat_type == CATALOG_TYPE_STAR3 ? cat_record_detail::decode_star3(buf)
	                                      : cat_record_detail::decode_star2(buf);
}

// Extract the B-V of one record in millimag; returns false when it is a
// missing sentinel (32.767 for Star1/2, 255 raw for Star3).
inline bool extract_record_bv(const uint8_t* buf, uint32_t cat_type, uint64_t& sid, int16_t& bv)
{
	std::memcpy(&sid, buf, 8);
	if (cat_type == CATALOG_TYPE_STAR1) {
		std::memcpy(&bv, buf + 32, 2);
		return bv != STAR2_BV_MISSING;
	}
	if (cat_type == CATALOG_TYPE_STAR2) {
		std::memcpy(&bv, buf + 24, 2);
		return bv != STAR2_BV_MISSING;
	}
	const uint8_t bvr = buf[14];
	if (bvr == STAR3_BV_MISSING)
		return false;
	bv = static_cast<int16_t>(std::lround((bvr / 40.0 - 1.0) * 1000.0));
	return true;
}

// Quantize B-V millimag to the Star3 raw byte (0.025 mag, offset -1.0),
// clamped to [0, 254]; 255 is the missing sentinel. Sets *clamped when given.
inline uint8_t bv_milli_to_star3_raw(int16_t bv_milli, bool* clamped = nullptr)
{
	int raw = static_cast<int>(std::lround((bv_milli / 1000.0 + 1.0) * 40.0));
	bool c = false;
	if (raw < 0)   { raw = 0;   c = true; }
	if (raw > 254) { raw = 254; c = true; }
	if (clamped) *clamped = c;
	return static_cast<uint8_t>(raw);
}

// Overwrite the B-V field of one record in place, encoded per record type.
inline void overwrite_bv_field(uint32_t cat_type, uint8_t* buf, int16_t bv_milli)
{
	if (cat_type == CATALOG_TYPE_STAR1) {
		std::memcpy(buf + 32, &bv_milli, 2);
	} else if (cat_type == CATALOG_TYPE_STAR2) {
		std::memcpy(buf + 24, &bv_milli, 2);
	} else {
		buf[14] = bv_milli_to_star3_raw(bv_milli);
	}
}
