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

#ifndef _GRID_H_
#define _GRID_H_

#include "stellarium.h"

class Grid  
{
public:
    Grid();
    virtual ~Grid();
    int GetNearest(vec3_t);
	void Draw(void);
	int Intersect(vec3_t pos, float fieldAngle, int * &result);
private:
    float Angle;     // Radius of each zone (in radians)
    int NbPoints;    // Number of zones
    vec3_t * Points; // The zones positions
};

#endif // _GRID_H_
