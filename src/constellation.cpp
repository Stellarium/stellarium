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

#include "constellation.h"
#include "projector.h"

#define RADIUS_CONST 1.

bool Constellation::gravity_label = false;
s_font* Constellation::constellation_font = NULL;

Constellation::Constellation() : name(NULL), inter(NULL), asterism(NULL), art_tex(NULL), art_on(0), art_intensity(0)
{
}

Constellation::~Constellation()
{   
	if (asterism) delete asterism;
    asterism = NULL;
    if (name) delete name;
    name = NULL;
    if (inter) delete inter;
    inter = NULL;
	if (art_tex) delete art_tex;
	art_tex = NULL;
}

int Constellation::read(FILE *  fic, Hip_Star_mgr * _VouteCeleste)
// Read Constellation datas and grab cartesian positions of stars
{   
	char buff1[40];
	char buff2[40];
    unsigned int HP;

	if (fscanf(fic,"%s %s %s %u ",short_name,buff1,buff2,&nb_segments)!=4) return 0;

	name=strdup(buff1);
	inter=strdup(buff2);

    asterism = new Hip_Star*[nb_segments*2];
    for (unsigned int i=0;i<nb_segments*2;++i)
    {
        if (fscanf(fic,"%u",&HP)!=1)
		{
			printf("ERROR while loading constellation data (reading %s)\n", inter);
			exit(-1);
		}

        asterism[i]=_VouteCeleste->search(HP);
		if (!asterism[i])
		{
			printf("Error in Constellation %s asterism : can't find star HP=%d\n",inter,HP);
		}
    }


    for(unsigned int ii=0;ii<nb_segments*2;++ii)
    {
		XYZname+=(*asterism[ii]).XYZ;
    }
    XYZname*=1./(nb_segments*2);

    return 1;
}

// fade on art
void Constellation::show_art(void)
{
  art_on = 1;
}

// fade off art
void Constellation::hide_art(void)
{
  art_on = 0;
}


// Draw the Constellation lines
void Constellation::draw(Projector* prj, const Vec3f& lines_color) const
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

	glColor3fv(lines_color);

    prj->set_orthographic_projection();	// set 2D coordinate

	static Vec3d star1;
	static Vec3d star2;

    for(unsigned int i=0;i<nb_segments;++i)
    {
      if(prj->project_earth_equ_line_check(asterism[2*i]->XYZ,star1,asterism[2*i+1]->XYZ,star2) ) {
	glBegin(GL_LINES);
	glVertex2f(star1[0],star1[1]);
	glVertex2f(star2[0],star2[1]);
	glEnd();
		
      }
    }

    prj->reset_perspective_projection();
}

// Draw the lines for the Constellation using the coords of the stars
// (optimized for use thru the class Constellation_mgr only)
void Constellation::draw_optim(Projector* prj) const
{
	static Vec3d star1;
	static Vec3d star2;

    for(unsigned int i=0;i<nb_segments;++i)
    {

      if(prj->project_earth_equ_line_check(asterism[2*i]->XYZ,star1,asterism[2*i+1]->XYZ,star2) ) {
	glBegin(GL_LINES);
	glVertex2f(star1[0],star1[1]);
	glVertex2f(star2[0],star2[1]);
	glEnd();
		
      }
    }

}


// Draw the name
void Constellation::draw_name(s_font * constfont, Projector* prj) const
{
	gravity_label ? prj->print_gravity180(constfont, XYname[0], XYname[1], inter, 1, -constfont->getStrLen(inter)/2) :
	constfont->print(XYname[0]-constfont->getStrLen(inter)/2, XYname[1], inter/*name*/);
}

