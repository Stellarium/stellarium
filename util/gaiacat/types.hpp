// SkyChart Gaia DR3 .dat → Stellarium .cat converter
// Self-contained C++17, no Qt dependency. Uses std::thread for parallelism.
//
// Architecture:
//   Pass 1: Scan .dat files in parallel → compute zone + counts → write bucket files
//   Pass 2: Sort each bucket in parallel → write final .cat file
//
// Memory usage: <4 GB peak (for 12 billion stars)

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>

// SkyChart .dat record layout (38 bytes per star)
#pragma pack(push, 1)
struct alignas(1) SkyChartRecord {
	uint32_t ra_raw;       // 0:  RA  × 3,600,000
	uint32_t dec_raw;      // 4:  (DEC + 90) × 3,600,000
	uint64_t gaia_id;      // 8:  Gaia DR3 source ID
	int16_t  g_mag;         // 16: G magnitude × 1000
	int16_t  bp_mag;        // 18: BP magnitude × 1000
	int16_t  rp_mag;        // 20: RP magnitude × 1000
	float    pmra;          // 22: mas/yr (Gaia DR3 units); actually arcsec/yr in SkyChart .dat
	float    pmdec;         // 26: mas/yr (Gaia DR3 units); actually arcsec/yr in SkyChart .dat
	float    plx;           // 30: mas (Gaia DR3 units); actually arcsec in SkyChart .dat
	uint32_t _pad;          // 34: unused
};
#pragma pack(pop)
static_assert(sizeof(SkyChartRecord) == 38, "SkyChartRecord must be 38 bytes");

// Intermediate bucket record (36 bytes, one star encoded for one level)
#pragma pack(push, 1)
struct alignas(1) BucketRecord {
	uint32_t zone;          // geodesic zone index
	int16_t  vmag;          // V magnitude × 1000 (millimag)
	int16_t  bv;            // B-V × 1000
	int32_t  ra_i;          // RA × 3,600,000 (int)
	int32_t  dec_i;         // DEC × 3,600,000 (int)
	int64_t  gaia_id;       // Gaia DR3 source ID
	int32_t  pmra_i;        // pmra × 1000 (int)
	int32_t  pmdec_i;       // pmdec × 1000 (int)
	int32_t  plx_i;         // parallax × 100 (int)
};
#pragma pack(pop)
static_assert(sizeof(BucketRecord) == 36, "BucketRecord must be 36 bytes");

// Star2 .cat record (32 bytes) — matches Star2::Data in Star.hpp
#pragma pack(push, 1)
struct alignas(1) CatRecord {
	int64_t  gaia_id;       // 8 bytes
	int32_t  x0;            // 4 bytes, RA in mas
	int32_t  x1;            // 4 bytes, DEC in mas
	int32_t  dx0;           // 4 bytes, pmra in uas/yr
	int32_t  dx1;           // 4 bytes, pmdec in uas/yr
	int16_t  b_v;           // 2 bytes, B-V × 1000
	int16_t  vmag;          // 2 bytes, Vmag × 1000 (millimag)
	uint16_t plx;           // 2 bytes, parallax in 10 uas
	uint16_t plx_err;       // 2 bytes, parallax error (set to 0)
};
#pragma pack(pop)
static_assert(sizeof(CatRecord) == 32, "CatRecord must be 32 bytes");

// .cat file header constants
inline constexpr uint32_t  FILE_MAGIC     = 0x835F040A;
inline constexpr uint32_t  CATALOG_TYPE   = 1;          // Star2
inline constexpr uint32_t  CATALOG_MAJOR  = 0;
inline constexpr uint32_t  CATALOG_MINOR  = 1;
inline constexpr double    CATALOG_EPOCH  = 2457389.0;  // STELLAR_CATALOG_JDEPOCH = J2016.0

