/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chéreau
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

#ifndef _LANDSCAPE_H_
#define _LANDSCAPE_H_

#include <string>
#include "s_texture.h"
#include "vecmath.h"
#include "fmath.h"
#include "tone_reproductor.h"
#include "projector.h"
#include "navigator.h"

enum LANDSCAPE_TYPE
{
	OLD_STYLE,
	FISHEYE
};

// Class which manages the displaying of the Landscape
class Landscape
{
public:
	Landscape(float _radius = 1.);
    virtual ~Landscape();
	virtual void load(const string& file_name, const string& section_name) = 0;
	void set_parameters(const Vec3f& sun_pos);
	void set_sky_brightness(float b) {sky_brightness = b;}
	virtual void draw(tone_reproductor * eye, const Projector* prj, const navigator* nav,
		bool flag_fog, bool flag_decor, bool flag_ground) = 0;
	static Landscape* create_from_file(const string& landscape_file, const string& section_name);
	static string get_file_content(const string& landscape_file);
protected:
	float radius;
	string name;
	float sky_brightness;
	bool valid_landscape;   // was a landscape loaded properly?
};

typedef struct
{
	s_texture* tex;
	float tex_coords[4];
} landscape_tex_coord;

class Landscape_old_style : public Landscape
{
public:
	Landscape_old_style(float _radius = 1.);
    virtual ~Landscape_old_style();
	virtual void load(const string& fileName, const string& section_name);
	virtual void draw(tone_reproductor * eye, const Projector* prj, const navigator* nav,
		bool flag_fog, bool flag_decor, bool flag_ground);
private:
	void draw_fog(tone_reproductor * eye, const Projector* prj, const navigator* nav) const;
	void draw_decor(tone_reproductor * eye, const Projector* prj, const navigator* nav) const;
	void draw_ground(tone_reproductor * eye, const Projector* prj, const navigator* nav) const;
	s_texture** side_texs;
	int nb_side_texs;
	int nb_side;
	landscape_tex_coord* sides;
	s_texture* fog_tex;
	landscape_tex_coord fog_tex_coord;
	s_texture* ground_tex;
	landscape_tex_coord ground_tex_coord;
	int nb_decor_repeat;
	float fog_alt_angle;
	float fog_angle_shift;
	float decor_alt_angle;
	float decor_angle_shift;
	float decor_angle_rotatez;
	float ground_angle_shift;
	float ground_angle_rotatez;
	int draw_ground_first;
};

class Landscape_fisheye : public Landscape
{
public:
	Landscape_fisheye(float _radius = 1.);
    virtual ~Landscape_fisheye();
	virtual void load(const string& fileName, const string& section_name);
	virtual void draw(tone_reproductor * eye, const Projector* prj, const navigator* nav,
		bool flag_fog, bool flag_decor, bool flag_ground);
private:
	s_texture* map_tex;
	float tex_fov;
};

#endif // _LANDSCAPE_H_
