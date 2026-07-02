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
#include <QHash>
#include <QMutex>
#include <QtConcurrent>
#include <QDebug>
#include <vector>

#ifdef __OpenBSD__
#include <unistd.h>
#endif

class StelPainter;

template<class Star> class SpecialZoneArray;

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
	//! Named public constructor for ZoneArray. Opens a catalog, reads its
	//! header info, and creates a SpecialZoneArray, HipZoneArray or
	//! DynamicZoneArray for loading.
	//! @param extended_file_name path of the star catalog to load from
	//! @param use_mmap whether or not to mmap the star catalog
	//! @param use_dynamic whether or not to use DynamicZoneArray
	//! @return an instance of SpecialZoneArray or HipZoneArray
	static ZoneArray *create(const QString &extended_file_name, bool use_mmap, bool use_dynamic = false);
	virtual ~ZoneArray()
	{
		nr_of_zones = 0;
	}

	//! Get the total number of stars in this catalog.
	unsigned int getNrOfStars() const { return nr_of_stars; }
	unsigned int getNrOfZones() const { return nr_of_zones; }

	//! Dummy method that does nothing. See subclass implementation.
	virtual void updateHipIndex(HipIndexStruct hipIndex[]) const {Q_UNUSED(hipIndex)}

	//! Pure virtual method. See subclass implementation.
	virtual void searchAround(const StelCore* core, int index, const Vec3d &v, const double withParallax, const Vec3d diffPos,
							  double cosLimFov, QList<StelObjectP > &result) = 0;
	virtual void searchWithin(const StelCore* core, int index, const SphericalRegionP region, const double withParallax, const Vec3d diffPos, const bool hipOnly, const float maxMag,
							  QList<StelObjectP > &result) const = 0;

	virtual StelObjectP searchGaiaID(int index, const StarId source_id, int& matched) const = 0;
	virtual void searchGaiaIDepochPos(const StarId source_id, float dyrs,
                                                  double & RA,
                                                  double & DEC,
                                                  double & Plx,
                                                  double & pmra,
                                                  double & pmdec,
                                                  double & RV) const = 0;

	//! Pure virtual method. See subclass implementation.
	virtual void draw(StelPainter* sPainter, int index,bool is_inside,
					  const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
					  int maxMagStarName, float names_brightness,
					  const QVector<SphericalCap>& boundingCaps,
					  const bool withAberration, const Vec3d vel, const double withParallax, const Vec3d diffPos, const bool withCommonNameI18n) const = 0;

	//! Get whether or not the catalog was successfully loaded.
	//! @return @c true if at least one zone was loaded, otherwise @c false
	bool isInitialized(void) const { return (nr_of_zones>0); }

	//! Initialize the ZoneData struct at the given index.
	void initTriangle(int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2);

	//! File path of the catalog.
	const QString fname;

	//! Level in StelGeodesicGrid.
	const int level;

	//! Lower bound of magnitudes in this level at the catalog epoch. Units: millimag.
	//! May be negative for brightest stars and stars might have different magnitudes in the past/future.
	const int mag_min;

protected:
	//! Load a catalog and display its progress on the splash screen.
	//! @return @c true if successful, or @c false if an error occurred
	static bool readFile(QFile& file, void *data, qint64 size);

	//! Protected constructor. Initializes fields and does not load anything.
	ZoneArray(const QString& fname, QFile* file, int level, int mag_min);
	unsigned int nr_of_zones;
	unsigned int nr_of_stars;
	ZoneData *zones;
	QFile* file;
};

//! @class ZoneArrayImpl
//! CRTP template that implements all ZoneArray virtual methods using shared logic.
//! Derived classes only need to provide:
//!   - loadZoneDraw(int index, const Star*& stars, uint32_t& size, bool& isGlobal) const
//!   - loadZoneSearch(int index, const Star*& stars, uint32_t& size, bool& isGlobal) const
//!   - numberOfZones() const → nr_of_zones
//! And optionally override prefetchRegion(), updateHipIndex().
template<class Derived, class Star>
class ZoneArrayImpl : public ZoneArray
{
protected:
	ZoneArrayImpl(const QString& fname, QFile* file, int level, int mag_min)
		: ZoneArray(fname, file, level, mag_min) {}

public:
	// ----- ZoneArray virtual overrides (shared implementation) -----

