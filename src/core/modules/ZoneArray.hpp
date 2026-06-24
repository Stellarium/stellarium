/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
 * The implementation of SpecialZoneArray<Star>::draw is based on
 * Stellarium, Copyright (C) 2002 Fabien Chereau,
 * and therefore has shared copyright.
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

#ifndef ZONEARRAY_HPP
#define ZONEARRAY_HPP

#include "ZoneData.hpp"
#include "Star.hpp"
#include "StarMgr.hpp"

#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"

#include <QString>
#include <QFile>
#include <QCache>
#include <QByteArray>
#include <QFuture>
#include <QHash>
#include <QMutex>
#include <QtConcurrent>
#include <QDebug>
#include <vector>

#ifdef __OpenBSD__
#include <unistd.h>
#endif

class StelPainter;

// Patch by Rainer Canavan for compilation on irix with mipspro compiler part 1
#ifndef MAP_NORESERVE
#  ifdef MAP_AUTORESRV
#    if (defined(__sgi) && defined(_COMPILER_VERSION))
#      define MAP_NORESERVE MAP_AUTORESRV
#    endif
#  else
#    define MAP_NORESERVE 0
#  endif
#endif


#define NR_OF_HIP 120416
#define FILE_MAGIC 0x835f040a
#define FILE_MAGIC_OTHER_ENDIAN 0x0a045f83
#define FILE_MAGIC_NATIVE 0x835f040b
#define MAX_MAJOR_FILE_VERSION 0

//! @struct HipIndexStruct
//! Container for Hipparcos information. Stores a pointer to a Hipparcos star,
//! its catalog and its triangle.
struct HipIndexStruct
{
	const SpecialZoneArray<Star1> *a;
	const SpecialZoneData<Star1> *z;
	const Star1 *s;
};

//! @class ZoneArray
//! Non-template abstract interface for a catalog loaded at one GeodesicGrid level.
//! Subclasses implement how star data is stored and loaded.
class ZoneArray
{
public:
	static ZoneArray *create(const QString &extended_file_name, bool use_mmap, bool use_dynamic = false);
	virtual ~ZoneArray() { nr_of_zones = 0; }

	unsigned int getNrOfStars() const { return nr_of_stars; }
	unsigned int getNrOfZones() const { return nr_of_zones; }

	virtual void updateHipIndex(HipIndexStruct hipIndex[]) const { Q_UNUSED(hipIndex); }
	virtual void prefetchRegion(const QVector<SphericalCap>& caps, int maxGridLevel) const { Q_UNUSED(caps); Q_UNUSED(maxGridLevel); }

	virtual void searchAround(const StelCore* core, int index, const Vec3d &v,
				  const double withParallax, const Vec3d diffPos,
				  double cosLimFov, QList<StelObjectP> &result) = 0;
	virtual void searchWithin(const StelCore* core, int index, const SphericalRegionP region,
				  const double withParallax, const Vec3d diffPos,
				  const bool hipOnly, const float maxMag,
				  QList<StelObjectP> &result) const = 0;
	virtual StelObjectP searchGaiaID(int index, const StarId source_id, int& matched) const = 0;
	virtual void searchGaiaIDepochPos(const StarId source_id, float dyrs,
					  double & RA, double & DEC, double & Plx,
					  double & pmra, double & pmdec, double & RV) const = 0;

	virtual void draw(StelPainter* sPainter, int index, bool is_inside,
			  const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
			  int maxMagStarName, float names_brightness,
			  const QVector<SphericalCap>& boundingCaps,
			  const bool withAberration, const Vec3d vel,
			  const double withParallax, const Vec3d diffPos,
			  const bool withCommonNameI18n) const = 0;

	bool isInitialized(void) const { return (nr_of_zones > 0); }
	void initTriangle(int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2);

	const QString fname;
	const int level;
	const int mag_min;

protected:
	static bool readFile(QFile& file, void *data, qint64 size);
	ZoneArray(const QString& fname, QFile* file, int level, int mag_min);
	unsigned int nr_of_zones;
	unsigned int nr_of_stars;
	ZoneData *zones;
	QFile* file;
};

//! @class ZoneArrayImpl
//! CRTP template that implements all ZoneArray virtual methods using shared logic.
//! Derived classes only need to provide:
//!   - loadZoneDraw(int index, const StarType*& stars, uint32_t& size, bool& isGlobal) const
//!   - loadZoneSearch(int index, const StarType*& stars, uint32_t& size, bool& isGlobal) const
//!   - numberOfZones() const → nr_of_zones
//! And optionally override prefetchRegion(), updateHipIndex().
template<class Derived, class StarType>
class ZoneArrayImpl : public ZoneArray
{
protected:
	ZoneArrayImpl(const QString& fname, QFile* file, int level, int mag_min)
		: ZoneArray(fname, file, level, mag_min) {}

	//! Draw a contiguous range of stars. Shared between all subclasses.
	void drawStarRange(const StarType* first, const StarType* last, bool globalzone,
			   StelPainter* sPainter, bool isInsideViewport,
			   const RCMag* rcmag_table, int cutoffMagStep, float cutoffMag,
			   float dyrs, StelCore* core, StelSkyDrawer* drawer,
			   const QVector<SphericalCap>& boundingCaps,
			   bool withAberration,
			   double withParallax, const Vec3d& diffPos,
			   int maxMagStarName, float names_brightness,
			   bool withExtinction, const Extinction& extinction,
			   bool withCommonNameI18n) const;

public:
	// ----- ZoneArray virtual overrides (shared implementation) -----

