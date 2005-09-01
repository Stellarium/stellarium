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

#ifndef _CONSTELLATION_MGR_H_
#define _CONSTELLATION_MGR_H_

#include <vector>
#include "constellation.h"
#include "fader.h"
#include "loadingbar.h"

class Constellation_mgr  
{
public:
    Constellation_mgr(Hip_Star_mgr *_hip_stars);
    virtual ~Constellation_mgr();
    void draw(Projector* prj, navigator* nav) const;
	void update(int delta_time);
	void load_names(const string& names_file);
	void load_lines_and_art(const string& lines_file, const string& art_file, LoadingBar& lb);
	void set_art_fade_duration(float duration);
	void set_art_intensity(float _max);
	void set_flag_art(bool b);
	void set_flag_lines(bool b);
	void set_flag_names(bool b);
	void set_flag_isolate_selected(bool s) { isolateSelected = s; set_selected_const(selected);}
	// TODO : all the asterisms.empty() will be removed
	bool get_flag_art(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_art() || (selected && selected->get_flag_art()));}
	bool get_flag_lines(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_lines() || (selected && selected->get_flag_lines()));}
	bool get_flag_names(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_name() || (selected && selected->get_flag_name()));}
	bool get_flag_isolate_selected(void) { return isolateSelected;}

	void set_flag_gravity_label(bool g) {Constellation::gravity_label = g;}
	void set_lines_color(const Vec3f& c) {lines_color=c;}
	void set_names_color(const Vec3f& c) {names_color=c;}
	void set_font(const string& _font_filename);
	void set_selected(const string& shortname) {set_selected_const(find_from_short_name(shortname));}
	void set_selected(const Hip_Star * s) {if (!s) set_selected_const(NULL); else set_selected_const(is_star_in(s));}
	unsigned int get_first_selected_HP(void) {if (selected != NULL) return selected->asterism[0]->get_hp_number(); else return 0;}  //Tony
	vector<string> getNames(void);
	vector<string> getShortNames(void);
private:
	void draw_lines(Projector * prj) const;
	void draw_art(Projector * prj, navigator * nav) const;
	void draw_names(Projector * prj) const;
	void set_selected_const(Constellation* c);
    Constellation* is_star_in(const Hip_Star *) const;
    Constellation* find_from_short_name(const string& shortname) const;		
    vector<Constellation*> asterisms;
    s_font * asterFont;
    Vec3f lines_color, names_color;
    Hip_Star_mgr * hipStarMgr;
	Constellation* selected;
	bool isolateSelected;     // Tony - flag for isolating the constellations 
};

#endif // _CONSTELLATION_MGR_H_
