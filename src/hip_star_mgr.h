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

#include <vector>
#include "hip_star.h"
#include "grid.h"

using namespace std ;

class Hip_Star_mgr  
{
public:
    Hip_Star_mgr();
    virtual ~Hip_Star_mgr();
    int read(FILE *);						// Used by Load
    void draw(float star_scale, float twinkle_amount, int name_ON, float maxMagStarName,
		Vec3f equ_vision, tone_reproductor* _eye, Projector* prj);	// Draw all the stars
    void save(void);                    	// Debug function
    // Load all the stars from the files
    void load(char * font_fileName, char * hipCatFile, char * commonNameFile, char * nameFile);
    Hip_Star * search(vec3_t Pos);  	// Search the star by position
	Hip_Star * search(unsigned int);	// Search the star by HP number
private:
	vector<Hip_Star*>* starZones;       // array of star vector with the grid id as array rank
    Grid HipGrid;                       // Grid for opimisation
    Hip_Star ** StarArray;  // The simple array of the star for sequential research
    int StarArraySize;      // Number of star in the array
	s_texture * starTexture;
	s_font * starFont;
};


#endif // _STAR_MGR_H_
