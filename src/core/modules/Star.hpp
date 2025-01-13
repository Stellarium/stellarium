/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
 *
 * Thanks go to Nigel Kerr for ideas and testing of BE/LE star repacking
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

#ifndef STAR_HPP
#define STAR_HPP

#include "StelObjectType.hpp"
#include "StelUtils.hpp"
#include "StarMgr.hpp"
#include "ZoneData.hpp"
#include <QString>
#include <QtEndian>
#include <cmath>

// Epoch in JD of the star catalog data
#define STAR_CATALOG_JDEPOCH 2457389.0

class StelObject;

extern const QString       STAR_TYPE;

typedef short int          Int16;
typedef unsigned short int Uint16;

template<class Star>
class SpecialZoneArray;
template<class Star>
struct SpecialZoneData;

// structs for storing the stars in binary form. The idea is
// to store much data for bright stars (Star1), but only little or even
// very little data for faints stars (Star3). Using only less bytes for Star3
// makes it feasible to store hundreds of millions of them in main memory.

static inline float IndexToBV(int bV)
{
   return static_cast<float>(bV) * (4.f / 127.f) - 0.5f;
}

static inline int BVToIndex(float bV)
{
   // convert B-V in mag to index understood by internal B-V to color conversion table
   return static_cast<int>((bV + 0.5f) * 31.75f);
}

