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

class Constellation_mgr  
{
public:
  //    Constellation_mgr(Vec3f lines_color = Vec3f(0.1, 0.15, 0.2), Vec3f lines_color = Vec3f(0.1,0.2,0.3));
    Constellation_mgr(string _data_dir, string _sky_culture, string _sky_locale, 
		Hip_Star_mgr *_hip_stars, string _font_filename, int barx, int bary, 
		Vec3f _lines_color, Vec3f _names_color);
    virtual ~Constellation_mgr();
    int set_sky_culture(string _sky_culture, const string& _font_fileName, int barx, int bary);
    void show_art(void);
    void hide_art(void);
    void draw(Projector* prj) const;
    // Draw one constellation of internationnal name Abr
    void draw(Projector* prj, char abr[4]) const;
    void draw_names(Projector* prj, bool gravity_label);
    void draw_one_name(Projector* prj, Constellation*, bool gravity_label) const;
    void draw_art(Projector* prj, navigator* nav, int delta_time);
    Constellation* is_star_in(const Hip_Star *) const;
    Constellation* find_from_short_name(const string& shortname) const;
    int  set_sky_locale(const string& _sky_locale);
    void set_art_fade_duration(float duration);
    void set_art_intensity(float intensity);

private:

    int load(const string& catName, const string& artCatName,
		Hip_Star_mgr * _VouteCeleste, const string& _font_fileName, int barx, int bary);

    vector<Constellation*> asterisms;
    s_font * asterFont;
    Vec3f lines_color, names_color;
    Hip_Star_mgr * hipStarMgr;
    string dataDir;
    string skyCulture;
    string skyLocale;
    bool JustLoaded;
};

#endif // _CONSTELLATION_MGR_H_
