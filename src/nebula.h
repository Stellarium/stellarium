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

#ifndef _NEBULA_H_
#define _NEBULA_H_

#include "stellarium.h"
#include "projector.h"
#include "s_texture.h"
#include "stel_object.h"
#include "s_font.h"
#include "tone_reproductor.h"

/*   Gx    Galaxy
     OC    Open star cluster
     Gb    Globular star cluster, usually in the Milky Way Galaxy
     Nb    Bright emission or reflection nebula
     Pl    Planetary nebula
     C+N   Cluster associated with nebulosity
     Ast   Asterism or group of a few stars
     Kt    Knot  or  nebulous  region  in  an  external galaxy
     ***   Triple star
     D*    Double star
     *     Single star
     ?     Uncertain type or may not exist
     blank Unidentified at the place given, or type unknown
     -     Object called nonexistent in the RNGC (Sulentic and Tifft 1973)
     PD    Photographic plate defect */

	enum nebula_type { NEB_N, NEB_SG, NEB_PN, NEB_LG, NEB_EG, NEB_OC, NEB_GC, NEB_DN, NEB_IG, NEB_UNKNOWN };

class Nebula : public stel_object
{
friend class Nebula_mgr;
public:
    Nebula();
    virtual ~Nebula();

	virtual void get_info_string(char *, const navigator * nav = NULL) const;
	virtual void get_short_info_string(char *, const navigator * nav = NULL) const;
	virtual STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_NEBULA;}
	virtual Vec3d get_earth_equ_pos(const navigator * nav = NULL) const {return nav->prec_earth_equ_to_earth_equ(XYZ);}
	virtual double get_close_fov(const navigator * nav = NULL) const;
	virtual float get_mag(const navigator * nav = NULL) const {return mag;}

	void set_label_color(Vec3f& v) {label_color = v;}
	void set_circle_color(Vec3f& v) {circle_color = v;}

	// Read the Nebula data from a file
    bool read(const string&);

	void draw_tex(const Projector* prj, tone_reproductor* eye, bool bright_nebulae);
    void draw_name(int hint_ON, const Projector* prj);
    void draw_circle(const Projector* prj, const navigator * nav);
    string get_name() { return name; };

protected:
	// Return the radius of a circle containing the object on screen
	virtual float get_on_screen_size(const Projector* prj, const navigator * nav = NULL);

private:
	unsigned int NGC_nb;			// NGC catalog number
	string name;					// Nebula name
	string longname;				// Tony Nebula name & description
	string credit;					// Nebula image credit
	float mag;						// Apparent magnitude
	float angular_size;				// Angular size in radians
	Vec3f XYZ;						// Cartesian equatorial position
	Vec3d XY;						// Store temporary 2D position
	char type[4];					// Nebulae type
	nebula_type nType;
	string typeDesc;

	s_texture * neb_tex;			// Texture
	Vec3f tex_quad_vertex[4];		// The 4 vertex used to draw the nebula texture
	float luminance;				// Object luminance to use (value computed to compensate
									// the texture avergae luminosity)
	float tex_avg_luminance;        // avg luminance of the texture (saved here for performance)
	float inc_lum;					// Local counter for symbol animation
	static s_texture * tex_circle;	// The symbolic circle texture
	static s_font* nebula_font;		// Font used for names printing
	static float hints_brightness;
	static bool gravity_label;

	static Vec3f label_color, circle_color;
};

#endif // _NEBULA_H_
