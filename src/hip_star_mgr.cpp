/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 *
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006
 *  (I will move these parts to new files,
 *   so that the Copyright is more clear).
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

// class used to manage groups of Stars

#include <string>
#include "hip_star_mgr.h"
#include "stel_object.h"
#include "stel_object_base.h"
#include "s_texture.h"
#include "bytes.h"
#include "stellarium.h"
#include "navigator.h"
#include "stel_utility.h"
#include "tone_reproductor.h"
#include "translator.h"
#include "geodesic_grid.h"

#define RADIUS_STAR 1.


#define NR_OF_HIP 120404

static
const char *cat_file_names[] = {
  "/usr/local/share/stellarium/data/stars0.cat",
  "/usr/local/share/stellarium/data/stars1.cat",
  "/usr/local/share/stellarium/data/stars2.cat",
  "/usr/local/share/stellarium/data/stars3.cat",
  "/usr/local/share/stellarium/data/stars4.cat",
  "/usr/local/share/stellarium/data/stars5.cat",
  "/usr/local/share/stellarium/data/stars6.cat",
  "/usr/local/share/stellarium/data/stars7.cat",
  0
};

static const char *spectral_file
 = "/usr/local/share/stellarium/data/stars_hip_sp.cat";

static const char *component_file
 = "/usr/local/share/stellarium/data/stars_hip_component_ids.cat";






struct HipIndexStruct;

template <class Star> struct SpecialZoneArray;
template <class Star> struct SpecialZoneData;


struct Star3 {  // 6 byte
  int x0:18;
  int x1:18;
  unsigned int b_v:7;
  unsigned int mag:5;
  enum {max_pos_val=((1<<17)-1)};
  StelObject createStelObject(const SpecialZoneArray<Star3> *a,
                              const SpecialZoneData<Star3> *z) const;
} __attribute__ ((__packed__)) ;


struct Star2 {  // 10 byte
  int x0:20;
  int x1:20;
  int pma:14;
  int pmd:14;
  unsigned int b_v:7;
  unsigned int mag:5;
  enum {max_pos_val=((1<<19)-1)};
  StelObject createStelObject(const SpecialZoneArray<Star2> *a,
                              const SpecialZoneData<Star2> *z) const;
} __attribute__ ((__packed__));


struct Star1 {
  int hip:24;                  // 17 bits needed
  unsigned char component_ids; //  5 bits needed
  int x0;                      // 32 bits needed
  int x1;                      // 32 bits needed
  unsigned char b_v;           //  7 bits needed
  unsigned char mag;           //  8 bits needed
  unsigned short sp_int;       // 14 bits needed
  int pma,pmd,plx;
  enum {max_pos_val=0x7FFFFFFF};
  StelObject createStelObject(const SpecialZoneArray<Star1> *a,
                              const SpecialZoneData<Star1> *z) const;
} __attribute__ ((__packed__));


struct ZoneData { // a single Triangle
    // no virtual functions!
  int getNrOfStars(void) const {return size;}
  void init(int c0,int c1,int c2,int nr_of_stars,double star_position_scale);
  Vec3d center;
  Vec3d axis0;
  Vec3d axis1;
  int size;
};

template <class Star>
struct SpecialZoneData : public ZoneData {
  Vec3d getJ2000Pos(const Star *s) const {
    Vec3d pos = (double)(s->x0)*axis0 + (double)(s->x1)*axis1;
    pos += sqrt(1.0-pos.lengthSquared()) * center;
    return pos;
  }
  StelObject createStelObject(const Star *s) const
    {return s->createStelObject(*this);}
  Star *stars; // array of stars in this zone
};

class ZoneArray {  // contains all zones of a given level
public:
  static ZoneArray *create(const HipStarMgr &hip_star_mgr,const char *fname);
  virtual ~ZoneArray(void) {nr_of_zones = 0;}
  int getNrOfStars(void) const {return nr_of_stars;}
  virtual void updateHipIndex(HipIndexStruct hip_index[]) const {}
  virtual void searchAround(int index,const Vec3d &v,double cos_lim_fov,
                            vector<StelObject> &result) = 0;
  virtual void draw(int index,bool is_inside,
                    const float *rmag_table,const Projector *prj) const = 0;
  bool isInitialized(void) const {return (nr_of_zones>0);}
  const int level;
  const int scale_int;
  const int mag_min;
  const int mag_range;
  const int mag_steps;
protected:
  ZoneArray(const HipStarMgr &hip_star_mgr,int level,int scale_int,
            int mag_min,int mag_range,int mag_steps);
  const HipStarMgr &hip_star_mgr;
  int nr_of_zones;
  int nr_of_stars;
};

template<class Star>
class SpecialZoneArray : public ZoneArray {
public:
  SpecialZoneArray(FILE *f,
                   const HipStarMgr &hip_star_mgr,int level,int scale_int,
                   int mag_min,int mag_range,int mag_steps);
  ~SpecialZoneArray(void) {
    if (stars) {delete[] stars;stars = 0;}
    if (zones) {delete[] zones;zones = 0;}
    nr_of_zones = 0;
    nr_of_stars = 0;
  }
protected:
  SpecialZoneData<Star> *zones;
  Star *stars;
private:
  void searchAround(int index,const Vec3d &v,double cos_lim_fov,
                    vector<StelObject> &result);
  void draw(int index,bool is_inside,
            const float *rmag_table,const Projector *prj) const;
};

struct HipIndexStruct {
  const SpecialZoneArray<Star1> *a;
  const SpecialZoneData<Star1> *z;
  const Star1 *s;
};

class ZoneArray1 : public SpecialZoneArray<Star1> {
public:
  ZoneArray1(FILE *f,const HipStarMgr &hip_star_mgr,int level,int scale_int,
             int mag_min,int mag_range,int mag_steps)
    : SpecialZoneArray<Star1>(f,hip_star_mgr,level,scale_int,
                                mag_min,mag_range,mag_steps) {}
private:
  void updateHipIndex(HipIndexStruct hip_index[]) const;
};















