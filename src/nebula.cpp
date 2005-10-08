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
		
#include <iostream>	
#include "nebula.h"
#include "s_texture.h"
#include "stellarium.h"
#include "s_font.h"
#include "navigator.h"
#include "stel_utility.h"
#include "s_gui.h"

#define RADIUS_NEB 1.

s_texture * Nebula::tex_circle = NULL;
s_font* Nebula::nebula_font = NULL;
float Nebula::nebula_scale = 1.f;
bool Nebula::gravity_label = false;
float Nebula::hints_brightness = 0;
Vec3f Nebula::label_color = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circle_color = Vec3f(0.8,0.8,0.1);


Nebula::Nebula() : NGC_nb(0), neb_tex(NULL)
{
	inc_lum = rand()/RAND_MAX*M_PI;	
	longname = "";
	name = "";
}

Nebula::~Nebula()
{
    delete neb_tex;
	neb_tex = NULL;
}

string Nebula::get_info_string(const navigator*) const
{
	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,XYZ);
	stringstream ss;

	ss << "Name : " << name << endl;
	ss << "Cat: NGC ";
	if (NGC_nb > 0) ss << NGC_nb; else ss << "-";
	ss << "  IC ";
	if (IC_nb > 0) ss << IC_nb; else ss << "-";
	ss << endl;
	
	ss << "RA : " << print_angle_hms(tempRA*180./M_PI) << endl;
	ss << "DE : " << print_angle_dms_stel(tempDE*180./M_PI) << endl;

	ss.setf(ios::fixed);
	ss.precision(2);
	ss << "Magnitude : " << mag << endl;
	ss << "Type : " << string(typeDesc == "" ? "-" : typeDesc) << endl;
	ss << "Size : " << print_angle_dms_stel0(angular_size*180./M_PI) << endl; 
	
	return ss.str();
}

string Nebula::get_short_info_string(const navigator*) const
{
	stringstream ss;
	
	if(mag == 99 || mag < 4)
		ss << name;	
	else
	{
		ss << name << mag;
		ss.setf(ios::fixed);
		ss.precision(1);
		ss << ": mag " << mag;
	}
	
	return ss.str();
}

double Nebula::get_close_fov(const navigator*) const
{
	return angular_size * 180./M_PI * 4;
}

// Read nebula data from file and compute x,y and z;
// returns false if can't parse record
bool Nebula::read(const string& record)
{
	string tex_name;
	int rahr;
    float ramin;
    int dedeg;
    float demin;
	float tex_angular_size;
	float tex_rotation;
	
	std::istringstream istr(record);

	if (!(istr >> NGC_nb >> type >> rahr >> ramin >> dedeg >> demin >> mag >> tex_angular_size >> tex_rotation >> name >> longname >> tex_name >> credit)) return false ;

    // Replace the "_" with " "
    for (string::size_type i=0;i<name.length();++i)
	{
		if (name[i]=='_') name[i]=' ';
	}

    for (string::size_type i=0;i<longname.length();++i)
	{
		if (longname[i]=='_') longname[i]=' ';
	}

	if (credit  == "none")
		credit = "";
	else
		credit = string("Credit: ") + credit;	
		
    for (string::size_type i=0;i<credit.length();++i)
	{
		if (credit[i]=='_') credit[i]=' ';
	}
	
    // Calc the RA and DE from the datas
    float RaRad = hms_to_rad(rahr, (double)ramin);
    float DecRad = dms_to_rad(dedeg, (double)demin);

    // Calc the Cartesian coord with RA and DE
    sphe_to_rect(RaRad,DecRad,XYZ);
    XYZ*=RADIUS_NEB;

	// Calc the angular size in radian : TODO this should be independant of tex_angular_size
	angular_size = tex_angular_size/2/60*M_PI/180;

    neb_tex = new s_texture(tex_name);

	//tex_angular_size*tex_angular_size*3600/4*M_PI
	//	luminance = mag_to_luminance(mag, tex_angular_size*tex_angular_size*3600) /	neb_tex->get_average_luminance() * 50;
	luminance = mag_to_luminance(mag, tex_angular_size*tex_angular_size*3600);

	// this is a huge performance drag if called every frame, so cache here
	tex_avg_luminance = neb_tex->get_average_luminance();

	float tex_size = RADIUS_NEB * sin(tex_angular_size/2/60*M_PI/180);

    // Precomputation of the rotation/translation matrix
	Mat4f mat_precomp = Mat4f::translation(XYZ) *
						Mat4f::zrotation(RaRad) *
						Mat4f::yrotation(-DecRad) *
						Mat4f::xrotation(tex_rotation*M_PI/180.);

	tex_quad_vertex[0] = mat_precomp * Vec3f(0.,-tex_size,-tex_size); // Bottom Right
	tex_quad_vertex[1] = mat_precomp * Vec3f(0., tex_size,-tex_size); // Bottom Right
	tex_quad_vertex[2] = mat_precomp * Vec3f(0.,-tex_size, tex_size); // Bottom Right
	tex_quad_vertex[3] = mat_precomp * Vec3f(0., tex_size, tex_size); // Bottom Right

	if (!strcmp(type,"N")) { nType = NEB_N; typeDesc = "Supernova reemnant"; }
	else if (!strcmp(type,"SG")) { nType = NEB_SG; typeDesc = "Sipral galaxy"; }
	else if (!strcmp(type,"PN")) { nType = NEB_PN; typeDesc = "Planetary nebula"; }
	else if (!strcmp(type,"LG")) { nType = NEB_LG; typeDesc = "Lenticular galaxy"; }
	else if (!strcmp(type,"EG")) { nType = NEB_EG; typeDesc = "Elliptical galaxy"; }
	else if (!strcmp(type,"OC")) { nType = NEB_OC; typeDesc = "Open cluster"; }
	else if (!strcmp(type,"GC")) { nType = NEB_GC; typeDesc = "Globular cluster"; }
	else if (!strcmp(type,"DN")) { nType = NEB_DN; typeDesc = "Globular cluster"; }
	else if (!strcmp(type,"IG")) { nType = NEB_IG; typeDesc = "Irregular galaxy"; }
	else { nType = NEB_UNKNOWN; typeDesc = "Unknown type"; }

    return true;
}

