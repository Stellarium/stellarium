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
	void set_datadir(const string& s) {dataDir = s;}
	void set_sky_locale(const string& _sky_locale);
	void set_sky_culture(string _sky_culture, LoadingBar& lb);
	void set_art_fade_duration(float duration);
	void set_art_intensity(float _max);
	void set_flag_art(bool b);
	void set_flag_lines(bool b);
	void set_flag_names(bool b);
	bool get_flag_art(void) {return ((*(asterisms.begin()))->get_flag_art() || (selected && selected->get_flag_art()));}
	bool get_flag_lines(void) {return ((*(asterisms.begin()))->get_flag_lines() || (selected && selected->get_flag_lines()));}
	bool get_flag_names(void) {return ((*(asterisms.begin()))->get_flag_name() || (selected && selected->get_flag_name()));}
	void set_flag_gravity_label(bool g) {Constellation::gravity_label = g;}
	void set_lines_color(const Vec3f& c) {lines_color=c;}
	void set_names_color(const Vec3f& c) {names_color=c;}
	void set_font(const string& _font_filename);
	void set_selected(const string& shortname) {set_selected_const(find_from_short_name(shortname));}
	void set_selected(const Hip_Star * s) {if (!s) set_selected_const(NULL); else set_selected_const(is_star_in(s));}
private:
    void load_line_and_art(const string& catName, const string& artCatName, LoadingBar& lb);
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
    string dataDir;
    string skyCulture;
    string skyLocale;
	Constellation* selected;
};

#endif // _CONSTELLATION_MGR_H_
