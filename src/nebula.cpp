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

#include "nebula.h"
#include "s_texture.h"
#include "stellarium.h"
#include "s_font.h"
#include "navigator.h"
#include "stel_utility.h"

#define RADIUS_NEB 1.

s_texture * Nebula::tex_circle = NULL;
s_font* Nebula::nebula_font = NULL;

Nebula::Nebula() : NGC_nb(0), name(NULL), neb_tex(NULL), tex_quad_vertex(NULL)
{
	inc_lum = rand()*M_PI;
}

Nebula::~Nebula()
{
	if (name) delete name;
	name = NULL;
    if (neb_tex) delete neb_tex;
	neb_tex = NULL;
	if (tex_quad_vertex) delete tex_quad_vertex;
	tex_quad_vertex = NULL;
}

void Nebula::get_info_string(char * s, const navigator * nav) const
{
	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,XYZ);
	sprintf(s,"Name : %s (NGC %u)\nRA : %s\nDE : %s\nMag : %.2f", name, NGC_nb,
		print_angle_hms(tempRA*180./M_PI), print_angle_dms_stel(tempDE*180./M_PI), mag);
}

int Nebula::read(FILE * catalogue)
// Lis les infos de la nébuleuse dans le fichier et calcule x,y et z;
{
	int rahr;
    float ramin;
    int dedeg;
    int demin;
	float tex_angular_size;
	float tex_rotation;
	char tex_name[255];
	char tempName[255];

    if (fscanf(catalogue,"%u %s %d %f %d %d %f %f %f %s %s\n",
		&NGC_nb, type, &rahr, &ramin,&dedeg,&demin,
		&mag,&tex_angular_size,&tex_rotation, tempName, tex_name)!=11)
	{
		return 0;
	}

	name = strdup(tempName);
    // Replace the "_" with " "
    char * cc;
    cc=strchr(name,'_');
    while(cc!=NULL)
    {
		(*cc)=' ';
        cc=strchr(name,'_');
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

	float tex_size = RADIUS_NEB * sin(tex_angular_size/2/60*M_PI/180);

    // Precomputation of the rotation/translation matrix
	Mat4f mat_precomp = Mat4f::translation(XYZ) *
						Mat4f::zrotation(RaRad) *
						Mat4f::yrotation(-DecRad) *
						Mat4f::xrotation(tex_rotation*M_PI/180.);

	tex_quad_vertex = new Vec3f[4];
	tex_quad_vertex[0] = mat_precomp * Vec3f(0.,-tex_size,-tex_size); // Bottom Right
	tex_quad_vertex[1] = mat_precomp * Vec3f(0., tex_size,-tex_size); // Bottom Right
	tex_quad_vertex[2] = mat_precomp * Vec3f(0.,-tex_size, tex_size); // Bottom Right
	tex_quad_vertex[3] = mat_precomp * Vec3f(0., tex_size, tex_size); // Bottom Right

    return 1;
}

void Nebula::draw_tex(const Projector* prj)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    float cmag=(1.-mag/12)*2;
    glColor3f(cmag,cmag,cmag);
    glBindTexture(GL_TEXTURE_2D, neb_tex->getID());

	static Vec3d v;

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
		prj->project_earth_equ(tex_quad_vertex[0],v); glVertex3dv(v);
        glTexCoord2i(0,0);              // Bottom Left
		prj->project_earth_equ(tex_quad_vertex[1],v); glVertex3dv(v);
        glTexCoord2i(1,1);              // Top Right
		prj->project_earth_equ(tex_quad_vertex[2],v); glVertex3dv(v);
        glTexCoord2i(0,1);              // Top Left
		prj->project_earth_equ(tex_quad_vertex[3],v); glVertex3dv(v);
    glEnd();
}

void Nebula::draw_circle(const Projector* prj)
{
	if (prj->get_fov()<sqrt(angular_size)*2) return;
    inc_lum++;
    glColor3f(sqrt(prj->get_fov())/10*(0.4+0.2*sin(inc_lum/100)),
		 sqrt(prj->get_fov())/10*(0.4+0.2*sin(inc_lum/100)),0.1);
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
float Nebula::get_on_screen_size(const navigator * nav, const Projector* prj)
{
	return angular_size/60./prj->get_fov()*prj->viewH();
}

void Nebula::draw_name(const Projector* prj)
{   
    glColor3f(0.4,0.3,0.5);
	nebula_font->print(XY[0]+3,XY[1]+3, name);
}