template<typename Derived>
struct Star
{
   // methods that must be implemented by the derived class are those not returning 0 or False here
   inline double getX0() const
   {
      return static_cast<const Derived *>(this)->getX0();
   } // either in rad (if no getX2()) or in internal astrometric unit (only if getX2())
   inline double getX1() const
   {
      return static_cast<const Derived *>(this)->getX1();
   } // either in rad (if no getX2()) or in internal astrometric unit (only if getX2())
   inline double getX2() const
   {
      return static_cast<const Derived *>(this)->getX2();
   } // either in rad (if no getX2()) or in internal astrometric unit (only if getX2())
   inline int getBVIndex() const
   {
      // need to check because some stars (e.g., Carbon stars) can have really high B-V
      // BV index to color table has only 128 entries, otherwise those stars will be black or weird color
      int index = BVToIndex(getBV());
      if (index < 0)
         return 0;
      if (index > 127)
         return 127;
      return index;
   }
   inline double  getMag() const { return static_cast<const Derived *>(this)->getMag(); } // should return in millimag
   inline float   getBV() const { return static_cast<const Derived *>(this)->getBV(); }   // should return in mag
   // VIP flag is for situation where it can bypass some check (e.g., magnitude display cutoff for Star1 in far
   // past/future)
   inline bool    isVIP() const { return static_cast<const Derived *>(this)->isVIP(); }
   inline StarId  getGaia() const { return static_cast<const Derived *>(this)->getGaia(); }
   inline bool    hasName() const { return static_cast<const Derived *>(this)->hasName(); }
   inline QString getNameI18n() const { return static_cast<const Derived *>(this)->getNameI18n(); }
   inline QString getScreenNameI18n() const { return static_cast<const Derived *>(this)->getScreenNameI18n(); }
   inline QString getDesignation() const { return static_cast<const Derived *>(this)->getDesignation(); }
   inline int     hasComponentID() const { return static_cast<const Derived *>(this)->hasComponentID(); }
   inline int     getObjType() const { return static_cast<const Derived *>(this)->getObjType(); }
   inline double  getPMTotal() const
   {
      return static_cast<const Derived *>(this)->getPMTotal();
   } // should return in mas/yr
   inline double getDx0() const
   {
      return static_cast<const Derived *>(this)->getDx0();
   } // should return in mas/yr (if pmra, then without cos(dec) components), 0 means no 1st proper motion
   inline double getDx1() const
   {
      return static_cast<const Derived *>(this)->getDx1();
   } // should return in mas/yr, 0 means no 2nd proper motion
   inline double getDx2() const
   {
      return static_cast<const Derived *>(this)->getDx2();
   } // should return in mas/yr, 0 means no 3rd proper motion
   inline double getPlx() const
   {
      return static_cast<const Derived *>(this)->getPlx();
   } // should return in mas, 0 means no parallax
   inline double getPlxErr() const
   {
      return static_cast<const Derived *>(this)->getPlxErr();
   } // should return in mas, 0 means no parallax error
   inline double getRV() const
   {
      return static_cast<const Derived *>(this)->getRV();
   } // should return in km/s, 0 means no radial velocity
   inline bool getPreciseAstrometricFlag() const
   {
      return static_cast<const Derived *>(this)->getPreciseAstrometricFlag();
   } // should only be true if full astrometric solution is available with getX2(), getDx2() too
   inline void getJ2000Pos(float dyrs, Vec3d & pos) const
   {
      // ideally whatever computation is done here should be done to get RA, DEC only
      // dont waste time computing other things because whoever calls this function dont need them
      if (getX2() != 0. ||
          getDx2() != 0.) { // as long as there is a 3rd component, we need to compute the full 3D position
         getEquatorialPos3D(dyrs, pos);
      } else {
         getEquatorialPos2D(dyrs, pos);
      }
   }
   // Special thanks to Anthony Brown's astrometry tutorial
   // This function only compute RA, DEC in rectangular coordinate system
   inline void getEquatorialPos3D(float dyrs, Vec3d & pos) const
   {
      double r0  = getX0();
      double r1  = getX1();
      double r2  = getX2();
      double pm0 = getDx0();
      double pm1 = getDx1();
      double pm2 = getDx2();
      double plx = getPlx();
      double vr  = getRV();
      Vec3d  r(r0, r1, r2);
      // proper motion
      Vec3d  pmvec0(pm0, pm1, pm2);
      pmvec0          = pmvec0 * MAS2RAD;
      double pmr0     = vr * plx / (AU / JYEAR_SECONDS) * MAS2RAD;
      double pmtotsqr = (pmvec0[0] * pmvec0[0] + pmvec0[1] * pmvec0[1] + pmvec0[2] * pmvec0[2]);

      double f        = 1. / sqrt(1. + 2. * pmr0 * dyrs + (pmtotsqr + pmr0 * pmr0) * dyrs * dyrs);
      Vec3d  u        = (r * (1. + pmr0 * dyrs) + pmvec0 * dyrs) * f;

      pos.set(u[0], u[1], u[2]);
   }
   inline void getEquatorialPos2D(float dyrs, Vec3d & pos) const
   {
      StelUtils::spheToRect(getX0() + dyrs * getDx0() * MAS2RAD, getX1() + dyrs * getDx1() * MAS2RAD, pos);
   }
   inline void getFull6DSolution(double & RA,
                                 double & DE,
                                 double & Plx,
                                 double & pmra,
                                 double & pmdec,
                                 double & RV,
                                 float    dyrs) const
   {
      if (getX2() != 0. || getDx2() != 0.) {
         // as long as there is a 3rd component, we need to compute the full 3D position using the equation below
         double r0  = getX0();
         double r1  = getX1();
         double r2  = getX2();
         double pm0 = getDx0();
         double pm1 = getDx1();
         double pm2 = getDx2();
         double plx = getPlx();
         double rv  = getRV();
         bool have_plx = plx != 0.;
         if (!have_plx)
            plx = 1.; // to avoid division by zero, any number will do
         Vec3d  r(r0, r1, r2);
         // proper motion
         Vec3d  pmvec0(pm0, pm1, pm2);
         pmvec0          = pmvec0 * MAS2RAD;
         double pmr0     = rv * plx / (AU / JYEAR_SECONDS) * MAS2RAD;
         double pmtotsqr = (pmvec0[0] * pmvec0[0] + pmvec0[1] * pmvec0[1] + pmvec0[2] * pmvec0[2]);

         double f2       = 1. / (1. + 2. * pmr0 * dyrs + (pmtotsqr + pmr0 * pmr0) * dyrs * dyrs);
         double f        = sqrt(f2);
         double f3       = f2 * f;
         Vec3d  u        = (r * (1. + pmr0 * dyrs) + pmvec0 * dyrs) * f;

         double Plx2   = plx * f;
         double pmr1   = (pmr0 + (pmtotsqr + pmr0 * pmr0) * dyrs) * f2;
         Vec3d  pmvec1 = pmvec0 * (1 + pmr0 * dyrs);

         pmvec1.set((pmvec1[0] - r[0] * pmtotsqr * dyrs) * f3,
                    (pmvec1[1] - r[1] * pmtotsqr * dyrs) * f3,
                    (pmvec1[2] - r[2] * pmtotsqr * dyrs) * f3);

         double xy = sqrt(u[0] * u[0] + u[1] * u[1]);
         Vec3d  p2(-u[1] / xy, u[0] / xy, 0.0);
         Vec3d  q2(-u[0] * u[2] / xy, -u[1] * u[2] / xy, xy);

         pmra = p2[0] * pmvec1[0] + p2[1] * pmvec1[1] + p2[2] * pmvec1[2];
         pmra /= MAS2RAD;
         pmdec = q2[0] * pmvec1[0] + q2[1] * pmvec1[1] + q2[2] * pmvec1[2];
         pmdec /= MAS2RAD;
         StelUtils::rectToSphe(&RA, &DE, u);
         if (RA < 0)
            RA += 2 * M_PI;
         if (have_plx)
            Plx = Plx2;
         else
            Plx = 0.;
         RV  = (pmr1 / MAS2RAD / Plx2) * (AU / JYEAR_SECONDS);
      } else {
         DE    = getX1();
         pmra  = getDx0() * cos(DE);
         pmdec = getDx1();
         RA    = getX0() + dyrs * getDx0() * MAS2RAD;
         DE    = getX1() + dyrs * getDx1() * MAS2RAD;
      }
   }

