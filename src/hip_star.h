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

#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"
#include "stel_object.h"
#include "tone_reproductor.h"
#include "projector.h"
#include "stel_utility.h"

class HipStar : public StelObject
{
	friend class HipStarMgr;
	friend class Constellation;  // for xyz coordinate

public:
    HipStar();
    virtual ~HipStar();
    int read(FILE * pFile);	// Read the star data in the stream
	bool readSAO(char *record, float maxmag);
	bool readHP(char *record, float maxmag);
    void draw(void);		// Draw the star
	void draw_point(void);	// Draw the star as a point
    bool draw_name(void);	// draw the name - returns true if something drawn (and texture re-assigned for fonts)
	virtual Vec3f get_RGB(void) const {return RGB;}
	virtual string get_info_string(const Navigator * nav = NULL) const;
	virtual string get_short_info_string(const Navigator * nav = NULL) const;
	virtual STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_STAR;}
	virtual Vec3d get_earth_equ_pos(const Navigator * nav = NULL) const {return nav->prec_earth_equ_to_earth_equ(XYZ);}
	virtual Vec3d get_prec_earth_equ_pos(void) const {return XYZ;}
	virtual double get_best_fov(const Navigator * nav = NULL) const {return (13.f - 2.f * Mag > 1.f) ? (13.f - Mag) : 1.f;}
	virtual float get_mag(const Navigator * nav = NULL) const {return Mag;}
	virtual unsigned int get_hp_number() { return HP; };
    void draw_chart(void);
	static void set_starname_format(int _format) {nameFormat = _format;}
	static int get_starname_format(void) {return nameFormat;}
	void set_label_color(Vec3f& v) {label_color = v;}
	void set_circle_color(Vec3f& v) {circle_color = v;}
private:
	void setColor(char sp);

    unsigned int HP;		// Hipparcos number
    unsigned int HD;		// HD number
    unsigned int SAO;		// SAO number
    float Mag;				// Apparent magnitude
    bool doubleStar;		// double star flag
    bool variableStar;		// not implemented yet
    Vec3f XYZ;				// Cartesian position
    Vec3f RGB;				// RGB colour values
    int ChartColorIndex;
    float MaxColorValue;	// Precalc of the max color value
    Vec3d XY;				// 2D Position + z homogeneous value
	float term1;			// Optimization term

    string CommonName;		// Common Name of the star
    string SciName;			// Scientific name incl constellation		
    string ShortSciName;	// Scientific name (no constellation)
    string OrigSciName;		// Scientific name (no greek)		
    char SpType;			// Spectral type code
	float Distance;         // Distance from Earth in light years

	static float twinkle_amount;
	static float star_scale;
	static float star_mag_scale;
	static float names_brightness;
	static ToneReproductor* eye;
	static Projector* proj;
	static bool gravity_label;
	static Vec3f circle_color, label_color;
	static int nameFormat;		// 0 - standard, 1 shortsciname, 2 sciname (incl constellation)
	static Vec3f ChartColors[20];
	static s_font *starFont;
};

struct Hip_Star_Mag_Comparer : public std::binary_function<HipStar*,HipStar*,bool> {
     const bool operator()( HipStar* const & x, HipStar* const & y) const {
       return x->get_mag() >= y->get_mag(); 
     }
};



#endif // _STAR_H_
