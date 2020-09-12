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

#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "StarMgr.hpp"

#include <QString>
#include <QFile>
#include <QDebug>

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
//! Manages all ZoneData structures of a given StelGeodesicGrid level. An
//! instance of this class is never created directly; the named constructor
//! returns an instance of one of its subclasses. All it really does is
//! bootstrap the loading process.
class ZoneArray
{
public:
	//! Named public constructor for ZoneArray. Opens a catalog, reads its
	//! header info, and creates a SpecialZoneArray or HipZoneArray for
	//! loading.
	//! @param extended_file_name path of the star catalog to load from
	//! @param use_mmap whether or not to mmap the star catalog
	//! @return an instance of SpecialZoneArray or HipZoneArray
	static ZoneArray *create(const QString &extended_file_name, bool use_mmap);
	virtual ~ZoneArray()
	{
		nr_of_zones = 0;
	}

	//! Get the total number of stars in this catalog.
	unsigned int getNrOfStars() const { return nr_of_stars; }

	//! Dummy method that does nothing. See subclass implementation.
	virtual void updateHipIndex(HipIndexStruct hipIndex[]) const {Q_UNUSED(hipIndex);}

	//! Pure virtual method. See subclass implementation.
	virtual void searchAround(const StelCore* core, int index,const Vec3d &v,double cosLimFov,
							  QList<StelObjectP > &result) = 0;

	//! Pure virtual method. See subclass implementation.
	virtual void draw(StelPainter* sPainter, int index,bool is_inside,
					  const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
					  int maxMagStarName, float names_brightness, bool designationUsage,
					  const QVector<SphericalCap>& boundingCaps) const = 0;

	//! Get whether or not the catalog was successfully loaded.
	//! @return @c true if at least one zone was loaded, otherwise @c false
	bool isInitialized(void) const { return (nr_of_zones>0); }

	//! Initialize the ZoneData struct at the given index.
	void initTriangle(int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2);
	
	virtual void scaleAxis() = 0;

	//! File path of the catalog.
	const QString fname;

	//! Level in StelGeodesicGrid.
	const int level;

	//! Lower bound of magnitudes in this level. Units: millimag. May be negative for brightest stars.
	const int mag_min;

	//! Range of magnitudes in this level. Units: millimags
	const int mag_range;

	//! Number of steps used to describe values in @em mag_range. Always positive. Individual stars have their mag entries from 0..mag_steps.
	const int mag_steps;

	float star_position_scale;

protected:
	//! Load a catalog and display its progress on the splash screen.
	//! @return @c true if successful, or @c false if an error occurred
	static bool readFile(QFile& file, void *data, qint64 size);

	//! Protected constructor. Initializes fields and does not load anything.
	ZoneArray(const QString& fname, QFile* file, int level, int mag_min, int mag_range, int mag_steps);
	unsigned int nr_of_zones;
	unsigned int nr_of_stars;
	ZoneData *zones;
	QFile* file;
};

//! @class SpecialZoneArray
//! Implements all the virtual methods in ZoneArray. Is only separate from
//! %ZoneArray because %ZoneArray decides on the template parameter.
//! @tparam Star either Star1, Star2 or Star3, depending on the brightness of
//! stars in this catalog.
template<class Star>
class SpecialZoneArray : public ZoneArray
{
public:
	//! Handles loading of the meaty part of star catalogs.
	//! @param file catalog to load from
	//! @param byte_swap whether to switch endianness of catalog data
	//! @param use_mmap whether or not to mmap the star catalog
	//! @param level level in StelGeodesicGrid
	//! @param mag_min lower bound of magnitudes
	//! @param mag_range range of magnitudes
	//! @param mag_steps number of steps used to describe values in range
	SpecialZoneArray(QFile* file,bool byte_swap,bool use_mmap,int level,int mag_min,
			 int mag_range,int mag_steps);
	~SpecialZoneArray(void);
protected:
	//! Get an array of all SpecialZoneData objects in this catalog.
	SpecialZoneData<Star> *getZones(void) const
	{
		return static_cast<SpecialZoneData<Star>*>(zones);
	}

	//! Draw stars and their names onto the viewport.
	//! @param sPainter the painter to use 
	//! @param index zone index to draw
	//! @param isInsideViewport whether the zone is inside the current viewport
	//! @param rcmag_table table of magnitudes
	//! @param limitMagIndex index from rcmag_table at which stars are not visible anymore
	//! @param core core to use for drawing
	//! @param maxMagStarName magnitude limit of stars that display labels
	//! @param names_brightness brightness of labels
	virtual void draw(StelPainter* sPainter, int index, bool isInsideViewport,
			  const RCMag *rcmag_table, int limitMagIndex, StelCore* core,
			  int maxMagStarName, float names_brightness, bool designationUsage,
			  const QVector<SphericalCap>& boundingCaps) const;

	virtual void scaleAxis();
	virtual void searchAround(const StelCore* core, int index,const Vec3d &v,double cosLimFov,
					  QList<StelObjectP > &result);

	Star *stars;
private:
	uchar *mmap_start;
};

//! @class HipZoneArray
//! ZoneArray of Hipparcos stars. It's just a SpecialZoneArray<Star1> that
//! implements updateHipIndex(HipIndexStruct).
class HipZoneArray : public SpecialZoneArray<Star1>
{
public:
	HipZoneArray(QFile* file,bool byte_swap,bool use_mmap,
		   int level,int mag_min,int mag_range,int mag_steps)
			: SpecialZoneArray<Star1>(file,byte_swap,use_mmap,level,
									  mag_min,mag_range,mag_steps) {}

	//! Add Hipparcos information for all stars in this catalog into @em hipIndex.
	//! @param hipIndex array of Hipparcos info structs
	void updateHipIndex(HipIndexStruct hipIndex[]) const;
};

#endif // ZONEARRAY_HPP