static
const Vec3f color_table[128] = {
  Vec3f(0.587877,0.755546,1.000000),
  Vec3f(0.609856,0.750638,1.000000),
  Vec3f(0.624467,0.760192,1.000000),
  Vec3f(0.639299,0.769855,1.000000),
  Vec3f(0.654376,0.779633,1.000000),
  Vec3f(0.669710,0.789527,1.000000),
  Vec3f(0.685325,0.799546,1.000000),
  Vec3f(0.701229,0.809688,1.000000),
  Vec3f(0.717450,0.819968,1.000000),
  Vec3f(0.733991,0.830383,1.000000),
  Vec3f(0.750857,0.840932,1.000000),
  Vec3f(0.768091,0.851637,1.000000),
  Vec3f(0.785664,0.862478,1.000000),
  Vec3f(0.803625,0.873482,1.000000),
  Vec3f(0.821969,0.884643,1.000000),
  Vec3f(0.840709,0.895965,1.000000),
  Vec3f(0.859873,0.907464,1.000000),
  Vec3f(0.879449,0.919128,1.000000),
  Vec3f(0.899436,0.930956,1.000000),
  Vec3f(0.919907,0.942988,1.000000),
  Vec3f(0.940830,0.955203,1.000000),
  Vec3f(0.962231,0.967612,1.000000),
  Vec3f(0.984110,0.980215,1.000000),
  Vec3f(1.000000,0.986617,0.993561),
  Vec3f(1.000000,0.977266,0.971387),
  Vec3f(1.000000,0.967997,0.949602),
  Vec3f(1.000000,0.958816,0.928210),
  Vec3f(1.000000,0.949714,0.907186),
  Vec3f(1.000000,0.940708,0.886561),
  Vec3f(1.000000,0.931787,0.866303),
  Vec3f(1.000000,0.922929,0.846357),
  Vec3f(1.000000,0.914163,0.826784),
  Vec3f(1.000000,0.905497,0.807593),
  Vec3f(1.000000,0.896884,0.788676),
  Vec3f(1.000000,0.888389,0.770168),
  Vec3f(1.000000,0.879953,0.751936),
  Vec3f(1.000000,0.871582,0.733989),
  Vec3f(1.000000,0.863309,0.716392),
  Vec3f(1.000000,0.855110,0.699088),
  Vec3f(1.000000,0.846985,0.682070),
  Vec3f(1.000000,0.838928,0.665326),
  Vec3f(1.000000,0.830965,0.648902),
  Vec3f(1.000000,0.823056,0.632710),
  Vec3f(1.000000,0.815254,0.616856),
  Vec3f(1.000000,0.807515,0.601243),
  Vec3f(1.000000,0.799820,0.585831),
  Vec3f(1.000000,0.792222,0.570724),
  Vec3f(1.000000,0.784675,0.555822),
  Vec3f(1.000000,0.777212,0.541190),
  Vec3f(1.000000,0.769821,0.526797),
  Vec3f(1.000000,0.762496,0.512628),
  Vec3f(1.000000,0.755229,0.498664),
  Vec3f(1.000000,0.748032,0.484926),
  Vec3f(1.000000,0.740897,0.471392),
  Vec3f(1.000000,0.733811,0.458036),
  Vec3f(1.000000,0.726810,0.444919),
  Vec3f(1.000000,0.719856,0.431970),
  Vec3f(1.000000,0.712983,0.419247),
  Vec3f(1.000000,0.706154,0.406675),
  Vec3f(1.000000,0.699375,0.394265),
  Vec3f(1.000000,0.692681,0.382075),
  Vec3f(1.000000,0.686003,0.369976),
  Vec3f(1.000000,0.679428,0.358120),
  Vec3f(1.000000,0.672882,0.346373),
  Vec3f(1.000000,0.666372,0.334740),
  Vec3f(1.000000,0.659933,0.323281),
  Vec3f(1.000000,0.653572,0.312004),
  Vec3f(1.000000,0.647237,0.300812),
  Vec3f(1.000000,0.640934,0.289709),
  Vec3f(1.000000,0.634698,0.278755),
  Vec3f(1.000000,0.628536,0.267954),
  Vec3f(1.000000,0.622390,0.257200),
  Vec3f(1.000000,0.616298,0.246551),
  Vec3f(1.000000,0.610230,0.235952),
  Vec3f(1.000000,0.604259,0.225522),
  Vec3f(1.000000,0.598288,0.215083),
  Vec3f(1.000000,0.592391,0.204756),
  Vec3f(1.000000,0.586501,0.194416),
  Vec3f(1.000000,0.580657,0.184120),
  Vec3f(1.000000,0.574901,0.173930),
  Vec3f(1.000000,0.569127,0.163645),
  Vec3f(1.000000,0.563449,0.153455),
  Vec3f(1.000000,0.557758,0.143147),
  Vec3f(1.000000,0.552134,0.132843),
  Vec3f(1.000000,0.546541,0.122458),
  Vec3f(1.000000,0.540984,0.111966),
  Vec3f(1.000000,0.535464,0.101340),
  Vec3f(1.000000,0.529985,0.090543),
  Vec3f(1.000000,0.524551,0.079292),
  Vec3f(1.000000,0.519122,0.068489),
  Vec3f(1.000000,0.513743,0.058236),
  Vec3f(1.000000,0.508417,0.048515),
  Vec3f(1.000000,0.503104,0.039232),
  Vec3f(1.000000,0.497805,0.030373),
  Vec3f(1.000000,0.492557,0.021982),
  Vec3f(1.000000,0.487338,0.014007),
  Vec3f(1.000000,0.482141,0.006417),
  Vec3f(1.000000,0.477114,0.000000),
  Vec3f(1.000000,0.473268,0.000000),
  Vec3f(1.000000,0.469419,0.000000),
  Vec3f(1.000000,0.465552,0.000000),
  Vec3f(1.000000,0.461707,0.000000),
  Vec3f(1.000000,0.457846,0.000000),
  Vec3f(1.000000,0.453993,0.000000),
  Vec3f(1.000000,0.450129,0.000000),
  Vec3f(1.000000,0.446276,0.000000),
  Vec3f(1.000000,0.442415,0.000000),
  Vec3f(1.000000,0.438549,0.000000),
  Vec3f(1.000000,0.434702,0.000000),
  Vec3f(1.000000,0.430853,0.000000),
  Vec3f(1.000000,0.426981,0.000000),
  Vec3f(1.000000,0.423134,0.000000),
  Vec3f(1.000000,0.419268,0.000000),
  Vec3f(1.000000,0.415431,0.000000),
  Vec3f(1.000000,0.411577,0.000000),
  Vec3f(1.000000,0.407733,0.000000),
  Vec3f(1.000000,0.403874,0.000000),
  Vec3f(1.000000,0.400029,0.000000),
  Vec3f(1.000000,0.396172,0.000000),
  Vec3f(1.000000,0.392331,0.000000),
  Vec3f(1.000000,0.388509,0.000000),
  Vec3f(1.000000,0.384653,0.000000),
  Vec3f(1.000000,0.380818,0.000000),
  Vec3f(1.000000,0.376979,0.000000),
  Vec3f(1.000000,0.373166,0.000000),
  Vec3f(1.000000,0.369322,0.000000),
  Vec3f(1.000000,0.365506,0.000000),
  Vec3f(1.000000,0.361692,0.000000),
};