   //to get new 3D cartesian position for parallax effect given current star 3D cartesian position, planet orbital period and radius and current delta time from catalog epoch
   inline void getPlxEffect(double plx, Vec3d& bS, const Vec3d diffPos) const {
		if (plx <= 0.) return;
		bS *= 1000./ plx;
		bS += diffPos * MAS2RAD * 1000.;
   }
};

struct Star1 : public Star<Star1>
{
private:
   struct Data
   {
      qint64  gaia_id; // 8 bytes
      qint32  x0;      // 4 bytes, internal astrometric unit
      qint32  x1;      // 4 bytes, internal astrometric unit
      qint32  x2;      // 4 bytes, internal astrometric unit
      qint32  dx0;     // 4 bytes, uas/yr
      qint32  dx1;     // 4 bytes, uas/yr
      qint32  dx2;     // 4 bytes, uas/yr
      qint16  b_v;     // 2 bytea, B-V in milli-mag
      qint16  vmag;    // 2 bytes, V magnitude in milli-mag
      quint16 plx;     // 2 bytes, parallax in 20 uas
      quint16 plx_err; // 2 bytes, parallax error in 10 uas
      qint16  rv;      // 2 bytes, radial velocity in 100 m/s
      quint16 spInt;   // 2 bytes
      quint8  objtype; // 1 byte
      quint8  hip[3];  // 3 bytes, HIP number combined with component ID (A, B, ...)
   } d;

public:
   StelObjectP   createStelObject(const SpecialZoneArray<Star1> * a, const SpecialZoneData<Star1> * z) const;
   inline int    getMag() const { return d.vmag; } // in milli-mag
   inline int    getSpInt() const { return d.spInt; }
   inline double getX0() const { return d.x0 / 2.e9; }
   inline double getX1() const { return d.x1 / 2.e9; }
   inline double getX2() const { return d.x2 / 2.e9; }
   inline double getDx0() const { return d.dx0 / 1000.; }
   inline double getDx1() const { return d.dx1 / 1000.; }
   inline double getDx2() const { return d.dx2 / 1000.; }
   inline double getPlx() const { return d.plx * 0.02; }
   inline double getPlxErr() const { return d.plx_err / 100.; }
   inline double getPMTotal() const
   {
      // need to go through the calculation to get pmra and pmdec, use dyr = 0
      double RA, DE, Plx, pmra, pmdec, RV;
      getFull6DSolution(RA, DE, Plx, pmra, pmdec, RV, 0.);
      return sqrt(pmra * pmra + pmdec * pmdec);
   }
   inline double getRV() const { return d.rv / 10.; }
   inline bool   getPreciseAstrometricFlag() const
   {
      // Flag if the star should have time dependent astrometry computed
      // the star need to has parallax, proper motion, or radial velocity
      // use OR in each in case one of them is actually exactly 0
      // no point of doing proper propagation if any of them is missing
      return (getPlx() || getPlxErr()) && (getPlx() / getPlxErr() > 5) && (getDx0() || getDx1() || getDx2()) && getRV();
   }
   inline StarId getHip() const
   {
      // Combine the 3 bytes into a 24-bit integer (little-endian)
      quint32 combined_value = d.hip[0] | d.hip[1] << 8 | d.hip[2] << 16;
      // Extract the 17-bit ID (shift right by 5 bits)
      quint64  hip_id         = combined_value >> 5;
      return hip_id;
   }

   inline StarId  getGaia() const { return d.gaia_id; }
   inline int     getComponentIds() const
   {
      // Combine the 3 bytes into a 24-bit integer (little-endian)
      quint32 combined_value = d.hip[0] | d.hip[1] << 8 | d.hip[2] << 16;
      // Extract the 5-bit component ID
      quint8  letter_value   = combined_value & 0x1F; // 0x1F = 00011111 in binary mask
      return letter_value;
   }
   inline int getObjType() const { return d.objtype; }
   float      getBV(void) const { return static_cast<float>(d.b_v) / 1000.f; }
   bool       isVIP() const { return true; }
   bool       hasName() const { return getHip(); } // OR gaia??
   QString    getNameI18n(void) const;
   QString    getScreenNameI18n(void) const;
   QString    getDesignation(void) const;
   int        hasComponentID(void) const;
};
static_assert(sizeof(Star1) == 48, "Size of Star1 must be 48 bytes");