	//! Draw stars and their names onto the viewport.
	//! @param sPainter the painter to use
	//! @param index zone index to draw
	//! @param isInsideViewport whether the zone is inside the current viewport. If false, we need to test more to skip stars.
	//! @param rcmag_table table of magnitudes
	//! @param limitMagIndex index from rcmag_table at which stars are not visible anymore
	//! @param core core to use for drawing
	//! @param maxMagStarName magnitude limit of stars that display labels
	//! @param names_brightness brightness of labels
	//! @param boundingCaps
	//! @param withAberration true if aberration to be applied
	//! @param vel velocity vector of observer planet
	//! @param withCommonNameI18n Use commonNameI18n when there is no cultural name
	void draw(StelPainter* sPainter, int index, bool isInsideViewport,
		  const RCMag *rcmag_table, int limitMagIndex, StelCore* core,
		  int maxMagStarName, float names_brightness,
		  const QVector<SphericalCap>& boundingCaps,
		  const bool withAberration, const Vec3d vel, const double withParallax, const Vec3d diffPos, const bool withCommonNameI18n) const override;

	void searchAround(const StelCore* core, int index, const Vec3d &v, const double withParallax,
					  const Vec3d diffPos, double cosLimFov, QList<StelObjectP > &result) override;
	void searchWithin(const StelCore* core, int index, const SphericalRegionP region, const double withParallax, const Vec3d diffPos, const bool hipOnly, const float maxMag,
			  QList<StelObjectP > &result) const override;
	StelObjectP searchGaiaID(int index, const StarId source_id, int& matched) const override;
 	void searchGaiaIDepochPos(const StarId source_id, float dyrs,
                                                  double & RA,
                                                  double & DEC,
                                                  double & Plx,
                                                  double & pmra,
                                                  double & pmdec,
                                                  double & RV) const override;
	StelObjectP wrapStar(const Star* s, int zoneIndex) const;

private:
	// CRTP helpers that delegate to Derived
	const Derived& self() const { return *static_cast<const Derived*>(this); }

	// Load a zone for drawing (may be async for DynamicZoneArray).
	bool zoneForDraw(int index, const Star*& stars, uint32_t& size, bool& isGlobal) const
	{
		return self().loadZoneDraw(index, stars, size, isGlobal);
	}

	// Load a zone for search (synchronous).
	bool zoneForSearch(int index, const Star*& stars, uint32_t& size, bool& isGlobal) const
	{
		return self().loadZoneSearch(index, stars, size, isGlobal);
	}
};

//! @class SpecialZoneArray
//! ZoneArray that mmap's or reads the entire star catalog into memory.
//! @tparam Star either Star1, Star2 or Star3.
template<class Star>
class SpecialZoneArray : public ZoneArrayImpl<SpecialZoneArray<Star>, Star>
{
	using Base = ZoneArrayImpl<SpecialZoneArray<Star>, Star>;
	friend Base;
public:
	SpecialZoneArray(QFile* file,bool byte_swap,bool use_mmap,int level,int mag_min);
	~SpecialZoneArray(void) override;
protected:
	SpecialZoneData<Star> *getZones(void) const
	{
		return static_cast<SpecialZoneData<Star>*>(this->zones);
	}
	Star *stars;
private:
	uchar *mmap_start;

	bool loadZoneDraw(int index, const Star*& outStars, uint32_t& size, bool& isGlobal) const;
	bool loadZoneSearch(int index, const Star*& outStars, uint32_t& size, bool& isGlobal) const;
	StelObjectP createStelObj(const Star* s, int zoneIndex) const;
};

//! @class HipZoneArray
//! ZoneArray of Hipparcos stars. It's just a SpecialZoneArray<Star1> that
//! implements updateHipIndex(HipIndexStruct).
class HipZoneArray : public SpecialZoneArray<Star1>
{
public:
	HipZoneArray(QFile* file,bool byte_swap,bool use_mmap,int level,int mag_min)
			: SpecialZoneArray<Star1>(file,byte_swap,use_mmap,level,mag_min) {}

	//! Add Hipparcos information for all stars in this catalog into @em hipIndex.
	//! @param hipIndex array of Hipparcos info structs
	void updateHipIndex(HipIndexStruct hipIndex[]) const override;
};

//! @class DynamicZoneArray
//! ZoneArray that loads star data on demand from disk, cached in an LRU QCache.
//! @tparam Star either Star1, Star2 or Star3.
template<class Star>
class DynamicZoneArray : public ZoneArrayImpl<DynamicZoneArray<Star>, Star>
{
	using Base = ZoneArrayImpl<DynamicZoneArray<Star>, Star>;
	friend Base;
public:
	DynamicZoneArray(const QString& fname, QFile* file, int level, int mag_min, bool byte_swap);
	~DynamicZoneArray(void) override;

	const Star* loadZone(int zone_index) const;
	const Star* loadZoneSync(int zone_index) const;
	uint32_t zoneStarCount(int zone_index) const { return zoneCounts_[zone_index]; }

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

	bool loadZoneDraw(int index, const Star*& outStars, uint32_t& size, bool& isGlobal) const;
	bool loadZoneSearch(int index, const Star*& outStars, uint32_t& size, bool& isGlobal) const;
	StelObjectP createStelObj(const Star* s, int zoneIndex) const;
};

#endif // ZONEARRAY_HPP
