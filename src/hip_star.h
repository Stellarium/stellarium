/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _HIP_STAR_H_
#define _HIP_STAR_H_

#include <functional>
#include <algorithm>

#include "StelObjectBase.hpp"
#include "stellarium.h"
#include "StelUtils.hpp"
#include "SFont.hpp"
#include "ToneReproducer.hpp"
#include "Projector.hpp"
#include "navigator.h"


class HipStar : public StelObjectBase
{
	friend class HipStarMgr;
	friend class Constellation;  // for xyz coordinate

public:
    HipStar(void);
    int read(FILE * pFile);	// Read the star data in the stream
///	bool readSAO(char *record, float maxmag);
///	bool readHP(char *record, float maxmag);
    void draw(const Vec3d &XY);		// Draw the star
	void draw_point(const Vec3d &XY);	// Draw the star as a point
    bool draw_name(const Vec3d &XY);	// draw the name - returns true if something drawn (and texture re-assigned for fonts)
	Vec3f get_RGB(void) const;
	wstring getNameI18n(void) const;
	string getEnglishName(void) const;
	wstring getInfoString(const Navigator * nav = NULL) const;
	wstring getShortInfoString(const Navigator * nav = NULL) const;
	STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_STAR;}
	Vec3d get_earth_equ_pos(const Navigator * nav = NULL) const {return nav->j2000_to_earth_equ(XYZ);}
	Vec3d getObsJ2000Pos(const Navigator *nav=0) const {return XYZ;}
///	double get_best_fov(const Navigator * nav = NULL) const {return (13.f - 2.f * Mag > 1.f) ? (13.f - Mag) : 1.f;}
	float get_mag(const Navigator * nav = NULL) const {return Mag;}
    char getSpectralType(void) const;
	unsigned int get_hp_number() { return HP; };
///    void draw_chart(const Vec3d &XY);
	void set_label_color(Vec3f& v) {label_color = v;}
	void set_circle_color(Vec3f& v) {circle_color = v;}
private:
    unsigned int HP;		// Hipparcos number
///    unsigned int HD;		// HD number
///    unsigned int SAO;		// SAO number
    float Mag;				// Apparent magnitude
    bool doubleStar;		// double star flag
    bool variableStar;		// not implemented yet
    Vec3f XYZ;				// Cartesian position
///    int ChartColorIndex;
	float term1;			// Optimization term

	string englishCommonName;	// English common Name of the star
    wstring commonNameI18;		// Common Name of the star
    wstring sciName;			// Scientific name incl constellation			
    unsigned char type;			// Spectral type coded as number in [0..12]
	float Distance;         // Distance from Earth in light years

	static float twinkle_amount;
	static float star_scale;
	static float star_mag_scale;
	static float names_brightness;
	static ToneReproducer* eye;
	static Projector* proj;
	static bool gravity_label;
	static Vec3f circle_color, label_color;
	static Vec3f ChartColors[20];
	static SFont *starFont;

	static bool flagSciNames;
};

struct HipStarMagComparer : public std::binary_function<HipStar*,HipStar*,bool> {
     const bool operator()( HipStar* const & x, HipStar* const & y) const {
       return x->get_mag() >= y->get_mag(); 
     }
};



#endif // _STAR_H_
