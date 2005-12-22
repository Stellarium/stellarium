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

class ConstellationMgr  
{
public:
    ConstellationMgr(HipStarMgr *_hip_stars);
    ~ConstellationMgr();
    void draw(Projector* prj, Navigator* nav) const;
	void update(int delta_time);
	
	//! @brief Read constellation names from the given file
	//! @param namesFile Name of the file containing the constellation names in english
	void loadNames(const string& names_file);
	
	//! @brief Update i18 names from english names according to current locale
	//! The translation is done using gettext with translated strings defined in translations.h
	void translateNames();
					   
	void load_lines_and_art(const string& lines_file, const string& art_file, const string &boundaryfileName, LoadingBar& lb);
	
	void set_art_fade_duration(float duration);
	void set_art_intensity(float _max);
	void set_flag_art(bool b);
	void set_flag_lines(bool b);
	void set_flag_boundaries(bool b);
	void set_flag_names(bool b);
	void set_flag_isolate_selected(bool s) { isolateSelected = s; set_selected_const(selected);}
	// TODO : all the asterisms.empty() will be removed
	bool get_flag_art(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_art() || (selected && selected->get_flag_art()));}
	bool get_flag_lines(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_lines() || (selected && selected->get_flag_lines()));}
	bool get_flag_names(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_name() || (selected && selected->get_flag_name()));}
	bool get_flag_boundaries(void) {return (!asterisms.empty() && (*(asterisms.begin()))->get_flag_boundaries() || (selected && selected->get_flag_boundaries()));}
	bool get_flag_isolate_selected(void) { return isolateSelected;}

	void set_flag_gravity_label(bool g) {Constellation::gravity_label = g;}
	void set_line_color(const Vec3f& c) {Constellation::set_line_color(c);}
	Vec3f get_line_color() {return Constellation::get_line_color();}
	void set_label_color(const Vec3f& c) {Constellation::set_label_color(c);}
	Vec3f get_label_color() {return Constellation::get_label_color();}
	void set_boundary_color(const Vec3f& c) {Constellation::set_boundary_color(c);}
	Vec3f get_boundary_color() {return Constellation::get_boundary_color();}
	void set_font(float font_size, const string& font_name);
	void set_selected(const string& shortname) {set_selected_const(findFromAbbreviation(shortname));}
	void set_selected(const HipStar * s) {if (!s) set_selected_const(NULL); else set_selected_const(is_star_in(s));}
	unsigned int get_first_selected_HP(void) {if (selected != NULL) return selected->asterism[0]->get_hp_number(); else return 0;}  //Tony
	vector<string> getNames(void);
	vector<string> getShortNames(void);
	string get_short_name_by_name(string _name);  // return short name from long common name
	bool load_boundaries(const string& conCatFile);
	void draw_boundaries(Projector* prj) const;
private:
	void draw_lines(Projector * prj) const;
	void draw_art(Projector * prj, Navigator * nav) const;
	void draw_names(Projector * prj) const;
	void set_selected_const(Constellation* c);

    Constellation* is_star_in(const HipStar *) const;
    Constellation* findFromAbbreviation(const string& abbreviation) const;		
    vector<Constellation*> asterisms;
    s_font *asterFont;
    HipStarMgr *hipStarMgr;
	Constellation* selected;
	bool isolateSelected;
	vector<vector<Vec3f> *> allBoundarySegments;
};

#endif // _CONSTELLATION_MGR_H_
