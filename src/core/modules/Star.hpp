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

class StelObject;

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

#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(1)
#elif defined(_MSC_VER)
#pragma pack(push, 1)
#endif
struct Star1 { // 28 byte
#ifdef _MSC_VER
	unsigned int hip:24;		// 24 bits needed
	unsigned int componentIds:8;	//  8 bits needed
#else
	int hip:24;			// 24 bits needed
	unsigned char componentIds;	//  8 bits needed
#endif
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
	float getBV(void) const {return IndexToBV(bV);}
	bool hasName() const {return hip;}
	QString getNameI18n(void) const;
	int hasComponentID(void) const;
	void repack(bool fromBe);
	void print(void);
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


#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(1)
#elif defined(_MSC_VER)
#pragma pack(push, 1)
#endif
struct Star2 {  // 10 byte
	int x0:20;		// 20 bits needed
	int x1:20;		// 20 bits needed
	int dx0:14;		// 14 bits needed
	int dx1:14;		// 14 bits needed
	unsigned int bV:7;	//  7 bits needed
	unsigned int mag:5;	//  5 bits needed
	enum {MaxPosVal=((1<<19)-1)};
	StelObjectP createStelObject(const SpecialZoneArray<Star2> *a, const SpecialZoneData<Star2> *z) const;
	void getJ2000Pos(const ZoneData *z,float movementFactor, Vec3f& pos) const
	{
		pos = z->axis0;
		pos*=((float)(x0)+movementFactor*dx0);
		pos+=((float)(x1)+movementFactor*dx1)*z->axis1;
		pos+=z->center;
	}
	float getBV(void) const {return IndexToBV(bV);}
	QString getNameI18n(void) const {return QString();}
	int hasComponentID(void) const {return 0;}
	bool hasName() const {return false;}
	void repack(bool fromBe);
	void print(void);
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
