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

#ifndef _STAR_MGR_H_
#define _STAR_MGR_H_

#include <vector.h>
#include "star.h"

using namespace std ;

class Star_mgr  
{
public:
    Star_mgr();
    virtual ~Star_mgr();
    int Read(FILE *);					// Used by Load
    void Draw(void);                    // Draw all the stars
    void Load(char * filename);         // Load all the stars from the file
    int Rechercher(vec3_t Pos);         // Search the star by position
    Star * Rechercher(unsigned int);    // Search the star by HR number
    // Return data about the selected star
    void InfoSelect(vec3_t & XYZ_t, float & Ra,float & De,float & Mag,char * & Name,unsigned int & HR, vec3_t & RGB, char * & CommonName) 
    {   XYZ_t=(*Selectionnee).XYZ;
        Ra=(*Selectionnee).RaRad;
        De=(*Selectionnee).DecRad;
        Mag=(*Selectionnee).Mag;
        Name=(*Selectionnee).Name;
        HR=(*Selectionnee).HR;
        RGB=(*Selectionnee).RGB;
        CommonName=((*Selectionnee).CommonName);//.c_str();
    }
private:
    vector<Star*> Liste;                // list of star*
    Star * Selectionnee;                // Selected star
};


extern Star_mgr * VouteCeleste;           // Class to manage all the stars from HR catalog


#endif // _STAR_MGR_H_
