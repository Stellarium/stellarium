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

#ifndef _PLANET_MGR_H_
#define _PLANET_MGR_H_

#include "planet.h"
#include "AstroOps.h"
#include "PlanetData.h"
#include "DateOps.h"

class Planet_mgr  
{
public:
    Planet_mgr();
    virtual ~Planet_mgr();
    void Compute(double date, ObsInfo lieu);    // Compute the position of the planets
    void Draw();                                // Draw the planets...  
    int Planet_mgr::Rechercher(vec3_t Pos);
	void loadTextures();
    // Return data about the selected planet
    void InfoSelect(vec3_t & XYZ_t, float & Ra,float & De, char * & Name, double & Distance, float & size) 
    {   vec3_t PosPlanet;
        RADE_to_XYZ(Selectionnee->RaRad,Selectionnee->DecRad, PosPlanet);
        XYZ_t=PosPlanet;
        Ra=(*Selectionnee).RaRad;
        De=(*Selectionnee).DecRad;
        Name=(*Selectionnee).Name;
        Distance=(*Selectionnee).Distance_to_obs;
        size=asin((*Selectionnee).Rayon*2/(*Selectionnee).Distance_to_obs)*180/PI;
    }
    void DrawMoonDaylight();
    vec3_t GetPos(int num);
private:
    Planet * Selectionnee;              // Selected planet
};

extern Planet_mgr * SolarSystem;      // Class to manage the planets

#endif // _PLANET_MGR_H_