class StarWrapperBase : public StelObjectBase {
protected:
  StarWrapperBase(void) : ref_count(0) {}
  virtual ~StarWrapperBase(void) {}
  STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_STAR;}

  string getEnglishName(void) const {
    return "";
  }
  wstring getNameI18n(void) const {
    return L"";
  }
  wstring getInfoString(const Navigator *nav) const;
  wstring getShortInfoString(const Navigator *nav) const
    {return getInfoString(nav);}
private:
  int ref_count;
  void retain(void) {assert(ref_count>=0);ref_count++;}
  void release(void) {assert(ref_count>0);if (--ref_count==0) delete this;}
};

wstring StarWrapperBase::getInfoString(const Navigator *nav) const {
  const Vec3d j2000_pos = getObsJ2000Pos(nav);
  double dec_j2000, ra_j2000;
  rect_to_sphe(&ra_j2000,&dec_j2000,j2000_pos);
  const Vec3d equatorial_pos = nav->j2000_to_earth_equ(j2000_pos);
  double dec_equ, ra_equ;
  rect_to_sphe(&ra_equ,&dec_equ,equatorial_pos);
  wostringstream oss;
  oss.setf(ios::fixed);
  oss.precision(2);
  oss << _("Magnitude: ") << get_mag(nav);
  oss << endl;
  oss << _("J2000") << L" " << _("RA/DE: ")
      << StelUtils::printAngleHMS(ra_j2000)
      << L"/" << StelUtils::printAngleDMS(dec_j2000) << endl;
  oss << _("Equ of date") << L" " << _("RA/DE: ")
      << StelUtils::printAngleHMS(ra_equ)
      << L"/" << StelUtils::printAngleDMS(dec_equ) << endl;

    // calculate alt az
  double az,alt;
  rect_to_sphe(&az,&alt,nav->earth_equ_to_local(equatorial_pos));
  az = 3*M_PI - az;  // N is zero, E is 90 degrees
  if(az > M_PI*2) az -= M_PI*2;    
  oss << _("Az/Alt: ") << StelUtils::printAngleDMS(az)
      << L"/" << StelUtils::printAngleDMS(alt) << endl;
  oss.precision(2);

  return oss.str();
}



template <class Star>
class StarWrapper : public StarWrapperBase {
protected:
  StarWrapper(const SpecialZoneArray<Star> *a,
              const SpecialZoneData<Star> *z,
              const Star *s) : a(a),z(z),s(s) {}
  Vec3d getObsJ2000Pos(const Navigator*) const {return z->getJ2000Pos(s);}
  Vec3d get_earth_equ_pos(const Navigator *nav) const
    {return nav->j2000_to_earth_equ(getObsJ2000Pos(0));}
  Vec3f get_RGB(void) const {return color_table[s->b_v];}
  float get_mag(const Navigator *nav) const
    {return 0.001f*a->mag_min + s->mag*(0.001f*a->mag_range)/a->mag_steps;}

  string getEnglishName(void) const {
    return "";
  }
  wstring getNameI18n(void) const {
    return L"";
  }
protected:
  const SpecialZoneArray<Star> *const a;
  const SpecialZoneData<Star> *const z;
  const Star *const s;
};

class StarWrapper1 : public StarWrapper<Star1> {
public:
  StarWrapper1(const SpecialZoneArray<Star1> *a,
               const SpecialZoneData<Star1> *z,
               const Star1 *s) : StarWrapper<Star1>(a,z,s) {}
  wstring getInfoString(const Navigator *nav) const;
  string getEnglishName(void) const;
  wstring getNameI18n(void) const;
};

string StarWrapper1::getEnglishName(void) const {
  if (s->hip) {
    char buff[64];
    sprintf(buff,"HP %d",s->hip);
    return buff;
  }
  return StarWrapperBase::getEnglishName();
}

wstring StarWrapper1::getNameI18n(void) const {
  if (s->hip) {
    const wstring commonNameI18 = HipStarMgr::getCommonName(s->hip);
    if (!commonNameI18.empty()) return commonNameI18;
    if (HipStarMgr::getFlagSciNames()) {
      const wstring sciName = HipStarMgr::getSciName(s->hip);
      if (!sciName.empty()) return sciName;
    }
    return L"HP " + StelUtils::intToWstring(s->hip);
  }
  return StarWrapperBase::getNameI18n();
}