struct Star2 : public Star<Star2>
{
private:
   struct Data
   {
      qint64  gaia_id; // 8 bytes
      qint32  x0;      // 4 bytes, RA in mas
      qint32  x1;      // 4 bytes, DEC in mas
      qint32  dx0;     // 4 bytes, pmra in uas/yr
      qint32  dx1;     // 4 bytes, pmdec in uas/yr
      qint16  b_v;     // 2 byte
      qint16  vmag;    // 2 bytes
      quint16 plx;     // 2 bytes, parallax in 10 uas
      quint16 plx_err; // 2 bytes, parallax error in 10 uas
   } d;

public:
   inline double getX0() const { return d.x0 * MAS2RAD; }
   inline double getX1() const { return d.x1 * MAS2RAD; }
   inline double getX2() const { return 0; }
   inline double getDx0() const { return d.dx0 / 1000.; }
   inline double getDx1() const { return d.dx1 / 1000.; }
   inline double getDx2() const { return 0.; }
   inline int    getMag() const { return d.vmag; } // in milli-mag
   inline double getPMTotal() const
   {
      return sqrt((getDx0() * cos(getX1()) * getDx0() * cos(getX1())) + (getDx1() * getDx1()));
   }
   StelObjectP    createStelObject(const SpecialZoneArray<Star2> * a, const SpecialZoneData<Star2> * z) const;
   StarId         getGaia() const { return d.gaia_id; }
   float          getBV(void) const { return static_cast<float>(d.b_v) / 1000.f; }
   QString        getNameI18n(void) const { return QString(); }
   QString        getScreenNameI18n(void) const { return QString(); }
   QString        getDesignation(void) const { return QString(); }
   int            hasComponentID(void) const { return 0; }
   bool           isVIP() const { return false; }
   bool           hasName() const { return getGaia() != 0; }
   double         getPlx() const { return d.plx / 100.; }
   double         getPlxErr() const { return d.plx_err / 100.; }
   double         getRV() const { return 0.; }
   bool           getPreciseAstrometricFlag() const
   { // Flag if the star should have time dependent astrometry computed
      return false;
   }
   void print(void) const;
};
static_assert(sizeof(Star2) == 32, "Size of Star2 must be 32 bytes");

struct Star3 : public Star<Star3>
{
private:
   struct Data
   {
      qint64 gaia_id; // 8 bytes
      quint8 x0[3];   // 3 bytes, RA in 0.1 arcsecond
      quint8 x1[3];   // 3 bytes, DEC in 0.1 arcsecond (offset by +90 degree)
      quint8 b_v;     // 1 byte, B-V in 0.05 mag
      quint8 vmag;    // 1 bytes, V magnitude in 0.02 mag (offset by -16 mag)
   } d;

public:
   StelObjectP   createStelObject(const SpecialZoneArray<Star3> * a, const SpecialZoneData<Star3> * z) const;
   inline double getX0() const
   {
      quint32 x0 = d.x0[0] | (d.x0[1] << 8) | (d.x0[2] << 16);
      return static_cast<double>(x0) * 100. * MAS2RAD;
   }
   inline double getX1() const
   {
      quint32 x1 = d.x1[0] | (d.x1[1] << 8) | (d.x1[2] << 16);
      return (static_cast<double>(x1) - (90. * 36000.)) * 100. * MAS2RAD;
   }
   inline double  getX2() const { return 0.; }
   inline double  getDx0() const { return 0.; }
   inline double  getDx1() const { return 0.; }
   inline double  getDx2() const { return 0.; }
   inline double  getPMTotal() const { return 0.; }
   double         getPlx() const { return 0.; }
   double         getPlxErr() const { return 0.; }
   double         getRV() const { return 0.; }
   double         getBV() const { return (0.025 * d.b_v) - 1.; } // in mag
   double         getMag() const { return d.vmag * 20 + 16000; } // in milli-mag
   StarId         getGaia() const { return d.gaia_id; }
   QString        getNameI18n() const { return QString(); }
   QString        getScreenNameI18n() const { return QString(); }
   QString        getDesignation() const { return QString(); }
   bool           isVIP() const { return false; }
   int            hasComponentID() const { return 0; }
   bool           hasName() const { return d.gaia_id != 0; }
   bool           getPreciseAstrometricFlag() const
   { // Flag if the star should have time dependent astrometry computed
      return false;
   }
};
static_assert(sizeof(Star3) == 16, "Size of Star3 must be 16 bytes");

#endif // STAR_HPP
