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
   inline StarId  getHip() const { return static_cast<const Derived *>(this)->getHip(); }
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
      if (getX2() != 0. || getDx2() != 0.) 
      {  // for star data type 0, have full astrometric solution
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
         return;
      }
      if (getDx1() != 0. || getDx2() != 0.)  // for star data type 1, have proper motion
      {
         DE    = getX1();
         pmra  = getDx0() * cos(DE);
         pmdec = getDx1();
         RA    = getX0() + dyrs * getDx0() * MAS2RAD;
         DE    = getX1() + dyrs * getDx1() * MAS2RAD;
         Plx   = getPlx();
         RV    = getRV();
         return;
      }
      else 
      {  // for star data type 2, have no proper motion
         RA    = getX0();
         DE    = getX1();
         Plx = 0.;
         RV = 0.;
         pmra = 0.;
         pmdec = 0.;
         return;
      }
   }

   //to get new 3D cartesian position for parallax effect given current star 3D cartesian position, planet orbital period and radius and current delta time from catalog epoch
   inline void getPlxEffect(double plx, Vec3d& bS, const Vec3d diffPos) const {
		if (plx <= 0.) return;
		bS *= 1000./ plx;
		bS += diffPos * MAS2RAD * 1000.;
   }

   inline void getBinaryOrbit(double epoch, Vec3d& v) const {
      double pmra = 0.;
      double pmdec = 0.;
      double rv = 0.;
      double plx = 0.;
      double ra = 0.;
      double dec = 0.;
      getBinaryOrbit(epoch, v, ra, dec, plx, pmra, pmdec, rv);
   }

   inline void getBinaryOrbit(double epoch, Vec3d& v, double& ra, double& dec, double& plx, double& pmra, double& pmdec, double& RV) const {
      double sep = 0.;
      double pa = 0.;
      getBinaryOrbit(epoch, v, ra, dec, plx, pmra, pmdec, RV, sep, pa);
   }

   inline void getBinaryOrbit(double epoch, Vec3d& v, double& ra, double& dec, double& plx, double& pmra, double& pmdec, double& RV, double& sep, double& pa) const {
      StarId star_id;  // star ID
      if (getGaia() == 0)
      {
         star_id = getHip();
      }
      else
      {
         star_id = getGaia();
      }

      // look up if hip is in the binary star map
      binaryorbitstar bso = StarMgr::getBinaryOrbitData(star_id);
      // check if bso is empty or not
      if (bso.hip == 0)
      {
         // exit the function because nothing to do, not a binary star
         return;
      }

      // Orbital elements of the secondary star
      double binary_period = bso.binary_period;  // Orbital period [days]
      float eccentricity = bso.eccentricity;  // Eccentricity
      float inclination = bso.inclination;  // Orbit inclination [rad]
      float big_omega = bso.big_omega;  // Position angle of ascending node [rad]
      float small_omega = bso.small_omega;  // Argument of periastron [rad]
      double periastron_epoch = bso.periastron_epoch;  // Julian Date at periastron passage
      double semi_major = bso.semi_major;  // Angular semi-major axis [arcsec]
      double bary_distance = bso.bary_distance;  // distance [parsec]
      double data_epoch = bso.data_epoch;  // Julian Date of the data
      double bary_ra = bso.bary_ra;  // Right Ascension of the barycenter [rad]
      double bary_dec = bso.bary_dec;  // Declination of the barycenter [rad]
      // motion of the barycenter
      double bary_rv = bso.bary_rv;  // Barycenter radial velocity [km/s]
      float primary_mass = bso.primary_mass;  // Primary star mass [solar mass]
      float secondary_mass = bso.secondary_mass;  // Secondary star mass [solar mass]
      double bary_pmra = bso.bary_pmra;  // Barycenter RA proper motion [mas/yr]
      double bary_pmdec = bso.bary_pmdec;  // Barycenter DEC proper motion [mas/yr]
      if (!data_epoch)
      {
         data_epoch = STAR_CATALOG_JDEPOCH;  // by default assume the date is the same as the catalog epoch
      }
      const double G = 4.49850215e-15;  // Gravitational constant in parsec^3 / (M_sun * yr^2)

      double dyrs = (epoch - data_epoch) / 365.25;
      semi_major = semi_major * bary_distance * MAS2RAD * 1000.;  // Convert to parsec
      double mass_ratio = secondary_mass / (primary_mass + secondary_mass);
      Vec3d bary_r;
      StelUtils::spheToRect(bary_ra, bary_dec, bary_r);  // barycenter position in equatorial cartesian coordinate

      // angular phase since periastron (at periastron_epoch)
      double ud = 2 * M_PI * (epoch - periastron_epoch) / binary_period;
      ud = fmod(ud, 2 * M_PI);  // warp u to [0, 2pi]
      
      // eccentric anomaly with newton's method
      double E = ud;
      // iterate until convergence or maximum iterations
      const int max_iterations = 100;
      const double tolerance = 1e-10;
      for (int j = 0; j < max_iterations; j++)
      {
         double delta = (E - eccentricity * sin(E) - ud) / (1 - eccentricity * cos(E));
         E = E - delta;
         if (fabs(delta) < tolerance)
         {
            break;
         }
      }

      // calculate true anomaly nu and bary_distance r
      double nu = 2 * atan(sqrt((1 + eccentricity) / (1 - eccentricity)) * tan(E / 2));
      double radius = semi_major * (1 - eccentricity * cos(E));  // in parsec
      Vec3d true_orbit(radius * cos(nu), radius * sin(nu), 0);  // all axis in parsec

      // Angular momentum per unit mass
      double h = sqrt(semi_major * (1 - eccentricity * eccentricity));
      double mu = G * (primary_mass + secondary_mass);
      double v_r = (1. / h) * eccentricity * sin(nu);  // radial velocity
      double v_theta = h / radius;  // Tangential velocity
      Vec3d true_orbit_vel(v_r * cos(nu) - v_theta * sin(nu), v_r * sin(nu) + v_theta * cos(nu), 0.);
      true_orbit_vel *= sqrt(mu);  // scale the velocity to physical units in parsec/yr

      // rotation matrix from true to sky plane
      Mat3d rot;
      rot.set(cos(big_omega) * cos(small_omega) - sin(big_omega) * sin(small_omega) * cos(inclination),
            -cos(big_omega) * sin(small_omega) - sin(big_omega) * cos(small_omega) * cos(inclination),
            sin(big_omega) * sin(inclination),
            sin(big_omega) * cos(small_omega) + cos(big_omega) * sin(small_omega) * cos(inclination),
            -sin(big_omega) * sin(small_omega) + cos(big_omega) * cos(small_omega) * cos(inclination),
            -cos(big_omega) * sin(inclination),
            sin(small_omega) * sin(inclination),
            cos(small_omega) * sin(inclination),
            cos(inclination));

      // to equatorial cartesian coordinate, similar to normal triad but with additional declination rotation
      Vec3d p(-sin(bary_ra), cos(bary_ra), 0.);
      Vec3d q(-sin(bary_dec) * cos(bary_ra), -sin(bary_dec) * sin(bary_ra), cos(bary_dec));
      Vec3d r(cos(bary_dec) * cos(bary_ra), cos(bary_dec) * sin(bary_ra), sin(bary_dec));

      // add the barycenter shift from proper motion
      Vec3d bary_pmvec0 = (p * bary_pmra + q * bary_pmdec) * MAS2RAD;
      double bary_pmr0 = bary_rv * (1000. / bary_distance) / (AU / JYEAR_SECONDS) * MAS2RAD;
      double bary_pmtotsqr = (bary_pmvec0[0] * bary_pmvec0[0] + bary_pmvec0[1] * bary_pmvec0[1] + bary_pmvec0[2] * bary_pmvec0[2]);
      double bary_f = 1. / sqrt(1. + 2. * bary_pmr0 * dyrs + (bary_pmtotsqr + bary_pmr0 * bary_pmr0) * dyrs * dyrs);
      Vec3d  bary_u = (bary_r * (1. + bary_pmr0 * dyrs) + bary_pmvec0 * dyrs) * bary_f;

      double xy = sqrt(bary_u[0] * bary_u[0] + bary_u[1] * bary_u[1]);
      Vec3d  p2(-bary_u[1] / xy, bary_u[0] / xy, 0.0);
      Vec3d  q2(-bary_u[0] * bary_u[2] / xy, -bary_u[1] * bary_u[2] / xy, xy);
      // to sky plane, all axis in parsec and parsec/yr
      Vec3d sky_orbit(0.);
      Vec3d sky_orbit_vel(0.);
      for (size_t i = 0; i < 3; ++i) {
         for (size_t j = 0; j < 3; ++j) {
               sky_orbit[i] += rot[i*3+j] * true_orbit[j];
               sky_orbit_vel[i] += rot[i*3+j] * true_orbit_vel[j];
         }
      }

      // swap the first two components of sky_orbit to match ra, dec axis
      double tmp = sky_orbit[0];
      sky_orbit[0] = sky_orbit[1];
      sky_orbit[1] = tmp;
      tmp = sky_orbit_vel[0];
      sky_orbit_vel[0] = sky_orbit_vel[1];
      sky_orbit_vel[1] = tmp;

      // compute on-sky separation and position angle
      sep = sqrt(sky_orbit[0] * sky_orbit[0] + sky_orbit[1] * sky_orbit[1]) / bary_distance / MAS2RAD / 1000.;  // in arcsecond
      pa = atan2(sky_orbit[0], sky_orbit[1]);
      if (pa < 0)
         pa += 2 * M_PI;

      if (bso.primary)
      {
         sky_orbit *= -mass_ratio;
         sky_orbit_vel *= -mass_ratio;
      }
      else
      {
         sky_orbit *= (1.0 - mass_ratio);
         sky_orbit_vel *= (1.0 - mass_ratio);
      }

      // sky_orbit is in parsec and sky_orbit_vel is in parsec/year, we want them arcsecond and arcsecond/year
      sky_orbit[0] /= bary_distance;
      sky_orbit[1] /= bary_distance;
      sky_orbit_vel *= 1.e-3 / MAS2RAD / bary_distance;

      // sky_orbit_vel is in arcsecond/year
      sky_orbit_vel.set(sky_orbit_vel[0] + bary_pmra / 1000., 
                        sky_orbit_vel[1] + bary_pmdec / 1000., 
                        sky_orbit_vel[2] + bary_rv * (JYEAR_SECONDS / AU) / bary_distance);

      sky_orbit_vel = p * sky_orbit_vel[0] + q * sky_orbit_vel[1] + r * sky_orbit_vel[2];  // arcsecond/year

      StelUtils::spheToRect(bary_ra + sky_orbit[0], 
                            bary_dec + sky_orbit[1], 
                            bary_distance + sky_orbit[2], 
                            sky_orbit);  // barycenter position in equatorial cartesian coordinate

      plx = 1000. / bary_distance * bary_f;
      RV  = sky_orbit_vel.dot(bary_u) * (1000./plx) * (AU / JYEAR_SECONDS);
      v = sky_orbit  - bary_r * bary_distance + bary_u * (1000. / plx);
      pmra = sky_orbit_vel.dot(p2) * 1000.;
      pmdec = sky_orbit_vel.dot(q2) * 1000.;
      StelUtils::rectToSphe(&ra, &dec, v);
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
   //! Retrieve the cultural screen label, translated version of the commonName (if withCommonNameI18n), or a designation.
   QString    getScreenNameI18n(const bool withCommonNameI18n) const;
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
   StarId         getHip() const { return 0; }
   StarId         getGaia() const { return d.gaia_id; }
   float          getBV(void) const { return static_cast<float>(d.b_v) / 1000.f; }
   QString        getNameI18n(void) const { return QString(); }
   QString        getScreenNameI18n(const bool) const { return QString(); }
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
   StarId         getHip() const { return 0; }
   StarId         getGaia() const { return d.gaia_id; }
   QString        getNameI18n() const { return QString(); }
   QString        getScreenNameI18n(const bool) const { return QString(); }
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
