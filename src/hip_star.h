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
#include "s_utility.h"

class Hip_Star  
{
friend class Hip_Star_mgr;

public:
    Hip_Star();
    virtual ~Hip_Star();
    int Read(FILE * pFile); // Read the star data in the stream
    void Draw();            // Draw the star
private:
    unsigned int HP;        // Hipparcos number
    float Mag;              // Apparent magnitude
    vec3_t XYZ;             // Cartesian position
    vec3_t RGB;             // 3 RGB colour values
    float rmag_t;           // Precalc of star size temporary
    float MaxColorValue;    // Precalc of the max color value
    double XY[2];           // 2D Position
    char SpType;            // Spectral type
};

#endif // _STAR_H_