wstring StarWrapper1::getInfoString(const Navigator *nav) const {
  const Vec3d j2000_pos = getObsJ2000Pos(nav);
  double dec_j2000, ra_j2000;
  rect_to_sphe(&ra_j2000,&dec_j2000,j2000_pos);
  const Vec3d equatorial_pos = nav->j2000_to_earth_equ(j2000_pos);
  double dec_equ, ra_equ;
  rect_to_sphe(&ra_equ,&dec_equ,equatorial_pos);
  wostringstream oss;
  if (s->hip) {
    const wstring commonNameI18 = HipStarMgr::getCommonName(s->hip);
    const wstring sciName = HipStarMgr::getSciName(s->hip);
    if (commonNameI18!=L"" || sciName!=L"") {
      oss << commonNameI18 << (commonNameI18 == L"" ? L"" : L" ");
      if (commonNameI18!=L"" && sciName!=L"") oss << L"(";
      oss << wstring(sciName==L"" ? L"" : sciName);
      if (commonNameI18!=L"" && sciName!=L"") oss << L")";
      oss << endl;
    }
    oss << L"HP " << s->hip;
    if (s->component_ids) {
      oss << L" "
          << HipStarMgr::convertToComponentIds(s->component_ids).c_str();
    }
    oss << endl;
  }

  oss.setf(ios::fixed);
  oss.precision(2);
  oss << _("Magnitude: ") << get_mag(nav);
  oss << endl;
  oss << _("J2000") << L" " << _("RA/DE: ")
      << StelUtils::printAngleHMS(ra_j2000)
      << L"/" << StelUtils::printAngleDMS(dec_j2000) << endl;
  oss << _("Equ of date") << L" " << _("RA/DE: ")
      << StelUtils::printAngleHMS(ra_equ)
      << L"/" << StelUtils::printAngleDMS(dec_equ) << endl;

    // calculate alt az
  double az,alt;
  rect_to_sphe(&az,&alt,nav->earth_equ_to_local(equatorial_pos));
  az = 3*M_PI - az;  // N is zero, E is 90 degrees
  if(az > M_PI*2) az -= M_PI*2;    
  oss << _("Az/Alt: ") << StelUtils::printAngleDMS(az)
      << L"/" << StelUtils::printAngleDMS(alt) << endl;

  if (s->plx) {
    oss.precision(5);
    oss << _("parallax: ") << (0.00001*s->plx) << endl;
    oss << _("distance: ") << ( (149.6e6/(300000.0*86400*365.25))
                               /(M_PI/180) * (0.00001*s->plx/3600)) << endl;
  }

  if (s->sp_int) {
    oss << _("Spectral Type: ")
        << HipStarMgr::convertToSpectralType(s->sp_int).c_str() << endl;
  }
  oss.precision(2);

  return oss.str();
}






class StarWrapper2 : public StarWrapper<Star2> {
public:
  StarWrapper2(const SpecialZoneArray<Star2> *a,
               const SpecialZoneData<Star2> *z,
               const Star2 *s) : StarWrapper<Star2>(a,z,s) {}
};

class StarWrapper3 : public StarWrapper<Star3> {
public:
  StarWrapper3(const SpecialZoneArray<Star3> *a,
               const SpecialZoneData<Star3> *z,
               const Star3 *s) : StarWrapper<Star3>(a,z,s) {}
};


StelObject Star1::createStelObject(const SpecialZoneArray<Star1> *a,
                                   const SpecialZoneData<Star1> *z) const {
  return new StarWrapper1(a,z,this);
}

StelObject Star2::createStelObject(const SpecialZoneArray<Star2> *a,
                                   const SpecialZoneData<Star2> *z) const {
  return new StarWrapper2(a,z,this);
}

StelObject Star3::createStelObject(const SpecialZoneArray<Star3> *a,
                                   const SpecialZoneData<Star3> *z) const {
  return new StarWrapper3(a,z,this);
}

void ZoneData::init(int c0,int c1,int c2,int nr_of_stars,
                    double star_position_scale) {
  center[0] = (double)c0 / (double)0x7FFFFFFF;
  center[1] = (double)c1 / (double)0x7FFFFFFF;
  center[2] = (double)c2 / (double)0x7FFFFFFF;
  size = nr_of_stars;
  axis0 = Vec3d(0.0,0.0,1.0) ^ center;
  axis0.normalize();
  axis1 = center ^ axis0;
  axis0 *= star_position_scale;
  axis1 *= star_position_scale;
}

void ZoneArray1::updateHipIndex(HipIndexStruct hip_index[]) const {
  for (const SpecialZoneData<Star1> *z=zones+nr_of_zones-1;z>=zones;z--) {
    for (const Star1 *s = z->stars+z->size-1;s>=z->stars;s--) {
      const int hip = s->hip;
      assert(0 <= hip && hip <= NR_OF_HIP);
      if (hip != 0) {
        hip_index[hip].a = this;
        hip_index[hip].z = z;
        hip_index[hip].s = s;
      }
    }
  }
}


static inline
int ReadInt(FILE *f,int &x) {
  return (4 == fread(&x,1,4,f)) ? 0 : -1;
}


#define FILE_MAGIC 0xde0955a3

ZoneArray *ZoneArray::create(const HipStarMgr &hip_star_mgr,
                             const char *fname) {
  ZoneArray *rval = 0;
  FILE *f = fopen(fname,"rb");
  if (f == 0) {
    fprintf(stderr,"ZoneArray::create(%s): fopen failed\n",fname);
  } else {
    int type,level,scale_int,mag_min,mag_range,mag_steps;
    if (ReadInt(f,type) < 0 ||
        ReadInt(f,level) < 0 ||
        ReadInt(f,scale_int) < 0 ||
        ReadInt(f,mag_min) < 0 ||
        ReadInt(f,mag_range) < 0 ||
        ReadInt(f,mag_steps) < 0) {
      fprintf(stderr,"ZoneArray::create(%s): bad file\n",fname);
    } else {
      type -= FILE_MAGIC;
      printf("ZoneArray::create(%s): type: %d level: %d"
             " mag_min: %d mag_range: %d mag_steps: %d stars: ",
             fname,
             type,level,mag_min,mag_range,mag_steps);
      switch (type) {
        case 0:
          rval = new ZoneArray1(f,hip_star_mgr,level,
                                scale_int,mag_min,mag_range,mag_steps);
          if (rval == 0) {
            fprintf(stderr,"ZoneArray::create(%s): no memory\n",
                    fname);
          }
          break;
        case 1:
          rval = new SpecialZoneArray<Star2>(f,hip_star_mgr,level,scale_int,
                                             mag_min,mag_range,mag_steps);
          if (rval == 0) {
            fprintf(stderr,"ZoneArray::create(%s): no memory\n",
                    fname);
          }
          break;
        case 2:
          rval = new SpecialZoneArray<Star3>(f,hip_star_mgr,level,scale_int,
                                             mag_min,mag_range,mag_steps);
          if (rval == 0) {
            fprintf(stderr,"ZoneArray::create(%s): no memory\n",
                    fname);
          }
          break;
        default:
          fprintf(stderr,"ZoneArray::create(%s): bad file type\n",
                  fname);
          break;
      }
      if (rval) {
        if (!rval->isInitialized()) {
          fprintf(stderr,"ZoneArray::create(%s): "
                         "initialization failed\n",fname);
          delete rval;
          rval = 0;
        } else {
          printf("%d\n",rval->getNrOfStars());
        }
      }
    }
    fclose(f);
  }
  return rval;
}



