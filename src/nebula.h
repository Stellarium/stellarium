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

#ifndef _NEBULA_H_
#define _NEBULA_H_

#include "stellarium.h"
#include "stel_utility.h"
#include "s_texture.h"
#include "stel_object.h"
#include "s_font.h"

class Nebula : public stel_object
{
friend class Nebula_mgr;
public:
    Nebula();
    virtual ~Nebula();
	void get_info_string(char *, navigator * nav = NULL) const;
    int Read(FILE *);       // Read the Nebulae data in the stream
    void Draw();            // Draw the nebulae
    void DrawName(s_font* nebulaFont);
    void DrawCircle(draw_utility * du);
	unsigned char get_type(void) const {return STEL_OBJECT_NEBULA;}
	Vec3d get_earth_equ_pos(navigator * nav = NULL) const {return Vec3d(XYZ[0],XYZ[1],XYZ[2]);}
	// Return the radius of a circle containing the object on screen
	virtual float get_on_screen_size(navigator * nav, draw_utility * du);
private:
	static s_texture * texCircle;
	short posDash;
	float incLum;
	unsigned int Messier;	// Messier catalog number
	unsigned int NGC;		// NGC catalog number
	char Name[40];          // Nebulae Name
	float Mag;              // Apparent magnitude
	Vec3f XYZ;             // Cartesian position
	char Type[4];           // Nebulae type
	Vec3d XY;           // 2D Position
	double RaRad;            // Right Ascention in radians
	double DecRad;           // Declinaison in radians
	float * matTransfo;     // Transformation matrix used to draw the nebulae
	float Rotation;         // Rotation pour la mettre dans le bon sens...
	float Taille;           // Taille en minute d'arc
	char Constellation[5];  // Constellation
	s_texture * nebTexture; // Texture
	float RayonPrecalc;     // Taille Précalculée

};

#endif // _NEBULA_H_
