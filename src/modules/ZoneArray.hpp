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


#ifndef _ZONE_ARRAY_HPP_
#define _ZONE_ARRAY_HPP_

#include <QString>
#include <QDebug>

#include "ZoneData.hpp"
#include "Star.hpp"

#include "GLee.h"

#include "LoadingBar.hpp"
#include "Projector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SkyDrawer.hpp"
#include "STexture.hpp"
#include "StarMgr.hpp"
#include "bytes.h"

#ifndef WIN32
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#else
#include <io.h>
#include <windows.h>
#endif

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


namespace BigStarCatalogExtension {

// A ZoneArray manages all ZoneData structures of a given GeodesicGrid level.

class ZoneArray {
public:
  static ZoneArray *create(const StarMgr &hip_star_mgr,
                           const QString &extended_file_name,
                           LoadingBar &lb);
  virtual ~ZoneArray(void) {nr_of_zones = 0;}
  virtual void generateNativeDebugFile(const char *fname) const = 0;
  unsigned int getNrOfStars(void) const {return nr_of_stars;}
  virtual void updateHipIndex(HipIndexStruct hip_index[]) const {}
  virtual void searchAround(int index,const Vec3d &v,double cos_lim_fov,
                            vector<StelObjectP > &result) = 0;
  virtual void draw(int index,bool is_inside,
                    const float *rcmag_table, Projector *prj,
                    unsigned int max_mag_star_name,float names_brightness,
                    SFont *starFont) const = 0;
  bool isInitialized(void) const {return (nr_of_zones>0);}
  void initTriangle(int index,
                    const Vec3d &c0,
                    const Vec3d &c1,
                    const Vec3d &c2);
  virtual void scaleAxis(void) = 0;
  const int level;
  const int mag_min;
  const int mag_range;
  const int mag_steps;
  double star_position_scale;
  const StarMgr &hip_star_mgr;
protected:
  static bool readFileWithLoadingBar(FILE *f,
                                     void *data,size_t size,LoadingBar &lb);
  ZoneArray(const StarMgr &hip_star_mgr,int level,
            int mag_min,int mag_range,int mag_steps);
  unsigned int nr_of_zones;
  unsigned int nr_of_stars;
  ZoneData *zones;
};

template<class Star>
class SpecialZoneArray : public ZoneArray {
public:
  SpecialZoneArray(FILE *f,bool byte_swap,bool use_mmap,
                   LoadingBar &lb,
                   const StarMgr &hip_star_mgr,int level,
                   int mag_min,int mag_range,int mag_steps);
  ~SpecialZoneArray(void);
  void generateNativeDebugFile(const char *fname) const;
protected:
  SpecialZoneData<Star> *getZones(void) const
    {return static_cast<SpecialZoneData<Star>*>(zones);}
  Star *stars;
private:
  void *mmap_start;
#ifdef WIN32
  HANDLE mapping_handle;
#endif
  void scaleAxis(void);
  void searchAround(int index,const Vec3d &v,double cos_lim_fov,
                    vector<StelObjectP > &result);
  void draw(int index,bool is_inside,
            const float *rcmag_table, Projector *prj,
            unsigned int max_mag_star_name,float names_brightness,
            SFont *starFont) const;
};

template<class Star>
void SpecialZoneArray<Star>::scaleAxis(void) {
  star_position_scale /= Star::max_pos_val;
  for (ZoneData *z=zones+(nr_of_zones-1);z>=zones;z--) {
    z->axis0 *= star_position_scale;
    z->axis1 *= star_position_scale;
  }
}


#define NR_OF_HIP 120416

struct HipIndexStruct {
  const SpecialZoneArray<Star1> *a;
  const SpecialZoneData<Star1> *z;
  const Star1 *s;
};

class ZoneArray1 : public SpecialZoneArray<Star1> {
public:
  ZoneArray1(FILE *f,bool byte_swap,bool use_mmap,
             LoadingBar &lb,const StarMgr &hip_star_mgr,
             int level,int mag_min,int mag_range,int mag_steps)
    : SpecialZoneArray<Star1>(f,byte_swap,use_mmap,lb,hip_star_mgr,level,
                              mag_min,mag_range,mag_steps) {}
private:
  void updateHipIndex(HipIndexStruct hip_index[]) const;
};


template<class Star>
void SpecialZoneArray<Star>::generateNativeDebugFile(const char *fname) const {
  FILE *f = fopen(fname,"wb");
  if (f) {
      // write first 10 stars in native format to a file:
    fwrite(stars,10,sizeof(Star),f);
    fclose(f);
  }
}


template<class Star>
SpecialZoneArray<Star>::SpecialZoneArray(FILE *f,bool byte_swap,bool use_mmap,
                                         LoadingBar &lb,
                                         const StarMgr &hip_star_mgr,
                                         int level,
                                         int mag_min,int mag_range,
                                         int mag_steps)
                       :ZoneArray(hip_star_mgr,level,
                                  mag_min,mag_range,mag_steps),
                        stars(0),
#ifndef WIN32
                        mmap_start(MAP_FAILED)
#else
                        mmap_start(NULL),
                        mapping_handle(NULL)
#endif                        
{
  if (nr_of_zones > 0) {
	lb.Draw(0.f);
    zones = new SpecialZoneData<Star>[nr_of_zones];
    if (zones == 0) {
      qWarning() << "ERROR: SpecialZoneArray(" << level 
                 << ")::SpecialZoneArray: no memory (1)";
      exit(1);
    }
    {
      unsigned int *zone_size = new unsigned int[nr_of_zones];
      if (zone_size == 0) {
        qWarning() << "ERROR: SpecialZoneArray(" << level 
                   << ")::SpecialZoneArray: no memory (2)";
        exit(1);
      }
      if (nr_of_zones != fread(zone_size,sizeof(unsigned int),nr_of_zones,f)) {
        delete[] getZones();
        zones = 0;
        nr_of_zones = 0;
      } else {
        const unsigned int *tmp = zone_size;
        for (unsigned int z=0;z<nr_of_zones;z++,tmp++) {
          const unsigned int tmp_spu_int32 = byte_swap?bswap_32(*tmp):*tmp;
          nr_of_stars += tmp_spu_int32;
          getZones()[z].size = tmp_spu_int32;
        }
      }
        // delete zone_size before allocating stars
        // in order to avoid memory fragmentation:
      delete[] zone_size;
    }
    
    if (nr_of_stars == 0) {
        // no stars ?
      if (zones) delete[] getZones();
      zones = 0;
      nr_of_zones = 0;
    } else {
      if (use_mmap) {
        const long start_in_file = ftell(f);
#ifndef WIN32
        const long page_size = sysconf(_SC_PAGE_SIZE);
#else
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        const long page_size = system_info.dwAllocationGranularity;
#endif
        const long mmap_offset = start_in_file % page_size;
#ifndef WIN32
        mmap_start = mmap(0,mmap_offset+sizeof(Star)*nr_of_stars,PROT_READ,
                          MAP_PRIVATE | MAP_NORESERVE,
                          fileno(f),start_in_file-mmap_offset);
        if (mmap_start == MAP_FAILED) {
          qWarning() << "ERROR: SpecialZoneArray(" << level 
                     << ")::SpecialZoneArray: mmap(" << fileno(f)
                     << ',' << start_in_file
                     << ',' << (sizeof(Star)*nr_of_stars)
                     << ") failed: " << strerror(errno);
          stars = 0;
          nr_of_stars = 0;
          delete[] getZones();
          zones = 0;
          nr_of_zones = 0;
        } else {
          stars = (Star*)(((char*)mmap_start)+mmap_offset);
          Star *s = stars;
          for (unsigned int z=0;z<nr_of_zones;z++) {
            getZones()[z].stars = s;
            s += getZones()[z].size;
          }
        }
#else
        HANDLE file_handle = (void*)_get_osfhandle(_fileno(f));
        if (file_handle == INVALID_HANDLE_VALUE) {
          qWarning() << "ERROR: SpecialZoneArray(" << level
                     << ")::SpecialZoneArray: _get_osfhandle(_fileno(f)) failed";
        } else {
          mapping_handle = CreateFileMapping(file_handle,NULL,PAGE_READONLY,
                                             0,0,NULL);
          if (mapping_handle == NULL) {
              // yes, NULL indicates failure, not INVALID_HANDLE_VALUE
            qWarning() << "ERROR: SpecialZoneArray(" << level
                       << ")::SpecialZoneArray: CreateFileMapping failed: " 
                       << GetLastError();
          } else {
            mmap_start = MapViewOfFile(mapping_handle, 
                                       FILE_MAP_READ, 
                                       0, 
                                       start_in_file-mmap_offset, 
                                       mmap_offset+sizeof(Star)*nr_of_stars);
            if (mmap_start == NULL) {
              qWarning() << "ERROR: SpecialZoneArray(" << level
                         << ")::SpecialZoneArray: "
                         << "MapViewOfFile failed: " 
                         << GetLastError()
                         << ", page_size: " << page_size;
              stars = 0;
              nr_of_stars = 0;
              delete[] getZones();
              zones = 0;
              nr_of_zones = 0;
            } else {
              stars = (Star*)(((char*)mmap_start)+mmap_offset);
              Star *s = stars;
              for (unsigned int z=0;z<nr_of_zones;z++) {
                getZones()[z].stars = s;
                s += getZones()[z].size;
              }
            }
          }
        }
#endif
      } else {
        stars = new Star[nr_of_stars];
        if (stars == 0) {
          qWarning() << "ERROR: SpecialZoneArray(" << level 
                     << ")::SpecialZoneArray: no memory (3)";
          exit(1);
        }
        if (!readFileWithLoadingBar(f,stars,sizeof(Star)*nr_of_stars,lb)) {
          delete[] stars;
          stars = 0;
          nr_of_stars = 0;
          delete[] getZones();
          zones = 0;
          nr_of_zones = 0;
        } else {
          Star *s = stars;
          for (unsigned int z=0;z<nr_of_zones;z++) {
            getZones()[z].stars = s;
            s += getZones()[z].size;
          }
          if (
#if (!defined(__GNUC__))
              true
#else
              byte_swap
#endif
             ) {
            s = stars;
            for (unsigned int i=0;i<nr_of_stars;i++,s++) {
              s->repack(
#ifdef WORDS_BIGENDIAN
  // need for byte_swap on a BE machine means that catalog is LE
                !byte_swap
#else
  // need for byte_swap on a LE machine means that catalog is BE
                byte_swap
#endif
              );
            }
          }
//           qDebug() << "\n"
//                << "SpecialZoneArray<Star>::SpecialZoneArray(" << level
//                << "): repack test start";
//           stars[0].print();
//           stars[1].print();
//           qDebug() << "SpecialZoneArray<Star>::SpecialZoneArray(" << level
//                << "): repack test end";
        }
      }
    }
	lb.Draw(1.f);
  }
}

template<class Star>
SpecialZoneArray<Star>::~SpecialZoneArray(void) {
  if (stars) {
#ifndef WIN32
    if (mmap_start != MAP_FAILED) {
      munmap(mmap_start,((char*)stars-(char*)mmap_start)
                        +sizeof(Star)*nr_of_stars);
    } else {
      delete[] stars;
    }
#else
    if (mmap_start != NULL) {
      CloseHandle(mapping_handle);
    } else {
      delete[] stars;
    }
#endif
    stars = 0;
  }
  if (zones) {delete[] getZones();zones = NULL;}
  nr_of_zones = 0;
  nr_of_stars = 0;
}


template<class Star>
void SpecialZoneArray<Star>::draw(int index,bool is_inside,
                                  const float *rcmag_table,
                                  Projector *prj,
                                  unsigned int max_mag_star_name,
                                  float names_brightness,
                                  SFont *starFont) const
{
	SkyDrawer* drawer = StelApp::getInstance().getCore()->getSkyDrawer();
	SpecialZoneData<Star> *const z = getZones() + index;
	Vec3d xy;
	const Star *const end = z->getStars() + z->size;
	const double d2000 = 2451545.0;
	const double movement_factor = (M_PI/180)*(0.0001/3600) * ((StarMgr::getCurrentJDay()-d2000)/365.25) / star_position_scale;            
	for (const Star *s=z->getStars();s<end;s++)
	{
		if (is_inside ? prj->project(s->getJ2000Pos(z,movement_factor),xy) : prj->projectCheck(s->getJ2000Pos(z,movement_factor),xy))
		{
			if (drawer->drawPointSource(xy[0],xy[1],rcmag_table + 2*(s->mag),s->b_v)==false)
			{
				break;
			}
			if (s->mag < max_mag_star_name)
			{
				const QString starname = s->getNameI18n();
				if (!starname.isEmpty())
				{
					const float offset = (rcmag_table + 2*(s->mag))[0]*0.7;
					const Vec3f& colorr = SkyDrawer::indexToColor(s->b_v)*0.75;
					glColor4f(colorr[0], colorr[1], colorr[2],names_brightness);
					prj->drawText(starFont,xy[0],xy[1], starname, 0, offset, offset, false);
				}
			}
		}
	}
}


template<class Star>
void SpecialZoneArray<Star>::searchAround(int index,const Vec3d &v,
                                          double cos_lim_fov,
                                          vector<StelObjectP > &result) {
  const double d2000 = 2451545.0;
  const double movement_factor = (M_PI/180)*(0.0001/3600)
                           * ((StarMgr::getCurrentJDay()-d2000)/365.25)
                           / star_position_scale;
  const SpecialZoneData<Star> *const z = getZones()+index;
  for (int i=0;i<z->size;i++) {
    if (z->getStars()[i].getJ2000Pos(z,movement_factor)*v >= cos_lim_fov) {
        // TODO: do not select stars that are too faint to display
      result.push_back(z->getStars()[i].createStelObject(this,z));
    }
  }
}

} // namespace BigStarCatalogExtension

#endif