ZoneArray::ZoneArray(const HipStarMgr &hip_star_mgr,int level,int scale_int,
                     int mag_min,int mag_range,int mag_steps)
          :level(level),scale_int(scale_int),
           mag_min(mag_min),mag_range(mag_range),mag_steps(mag_steps),
           hip_star_mgr(hip_star_mgr) {
  nr_of_zones = GeodesicGrid::nrOfZones(level);
  nr_of_stars = 0;
}

struct TmpZoneData {
  int center_0;
  int center_1;
  int center_2;
  int size;
};

static inline
int DoubleToInt(double x) {
  return (int)floor(0.5+x*0x7FFFFFFF);
}

static inline
double IntToDouble(int x) {
  double rval = x;
  rval /= 0x7FFFFFFF;
  return rval;
}


template<class Star>
SpecialZoneArray<Star>::SpecialZoneArray(FILE *f,
                                         const HipStarMgr &hip_star_mgr,
                                         int level,int scale_int,
                                         int mag_min,int mag_range,
                                         int mag_steps)
                       :ZoneArray(hip_star_mgr,level,scale_int,
                                  mag_min,mag_range,mag_steps),
                  zones(0),stars(0) {
  if (nr_of_zones > 0) {
    zones = new SpecialZoneData<Star>[nr_of_zones];
    assert(zones!=0);
    {
      TmpZoneData *const tmp_zones = new TmpZoneData[nr_of_zones];
      assert(tmp_zones!=0);
      if (nr_of_zones != (int)fread(tmp_zones,sizeof(TmpZoneData),
                                    nr_of_zones,f)) {
        delete[] zones;
        zones = 0;
        nr_of_zones = 0;
      } else {
        TmpZoneData *tmp = tmp_zones;
        const double star_position_scale =
          IntToDouble(scale_int)/Star::max_pos_val;
        for (int z=0;z<nr_of_zones;z++,tmp++) {
          nr_of_stars += tmp->size;
          zones[z].init(tmp->center_0,tmp->center_1,tmp->center_2,tmp->size,
                        star_position_scale);
        }
      }
        // delete tmp_zones before allocating stars
        // in order to avoid memory fragmentation:
      delete[] tmp_zones;
    }
    
    if (nr_of_stars <= 0) {
        // no stars ?
      if (zones) delete[] zones;
      zones = 0;
      nr_of_zones = 0;
    } else {
      stars = new Star[nr_of_stars];
      assert(stars!=0);
      if (nr_of_stars != (int)fread(stars,sizeof(Star),nr_of_stars,f)) {
        delete[] stars;
        stars = 0;
        nr_of_stars = 0;
        delete[] zones;
        zones = 0;
        nr_of_zones = 0;
      } else {
        Star *s = stars;
        for (int z=0;z<nr_of_zones;z++) {
          zones[z].stars = s;
          s += zones[z].size;
        }
      }
    }
  }
}




    












// construct and load all data
HipStarMgr::HipStarMgr(void) :
    limitingMag(6.5f),
    starTexture(),
    hip_index(new HipIndexStruct[NR_OF_HIP+1]),
    starFont(0)
{
  assert(hip_index);
  max_geodesic_grid_level = -1;
  last_max_search_level = -1;
}


HipStarMgr::~HipStarMgr(void) {
  ZoneArrayMap::iterator it(zone_arrays.end());
  while (it!=zone_arrays.begin()) {
    --it;
    delete it->second;
  }
  zone_arrays.clear();

  if (starTexture) {delete starTexture;starTexture = 0;}
  if (starFont) {delete starFont;starFont = 0;}
  if (hip_index) delete hip_index;
}

bool HipStarMgr::flagSciNames = true;
map<int,string> HipStarMgr::common_names_map;
map<int,wstring> HipStarMgr::common_names_map_i18n;
map<string,int> HipStarMgr::common_names_index;
map<wstring,int> HipStarMgr::common_names_index_i18n;

map<int,wstring> HipStarMgr::sci_names_map_i18n;
map<wstring,int> HipStarMgr::sci_names_index_i18n;
vector<string> HipStarMgr::spectral_array;
vector<string> HipStarMgr::component_array;

wstring HipStarMgr::getCommonName(int hip) {
  map<int,wstring>::const_iterator it(common_names_map_i18n.find(hip));
  if (it!=common_names_map_i18n.end()) return it->second;
  return L"";
}

wstring HipStarMgr::getSciName(int hip) {
  map<int,wstring>::const_iterator it(sci_names_map_i18n.find(hip));
  if (it!=sci_names_map_i18n.end()) return it->second;
  return L"";
}

void HipStarMgr::init(float font_size, const string& font_name,
                      LoadingBar& lb) {
  load_data(lb);
  starTexture = new s_texture("star16x16.png",TEX_LOAD_TYPE_PNG_SOLID,false);  // Load star texture no mipmap
  starFont = new s_font(font_size, font_name);
  if (!starFont)
  {
      printf("Can't create starFont\n");
      assert(0);
  }
}

// Load from file
void HipStarMgr::load_data(LoadingBar& lb) {

    // Please do not init twice:
  assert(max_geodesic_grid_level<0);

  printf("Loading star data...");
  for (int i=0;cat_file_names[i];i++) {
    lb.SetMessage(_("Loading catalog ") + StelUtils::intToWstring(i));
    lb.Draw(i/8.0);
    ZoneArray *const z = ZoneArray::create(*this,cat_file_names[i]);
    if (z) {
      if (max_geodesic_grid_level < z->level) {
        max_geodesic_grid_level = z->level;
      }
      ZoneArray *&pos(zone_arrays[z->level]);
      if (pos) {
        fprintf(stderr,"%s, %d: duplicate level\n",
                cat_file_names[i],z->level);
        delete z;
      } else {
        pos = z;
      }
    }
  }
  for (int i=0;i<=NR_OF_HIP;i++) {
    hip_index[i].a = 0;
    hip_index[i].z = 0;
    hip_index[i].s = 0;
  }
  for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
       it!=zone_arrays.end();it++) {
    it->second->updateHipIndex(hip_index);
  }
  
  FILE *f = fopen(spectral_file,"r");
  if (f) {
    char line[256];
    while (fgets(line, sizeof(line), f)) {
      string s = line;
      // remove newline
      s.erase(s.length()-1,1);
      spectral_array.push_back(s);
    }
    fclose(f);
  }
  f = fopen(component_file,"r");
  if (f) {
    char line[256];
    while (fgets(line, sizeof(line), f)) {
      string s = line;
      // remove newline
      s.erase(s.length()-1,1);
      component_array.push_back(s);
    }
    fclose(f);
  }
  last_max_search_level = max_geodesic_grid_level;
  printf("finished, max_geodesic_level: %d\n",max_geodesic_grid_level);
}

