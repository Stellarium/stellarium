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

#ifndef _NEBULA_MGR_H_
#define _NEBULA_MGR_H_

#include <vector>
#include "nebula.h"

using namespace std;

class Nebula_mgr  
{
public:
    Nebula_mgr();
    virtual ~Nebula_mgr();
    int Read(char * fileName);  
    void Draw();            // Draw all the Nebulaes
    int ReadTexture();
    int Rechercher(vec3_t Pos);             // Search the Nebulae by position
    // Return data about the selected star
    void InfoSelect(vec3_t & XYZ_t, float & Ra,float & De,float & Mag,char * & Name,unsigned int & MessierNum,unsigned int & NGCNum, float & size) 
    {   XYZ_t=(*Selectionnee).XYZ;
        Ra=(*Selectionnee).RaRad;
        De=(*Selectionnee).DecRad;
        Mag=(*Selectionnee).Mag;
        Name=(*Selectionnee).Name;
        MessierNum=(*Selectionnee).Messier;
        NGCNum=(*Selectionnee).NGC;
        size=((*Selectionnee).Taille);
    }
private:
    vector<Nebula*> Liste;              // list of Nebulaes*
    Nebula * Selectionnee;              // Selected nebulae
};

extern Nebula_mgr * messiers;             // Class to manage the messier objects

#endif // _NEBULA_MGR_H_
