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
    Constellation_mgr(string _data_dir, string _sky_culture, string _sky_locale, 
		Hip_Star_mgr *_hip_stars, string _font_filename, LoadingBar& lb, 
		Vec3f _lines_color, Vec3f _names_color);
    virtual ~Constellation_mgr();
    void draw(Projector* prj, navigator* nav) const;
    Constellation* is_star_in(const Hip_Star *) const;
    Constellation* find_from_short_name(const string& shortname) const;
	void set_sky_locale(const string& _sky_locale);
	void set_sky_culture(string _sky_culture, LoadingBar& lb);
	void set_art_fade_duration(float duration);
	void set_art_intensity(float _max);
	void update(int delta_time);
	void show_art(bool b);
	void show_lines(bool b);
	void show_names(bool b);
	void set_gravity_label(bool g) {Constellation::gravity_label = g;}
	void set_selected(Constellation* c);
private:
    void load_line_and_art(const string& catName, const string& artCatName, LoadingBar& lb);
	void draw_lines(Projector * prj) const;
	void draw_art(Projector * prj, navigator * nav) const;
	void draw_names(Projector * prj) const;
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
