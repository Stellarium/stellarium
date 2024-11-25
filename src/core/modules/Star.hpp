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

#include "ZoneData.hpp"
#include "StelObjectType.hpp"
#include "StelUtils.hpp"
#include <QString>
#include <QtEndian>
#include <cmath>

// Epoch in JD of the star catalog data
#define STAR_CATALOG_JDEPOCH 2457389.0

class StelObject;

extern const QString STAR_TYPE;

typedef short int Int16;
typedef unsigned short int Uint16;

template <class Star> class SpecialZoneArray;
template <class Star> struct SpecialZoneData;

// structs for storing the stars in binary form. The idea is
// to store much data for bright stars (Star1), but only little or even
// very little data for faints stars (Star3). Using only 6 bytes for Star3
// makes it feasible to store hundreds of millions of them in main memory.


static inline float IndexToBV(int bV)
{
	return static_cast<float>(bV)*(4.f/127.f)-0.5f;
}

static inline int BVToIndex(int bV)
{
	return static_cast<int>(bV+0.5f)*31.75f;
}

template <typename Derived>
struct Star {
	// methods that must be implemented by the derived class are those not returning 0 or False here
	inline double getX0() const { return static_cast<const Derived*>(this)->getX0(); }  // either in rad or in internal astrometric unit
	inline double getX1() const { return static_cast<const Derived*>(this)->getX1(); }  // either in rad or in internal astrometric unit
	inline double getX2() const { return static_cast<const Derived*>(this)->getX2(); }  // either in rad or in internal astrometric unit
	inline int getBVIndex() const { return static_cast<const Derived*>(this)->getBVIndex(); }
	inline double getMag() const { return static_cast<const Derived*>(this)->getMag(); }
	inline double getBV() const { return static_cast<const Derived*>(this)->getBV(); }
	inline bool isVIP() const { return static_cast<const Derived*>(this)->isVIP(); }
	inline bool hasName() const { return static_cast<const Derived*>(this)->hasName(); }
	inline QString getNameI18n() const { return static_cast<const Derived*>(this)->getNameI18n(); }
	inline QString getScreenNameI18n() const { return static_cast<const Derived*>(this)->getScreenNameI18n(); }
	inline QString getDesignation() const { return static_cast<const Derived*>(this)->getDesignation(); }
	inline int hasComponentID() const { return static_cast<const Derived*>(this)->hasComponentID(); }
	inline double getDx0() const { return static_cast<const Derived*>(this)->getDx0(); }  // should return in mas/yr (if pmra, then without cos(dec) components), 0 means no 1rd proper motion
	inline double getDx1() const { return static_cast<const Derived*>(this)->getDx1(); }  // should return in mas/yr, 0 means no 2rd proper motion
	inline double getDx2() const { return static_cast<const Derived*>(this)->getDx2(); }  // should return in mas/yr, 0 means no 3rd proper motion
	inline double getPlx() const { return static_cast<const Derived*>(this)->getPlx(); }  // should return in mas, 0 means no parallax
	inline double getPlxErr() const { return static_cast<const Derived*>(this)->getPlxErr(); }  // should return in mas, 0 means no parallax error
	inline double getRV() const { return static_cast<const Derived*>(this)->getRV(); }  // should return in km/s, 0 means no parallax error
	inline bool getPreciseAstrometricFlag() const { return static_cast<const Derived*>(this)->getPreciseAstrometricFlag();}  // should only be true if full astrometric solution is available with getX2(), getDx2() too
	inline void getJ2000Pos(float dyrs, Vec3f& pos) const
	{
		// ideally whatever computation is done here should be done to get RA, DEC only
		// dont waste time computing other things because whoever calls this function dont need them
		if (getX2() != 0. || getDx2() != 0.) {  // as long as there is a 3rd component, we need to compute the full 3D position
			getEquatorialPos3D(dyrs, pos);
		}
		else {
			getEquatorialPos2D(dyrs, pos);
		}
	}
	// Special thanks to Anthony Brown's astrometry tutorial
	// This function only compute RA, DEC in rectangular coordinate system
	inline void getEquatorialPos3D(float dyrs, Vec3f& pos) const 
	{
		double r0 = getX0();
		double r1 = getX1();
		double r2 = getX2();
		double pm0 = getDx0();
		double pm1 = getDx1();
		double pm2 = getDx2();
		double plx = getPlx();
		double vr = getRV();
		Vec3d r(r0, r1, r2);
		// proper motion
		Vec3d pmvec0(pm0, pm1, pm2);
		pmvec0 = pmvec0 * MAS2RAD;
		double pmr0 = vr * plx / (AU / JYEAR_SECONDS) * MAS2RAD;
		double pmtotsqr =  (pmvec0[0] * pmvec0[0] + pmvec0[1] * pmvec0[1] + pmvec0[2] * pmvec0[2]);

		double f = 1. / sqrt(1. + 2. * pmr0 * dyrs + (pmtotsqr + pmr0*pmr0)*dyrs*dyrs);
		Vec3d u = (r * (1. + pmr0 * dyrs) + pmvec0 * dyrs) * f;

		pos.set(u[0], u[1], u[2]);
	}
	inline void getEquatorialPos2D(float dyrs, Vec3f& pos) const
	{
		StelUtils::spheToRect(getX0() + dyrs * getDx0() * MAS2RAD, getX1() + dyrs * getDx1() * MAS2RAD, pos);
	}
	inline void getFull6DSolution(double& RA, double& DE, double& Plx, double& pmra, double& pmdec, double& RV, float dyrs) const
	{
		if (getPreciseAstrometricFlag()) {
			double r0 = getX0();
			double r1 = getX1();
			double r2 = getX2();
			double pm0 = getDx0();
			double pm1 = getDx1();
			double pm2 = getDx2();
			double plx = getPlx();
			double rv = getRV();
			Vec3d r(r0, r1, r2);
			// proper motion
			Vec3d pmvec0(pm0, pm1, pm2);
			pmvec0 = pmvec0 * MAS2RAD;
			double pmr0 = rv * plx / (AU / JYEAR_SECONDS) * MAS2RAD;
			double pmtotsqr =  (pmvec0[0] * pmvec0[0] + pmvec0[1] * pmvec0[1] + pmvec0[2] * pmvec0[2]);

			double f = 1. / sqrt(1. + 2. * pmr0 * dyrs + (pmtotsqr + pmr0*pmr0)*dyrs*dyrs);
			Vec3d u = (r * (1. + pmr0 * dyrs) + pmvec0 * dyrs) * f;

			// cartesian to spherical
			double lon = atan2(u[1], u[0]);
			// warp pi
			if (lon < 0.) lon += 2. * M_PI;
			double lat = atan2(u[2], sqrt(u[0] * u[0] + u[1] * u[1]));
			// double d = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);

			double Plx2 = plx * f;
			double pmr1 = (pmr0 + (pmtotsqr + pmr0 * pmr0) * dyrs) * f * f;
			Vec3d pmvel1 = pmvec0 * (1 + pmr0 * dyrs);
			pmvel1.set((pmvel1[0] - r[0] * pmr0 * pmr0 * dyrs) * f * f * f, (pmvel1[1] - r[1] * pmr0 * pmr0 * dyrs) * f * f * f, (pmvel1[2] - r[2] * pmr0 * pmr0 * dyrs) * f * f * f);

			double slon = sin(lon);
			double slat = sin(lat);
			double clon = cos(lon);
			double clat = cos(lat);

			// normal triad of a spherical coordinate system
			Vec3d p2(-slon, clon, 0.);
			Vec3d q2(-slat * clon, -slat * slon, clat);

			pmra = p2[0] * pmvel1[0] + p2[1] * pmvel1[1] + p2[2] * pmvel1[2];
			pmra /= MAS2RAD;
			pmdec = q2[0] * pmvel1[0] + q2[1] * pmvel1[1] + q2[2] * pmvel1[2];
			pmdec /= MAS2RAD;
			RA = lon;
			DE = lat;
			Plx = Plx2;
			RV = (pmr1 / MAS2RAD / plx) * (AU / JYEAR_SECONDS);
		}
		else {
			pmra = getDx0() * cos(DE);
			pmdec = getDx1();
			RA = getX0() + dyrs * getDx0() * MAS2RAD;
			DE = getX1() + dyrs * getDx1() * MAS2RAD;
		}
	}
	// void getJ2000withParallaxEffect(double& RA, double& DE, double& Plx, double& pmra, double& pmdec, double& vr, float& dyrs) const 
	// {
	// 	// RA and DE in radian
	// 	// Plx in mas
	// 	// pmra, pmdec in mas/yr
	// 	// vr in km/s
	// 	// dyrs in Julian year
	// 	// cant do this without Anthony Brown's astrometry tutorial
	// 	// this function assume RA, DE observed at J2000.0
	// 	static const double refepoch = 2000.0;
	// 	const double obs_epoch = refepoch + dyrs;
	// 	static const double au_in_meter = 149597870700.;
	// 	static const double au_mas_parsec = 1000.;  // AU expressed in mas*pc
	// 	static const double julian_year_seconds = 365.25 * 86400.;
	// 	static const double au_km_year_per_second = au_in_meter / julian_year_seconds / 1000.;
	// 	// static const double parsec = au_in_meter / 1000. / MAS2RAD;
	// 	static const double parsec = 30856775814913670;
	// 	static const double orbital_period = 1.0;  // in Julian year
	// 	static const double orbital_radius = 1.0;  // in AU

