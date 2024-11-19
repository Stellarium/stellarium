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

class StelObject;

extern const QString STAR_TYPE;

typedef short int Int16;
typedef unsigned short int Uint16;

template <class Star> class SpecialZoneArray;
template <class Star> struct SpecialZoneData;

#define MAS2RAD_SCALE M_PI / (3600000. * 180.);

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
		qint32  x0;           // 4 bytes
		qint32  x1;           // 4 bytes
		qint16  b_v; 		  // 2 byte
		qint16  vmag;         // 2 bytes
		quint16 spInt;        // 2 bytes
		qint32  dx0;          // 4 bytes
		qint32  dx1;          // 4 bytes
		quint16 plx;          // 2 bytes
		quint16 plx_err;      // 2 bytes 
	} d;  // total is 30 bytes

public:
	enum {MaxPosVal=0x7FFFFFFF};
	StelObjectP createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const;
	void getJ2000Pos(float dyr, Vec3f& pos) const
	{
		// conversion from mas to rad
		const double RA_rad = getX0() * MAS2RAD_SCALE;
		const double DE_rad = getX1() * MAS2RAD_SCALE;
		// conversion from mas/yr to rad/yr, so dra in rad
		const double dra = dyr * (getDx0() / 1000.f) / cos(DE_rad) * MAS2RAD_SCALE;
		// getDx1 already in mas/yr, so ddec in rad
		const double ddec = dyr * (getDx1() / 1000.f) * MAS2RAD_SCALE;
		StelUtils::spheToRect(RA_rad - dra, DE_rad - ddec, pos);
	}
	inline int getBVIndex() const {return BVToIndex(getBV());}
	inline int getMag() const { return d.vmag; }
	inline int getInternalMag(int mag_steps, int mag_min, int mag_range) const { return mag_steps * (d.vmag / 1000.f - 0.001f*mag_min) / (0.001f*mag_range); }
	inline int getSpInt() const {return d.spInt;}
	inline int getX0() const { return d.x0;}
	inline int getX1() const { return d.x1;}
	inline int getDx0() const {return d.dx0;}
	inline int getDx1() const {return d.dx1;}
	inline int getPlx() const {return d.plx;}
	inline int getPlxErr() const {return d.plx_err;}
	inline int getHip() const {
		quint32 v = d.hip[0] | d.hip[1] << 8 | d.hip[2] << 16;
		return (static_cast<qint32>(v)) << 8 >> 8;
	}
	inline long getGaia() const { return d.gaia_id; }
	inline int getComponentIds() const
	{
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
static_assert(sizeof(Star1) == 38, "Size of Star1 must be 38 bytes");
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
	inline int getInternalMag(int mag_steps, int mag_min, int mag_range) const { return mag_steps * (d.vmag / 1000.f - 0.001f*mag_min) / (0.001f*mag_range); }

	enum {MaxPosVal=((1<<19)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star2> *a, const SpecialZoneData<Star2> *z) const;
	void getJ2000Pos(float dyr, Vec3f& pos) const
	{
		// conversion from mas to rad
		const double RA_rad = getX0() * MAS2RAD_SCALE;
		const double DE_rad = getX1() * MAS2RAD_SCALE;
		// conversion from mas/yr to rad/yr, so dra in rad
		const double dra = dyr * (getDx0() / 1000.f) / cos(DE_rad) * MAS2RAD_SCALE;
		// getDx1 already in mas/yr, so ddec in rad
		const double ddec = dyr * (getDx1() / 1000.f) * MAS2RAD_SCALE;
		StelUtils::spheToRect(RA_rad - dra, DE_rad - ddec, pos);
	}
	inline long getGaia() const { return d.gaia_id; }
	float getBV(void) const {return static_cast<float>(d.b_v) / 1000.f;}
	QString getNameI18n(void) const {return QString();}
	QString getScreenNameI18n(void) const {return QString();}
	QString getDesignation(void) const {return QString();}
	int hasComponentID(void) const {return 0;}
	bool hasName() const {return getGaia() != -1;}
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
	inline int getInternalMag(int mag_steps, int mag_min, int mag_range) const { return mag_steps * (getMag() / 1000.f - 0.001f*mag_min) / (0.001f*mag_range); }

	enum {MaxPosVal=((1<<17)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star3> *a, const SpecialZoneData<Star3> *z) const;
	void getJ2000Pos(float dyr, Vec3f& pos) const
	{
		Q_UNUSED(dyr);  // they don't have proper motion
		// conversion from mas to rad
		const double RA_rad = getX0() * MAS2RAD_SCALE;
		const double DE_rad = getX1() * MAS2RAD_SCALE;
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
