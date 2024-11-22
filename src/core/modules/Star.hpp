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


#pragma pack(push, 1)
struct Star1 {
private:
	struct Data {
		quint8  hip[3];	      // 3 bytes
		qint64  gaia_id;	  // 8 bytes
		quint8  componentIds; // 1 byte
		qint32  x0;           // 4 bytes, ra in mas
		qint32  x1;           // 4 bytes, dec in mas
		qint16  b_v; 		  // 2 byte, B-V in milli-mag
		qint16  vmag;         //` 2 bytes, V magnitude in milli-mag
		quint16 spInt;        // 2 bytes
		qint32  dx0;          // 4 bytes, pmra in uas/yr
		qint32  dx1;          // 4 bytes, pmdec in uas/yr
		quint16 plx;          // 2 bytes, parallax in 10 uas
		quint16 plx_err;      // 2 bytes, parallax error in 10 uas
		qint16  rv;		      // 2 bytes, radial velocity in 10 m/s
	} d;

public:
	enum {MaxPosVal=0x7FFFFFFF};
	StelObjectP createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const;

	void getJ2000withParallaxEffect(double& RA, double& DE, double& Plx, double& pmra, double& pmdec, double& vr, float& dyr) const 
	{
		// RA and DE in radian
		// Plx in mas
		// pmra, pmdec in mas/yr
		// vr in km/s
		// dyr in Julian year
		// cant do this without Anthony Brown's astrometry tutorial
		// this function assume RA, DE observed at J2000.0
		static const double refepoch = 2000.0;
		const double obs_epoch = refepoch + dyr;
		static const double au_in_meter = 149597870700.;
		static const double au_mas_parsec = 1000.;  // AU expressed in mas*pc
		static const double julian_year_seconds = 365.25 * 86400.;
		static const double au_km_year_per_second = au_in_meter / julian_year_seconds / 1000.;
		// static const double parsec = au_in_meter / 1000. / MAS2RAD;
		static const double parsec = 30856775814913670;
		static const double orbital_period = 1.0;  // in Julian year
		static const double orbital_radius = 1.0;  // in AU

		double sra = sin(RA);
		double sde = sin(DE);
		double cra = cos(RA);
		double cde = cos(DE);

		// need 3D spherical coordinate system
		double radius = au_mas_parsec / Plx;
		Vec3f xyz(radius * cra * cde, radius * sra * cde, radius * sde);

		// normal triad of a spherical coordinate system
		Vec3d p(-sra, cra, 0.);
		Vec3d q(-sde * cra, -sde * sra, cde);
		Vec3d r(cde * cra, cde * sra, sde);
		Vec3d transverse_motion(pmra * au_km_year_per_second / Plx, pmdec * au_km_year_per_second / Plx, vr);
		Mat3d md = Mat3d(p, q, r);
		Vec3d vxvyvz = md.transpose() * transverse_motion;

		// from observer's ephemeris
		Vec3d bxyz(orbital_radius * cos(2. * M_PI / orbital_period * obs_epoch), orbital_radius * sin(2. * M_PI / orbital_period * obs_epoch), 0.);

		// include Roemer delay
		double tB = obs_epoch + (r * bxyz) * au_in_meter / julian_year_seconds / SPEED_OF_LIGHT;

		// phase space coordinates
		double vxyz_factor = (tB - refepoch) * (1000. * julian_year_seconds / parsec);
		Vec3d bS(xyz[0] + vxvyvz[0] * vxyz_factor, xyz[1] + vxvyvz[1] * vxyz_factor, xyz[2] + vxvyvz[2] * vxyz_factor);
		Vec3d u0 = bS - bxyz * au_in_meter / parsec;
		double RA_obs = atan2(u0[1], u0[0]);
		// wrap pi
		if (RA_obs < 0.) RA_obs += 2. * M_PI;
		double DEC_obs = atan2(u0[2], sqrt(u0[0] * u0[0] + u0[1] * u0[1]));

		// change RA, DEC
		RA = RA_obs;
		DE = DEC_obs;
	}

	void getJ2000pos3D(double& RA, double& DE, double& Plx, double pmra, double pmdec, double vr, float dyr, Vec3f& pos) const {
		// cant do this without Anthony Brown's astrometry tutorial

		double sra = sin(RA);
		double sde = sin(DE);
		double cra = cos(RA);
		double cde = cos(DE);

		// normal triad of a spherical coordinate system
		Vec3d p(-sra, cra, 0.);
		Vec3d q(-sde * cra, -sde * sra, cde);
		Vec3d r(cde * cra, cde * sra, sde);

		pmra *= MAS2RAD;
		pmdec *= MAS2RAD;
		double pmr0 = vr * Plx / (AU / JYEAR_SECONDS) * MAS2RAD;
		double pmtotsqr =  (pmra * pmra + pmdec * pmdec);

		// proper motion
		Vec3d pm0 = pmra * p + pmdec * q;

		double f = 1. / sqrt(1. + 2. * pmr0 * dyr + (pmtotsqr + pmr0*pmr0)*dyr*dyr);
		Vec3d u = (r * (1. + pmr0 * dyr) + pm0 * dyr) * f;

		pos.set(u[0], u[1], u[2]);
		// no need to map back to RA/DE
	}

