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

#ifndef _CONSTELLATION_H_
#define _CONSTELLATION_H_

#include "hip_star_mgr.h"
#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"
#include "fader.h"

class Constellation
{
    friend class Constellation_mgr;
public:
    Constellation();
    ~Constellation();
    bool read(const string& record, Hip_Star_mgr * _VouteCeleste);
    void draw(Projector* prj) const;
    void draw_name(s_font * constfont, Projector* prj) const;
    void draw_art(Projector* prj, navigator* nav) const;
    const Constellation* is_star_in(const Hip_Star *) const;
    string getName(void) { return name; };
    string getShortName(void) { return short_name; };
    static void set_line_color(const Vec3f& c) { line_color = c; }
    static void set_label_color(const Vec3f& c) { label_color = c; }
private:
    void draw_optim(Projector* prj) const;
    void draw_art_optim(Projector* prj, navigator* nav) const;
    void set_name(string _name) { name = _name; }
	void update(int delta_time);
	void set_flag_lines(bool b) {line_fader=b;}
	void set_flag_name(bool b) {name_fader=b;}
	void set_flag_art(bool b) {art_fader=b;}
	bool get_flag_lines(void) {return line_fader;}
	bool get_flag_name(void) {return name_fader;}
	bool get_flag_art(void) {return art_fader;}
	
    string name;
    char short_name[4];
    Vec3f XYZname;
	Vec3d XYname;
    unsigned int nb_segments;
    Hip_Star ** asterism;
	static bool gravity_label;
	s_texture* art_tex;
	Vec3d art_vertex[9];
	linear_fader art_fader, line_fader, name_fader;
	static Vec3f line_color, label_color;
};

#endif // _CONSTELLATION_H_
