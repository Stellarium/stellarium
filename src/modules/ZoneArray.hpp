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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _ZONEARRAY_HPP_
#define _ZONEARRAY_HPP_

#include <QString>
#include <QFile>
#include <QDebug>

#include "ZoneData.hpp"
#include "Star.hpp"

#include "StelLoadingBar.hpp"
#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTexture.hpp"
#include "StarMgr.hpp"
#include "bytes.h"

#include "StelPainter.hpp"

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

namespace BigStarCatalogExtension
{

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
	//! @param lb the loading bar on the splash screen
	//! @return an instance of SpecialZoneArray or HipZoneArray
	static ZoneArray *create(const QString &extended_file_name,
	                         bool use_mmap,
	                         StelLoadingBar &lb);
	virtual ~ZoneArray(void)
	{
		nr_of_zones = 0;
	}
	
	//! Pure virtual method. See subclass implementation.
	virtual void generateNativeDebugFile(const QString& fname) const = 0;
	
	//! Get the total number of stars in this catalog.
	unsigned int getNrOfStars(void) const { return nr_of_stars; }
	
	//! Dummy method that does nothing. See subclass implementation.
	virtual void updateHipIndex(HipIndexStruct hipIndex[]) const {}
	
	//! Pure virtual method. See subclass implementation.
	virtual void searchAround(int index,const Vec3d &v,double cosLimFov,
	                          QList<StelObjectP > &result) = 0;
	
	//! Pure virtual method. See subclass implementation.
	virtual void draw(int index,bool is_inside,
	                  const float *rcmag_table, const StelProjectorP& prj,
	                  unsigned int maxMagStarName,float names_brightness,
	                  StelFont *starFont) const = 0;
	
	//! Get whether or not the catalog was successfully loaded.
	//! @return @c true if at least one zone was loaded, otherwise @c false
	bool isInitialized(void) const { return (nr_of_zones>0); }
	
	//! Initialize the ZoneData struct at the given index.
	void initTriangle(int index,
	                  const Vec3d &c0,
	                  const Vec3d &c1,
	                  const Vec3d &c2);
	virtual void scaleAxis(void) = 0;
	
	//! File path of the catalog.
	const QString fname;
	
	//! Level in StelGeodesicGrid.
	const int level;
	
	//! Lower bound of magnitudes in this catalog.
	const int mag_min;
	
	//! Range of magnitudes in this catalog.
	const int mag_range;
	
	//! Number of steps used to describe values in @em mag_range.
	const int mag_steps;
	
	double star_position_scale;
protected:
	//! Load a catalog and display its progress on the splash screen.
	//! @return @c true if successful, or @c false if an error occurred
	static bool readFileWithStelLoadingBar(QFile& file, void *data,
					       qint64 size,StelLoadingBar &lb);
	
	//! Protected constructor. Initializes fields and does not load anything.
	ZoneArray(QString fname, int level,int mag_min,int mag_range,int mag_steps);
	unsigned int nr_of_zones;
	unsigned int nr_of_stars;
	ZoneData *zones;
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
	//! @param lb the loading bar on the splash screen
	//! @param level level in StelGeodesicGrid
	//! @param mag_min lower bound of magnitudes
	//! @param mag_range range of magnitudes
	//! @param mag_steps number of steps used to describe values in range
	SpecialZoneArray(QFile& file,bool byte_swap,bool use_mmap,
	                 StelLoadingBar &lb,int level,int mag_min,
			 int mag_range,int mag_steps);
	~SpecialZoneArray(void);
	
	//! Output the first ten stars in raw format to @em fname.
	//! @param fname path to the output file
	void generateNativeDebugFile(const QString& fname) const;
protected:
	//! Get an array of all SpecialZoneData objects in this catalog.
	SpecialZoneData<Star> *getZones(void) const
	{
		return static_cast<SpecialZoneData<Star>*>(zones);
	}
	
	//! Draw stars and their names onto the viewport.
	//! @param index zone index to draw
	//! @param is_inside whether the zone is inside the current viewport
	//! @param rcmag_table table of magnitudes
	//! @param prj projector to draw on
	//! @param maxMagStarName magnitude limit of stars that display labels
	//! @param names_brightness brightness of labels
	//! @param starFont font of labels
	void draw(int index,bool is_inside,
	          const float *rcmag_table, const StelProjectorP& prj,
	          unsigned int maxMagStarName,float names_brightness,
	          StelFont *starFont) const;
	
	void scaleAxis(void);
	void searchAround(int index,const Vec3d &v,double cosLimFov,
	                  QList<StelObjectP > &result);
	
	Star *stars;
private:
	uchar *mmap_start;
#ifdef WIN32
	HANDLE mapping_handle;
#endif
};

//! @class HipZoneArray
//! ZoneArray of Hipparcos stars. It's just a SpecialZoneArray<Star1> that
//! implements updateHipIndex(HipIndexStruct).
class HipZoneArray : public SpecialZoneArray<Star1>
{
public:
	HipZoneArray(QFile& file,bool byte_swap,bool use_mmap,StelLoadingBar &lb,
		   int level,int mag_min,int mag_range,int mag_steps)
			: SpecialZoneArray<Star1>(file,byte_swap,use_mmap,lb,level,
			                          mag_min,mag_range,mag_steps) {}
	
	//! Add Hipparcos information for all stars in this catalog into @em hipIndex.
	//! @param hipIndex array of Hipparcos info structs
	void updateHipIndex(HipIndexStruct hipIndex[]) const;
};

} // namespace BigStarCatalogExtension

#endif // _ZONEARRAY_HPP_
