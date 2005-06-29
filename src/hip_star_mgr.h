/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�eau
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
#include <string>
#include "hip_star.h"
#include "grid.h"
#include "fader.h"
#include "loadingbar.h"

using namespace std ;

class Hip_Star_mgr  
{
public:
    Hip_Star_mgr();
    virtual ~Hip_Star_mgr();
	void init(const string& font_fileName, const string& hipCatFile, const string& commonNameFile, const string& sciNameFile, LoadingBar& lb);
	void update(int delta_time) {names_fader.update(delta_time);}
	void set_names_fade_duration(float duration) {names_fader.set_duration((int) (duration * 1000.f));}
	int load_common_names(const string& commonNameFile);
	void load_sci_names(const string& sciNameFile);
    void draw(float star_scale, float star_mag_scale, float twinkle_amount, float maxMagStarName,
		Vec3f equ_vision, tone_reproductor* _eye, Projector* prj, bool _gravity_label);	// Draw all the stars
    void draw_point(float star_scale, float star_mag_scale, float twinkle_amount, float maxMagStarName,
		Vec3f equ_vision, tone_reproductor* _eye, Projector* prj, bool _gravity_label);	// Draw all the stars as points
    void save(void);                    	// Debug function
	void set_flag_names(bool b) {names_fader=b;}
    Hip_Star * search(Vec3f Pos);  	// Search the star by position
	Hip_Star * search(unsigned int);	// Search the star by HP number
	// Return a stl vector containing the stars located inside the lim_fov circle around position v
	vector<stel_object*> search_around(Vec3d v, double lim_fov);

private:

	// Load all the stars from the files
	void load_data(const string& hipCatFile, LoadingBar& lb);

	vector<Hip_Star*>* starZones;       // array of star vector with the grid id as array rank
	Grid HipGrid;                       // Grid for opimisation
	Hip_Star * StarArray;  				// Sequential Array of the star for memory allocation optimization
	Hip_Star ** StarFlatArray; 			// The simple array of the star for sequential research
	int StarArraySize;      // Number of star in the array
	s_texture * starTexture;
	s_font * starFont;
	linear_fader names_fader;
};


#endif // _STAR_MGR_H_