	void draw(StelPainter* sPainter, int index, bool isInsideViewport,
		  const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
		  int maxMagStarName, float names_brightness,
		  const QVector<SphericalCap>& boundingCaps,
		  const bool withAberration, const Vec3d vel,
		  const double withParallax, const Vec3d diffPos,
		  const bool withCommonNameI18n) const final;

	void searchAround(const StelCore* core, int index, const Vec3d &v,
			  const double withParallax, const Vec3d diffPos,
			  double cosLimFov, QList<StelObjectP> &result) final;

	void searchWithin(const StelCore* core, int index, const SphericalRegionP region,
			  const double withParallax, const Vec3d diffPos,
			  const bool hipOnly, const float maxMag,
			  QList<StelObjectP> &result) const final;

	StelObjectP searchGaiaID(int index, const StarId source_id, int& matched) const final;

	void searchGaiaIDepochPos(const StarId source_id, float dyrs,
				  double & RA, double & DEC, double & Plx,
				  double & pmra, double & pmdec, double & RV) const final;

	StelObjectP wrapStar(const StarType* s, int zoneIndex) const;

private:
	// CRTP helpers that delegate to Derived
	const Derived& self() const { return *static_cast<const Derived*>(this); }

	// Load a zone for drawing (may be async for DynamicZoneArray).
	bool zoneForDraw(int index, const StarType*& stars, uint32_t& size, bool& isGlobal) const
	{
		return self().loadZoneDraw(index, stars, size, isGlobal);
	}

	// Load a zone for search (synchronous).
	bool zoneForSearch(int index, const StarType*& stars, uint32_t& size, bool& isGlobal) const
	{
		return self().loadZoneSearch(index, stars, size, isGlobal);
	}

	StelObjectP starToStelObject(const StarType* s) const
	{
		return self().createStelObj(s);
	}
};

//! @class SpecialZoneArray
//! ZoneArray that mmap's or reads the entire star catalog into memory.
//! @tparam StarType either Star1, Star2 or Star3.
template<class StarType>
class SpecialZoneArray : public ZoneArrayImpl<SpecialZoneArray<StarType>, StarType>
{
	using Base = ZoneArrayImpl<SpecialZoneArray<StarType>, StarType>;
	friend Base;
public:
	SpecialZoneArray(QFile* file, bool byte_swap, bool use_mmap, int level, int mag_min);
	~SpecialZoneArray(void) override;
protected:
	SpecialZoneData<StarType> *getZones(void) const
	{
		return static_cast<SpecialZoneData<StarType>*>(this->zones);
	}
	StarType *stars;
private:
	uchar *mmap_start;

	// CRTP hooks for ZoneArrayImpl
	bool loadZoneDraw(int index, const StarType*& outStars, uint32_t& size, bool& isGlobal) const;
	bool loadZoneSearch(int index, const StarType*& outStars, uint32_t& size, bool& isGlobal) const;
	StelObjectP createStelObj(const StarType* s) const;
};

//! @class HipZoneArray
//! ZoneArray of Hipparcos stars.
class HipZoneArray : public SpecialZoneArray<Star1>
{
public:
	HipZoneArray(QFile* file, bool byte_swap, bool use_mmap, int level, int mag_min)
		: SpecialZoneArray<Star1>(file, byte_swap, use_mmap, level, mag_min) {}
	void updateHipIndex(HipIndexStruct hipIndex[]) const override;
};

//! @class DynamicZoneArray
//! ZoneArray that loads star data on demand from disk, cached in an LRU QCache.
//! @tparam StarType either Star1, Star2 or Star3.
template<class StarType>
class DynamicZoneArray : public ZoneArrayImpl<DynamicZoneArray<StarType>, StarType>
{
	using Base = ZoneArrayImpl<DynamicZoneArray<StarType>, StarType>;
	friend Base;
public:
	DynamicZoneArray(const QString& fname, QFile* file, int level, int mag_min, bool byte_swap);
	~DynamicZoneArray(void) override;

	const StarType* loadZone(int zone_index) const;
	const StarType* loadZoneSync(int zone_index) const;
	uint32_t zoneStarCount(int zone_index) const { return zoneCounts_[zone_index]; }
	void prefetchRegion(const QVector<SphericalCap>& caps, int maxGridLevel) const;

private:
	static constexpr int BLOCK_SIZE = 128;

	uint32_t* zoneCounts_;
	uchar* zoneTableMmapStart_;
	std::vector<uint64_t> blockOffsets_;

	mutable QCache<int, QByteArray> zoneCache_;
	mutable QHash<int, QFuture<QByteArray*>> pendingLoads_;
	mutable QMutex fileMutex_;
	mutable bool fileValid_{true};

	void drainPendingLoads() const;

	// CRTP hooks for ZoneArrayImpl
	bool loadZoneDraw(int index, const StarType*& outStars, uint32_t& size, bool& isGlobal) const;
	bool loadZoneSearch(int index, const StarType*& outStars, uint32_t& size, bool& isGlobal) const;
	StelObjectP createStelObj(const StarType* s) const;
};

#endif // ZONEARRAY_HPP
