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

class Hip_Star : public stel_object
{
friend class Hip_Star_mgr;
friend class constellation;

public:
    Hip_Star();
    virtual ~Hip_Star();
    int Read(FILE * pFile);	// Read the star data in the stream
    void Draw(draw_utility * du);		// Draw the star
    void DrawName(s_font * star_font);
	vec3_t get_RGB(void) const {return RGB;}
	void get_info_string(char * s) const;
	unsigned char get_type(void) const {return STEL_OBJECT_STAR;}
	Vec3d get_earth_equ_pos(navigator* nav=NULL) const {return Vec3d(XYZ[0],XYZ[1],XYZ[2]);}

private:
    unsigned int HP;        // Hipparcos number
    float Mag;              // Apparent magnitude
    vec3_t XYZ;             // Cartesian position
    vec3_t RGB;             // 3 RGB colour values
    float rmag_t;           // Precalc of star size temporary
    float MaxColorValue;    // Precalc of the max color value
    double XY[2];           // 2D Position
    char SpType;            // Spectral type
    char * CommonName;      // Common Name of the star
    char * Name;            // Scientific name
    
    unsigned short int typep; //temp
    unsigned short int magp;  //temp
    float r,d;

	static float twinkle_amount;
	static float star_scale;
};


#endif // _STAR_H_