	void getJ2000Pos6DTreatment(double& RA, double& DE, double& Plx, double& pmra, double& pmdec, double& vr, float& dyr) const {
		// cant do this without Anthony Brown's astrometry tutorial

		double sra = sin(RA);
		double sde = sin(DE);
		double cra = cos(RA);
		double cde = cos(DE);

		// normal triad of a spherical coordinate system
		Vec3d p(-sra, cra, 0.);
		Vec3d q(-sde * cra, -sde * sra, cde);
		Vec3d r(cde * cra, cde * sra, sde);

		pmra *= MAS2RAD;
		pmdec *= MAS2RAD;
		double pmr0 = vr * Plx / (AU / JYEAR_SECONDS) * MAS2RAD;
		double pmtotsqr =  (pmra * pmra + pmdec * pmdec);

		// proper motion
		Vec3d pm0 = pmra * p + pmdec * q;

		double f = 1. / sqrt(1. + 2. * pmr0 * dyr + (pmtotsqr + pmr0*pmr0)*dyr*dyr);
		Vec3d u = (r * (1. + pmr0 * dyr) + pm0 * dyr) * f;

		// cartesian to spherical
		double lon = atan2(u[1], u[0]);
		// warp pi
		if (lon < 0.) lon += 2. * M_PI;
		double lat = atan2(u[2], sqrt(u[0] * u[0] + u[1] * u[1]));
		// double d = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);

		double Plx2 = Plx * f;
		double pmr1 = (pmr0 + (pmtotsqr + pmr0 * pmr0) * dyr) * f * f;
		Vec3d pmvel1 = pm0 * (1 + pmr0 * dyr);
		pmvel1.set((pmvel1[0] - r[0] * pmr0 * pmr0 * dyr) * f * f * f, (pmvel1[1] - r[1] * pmr0 * pmr0 * dyr) * f * f * f, (pmvel1[2] - r[2] * pmr0 * pmr0 * dyr) * f * f * f);

		double slon = sin(lon);
		double slat = sin(lat);
		double clon = cos(lon);
		double clat = cos(lat);

		// normal triad of a spherical coordinate system
		Vec3d p2(-slon, clon, 0.);
		Vec3d q2(-slat * clon, -slat * slon, clat);
		// Vec3d r2(clat * clon, clat * slon, slat);

		pmra = p2[0] * pmvel1[0] + p2[1] * pmvel1[1] + p2[2] * pmvel1[2];
		pmra /= MAS2RAD;
		pmdec = q2[0] * pmvel1[0] + q2[1] * pmvel1[1] + q2[2] * pmvel1[2];
		pmdec /= MAS2RAD;
		RA = lon;
		DE = lat;
		Plx = Plx2;
		vr = (pmr1 / MAS2RAD / Plx) * (AU / JYEAR_SECONDS);
	}