void Nebula::draw_tex(const Projector* prj, tone_reproductor* eye, bool bright_nebulae)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

	if (!neb_tex) return;
	
	// if start zooming in, turn up brightness to full for DSO images
	// gradual change might be better

	if(bright_nebulae)
	{
		glColor3f(1.0,1.0,1.0);
	}
	else
	{
		float ad_lum=eye->adapt_luminance(luminance);

		// TODO this should be revisited to be less ad hoc
		// 3 is a fudge factor since only about 1/3 of a texture is not black background
		float cmag = 3 * ad_lum / tex_avg_luminance;
		glColor3f(cmag,cmag,cmag);

		//		printf("%s: lum %f ad_lum %f cmag %f angle %f\n", name.c_str(), luminance, ad_lum, cmag, angular_size);
	}

	glBindTexture(GL_TEXTURE_2D, neb_tex->getID());

	static Vec3d v;

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
		prj->project_prec_earth_equ(tex_quad_vertex[0],v); glVertex3dv(v);
        glTexCoord2i(0,0);              // Bottom Left
		prj->project_prec_earth_equ(tex_quad_vertex[1],v); glVertex3dv(v);
        glTexCoord2i(1,1);              // Top Right
		prj->project_prec_earth_equ(tex_quad_vertex[2],v); glVertex3dv(v);
        glTexCoord2i(0,1);              // Top Left
		prj->project_prec_earth_equ(tex_quad_vertex[3],v); glVertex3dv(v);
    glEnd();
}

void Nebula::draw_circle(const Projector* prj, const navigator * nav)
{
	if (2.f/get_on_screen_size(prj, nav)<0.1) return;
    inc_lum++;
	float lum = MY_MIN(1,2.f/get_on_screen_size(prj, nav))*(0.8+0.2*sinf(inc_lum/10));
    glColor3fv(circle_color*lum*hints_brightness);
    glBindTexture (GL_TEXTURE_2D, Nebula::tex_circle->getID());
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
        glVertex3f(XY[0] + 4, XY[1] - 4, 0.0f);
        glTexCoord2i(0,0);              // Bottom Left
        glVertex3f(XY[0] - 4, XY[1] - 4, 0.0f);
        glTexCoord2i(1,1);              // Top Right
        glVertex3f(XY[0] + 4, XY[1] + 4,0.0f);
        glTexCoord2i(0,1);              // Top Left
        glVertex3f(XY[0] - 4, XY[1] + 4,0.0f);
    glEnd ();
}

// Return the radius of a circle containing the object on screen
float Nebula::get_on_screen_size(const Projector* prj, const navigator * nav)
{
	return angular_size*180./M_PI/prj->get_fov()*prj->viewH();
}

void Nebula::draw_name(int hint_ON, const Projector* prj)
{
    glColor3fv(label_color*hints_brightness);
    float size = get_on_screen_size(prj);
    float shift = 8.f + size/2.f;

	std::stringstream ss;
	ss << longname;
	if (typeDesc != "") ss << " (" << typeDesc << ")";
	string lname = ss.str();

    if (gravity_label)
		prj->print_gravity180(nebula_font, XY[0]+shift, XY[1]+shift, (hint_ON==1?lname:name), 1, 0, 0);
	else
    	nebula_font->print(XY[0]+shift, XY[1]+shift, (hint_ON==1?lname:name));

    // draw image credit, if it fits easily
    if(credit != "" && size > nebula_font->getStrLen(credit))
	{
		gravity_label ? prj->print_gravity180(nebula_font, XY[0]-shift-40, XY[1]+-shift-40, credit, 1, 0, 0) :
			nebula_font->print(XY[0]-shift, XY[1]-shift-60, credit);
    }
}