	// 	double sra = sin(RA);
	// 	double sde = sin(DE);
	// 	double cra = cos(RA);
	// 	double cde = cos(DE);

	// 	// need 3D spherical coordinate system
	// 	double radius = au_mas_parsec / Plx;
	// 	Vec3f xyz(radius * cra * cde, radius * sra * cde, radius * sde);

	// 	// normal triad of a spherical coordinate system
	// 	Vec3d p(-sra, cra, 0.);
	// 	Vec3d q(-sde * cra, -sde * sra, cde);
	// 	Vec3d r(cde * cra, cde * sra, sde);
	// 	Vec3d transverse_motion(pmra * au_km_year_per_second / Plx, pmdec * au_km_year_per_second / Plx, vr);
	// 	Mat3d md = Mat3d(p, q, r);
	// 	Vec3d vxvyvz = md.transpose() * transverse_motion;

	// 	// from observer's ephemeris
	// 	Vec3d bxyz(orbital_radius * cos(2. * M_PI / orbital_period * obs_epoch), orbital_radius * sin(2. * M_PI / orbital_period * obs_epoch), 0.);

	// 	// include Roemer delay
	// 	double tB = obs_epoch + (r * bxyz) * au_in_meter / julian_year_seconds / SPEED_OF_LIGHT;

