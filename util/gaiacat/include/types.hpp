// Gaia DR3 .bin → Stellarium .cat converter
// Self-contained C++17, no Qt dependency. Uses std::thread for parallelism.
//
// Architecture:
//   Pass 1: Scan .bin files in parallel → compute zone + counts → write bucket files
//   Pass 2: Sort each bucket in parallel → write final .cat file
//
// Memory usage: <4 GB peak (for 12 billion stars)

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>

// Intermediate bucket record (44 bytes, one star encoded for one level)
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
	int32_t  plx_err_i;     // parallax error × 100 (int)
	int32_t  rv_i;          // radial velocity × 10 (int, 0.1 km/s)
};
#pragma pack(pop)
static_assert(sizeof(BucketRecord) == 44, "BucketRecord must be 44 bytes");

// Star1 .cat record (48 bytes) — matches Star1::Data in Star.hpp
#pragma pack(push, 1)
struct alignas(1) CatRecord1 {
	int64_t  gaia_id;       // 8 bytes
	int32_t  x0, x1, x2;    // 12 bytes, position unit vector × 2e9
	int32_t  dx0, dx1, dx2; // 12 bytes, 3D proper-motion vector in uas/yr
	int16_t  b_v;           // 2 bytes, B-V × 1000 (32767 = missing)
	int16_t  vmag;          // 2 bytes, V × 1000 (millimag)
	uint16_t plx;           // 2 bytes, parallax in 20 uas
	uint16_t plx_err;       // 2 bytes, parallax error in 10 uas
	int16_t  rv;            // 2 bytes, radial velocity in 0.1 km/s
	uint16_t spInt;         // 2 bytes, spectral type index (0 = no information)
	uint8_t  objtype;       // 1 byte, object type index (0 = no information)
	uint8_t  hip[3];        // 3 bytes, HIP number (17 bit) + component ID (5 bit)
};
#pragma pack(pop)
static_assert(sizeof(CatRecord1) == 48, "CatRecord1 must be 48 bytes");

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
	uint16_t plx_err;       // 2 bytes, parallax error in 10 uas
};
#pragma pack(pop)
static_assert(sizeof(CatRecord) == 32, "CatRecord must be 32 bytes");

// Star3 .cat record (16 bytes) — matches Star3::Data in Star.hpp
// No proper motion or parallax; coarser quantization for faint stars.
#pragma pack(push, 1)
struct alignas(1) CatRecord3 {
	int64_t  gaia_id;       // 8 bytes
	uint8_t  x0[3];         // 3 bytes, RA in 0.1 arcsecond (24-bit little-endian)
	uint8_t  x1[3];         // 3 bytes, DEC in 0.1 arcsecond, offset +90 deg (24-bit LE)
	uint8_t  b_v;           // 1 byte, B-V: raw = (B-V + 1.0) / 0.025; 255 = missing
	uint8_t  vmag;          // 1 byte, Vmag: raw = (V - 16.0 mag) / 0.02 mag
};
#pragma pack(pop)
static_assert(sizeof(CatRecord3) == 16, "CatRecord3 must be 16 bytes");

// .cat file header constants
inline constexpr uint32_t  FILE_MAGIC         = 0x835F040A;
inline constexpr uint32_t  CATALOG_TYPE_STAR1 = 0;      // Star1
inline constexpr uint32_t  CATALOG_TYPE_STAR2 = 1;      // Star2
inline constexpr uint32_t  CATALOG_TYPE_STAR3 = 2;      // Star3
// Default catalog version (major/minor); the actual file name and header
// version are derived from the official starsConfig.json entry (minor + 1).
inline constexpr uint32_t  CATALOG_MAJOR      = 0;
inline constexpr uint32_t  CATALOG_MINOR      = 1;
inline constexpr double    CATALOG_EPOCH      = 2457389.0;  // STELLAR_CATALOG_JDEPOCH = J2016.0

// Missing-BP-RP sentinels in raw on-disk representation
inline constexpr int16_t  STAR2_BV_MISSING = 32767;     // std::numeric_limits<int16_t>::max()
inline constexpr uint8_t  STAR3_BV_MISSING = 255;       // std::numeric_limits<uint8_t>::max()

// On-disk record size for a given catalog type
inline constexpr size_t catalog_record_size(uint32_t type)
{
	return type == CATALOG_TYPE_STAR1 ? sizeof(CatRecord1)
	     : type == CATALOG_TYPE_STAR2 ? sizeof(CatRecord)
	     : type == CATALOG_TYPE_STAR3 ? sizeof(CatRecord3)
	     : 0;
}

