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

#ifndef _STAR_HPP_
#define _STAR_HPP_

#include "ZoneData.hpp"
#include "StelObjectType.hpp"
#include <QString>
#include <QtEndian>

class StelObject;

typedef unsigned char Uint8;
typedef int Int32;
typedef unsigned int Uint32;
typedef short int Int16;
typedef unsigned short int Uint16;

template <class Star> class SpecialZoneArray;
template <class Star> struct SpecialZoneData;


// structs for storing the stars in binary form. The idea is
// to store much data for bright stars (Star1), but only little or even
// very little data for faints stars (Star3). Using only 6 bytes for Star3
// makes it feasable to store hundreds of millions of them in main memory.


static inline float IndexToBV(unsigned char bV)
{
	return (float)bV*(4.f/127.f)-0.5f;
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
	// Int32 x0             32
	// Int32 x1             32
	//
	// unsigned char bV     8
	// unsigned char mag    8
	// Uint16 spInt         16
	//
	// Int32 dx0,dx1,plx    32
private:
	Uint8 d[28];

public:
	enum {MaxPosVal=0x7FFFFFFF};
	StelObjectP createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const;
	void getJ2000Pos(const ZoneData *z,float movementFactor, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=((float)(getX0())+movementFactor*getDx0());
		pos+=((float)(getX1())+movementFactor*getDx1())*z->axis1;
		pos+=z->center;
	}
	inline int getBVIndex() const {return d[12];}
	inline int getMag() const {return d[13];}
	inline int getSpInt() const {return ((Uint16*)d)[7];}
	inline int getX0() const { return qFromLittleEndian(((Int32*)d)[1]); }
	inline int getX1() const { return qFromLittleEndian(((Int32*)d)[2]); }
	inline int getDx0() const {return qFromLittleEndian(((Int32*)d)[4]);}
	inline int getDx1() const {return qFromLittleEndian(((Int32*)d)[5]);}
	inline int getPlx() const {return qFromLittleEndian(((Int32*)d)[6]);}

	inline int getHip() const
	{
		Uint32 v = d[0] | d[1] << 8 | d[2] << 16;
		return ((Int32)v) << 8 >> 8;
	}

	inline int getComponentIds() const
	{
		return (Int32)d[3];
	}

	float getBV(void) const {return IndexToBV(getBVIndex());}
	bool hasName() const {return getHip();}
	QString getNameI18n(void) const;
	int hasComponentID(void) const;
	void print(void);
};


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
	Uint8 d[10];

public:
	inline int getX0() const
	{
		Uint32 v = d[0] | d[1] << 8 | (d[2] & 0xF) << 16;
		return ((Int32)v) << 12 >> 12;
	}

	inline int getX1() const
	{
		Uint32 v = d[2] >> 4 | d[3] << 4 | d[4] << 12;
		return ((Int32)v) << 12 >> 12;
	}

	inline int getDx0() const
	{
		Uint16 v = d[5] | (d[6] & 0x3F) << 8;
		return (Int16)v;
	}

	inline int getDx1() const
	{
		Uint16 v = d[6] >> 6 | d[7] << 2 | (d[8] & 0xF) << 10;
		return (Int16)v;
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
		pos*=((float)(getX0())+movementFactor*getDx0());
		pos+=((float)(getX1())+movementFactor*getDx1())*z->axis1;
		pos+=z->center;
	}
	float getBV(void) const {return IndexToBV(getBVIndex());}
	QString getNameI18n(void) const {return QString();}
	int hasComponentID(void) const {return 0;}
	bool hasName() const {return false;}
	void print(void);
};

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
	Uint8 d[6];

public:
	inline int getX0() const
	{
		Uint32 v = d[0] | d[1] << 8 | (d[2] & 0x3) << 16;
		return ((Int32)v) << 14 >> 14;
	}

	inline int getX1() const
	{
		Uint32 v = d[2] >> 2 | d[3] << 6 | (d[4] & 0xF) << 14;
		return ((Int32)v) << 14 >> 14;
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
		pos*=(float)(getX0());
		pos+=z->center;
		pos+=(float)(getX1())*z->axis1;
	}
	float getBV() const {return IndexToBV(getBVIndex());}
	QString getNameI18n() const {return QString();}
	int hasComponentID() const {return 0;}
	bool hasName() const {return false;}
	void print();
};

#endif // _STAR_HPP_
