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
	// componentIds		 8 bits
	// hip				24 bits
	Uint32 hip_componentIds;

	Int32 x0;			// 32 bits needed
	Int32 x1;			// 32 bits needed
	unsigned char bV;		//  8 bits needed
	unsigned char mag;		//  8 bits needed
	Uint16 spInt;			// 16 bits needed
	Int32 dx0,dx1,plx;		// 32 bits needed (x3)
	enum {MaxPosVal=0x7FFFFFFF};
	StelObjectP createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const;
	void getJ2000Pos(const ZoneData *z,float movementFactor, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=((float)(x0)+movementFactor*dx0);
		pos+=((float)(x1)+movementFactor*dx1)*z->axis1;
		pos+=z->center;
	}
	inline int getBVIndex() const {return bV;}
	inline int getMag() const {return mag;}
	inline int getHip() const {
		return qFromLittleEndian(hip_componentIds) & 0x00FFFFFF;
	}
	inline int getComponentIds() const {
		return qFromLittleEndian(hip_componentIds) >> 24;
	}
	float getBV(void) const {return IndexToBV(bV);}
	bool hasName() const {return getHip();}
	QString getNameI18n(void) const;
	int hasComponentID(void) const;
	void repack(bool fromBe);
	void print(void);
};

struct Star2 {  // 10 byte

	/*
	The original fields, all packed into 'd'.
	int x0:20;
	int x1:20;
	int dx0:14;
	int dx1:14;
	unsigned int bV:7;
	unsigned int mag:5;
	*/

	Uint8 d[10];

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
	void repack(bool fromBe);
	void print(void);
};

#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(1)
#elif defined(_MSC_VER)
#pragma pack(push, 1)
#endif
struct Star3 {  // 6 byte
	int x0:18;		// 18 bits needed
	int x1:18;		// 18 bits needed
	unsigned int bV:7;	//  7 bits needed
	unsigned int mag:5;	//  5 bits needed
	int getBVIndex() const {return bV;}
	int getMag() const {return mag;}
	enum {MaxPosVal=((1<<17)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star3> *a, const SpecialZoneData<Star3> *z) const;
	void getJ2000Pos(const ZoneData *z,float, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=(float)(x0);
		pos+=z->center;
		pos+=(float)(x1)*z->axis1;
	}
	float getBV() const {return IndexToBV(bV);}
	QString getNameI18n() const {return QString();}
	int hasComponentID() const {return 0;}
	bool hasName() const {return false;}
	void repack(bool fromBe);
	void print();
}
#if defined(__GNUC__)
	__attribute__ ((__packed__))
#endif
;
#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(0)
#elif defined(_MSC_VER)
#pragma pack(pop)
#endif

#endif // _STAR_HPP_