// Load common names from file 
int HipStarMgr::load_common_names(const string& commonNameFile) {
  cout << "Load star names from " << commonNameFile << endl;

  FILE *cnFile;
  cnFile=fopen(commonNameFile.c_str(),"r");
  if (!cnFile) {
    cerr << "Warning " << commonNameFile << " not found." << endl;
    return 0;
  }

  common_names_map.clear();
  common_names_map_i18n.clear();
  common_names_index.clear();
  common_names_index_i18n.clear();

  // Assign names to the matching stars, now support spaces in names
  char line[256];
  while (fgets(line, sizeof(line), cnFile)) {
    line[sizeof(line)-1] = '\0';
    unsigned int hip;
    assert(1==sscanf(line,"%u",&hip));
    unsigned int i = 0;
    while (line[i]!='|' && i<sizeof(line)-2) ++i;
    i++;
    string englishCommonName =  line+i;
    // remove newline
    englishCommonName.erase(englishCommonName.length()-1, 1);

//cout << hip << ": \"" << englishCommonName << '"' << endl;

    // remove underscores
    for (string::size_type j=0;j<englishCommonName.length();++j) {
        if (englishCommonName[j]=='_') englishCommonName[j]=' ';
    }
    const wstring commonNameI18n = _(englishCommonName.c_str());
    wstring commonNameI18n_cap = commonNameI18n;
    transform(commonNameI18n.begin(), commonNameI18n.end(),
              commonNameI18n_cap.begin(), ::toupper);

    common_names_map[hip] = englishCommonName;
    common_names_index[englishCommonName] = hip;
    common_names_map_i18n[hip] = commonNameI18n;
    common_names_index_i18n[commonNameI18n_cap] = hip;
  }

  fclose(cnFile);
  return 1;
}


// Load scientific names from file 
void HipStarMgr::load_sci_names(const string& sciNameFile) {
  cout << "Load sci names from " << sciNameFile << endl;

  FILE *snFile;
  snFile=fopen(sciNameFile.c_str(),"r");
  if (!snFile) {
    cerr << "Warning " << sciNameFile.c_str() << " not found" << endl;
    return;
  }

  sci_names_map_i18n.clear();
  sci_names_index_i18n.clear();

  // Assign names to the matching stars, now support spaces in names
  char line[256];
  while (fgets(line, sizeof(line), snFile)) {
    line[sizeof(line)-1] = '\0';
    unsigned int hip;
    assert(1==sscanf(line,"%u",&hip));
    unsigned int i = 0;
    while (line[i]!='|' && i<sizeof(line)-2) ++i;
    i++;
    char *tempc = line+i;
    while (line[i]!='_' && i<sizeof(line)-1) ++i;
    line[i-1]=' ';
    string sci_name = tempc;
    sci_name.erase(sci_name.length()-1, 1);
    const wstring sci_name_i18n = Translator::UTF8stringToWstring(sci_name);
    wstring sci_name_i18n_cap = sci_name_i18n;
    transform(sci_name_i18n.begin(), sci_name_i18n.end(),
              sci_name_i18n_cap.begin(), ::toupper);
    
    sci_names_map_i18n[hip] = sci_name_i18n;
    sci_names_index_i18n[sci_name_i18n_cap] = hip;
  }

  fclose(snFile);
}












int HipStarMgr::drawStar(const Vec3d &XY,float rmag,const Vec3f &color) const {
  float cmag = 1.f;

  // if size of star is too small (blink) we put its size to 1.2 --> no more blink
  // And we compensate the difference of brighteness with cmag
  if (rmag<1.2f) {
    cmag = rmag*rmag/1.44f;
    if (rmag < 0.1f*star_scale ||
        cmag * starMagScale < 0.1) return -1;
    rmag=1.2f;
  } else {
//if (rmag>4.f) {
//  rmag=4.f+2.f*sqrtf(1.f+rmag-4.f)-2.f;
  if (rmag>8.f) {
    rmag=8.f+2.f*sqrtf(1.f+rmag-8.f)-2.f;
  }
//}
//    if (rmag > 5.f) {
//      rmag=5.f;
//    }
  }

  // Calculation of the luminosity
  // Random coef for star twinkling
  cmag *= (1.-twinkle_amount*rand()/RAND_MAX);

  // Global scaling
  rmag *= star_scale;
  cmag *= starMagScale;

  glColor3fv(color*cmag);

  glBlendFunc(GL_ONE, GL_ONE);

  glBegin(GL_QUADS );
    glTexCoord2i(0,0);glVertex2f(XY[0]-rmag,XY[1]-rmag);    // Bottom left
    glTexCoord2i(1,0);glVertex2f(XY[0]+rmag,XY[1]-rmag);    // Bottom right
    glTexCoord2i(1,1);glVertex2f(XY[0]+rmag,XY[1]+rmag);    // Top right
    glTexCoord2i(0,1);glVertex2f(XY[0]-rmag,XY[1]+rmag);    // Top left
  glEnd();
  return 0;
}



template<class Star>
void SpecialZoneArray<Star>::draw(int index,bool is_inside,
                                  const float *rmag_table,
                                  const Projector *prj) const {
  SpecialZoneData<Star> &zone(zones[index]);
  Vec3d xy;
  const Star *const end = zone.stars + zone.size;
  for (const Star *s=zone.stars;s<end;s++) {
    if (is_inside) {
      if (prj->project_j2000(zone.getJ2000Pos(s),xy)) {
        if (0 > hip_star_mgr.drawStar(xy,rmag_table[s->mag],
                                      color_table[s->b_v])) break;
      }
    } else {
      if (prj->project_j2000_check(zone.getJ2000Pos(s),xy)) {
        if (0 > hip_star_mgr.drawStar(xy,rmag_table[s->mag],
                         color_table[s->b_v])) break;
      }
    }
  }
}




