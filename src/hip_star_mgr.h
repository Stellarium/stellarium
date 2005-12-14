/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

class HipStarMgr  
{
public:
    HipStarMgr();
    virtual ~HipStarMgr();
	void init(float font_size, const string& font_name, const string& hipCatFile, const string& commonNameFile, const string& sciNameFile, LoadingBar& lb);
	void update(int delta_time) {names_fader.update(delta_time);}
	void set_names_fade_duration(float duration) {names_fader.set_duration((int) (duration * 1000.f));}
	int load_common_names(const string& commonNameFile);
	void load_sci_names(const string& sciNameFile);
    void draw(float star_scale, float star_mag_scale, float twinkle_amount, float maxMagStarName,
		Vec3f equ_vision, ToneReproductor* _eye, Projector* prj, bool _gravity_label);	// Draw all the stars
    void draw_point(float star_scale, float star_mag_scale, float twinkle_amount, float maxMagStarName,
		Vec3f equ_vision, ToneReproductor* _eye, Projector* prj, bool _gravity_label);	// Draw all the stars as points
    void save(void);                    	// Debug function
	void set_flag_names(bool b) {names_fader=b;}
	void set_limiting_mag(float _mag) {limiting_mag=_mag;}
	float get_limiting_mag() {return limiting_mag;}
    HipStar *search(Vec3f Pos);  		// Search the star by position
	HipStar *search(const string&);	// Search the star by string (incl catalog prefix)
	HipStar *searchHP(unsigned int);	// Search the star by HP number
	HipStar *searchSAO(unsigned int);	// Search the star by SAO number
	HipStar *searchHD(unsigned int);	// Search the star by HD number
	// Return a stl vector containing the stars located inside the lim_fov circle around position v
	vector<StelObject*> search_around(Vec3d v, double lim_fov);
	vector<string> getNames(void) { return lstCommonNames; }
	unsigned int getCommonNameHP(string _commonname);
	void set_starname_format(int format) { HipStar::set_starname_format(format); }
	int get_starname_format(void) { return HipStar::get_starname_format(); }
	void set_label_color(const Vec3f& c) {HipStar::label_color = c;}
	void set_circle_color(const Vec3f& c) {HipStar::circle_color = c;}
private:

	// Load all the stars from the files
	void load_data(const string& hipCatFile, LoadingBar& lb);

	vector<HipStar*>* starZones;       // array of star vector with the grid id as array rank
	Grid HipGrid;                       // Grid for opimisation
	HipStar * StarArray;  				// Sequential Array of the star for memory allocation optimization
	HipStar ** StarFlatArray; 			// The simple array of the star for sequential research
	int starArraySize;                  // Number of star in the array

	int CombinedTotal;
	s_texture * starTexture;
	s_texture *starcTexture;			// charted interior disc
	LinearFader names_fader;
	float limiting_mag;                  // limiting magnitude at 60 degree fov
	vector<string> lstCommonNames;
	vector<unsigned int> lstCommonNamesHP;
};


#endif // _STAR_MGR_H_
