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

Constellation::Constellation() : name(NULL), inter(NULL), asterism(NULL), art_tex(NULL)
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
			printf("ERROR while loading constellation data\n");
			exit(-1);
		}

        asterism[i]=_VouteCeleste->search(HP);
		if (!asterism[i])
		{
			printf("Error in Constellation %s asterism : can't find star HP=%d\n",inter,HP);
		}
    }

	if (fscanf(fic,"\n"))
	{
		printf("ERROR while loading constellation data\n");
		exit(-1);
	}

    for(unsigned int ii=0;ii<nb_segments*2;++ii)
    {
		XYZname+=(*asterism[ii]).XYZ;
    }
    XYZname*=1./(nb_segments*2);

    return 1;
}

// Draw the lines for the Constellation using the coords of the stars
// (optimized for use thru the class Constellation_mgr only)
void Constellation::draw(Projector* prj)
{
	static Vec3d star1;
	static Vec3d star2;

    for(unsigned int i=0;i<nb_segments;++i)
    {
		if (prj->project_earth_equ(asterism[2*i]->XYZ,star1) &&
			prj->project_earth_equ(asterism[2*i+1]->XYZ,star2))
		{
			glBegin(GL_LINES);
				glVertex2f(star1[0],star1[1]);
				glVertex2f(star2[0],star2[1]);
        	glEnd();
		}
    }
}

// Same thing but for only one separate Constellation (can be used without the class Constellation_mgr )
void Constellation::draw_alone(Projector* prj) const
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
    prj->set_orthographic_projection();	// set 2D coordinate

	static Vec3d star1;
	static Vec3d star2;

    for(unsigned int i=0;i<nb_segments;++i)
    {
		if (prj->project_earth_equ(asterism[2*i]->XYZ,star1) &&
			prj->project_earth_equ(asterism[2*i+1]->XYZ,star2))
		{
			glBegin (GL_LINES);
				glVertex2f(star1[0],star1[1]);
				glVertex2f(star2[0],star2[1]);
        	glEnd ();
		}
    }

    prj->reset_perspective_projection();
}

// Draw the name
void Constellation::draw_name(s_font * constfont, Projector* prj) const
{
	gravity_label ? prj->print_gravity(constfont, XYname[0], XYname[1], inter, -constfont->getStrLen(inter)/2) :
	constfont->print(XYname[0]-constfont->getStrLen(inter)/2, XYname[1], inter/*name*/);
}

const Constellation* Constellation::is_star_in(const Hip_Star * s) const
{
    for(unsigned int i=0;i<nb_segments*2;++i)
    {
		if (asterism[i]==s) return this;
    }
	return NULL;
}