int HipStarMgr::getMaxSearchLevel(const ToneReproductor *eye,
                                  const Projector *prj) const {
  int rval = -1;
  float fov_q = prj->get_fov();
  if (fov_q > 60) fov_q = 60;
  fov_q = 1.f/(fov_q*fov_q);
  for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
       it!=zone_arrays.end();it++) {
    const float mag_min = 0.001f*it->second->mag_min;
    const float rmag =
      sqrtf(eye->adapt_luminance(
        expf(-0.92103f*(mag_min + 12.12331f)) * 108064.73f * fov_q)) * 30.f;
    if (rmag<1.2f) {
      const float cmag = rmag*rmag/1.44f;
      if (rmag < 0.1f*star_scale ||
          cmag * starMagScale < 0.1) break;
    }
    rval = it->first;
  }
  return rval;
}


// Draw all the stars
void HipStarMgr::draw(const ToneReproductor *eye,const Projector *prj) {
    // If stars are turned off don't waste time below
    // projecting all stars just to draw disembodied labels
    if(!starsFader.getInterstate()) return;

    float fov_q = prj->get_fov();
    if (fov_q > 60) fov_q = 60;
    fov_q = 1.f/(fov_q*fov_q);

    // Set temporary static variable for optimization
    if (flagStarTwinkle) twinkle_amount = twinkleAmount;
    else twinkle_amount = 0;
    star_scale = starScale * starsFader.getInterstate();
    names_brightness = names_fader.getInterstate() 
        * starsFader.getInterstate();
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
    // FOV is currently measured vertically, so need to adjust for wide screens
    // TODO: projector should probably use largest measurement itself
//    const float max_fov =
//      MY_MAX( prj->get_fov(),
//              prj->get_fov()*prj->getViewportWidth()/prj->getViewportHeight());
    //    float maxMag = limiting_mag-1 + 60.f/prj->get_fov();
//    const float maxMag = limitingMag-1 + 60.f/max_fov;

    prj->set_orthographic_projection();    // set 2D coordinate

    // Bind the star texture
    glBindTexture (GL_TEXTURE_2D, starTexture->getID());

    // Set the draw mode
    glBlendFunc(GL_ONE, GL_ONE);

    // draw all the stars of all the selected zones
    float rmag_table[256];
//static int count = 0;
//count++;
    for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
         it!=zone_arrays.end();it++) {
      const float mag_min = 0.001f*it->second->mag_min;
//      if (maxMag < mag_min) break;
      const float k = (0.001f*it->second->mag_range)/it->second->mag_steps;
      for (int i=it->second->mag_steps-1;i>=0;i--) {
        const float mag = mag_min+k*i;
          // Compute the equivalent star luminance for a 5 arc min circle
          // and convert it in function of the eye adaptation
//        rmag_table[i] =
//          eye->adapt_luminance(expf(-0.92103f*(mag + 12.12331f)) * 108064.73f)
//            * powf(prj->get_fov(),-0.85f) * 70.f;
        rmag_table[i] =
          sqrtf(eye->adapt_luminance(
            expf(-0.92103f*(mag + 12.12331f)) * 108064.73f * fov_q)) * 30.f;
        if (i==0 && rmag_table[0]<1.2f) {
          const float cmag = rmag_table[0]*rmag_table[0]/1.44f;
          if (rmag_table[0] < 0.1f*star_scale ||
              cmag * starMagScale < 0.1) goto exit_loop;
        }
      }
      last_max_search_level = it->first;

      int zone;
//if ((count&63)==0) cout << "inside(" << it->first << "):";
      for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it->first);
           (zone = it1.next()) >= 0;) {
        it->second->draw(zone,true,rmag_table,prj);
//if ((count&63)==0) cout << " " << zone;
      }
//if ((count&63)==0) cout << endl << "border(" << it->first << "):";
      for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it->first);
           (zone = it1.next()) >= 0;) {
        it->second->draw(zone,false,rmag_table,prj);
//if ((count&63)==0) cout << " " << zone;
      }
//if ((count&63)==0) cout << endl;
    }
    exit_loop:
//if ((count&63)==0) cout << endl;
    prj->reset_perspective_projection();
}






// Look for a star by XYZ coords
StelObject HipStarMgr::search(Vec3d pos) const {
  pos.normalize();
  vector<StelObject> v = search_around(pos,
                                       0.8 // just an arbitrary number
                                       );
  StelObject nearest;
  double cos_angle_nearest = -10.0;
  for (vector<StelObject>::const_iterator it(v.begin());it!=v.end();it++) {
    const double c = it->getObsJ2000Pos(0)*pos;
    if (c > cos_angle_nearest) {
      cos_angle_nearest = c;
      nearest = *it;
    }
  }
  return nearest;
}

// Return a stl vector containing the stars located
// inside the lim_fov circle around position v
vector<StelObject> HipStarMgr::search_around(Vec3d v,
                                             double lim_fov // degrees
                                             ) const {
  v.normalize();
    // find any vectors h0 and h1 (length 1), so that h0*v=h1*v=h0*h1=0
  int i;
  {
    const double a0 = fabs(v[0]);
    const double a1 = fabs(v[1]);
    const double a2 = fabs(v[2]);
    if (a0 <= a1) {
      if (a0 <= a2) i = 0;
      else i = 2;
    } else {
      if (a1 <= a2) i = 1;
      else i = 2;
    }
  }
  Vec3d h0(0.0,0.0,0.0);
  h0[i] = 1.0;
  Vec3d h1 = h0 ^ v;
  h1.normalize();
  h0 = h1 ^ v;
  h0.normalize();
    // now we have h0*v=h1*v=h0*h1=0.
    // construct a region with 4 corners e0,e1,e2,e3 inside which
    // all desired stars must be:
  double f = 1.4142136 * tan(lim_fov * M_PI/180.0);
  h0 *= f;
  h1 *= f;
  Vec3d e0 = v + h0;
  Vec3d e1 = v + h1;
  Vec3d e2 = v - h0;
  Vec3d e3 = v - h1;
  f = 1.0/e0.length();
  e0 *= f;
  e1 *= f;
  e2 *= f;
  e3 *= f;
    // search the triangles
  geodesic_search_result->search(e0,e1,e2,e3,last_max_search_level);
    // iterate over the stars inside the triangles:
  f = cos(lim_fov * M_PI/180.);
  vector<StelObject> result;
  for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
       it!=zone_arrays.end();it++) {
//cout << "search inside(" << it->first << "):";
    int zone;
    for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it->first);
         (zone = it1.next()) >= 0;) {
      it->second->searchAround(zone,v,f,result);
//cout << " " << zone;
    }
