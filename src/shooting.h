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

#ifndef _SHOOTING_STAR_H_
#define _SHOOTING_STAR_H_

#include "stellarium.h"
#include "s_utility.h"
#include "s_texture.h"

class ShootingStar  
{
//friend class ShootingStar_mgr;

public:
    ShootingStar();
    virtual ~ShootingStar();
    void Draw();            // Draw the shootingstar
	int IsDead(void){return Dead;}
private:
    float Mag;              // Apparent magnitude
    vec3_t XYZ0;            // Cartesian initial position
	vec3_t XYZ1;			// Parametric coefs
	float coef;
    vec3_t RGB;             // 3 RGB colour values
    double XY[2];           // 2D Position
	s_texture *ShootTexture;// texture ID
	int Dead;
};

#endif // _SHOOTING_STAR_H_