	// 	// phase space coordinates
	// 	double vxyz_factor = (tB - refepoch) * (1000. * julian_year_seconds / parsec);
	// 	Vec3d bS(xyz[0] + vxvyvz[0] * vxyz_factor, xyz[1] + vxvyvz[1] * vxyz_factor, xyz[2] + vxvyvz[2] * vxyz_factor);
	// 	Vec3d u0 = bS - bxyz * au_in_meter / parsec;
	// 	double RA_obs = atan2(u0[1], u0[0]);
	// 	// wrap pi
	// 	if (RA_obs < 0.) RA_obs += 2. * M_PI;
	// 	double DEC_obs = atan2(u0[2], sqrt(u0[0] * u0[0] + u0[1] * u0[1]));

	// 	// change RA, DEC
	// 	RA = RA_obs;
	// 	DE = DEC_obs;
	// }
};

#pragma pack(push, 1)
struct Star1 : public Star<Star1> {
private:
	struct Data {
		quint8  hip[3];	      // 3 bytes
		qint64  gaia_id;	  // 8 bytes
		quint8  componentIds; // 1 byte
		qint32  x0;           // 4 bytes, internal astrometric unit
		qint32  x1;           // 4 bytes, internal astrometric unit
		qint32  x2;           // 4 bytes, internal astrometric unit
		qint16  b_v; 		  // 2 bytea, B-V in milli-mag
		qint16  vmag;         // 2 bytes, V magnitude in milli-mag
		quint16 spInt;        // 2 bytes
		qint32  dx0;          // 4 bytes, uas/yr
		qint32  dx1;          // 4 bytes, uas/yr
		qint32  dx2;          // 4 bytes, uas/yr
		quint16 plx;          // 2 bytes, parallax in 20 uas
		quint16 plx_err;      // 2 bytes, parallax error in 10 uas
		qint16  rv;		      // 2 bytes, radial velocity in 100 m/s
	} d;

public:
	StelObjectP createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const;
	inline int getBVIndex() const {return BVToIndex(getBV());}
	inline int getMag() const { return d.vmag; }
	inline int getSpInt() const {return d.spInt;}
	inline double getX0() const { return d.x0 / 2.e9;}
	inline double getX1() const { return d.x1 / 2.e9;}
	inline double getX2() const { return d.x2 / 2.e9;}
	inline double getDx0() const {return d.dx0 / 1000.;}
	inline double getDx1() const {return d.dx1 / 1000.;}
	inline double getDx2() const {return d.dx2 / 1000.;}
	inline double getPlx() const {return d.plx * 0.02;}
	inline double getRV() const {return d.rv / 10.;}
	inline bool getPreciseAstrometricFlag() const {
		// Flag if the star should have time dependent astrometry computed
		// the star need to has parallax, proper motion, or radial velocity
		// use OR in each in case one of them is actually exactly 0
		// no point of doing proper propagation if any of them is missing
		return (getPlx() || getPlxErr()) && (getDx0() || getDx1() || getDx2()) && getRV();
	}
	inline int getPlxErr() const {return d.plx_err;}
	inline int getHip() const {
		quint32 v = d.hip[0] | d.hip[1] << 8 | d.hip[2] << 16;
		return (static_cast<qint32>(v)) << 8 >> 8;
	}
	inline long getGaia() const { return d.gaia_id; }
	inline int getComponentIds() const {
		return d.componentIds;
	}

