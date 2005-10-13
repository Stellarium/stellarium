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

#ifndef _NEBULA_MGR_H_
#define _NEBULA_MGR_H_

#include <vector>
#include "nebula.h"
#include "s_font.h"
#include "loadingbar.h"
#include "fader.h"

using namespace std;

class Nebula_mgr  
{
public:
	Nebula_mgr();
	virtual ~Nebula_mgr();
	
	// Read the Nebulas data from a file
	bool read(float font_size, const string& font_name, const string& fileName, LoadingBar& lb);
	
	// Draw all the Nebulas
	void draw(int hint_ON,Projector *prj, const navigator *nav, tone_reproductor *eye, bool draw_tex, // Tony
		bool _gravity_label, float max_mag_name, bool bright_nebulae); 
	
	stel_object *search(const string& name);  // search by name M83, NGC 1123, IC 1234
	stel_object *search(Vec3f Pos);    // Search the Nebulae by position
	
	void set_label_color(const Vec3f& c) {Nebula::label_color = c;}
	void set_circle_color(const Vec3f& c) {Nebula::circle_color = c;}
	void set_nebula_scale(float scale) {Nebula::set_nebula_scale(scale);} 	
	void update(int delta_time) {hints_fader.update(delta_time);}
	void set_hints_fade_duration(float duration) {hints_fader.set_duration((int) (duration * 1000.f));}
	void set_flag_hints(bool b) {hints_fader=b;}
	bool get_flag_hints() {return hints_fader;}

	// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
	vector<stel_object*> search_around(Vec3d v, double lim_fov);
	void set_nebulaname_format(int format) { Nebula::set_nebulaname_format(format); }
	int get_nebulaname_format(void) { return Nebula::get_nebulaname_format(); }
private:
	stel_object *searchNGC(unsigned int NGC);
	stel_object *searchIC(unsigned int NGC);
	stel_object *searchMessier(unsigned int M);
	bool read_NGC_catalog(const string& fileName, LoadingBar& lb);
	bool read_messier_textures(const string& fileName, LoadingBar& lb);

	FILE *nebula_fic;
	vector<Nebula*> neb_array;	// The nebulas list
	linear_fader hints_fader;
};

#endif // _NEBULA_MGR_H_
