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

#ifndef _HIP_STAR_MGR_H_
#define _HIP_STAR_MGR_H_

#include <map>
#include "hip_star.h"
#include "grid.h"
        
using namespace std ;

class Hip_Star_mgr  
{
public:
    Hip_Star_mgr();
    virtual ~Hip_Star_mgr();
    int Read(FILE *);					// Used by Load
    void Draw(void);                    // Draw all the stars
    void Save(void);                    // Debug function
    // Load all the stars from the files
    void Load(char * hipCatFile, char * commonNameFile, char * nameFile);         
    int Rechercher(vec3_t Pos);         // Search the star by position
	Hip_Star * Rechercher(unsigned int);// Search the star by HP number
    // Return data about the selected star
    void InfoSelect(vec3_t & XYZ_t, float & Ra,float & De,float & Mag,
    	char * & Name,unsigned int & HP, vec3_t & RGB, char * & CommonName) 
    {   
        XYZ_t=(*Selectionnee).XYZ;
        double RA, DE;
        XYZ_to_RADE(RA, DE, (*Selectionnee).XYZ);
        Ra=(float)RA; De=(float)DE;
        Mag=(*Selectionnee).Mag;
        Name=(*Selectionnee).Name;
        HP=(*Selectionnee).HP;
        RGB=(*Selectionnee).RGB;
        CommonName=(*Selectionnee).CommonName;
    }
private:
    multimap<int, Hip_Star*> Liste;         // map of star* with the grid id as key
    Grid HipGrid;                           // Grid for opimisation
    Hip_Star * Selectionnee;                // Selected star
    Hip_Star ** StarArray;  // The simple array of the star for sequential research
    int StarArraySize;      // Number of star in the array
};


extern Hip_Star_mgr * HipVouteCeleste;           // Class to manage all the stars from Hipparcos catalog


#endif // _STAR_MGR_H_
