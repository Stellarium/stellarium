/*
 * Stellarium
 * Copyright (C) 2026 Stellarium Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef DYNAMICZONEARRAY_HPP
#define DYNAMICZONEARRAY_HPP

#include "ZoneArray.hpp"
#include "Star.hpp"

#include <QCache>
#include <QFile>
#include <QByteArray>
#include <vector>

//! @class DynamicZoneArray
//! ZoneArray variant that loads star data on demand from disk.
//! Supports Star2 (type=1) records. Zone table is mmap'd; star data is
//! fetched per-zone via seek+read and cached in an LRU QCache.
class DynamicZoneArray : public ZoneArray
{
public:
	DynamicZoneArray(const QString& fname, QFile* file, int level, int mag_min);
	~DynamicZoneArray(void) override;

	void draw(StelPainter* sPainter, int index, bool isInsideViewport,
		  const RCMag *rcmag_table, int limitMagIndex, StelCore* core,
		  int maxMagStarName, float names_brightness,
		  const QVector<SphericalCap>& boundingCaps,
		  const bool withAberration, const Vec3d vel, const double withParallax,
		  const Vec3d diffPos, const bool withCommonNameI18n) const override;

	void searchAround(const StelCore* core, int index, const Vec3d &v,
			  const double withParallax, const Vec3d diffPos,
			  double cosLimFov, QList<StelObjectP> &result) override;

	void searchWithin(const StelCore* core, int index, const SphericalRegionP region,
			  const double withParallax, const Vec3d diffPos,
			  const bool hipOnly, const float maxMag,
			  QList<StelObjectP> &result) const override;

	StelObjectP searchGaiaID(int index, const StarId source_id, int& matched) const override;

	void searchGaiaIDepochPos(const StarId source_id, float dyrs,
				  double & RA, double & DEC, double & Plx,
				  double & pmra, double & pmdec, double & RV) const override;

	//! Load zone data from disk into memory. Returns pointer to Star2 array.
	const Star2* loadZone(int zone_index) const;

	//! Get star count for a zone (0 if empty).
	uint32_t zoneStarCount(int zone_index) const { return zoneCounts_[zone_index]; }

	//! Prefetch zones in the given viewport region (async-friendly).
	//! Zones already in cache are skipped. Call after draw() to warm the cache
	//! for anticipated user scrolling.
	void prefetchRegion(const QVector<SphericalCap>& caps, int maxGridLevel) const;

private:
	//! Zone size granularity for the block-based offset table.
	//! Smaller values = smaller block table, faster per-zone block-local summation.
	static constexpr int BLOCK_SIZE = 128;

	//! Mmap'd zone table from the .cat file (star count per zone, N × 4 bytes).
	const uint32_t* zoneCounts_;

	//! Mmap start for zone table unmap.
	uchar* zoneTableMmapStart_;

	//! Block-level cumulative star data offsets.
	//! blockOffsets_[b] = byte offset of the first star in zone b*BLOCK_SIZE,
	//! relative to the star data area start.
	std::vector<uint64_t> blockOffsets_;

	//! LRU cache of zone data (zone index → raw Star3 buffer).
	//! QCache takes ownership and deletes on eviction.
	mutable QCache<int, QByteArray> zoneCache_;
};

#endif // DYNAMICZONEARRAY_HPP
