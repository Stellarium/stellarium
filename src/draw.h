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

#ifndef __DRAW_H__
#define __DRAW_H__

#include <string>
#include <fstream>
#include "stellarium.h"
#include "s_font.h"
#include "projector.h"
#include "navigator.h"
#include "tone_reproductor.h"
#include "fader.h"

enum SKY_GRID_TYPE
{
	EQUATORIAL,
	ALTAZIMUTAL
};

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(SKY_GRID_TYPE grid_type = EQUATORIAL, unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 17,
	 double _radius = 1., unsigned int _nb_alt_segment = 18, unsigned int _nb_azi_segment = 50);
    virtual ~SkyGrid();
	void draw(const Projector* prj) const;
	void set_font(float font_size, const string& font_name);
	void set_color(const Vec3f& c) {color = c;}
	void update(int delta_time) {fader.update(delta_time);}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void show(bool b){fader = b;}
	void set_top_transparancy(bool b) { transparent_top= b; }
private:
	unsigned int nb_meridian;
	unsigned int nb_parallel;
	double radius;
	unsigned int nb_alt_segment;
	unsigned int nb_azi_segment;
	bool transparent_top;
	Vec3f color;
	Vec3f** alt_points;
	Vec3f** azi_points;
	bool (Projector::*proj_func)(const Vec3d&, Vec3d&) const;
	s_font* font;
	SKY_GRID_TYPE gtype;
	linear_fader fader;
};


enum SKY_LINE_TYPE
{
	EQUATOR,
	ECLIPTIC,
	LOCAL,
	MERIDIAN
};

// Class which manages a line to display around the sky like the ecliptic line
class SkyLine
{
public:
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR, double _radius = 1., unsigned int _nb_segment = 48);
    virtual ~SkyLine();
	void draw(const Projector* prj) const;
	void set_color(const Vec3f& c) {color = c;}
	void update(int delta_time) {fader.update(delta_time);}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void show(bool b){fader = b;}
	void set_font(float font_size, const string& font_name);

private:
	double radius;
	unsigned int nb_segment;
	SKY_LINE_TYPE line_type;
	Vec3f color;
	Vec3f* points;
	bool (Projector::*proj_func)(const Vec3d&, Vec3d&) const;
	linear_fader fader;
	s_font * font;
};

// Class which manages the cardinal points displaying
class Cardinals
{
public:
	Cardinals(float _radius = 1.);
    virtual ~Cardinals();
	void draw(const Projector* prj, double latitude, bool gravityON = false) const;
	void set_color(const Vec3f& c) {color = c;}
	void set_font(float font_size, const string& font_name);
	int load_labels(string filename);  // for i18n
	void update(int delta_time) {fader.update(delta_time);}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void set_flag_show(bool b){fader = b;}
	bool get_flag_show(){return fader;}

private:
	float radius;
	s_font* font;	
	Vec3f color;
	string sNorth, sSouth, sEast, sWest;
	linear_fader fader;
};

// Class which manages the displaying of the Milky Way
class MilkyWay
{
public:
	MilkyWay(float _radius = 1.);
    virtual ~MilkyWay();
	void draw(tone_reproductor * eye, const Projector* prj, const navigator* nav) const;
	void set_intensity(float _intensity);
	float get_intensity() { return intensity; };
	void set_texture(const string& tex_file, bool blend = false);
private:
	float radius;
	s_texture* tex;
	Vec3f color;
	float intensity;
	float tex_avg_luminance;
};

void DrawPoint(float X,float Y,float Z);


#endif // __DRAW_H__
