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

#ifndef _PLANET_H_
#define _PLANET_H_

#include "AstroOps.h"
#include "PlanetData.h"
#include "DateOps.h"
#include "stellarium.h"
#include "s_utility.h"
#include "draw.h"

#define DIST_PLANET 500

class Planet  
{
friend class Planet_mgr;
public:
    Planet(PlanetNb LeNum);
    virtual ~Planet();
    void Compute(double date, ObsInfo lieu, vec3_t helioPosObs);
    void Draw(vec3_t coordLight);
    void DrawSpecialDaylight(vec3_t coordLight);
private:
    void testDistance(vec3_t Pos,float &anglePlusProche, Planet * &plusProche);
    PlanetNb num;
    char * Name;
    double Rayon;
    double RayonOrbite;
    double Distance_to_obs;
    vec3_t GeoCoord , HelioCoord , Colour;  // Coords in UA
    double RaRad, DecRad;
    s_texture * planetTexture;
};

#endif // _PLANET_H_