	float getBV(void) const {return static_cast<float>(d.b_v) / 1000.f;}
	bool isVIP() const {return true;}
	bool hasName() const {return getHip();}  // OR gaia??
	QString getNameI18n(void) const;
	QString getScreenNameI18n(void) const;
	QString getDesignation(void) const;
	int hasComponentID(void) const;
};
static_assert(sizeof(Star1) == 48, "Size of Star1 must be 48 bytes");
#pragma pack(pop) // Restore the previous packing alignment

#pragma pack(push, 1)
struct Star2 : public Star<Star2> {
private:
	struct Data {
		qint64  gaia_id;	  // 8 bytes
		qint32  x0;           // 4 bytes, RA in mas
		qint32  x1;           // 4 bytes, DEC in mas
		qint32  dx0;          // 4 bytes, pmra in uas/yr
		qint32  dx1;          // 4 bytes, pmdec in uas/yr
		qint16  b_v; 		  // 2 byte
		qint16  vmag;         // 2 bytes
	} d;  // total is 28 bytes
public:
	inline double getX0() const { return d.x0 * MAS2RAD;}
	inline double getX1() const { return d.x1 * MAS2RAD;}
	inline double getX2() const {return 0;}
	inline double getDx0() const {return d.dx0 / 1000.;}
	inline double getDx1() const {return d.dx1 / 1000.;}
	inline double getDx2() const {return 0.;}
	inline int getBVIndex() const {return BVToIndex(getBV());}
	inline int getMag() const { return d.vmag; }
	StelObjectP createStelObject(const SpecialZoneArray<Star2> *a, const SpecialZoneData<Star2> *z) const;
	inline long getGaia() const { return d.gaia_id; }
	float getBV(void) const {return static_cast<float>(d.b_v) / 1000.f;}
	QString getNameI18n(void) const {return QString();}
	QString getScreenNameI18n(void) const {return QString();}
	QString getDesignation(void) const {return QString();}
	int hasComponentID(void) const {return 0;}
	bool isVIP() const {return false;}
	bool hasName() const {return getGaia();}
	double getPlx() const {return 0.;}
	double getPlxErr() const {return 0.;}
	double getRV() const {return 0.;}
	bool getPreciseAstrometricFlag() const { // Flag if the star should have time dependent astrometry computed
		return false;
	}
	void print(void) const;
};
static_assert(sizeof(Star2) == 28, "Size of Star2 must be 28 bytes");
#pragma pack(pop) // Restore the previous packing alignment

struct Star3 : public Star<Star3> {
	/*
	          _______________
	0     x0 |               |
	1        |___________    |
	2     x1 |           |___|
	3        |_______        |
	4     bV |_______|_______|
	5    mag |_________|_____| bV

	int x0               :18
	int x1               :18
	unsigned int bV      :7
	unsigned int mag     :5
	*/
private:
	quint8 d[6];

public:
	StelObjectP createStelObject(const SpecialZoneArray<Star3> *a, const SpecialZoneData<Star3> *z) const;
	inline double getX0() const
	{
		quint32 v = d[0] | d[1] << 8 | (d[2] & 0x3) << 16;
		return (static_cast<int>(v)) << 14 >> 14;
	}

	inline double getX1() const
	{
		quint32 v = d[2] >> 2 | d[3] << 6 | (d[4] & 0xF) << 14;
		return (static_cast<int>(v)) << 14 >> 14;
	}
	inline double getX2() const {return 0.;}
	inline double getDx0() const {return 0.;}
	inline double getDx1() const {return 0.;}
	inline double getDx2() const {return 0.;}
	double getPlx() const {return 0.;}
	double getPlxErr() const {return 0.;}
	double getRV() const {return 0.;}
	inline int getBVIndex() const
	{
		return d[4] >> 4 | (d[5] & 0x7) << 4;
	}

	inline int getMag() const
	{
		return d[5] >> 3;
	}

	float getBV() const {return IndexToBV(getBVIndex());}
	QString getNameI18n() const {return QString();}
	QString getScreenNameI18n() const {return QString();}
	QString getDesignation() const {return QString();}
	bool isVIP() const {return false;}
	int hasComponentID() const {return 0;}
	bool hasName() const {return false;}
	bool getPreciseAstrometricFlag() const { // Flag if the star should have time dependent astrometry computed
		return false;
	}
};
static_assert(sizeof(Star3) == 6, "Size of Star3 must be 6 bytes");

#endif // STAR_HPP