	void getJ2000Pos(float dyr, Vec3f& pos) const
	{
		// conversion from mas to rad
		double RA_rad = getX0() * MAS2RAD;
		double DE_rad = getX1() * MAS2RAD;

		
		// if (getTimeDependence() && -1 == 0) {  // on top of proper motion, also consider parallax
		// 	double Plx = getPlx() * 0.01;
		// 	double pmra = getDx0() / 1000.;
		// 	double pmdec = getDx1() / 1000.;
		// 	double vr = getRV() / 100.;
		// 	getJ2000withParallaxEffect(RA_rad, DE_rad, Plx, pmra, pmdec, vr, dyr);
		// }
		if (getTimeDependence()){  // 3D astrometry propagation
			double Plx = getPlx() / 100.;
			double pmra = getDx0() / 1000.;
			double pmdec = getDx1() / 1000.;
			double vr = getRV() / 10.;
			getJ2000pos3D(RA_rad, DE_rad, Plx, pmra, pmdec, vr, dyr, pos);
		}
		else {
			// no parallax no radial velocity, just proper motion
			// conversion from mas/yr to rad/yr, so dra in rad
			// getDx0 already has cos(DE) factor
			RA_rad += dyr * (getDx0() / 1000.f) * MAS2RAD / cos(DE_rad);
			DE_rad += dyr * (getDx1() / 1000.f) * MAS2RAD;
			StelUtils::spheToRect(RA_rad, DE_rad, pos);
		}

	}
	inline int getBVIndex() const {return BVToIndex(getBV());}
	inline int getMag() const { return d.vmag; }
	inline int getSpInt() const {return d.spInt;}
	inline int getX0() const { return d.x0;}
	inline int getX1() const { return d.x1;}
	inline int getDx0() const {return d.dx0;}
	inline int getDx1() const {return d.dx1;}
	inline int getPlx() const {return d.plx;}
	inline int getRV() const {return d.rv;}
	inline void get6Dsolution(double& RA, double& DE, double& Plx, double& pmra, double& pmdec, double& vr, float dyr) const {
		RA = getX0() * MAS2RAD;
		DE = getX1() * MAS2RAD;
		Plx = getPlx() * 0.01;
		pmra = getDx0() / 1000.;
		pmdec = getDx1() / 1000.;
		vr = getRV() / 10.;
		getJ2000Pos6DTreatment(RA, DE, Plx, pmra, pmdec, vr, dyr);
	}
	inline bool getTimeDependence() const {
		// Flag if the star has time dependent data
		// the star need to has parallax, proper motion, or radial velocity
		// use OR in each in case one of them is actually exactly 0
		// no point of doing proper propagation if any of them is missing
		return (getPlx() || getPlxErr()) && (getDx0() || getDx1()) && getRV();
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
	bool hasName() const {return getHip();}  // OR gaia??
	QString getNameI18n(void) const;
	QString getScreenNameI18n(void) const;
	QString getDesignation(void) const;
	int hasComponentID(void) const;
	void print(void) const;
};
static_assert(sizeof(Star1) == 40, "Size of Star1 must be 40 bytes");
#pragma pack(pop) // Restore the previous packing alignment

#pragma pack(push, 1)
struct Star2 {  // 20 byte
private:
	struct Data {
		qint64  gaia_id;	  // 8 bytes
		qint32  x0;           // 4 bytes
		qint32  x1;           // 4 bytes
		qint32  dx0;          // 4 bytes
		qint32  dx1;          // 4 bytes
		qint16  b_v; 		  // 2 byte
		qint16  vmag;         // 2 bytes
	} d;  // total is 28 bytes
public:
	inline int getX0() const { return d.x0;}
	inline int getX1() const { return d.x1;}
	inline int getDx0() const {return d.dx0;}
	inline int getDx1() const {return d.dx1;}
	inline int getBVIndex() const {return BVToIndex(getBV());}
	inline int getMag() const { return d.vmag; }

	enum {MaxPosVal=((1<<19)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star2> *a, const SpecialZoneData<Star2> *z) const;
	void getJ2000Pos(float dyr, Vec3f& pos) const
	{
		// conversion from mas to rad
		const double RA_rad = getX0() * MAS2RAD;
		const double DE_rad = getX1() * MAS2RAD;
		// conversion from mas/yr to rad/yr, so dra in rad
		const double dra = dyr * (getDx0() / 1000.f) / cos(DE_rad) * MAS2RAD;
		// getDx1 already in mas/yr, so ddec in rad
		const double ddec = dyr * (getDx1() / 1000.f) * MAS2RAD;
		StelUtils::spheToRect(RA_rad + dra, DE_rad + ddec, pos);
	}
	inline long getGaia() const { return d.gaia_id; }
	float getBV(void) const {return static_cast<float>(d.b_v) / 1000.f;}
	QString getNameI18n(void) const {return QString();}
	QString getScreenNameI18n(void) const {return QString();}
	QString getDesignation(void) const {return QString();}
	int hasComponentID(void) const {return 0;}
	bool hasName() const {return getGaia();}
	void print(void) const;
};
static_assert(sizeof(Star2) == 28, "Size of Star2 must be 28 bytes");
#pragma pack(pop) // Restore the previous packing alignment

struct Star3 {  // 6 byte
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
	inline int getX0() const
	{
		quint32 v = d[0] | d[1] << 8 | (d[2] & 0x3) << 16;
		return (static_cast<qint32>(v)) << 14 >> 14;
	}

	inline int getX1() const
	{
		quint32 v = d[2] >> 2 | d[3] << 6 | (d[4] & 0xF) << 14;
		return (static_cast<qint32>(v)) << 14 >> 14;
	}

	inline int getBVIndex() const
	{
		return d[4] >> 4 | (d[5] & 0x7) << 4;
	}

	inline int getMag() const
	{
		return d[5] >> 3;
	}

	enum {MaxPosVal=((1<<17)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star3> *a, const SpecialZoneData<Star3> *z) const;
	void getJ2000Pos(float dyr, Vec3f& pos) const
	{
		Q_UNUSED(dyr);  // they don't have proper motion
		// conversion from mas to rad
		const double RA_rad = getX0() * MAS2RAD;
		const double DE_rad = getX1() * MAS2RAD;
		StelUtils::spheToRect(RA_rad, DE_rad, pos);
	}
	float getBV() const {return IndexToBV(getBVIndex());}
	QString getNameI18n() const {return QString();}
	QString getScreenNameI18n() const {return QString();}
	QString getDesignation() const {return QString();}
	int hasComponentID() const {return 0;}
	bool hasName() const {return false;}
	void print();
};
static_assert(sizeof(Star3) == 6, "Size of Star3 must be 6 bytes");

#endif // STAR_HPP