// Draw the art texture, optimized function to be called thru a constellation manager only
void Constellation::draw_art_optim(Projector* prj, int delta_time) 
{
	if (art_tex)
	{
		// 2 second fade
		float delta_intensity = delta_time/2000.f;

		// update fade
		if(art_on)
		{
			if(art_intensity + delta_intensity <= 0.5) {art_intensity += delta_intensity;} 
			else {art_intensity = 0.5;}
		} 
		else
		{
			if(art_intensity > delta_intensity) {art_intensity -= delta_intensity;} 
			else {art_intensity = 0; return;}
		}

		static Vec3d v;

		// for fade in
		glColor3f(art_intensity,art_intensity,art_intensity);

		glBindTexture(GL_TEXTURE_2D, art_tex->getID());
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[0],v)) {glTexCoord2f(0,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[1],v)) {glTexCoord2f(0.5,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[2],v)) {glTexCoord2f(0.5,0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[3],v)) {glTexCoord2f(0,0.5); glVertex2f(v[0],v[1]);}
		glEnd();
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[4],v)) {glTexCoord2f(0.5 + 0,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[5],v)) {glTexCoord2f(0.5 + 0.5,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[6],v)) {glTexCoord2f(0.5 + 0.5,0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[7],v)) {glTexCoord2f(0.5 + 0,0.5); glVertex2f(v[0],v[1]);}
		glEnd();
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[8],v)) {glTexCoord2f(0.5 + 0, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[9],v)) {glTexCoord2f(0.5 + 0.5, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[10],v)) {glTexCoord2f(0.5 + 0.5, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[11],v)) {glTexCoord2f(0.5 +  0, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
		glEnd();
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[12],v)) {glTexCoord2f(0, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[13],v)) {glTexCoord2f(0.5, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[14],v)) {glTexCoord2f(0.5, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[15],v)) {glTexCoord2f(0, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
		glEnd();
	}
}

// Draw the art texture
void Constellation::draw_art(Projector* prj, int delta_time) 
{
	if (art_tex)
	{
		// 2 second fade
		float delta_intensity = delta_time/2000.f;

		// update fade
		if(art_on)
		{
			if(art_intensity + delta_intensity <= 0.5) {art_intensity += delta_intensity;} 
			else {art_intensity = 0.5;}
		} 
		else
		{
			if(art_intensity > delta_intensity) {art_intensity -= delta_intensity;} 
			else {art_intensity = 0; return;}
		}
		
		static Vec3d v;

		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		// for fade in
		glColor3f(art_intensity,art_intensity,art_intensity);
		glBindTexture(GL_TEXTURE_2D, art_tex->getID());

		prj->set_orthographic_projection();

		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[0],v)) {glTexCoord2f(0,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[1],v)) {glTexCoord2f(0.5,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[2],v)) {glTexCoord2f(0.5,0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[3],v)) {glTexCoord2f(0,0.5); glVertex2f(v[0],v[1]);}
		glEnd();
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[4],v)) {glTexCoord2f(0.5 + 0,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[5],v)) {glTexCoord2f(0.5 + 0.5,0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[6],v)) {glTexCoord2f(0.5 + 0.5,0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[7],v)) {glTexCoord2f(0.5 + 0,0.5); glVertex2f(v[0],v[1]);}
		glEnd();
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[8],v)) {glTexCoord2f(0.5 + 0, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[9],v)) {glTexCoord2f(0.5 + 0.5, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[10],v)) {glTexCoord2f(0.5 + 0.5, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[11],v)) {glTexCoord2f(0.5 +  0, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
		glEnd();
		glBegin(GL_QUADS);
			if (prj->project_earth_equ(art_vertex[12],v)) {glTexCoord2f(0, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[13],v)) {glTexCoord2f(0.5, 0.5 + 0); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[14],v)) {glTexCoord2f(0.5, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
			if (prj->project_earth_equ(art_vertex[15],v)) {glTexCoord2f(0, 0.5 + 0.5); glVertex2f(v[0],v[1]);}
		glEnd();

		prj->reset_perspective_projection();

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}
}

const Constellation* Constellation::is_star_in(const Hip_Star * s) const
{
    for(unsigned int i=0;i<nb_segments*2;++i)
    {
		if (asterism[i]==s) return this;
    }
	return NULL;
}
