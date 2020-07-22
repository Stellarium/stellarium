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
#include <QString>
#include <QtEndian>

class StelObject;

extern const QString STAR_TYPE;

typedef short int Int16;
typedef unsigned short int Uint16;

template <class Star> class SpecialZoneArray;
template <class Star> struct SpecialZoneData;


// structs for storing the stars in binary form. The idea is
// to store much data for bright stars (Star1), but only little or even
// very little data for faints stars (Star3). Using only 6 bytes for Star3
// makes it feasable to store hundreds of millions of them in main memory.


static inline float IndexToBV(int bV)
{
	return static_cast<float>(bV)*(4.f/127.f)-0.5f;
}


struct Star1 { // 28 byte
	/*
	          _______________
	0    hip |               |
	1        |               |
	2        |_______________|
	3   cIds |_______________|
	4     x0 |               |
	5        |               |
	6        |               |
	7        |_______________|
	8     x1 |               |
	9        |               |
	10       |               |
	11       |_______________|
	12    bV |_______________|
	13   mag |_______________|
	14 spInt |               |
	15       |_______________|
	16   dx0 |               |
	17       |               |
	18       |               |
	19       |_______________|
	20   dx1 |               |
	21       |               |
	22       |               |
	23       |_______________|
	24   plx |               |
	25       |               |
	26       |               |
	27       |_______________|

	*/

	// componentIds         8
	// hip                  24
	//
	// qint32 x0            32
	// qint32 x1            32
	//
	// unsigned char bV     8
	// unsigned char mag    8
	// Uint16 spInt         16
	//
	// qint32 dx0,dx1,plx   32
private:
	// Use an union so we can access the data as different types without
	// aliasing issues.
	union {
		quint8  uint8[28];
		quint16 uint16[14];
		qint32  int32[7];
	} d;

public:
	enum {MaxPosVal=0x7FFFFFFF};
	StelObjectP createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const;
	void getJ2000Pos(const ZoneData *z,float movementFactor, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=(static_cast<float>(getX0())+movementFactor*getDx0());
		pos+=(static_cast<float>(getX1())+movementFactor*getDx1())*z->axis1;
		pos+=z->center;
	}
	inline int getBVIndex() const {return d.uint8[12];}
	inline int getMag() const {return d.uint8[13];}
	inline int getSpInt() const {return d.uint16[7];}
	inline int getX0() const { return qFromLittleEndian(d.int32[1]);}
	inline int getX1() const { return qFromLittleEndian(d.int32[2]);}
	inline int getDx0() const {return qFromLittleEndian(d.int32[4]);}
	inline int getDx1() const {return qFromLittleEndian(d.int32[5]);}
	inline int getPlx() const {return qFromLittleEndian(d.int32[6]);}

	inline int getHip() const
	{
		quint32 v = d.uint8[0] | d.uint8[1] << 8 | d.uint8[2] << 16;
		return (static_cast<qint32>(v)) << 8 >> 8;
	}

	inline int getComponentIds() const
	{
		return d.uint8[3];
	}

	float getBV(void) const {return IndexToBV(getBVIndex());}
	bool hasName() const {return getHip();}
	QString getNameI18n(void) const;
	QString getDesignation(void) const;
	int hasComponentID(void) const;
	void print(void) const;
};
static_assert(sizeof(Star1) == 28, "Size of Star1 must be 28 bytes");

struct Star2 {  // 10 byte
	/*
	          _______________
	0     x0 |               |
	1        |_______        |
	2     x1 |       |_______|
	3        |               |
	4        |_______________|
	5    dx0 |___            |
	6    dx1 |   |___________|
	7        |_______        |
	8     bV |_______|_______|
	9    mag |_________|_____| bV

	int x0          :20;
	int x1          :20;
	int dx0         :14;
	int dx1         :14;
	unsigned int bV :7;
	unsigned int mag:5;
	*/

private:
	quint8 d[10];

public:
	inline int getX0() const
	{
		quint32 v = d[0] | d[1] << 8 | (d[2] & 0xF) << 16;
		return (static_cast<qint32>(v)) << 12 >> 12;
	}

	inline int getX1() const
	{
		quint32 v = d[2] >> 4 | d[3] << 4 | d[4] << 12;
		return (static_cast<qint32>(v)) << 12 >> 12;
	}

	inline int getDx0() const
	{
		Uint16 v = d[5] | (d[6] & 0x3F) << 8;
		return (static_cast<Int16>(v << 2)) >> 2;
	}

	inline int getDx1() const
	{
		Uint16 v = d[6] >> 6 | d[7] << 2 | (d[8] & 0xF) << 10;
		return (static_cast<Int16>(v << 2)) >> 2;
	}

	inline int getBVIndex() const
	{
		return d[8] >> 4 | (d[9] & 0x7) << 4;
	}

	inline int getMag() const
	{
		return d[9] >> 3;
	}

	enum {MaxPosVal=((1<<19)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star2> *a, const SpecialZoneData<Star2> *z) const;
	void getJ2000Pos(const ZoneData *z,float movementFactor, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=(static_cast<float>(getX0())+movementFactor*getDx0());
		pos+=(static_cast<float>(getX1())+movementFactor*getDx1())*z->axis1;
		pos+=z->center;
	}
	float getBV(void) const {return IndexToBV(getBVIndex());}
	QString getNameI18n(void) const {return QString();}
	QString getDesignation(void) const {return QString();}
	int hasComponentID(void) const {return 0;}
	bool hasName() const {return false;}
	void print(void) const;
};
static_assert(sizeof(Star2) == 10, "Size of Star2 must be 10 bytes");

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
	void getJ2000Pos(const ZoneData *z,float, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=static_cast<float>(getX0());
		pos+=z->center;
		pos+=static_cast<float>(getX1())*z->axis1;
	}
	float getBV() const {return IndexToBV(getBVIndex());}
	QString getNameI18n() const {return QString();}
	QString getDesignation() const {return QString();}
	int hasComponentID() const {return 0;}
	bool hasName() const {return false;}
	void print();
};
static_assert(sizeof(Star3) == 6, "Size of Star3 must be 6 bytes");

#endif // STAR_HPP
