/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"
#include "stel_object.h"
#include "tone_reproductor.h"
#include "projector.h"

class Hip_Star : public stel_object
{
friend class Hip_Star_mgr;
friend class Constellation;

public:
    Hip_Star();
    virtual ~Hip_Star();
    int read(FILE * pFile);	// Read the star data in the stream
    void draw(void);		// Draw the star
	void draw_point(void);	// Draw the star as a point
    void draw_name(const s_font * star_font);
	virtual Vec3f get_RGB(void) const {return RGB;}
	virtual void get_info_string(char * s, const navigator * nav = NULL) const;
	virtual void get_short_info_string(char * s, const navigator * nav = NULL) const;
	virtual STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_STAR;}
	virtual Vec3d get_earth_equ_pos(const navigator* nav=NULL) const {return Vec3d(XYZ[0],XYZ[1],XYZ[2]);}
	virtual double get_best_fov(const navigator * nav = NULL) const {return (13.f - 2.f * Mag > 1.f) ? (13.f - Mag) : 1.f;}

private:
    unsigned int HP;		// Hipparcos number
    float Mag;				// Apparent magnitude
    Vec3f XYZ;				// Cartesian position
    Vec3f RGB;				// 3 RGB colour values
    float MaxColorValue;	// Precalc of the max color value
    Vec3d XY;				// 2D Position + z homogeneous value
    char SpType;			// Spectral type
    char * CommonName;		// Common Name of the star
    char * Name;			// Scientific name

	float term1;			// Optimization term

	static float twinkle_amount;
	static float star_scale;
	static float star_mag_scale;
	static tone_reproductor* eye;
	static Projector* proj;
	static bool gravity_label;
};


#endif // _STAR_H_