//cout << endl << "search border(" << it->first << "):";
    for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it->first);
         (zone = it1.next()) >= 0;) {
      it->second->searchAround(zone,v,f,result);
//cout << " " << zone;
    }
//cout << endl << endl;
  }
  return result;
}






template<class Star>
void SpecialZoneArray<Star>::searchAround(int index,const Vec3d &v,
                                          double cos_lim_fov,
                                          vector<StelObject> &result) {
  const SpecialZoneData<Star> *const zone = zones+index;
  for (int i=0;i<zone->size;i++) {
    if (zone->getJ2000Pos(zone->stars+i)*v >= cos_lim_fov) {
      result.push_back(zone->stars[i].createStelObject(this,zone));
    }
  }
}

//! @brief Update i18 names from english names according to passed translator
//! The translation is done using gettext with translated strings defined in translations.h
void HipStarMgr::translateNames(Translator& trans) {
  common_names_map_i18n.clear();
  common_names_index_i18n.clear();
  for (map<int,string>::iterator it(common_names_map.begin());
       it!=common_names_map.end();it++) {
    const int i = it->first;
    const wstring t(trans.translate(it->second));
    common_names_map_i18n[i] = t;
    wstring t_cap = t;
    transform(t.begin(), t.end(), t_cap.begin(), ::toupper);
    common_names_index_i18n[t_cap] = i;
  }
}


StelObject HipStarMgr::search(const string& name) const
{
    const string catalogs("HP HD SAO");

    string n = name;
    for (string::size_type i=0;i<n.length();++i)
    {
        if (n[i]=='_') n[i]=' ';
    }    
    
    istringstream ss(n);
    string cat;
    unsigned int num;
    
    ss >> cat;
    
    // check if a valid catalog reference
    if (catalogs.find(cat,0) == string::npos)
    {
        // try see if the string is a HP number
        istringstream cat_to_num(cat);
        cat_to_num >> num;
        if (!cat_to_num.fail()) return searchHP(num);
        return NULL;
    }

    ss >> num;
    if (ss.fail()) return NULL;

    if (cat == "HP") return searchHP(num);
    assert(0);
    return NULL;
}    

// Search the star by HP number
StelObject HipStarMgr::searchHP(int _HP) const {
  if (0 < _HP && _HP <= NR_OF_HIP) {
    const Star1 *const s = hip_index[_HP].s;
    if (s) {
      const SpecialZoneArray<Star1> *const a = hip_index[_HP].a;
      const SpecialZoneData<Star1> *const z = hip_index[_HP].z;
      return s->createStelObject(a,z);
    }
  }
  return StelObject();
}

StelObject HipStarMgr::searchByNameI18n(const wstring& nameI18n) const
{
    wstring objw = nameI18n;
    transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
    
    // Search by HP number if it's an HP formated number
    // Please help, if you know a better way to do this:
    if (nameI18n.length() >= 2 && nameI18n[0]==L'H' && nameI18n[1]==L'P')
    {
        bool hp_ok = false;
        wstring::size_type i=2;
        // ignore spaces
        for (;i<nameI18n.length();i++)
        {
            if (nameI18n[i] != L' ') break;
        }
        // parse the number
        unsigned int nr = 0;
        for (;i<nameI18n.length();i++)
        {
            if (hp_ok = (L'0' <= nameI18n[i] && nameI18n[i] <= L'9'))
            {
                nr = 10*nr+(nameI18n[i]-L'0');
            }
            else
            {
                break;
            }
        }
        if (hp_ok)
        {
            return searchHP(nr);
        }
    }

    // Search by I18n common name
  map<wstring,int>::const_iterator it(common_names_index_i18n.find(objw));
  if (it!=common_names_index_i18n.end()) {
    return searchHP(it->second);
  }

    // Search by sci name
  it = sci_names_index_i18n.find(objw);
  if (it!=sci_names_index_i18n.end()) {
    return searchHP(it->second);
  }

  return StelObject();
}

//! Find and return the list of at most maxNbItem objects auto-completing
//! the passed object I18n name
vector<wstring> HipStarMgr::listMatchingObjectsI18n(
                              const wstring& objPrefix,
                              unsigned int maxNbItem) const {
  vector<wstring> result;
  if (maxNbItem==0) return result;

  wstring objw = objPrefix;
  transform(objw.begin(), objw.end(), objw.begin(), ::toupper);

    // Search for common names
  for (map<wstring,int>::const_iterator
       it(common_names_index_i18n.lower_bound(objw));
       it!=common_names_index_i18n.end();it++) {
    const wstring constw(it->first.substr(0,objw.size()));
    if (constw == objw) {
      if (maxNbItem == 0) break;
      result.push_back(getCommonName(it->second));
      maxNbItem--;
    } else {
      break;
    }
  }
    // Search for sci names
  for (map<wstring,int>::const_iterator
       it(sci_names_index_i18n.lower_bound(objw));
       it!=sci_names_index_i18n.end();it++) {
    const wstring constw(it->first.substr(0,objw.size()));
    if (constw == objw) {
      if (maxNbItem == 0) break;
      result.push_back(getSciName(it->second));
      maxNbItem--;
    } else {
      break;
    }
  }

  sort(result.begin(), result.end());

  return result;
}


//! Define font file name and size to use for star names display
void HipStarMgr::setFont(float font_size, const string& font_name) {
    if (starFont) delete starFont;
    starFont = new s_font(font_size, font_name);
    assert(starFont);

}
