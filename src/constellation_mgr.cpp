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


// Class used to manage group of constellation
#include "constellation.h"
#include "constellation_mgr.h"


Constellation_mgr::Constellation_mgr(Vec3f _lines_color, Vec3f _names_color) :
	asterFont(NULL), lines_color(_lines_color), names_color(_names_color)
{
}

Constellation_mgr::~Constellation_mgr()
{
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		delete (*iter);
    }
    if (asterFont) delete asterFont;
    asterFont = NULL;
}

// Load from file
void Constellation_mgr::load(const string& font_fileName, const string& fileName, const string& artfileName, Hip_Star_mgr * _VouteCeleste)
{
	printf("Loading constellation data...\n");

    asterFont = new s_font(12.,"spacefont", font_fileName); // load Font
    if (!asterFont)
    {
		printf("Can't create asterFont\n");
        exit(-1);
    }

	FILE * fic = fopen(fileName.c_str(),"r");
    if (!fic)
    {
		printf("Can't open %s\n",fileName.c_str());
        exit(-1);
    }

    Constellation * cons = NULL;
    while(!feof(fic))
    {
        cons = new Constellation;
        if (cons && cons->read(fic, _VouteCeleste))
        {
            asterisms.push_back(cons);
        }
        else
        {
        	if (cons) delete cons;
        }
    }
    fclose(fic);

	fic = fopen(artfileName.c_str(),"r");
    if (!fic)
    {
		printf("Can't open %s\n",artfileName.c_str());
        exit(-1);
    }

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	char shortname[20];
	char texfile[255];
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;
	int texSize;
    while(!feof(fic))
    {
        if (fscanf(fic,"%s %s %u %u %u %u %u %u %u %u %u\n",shortname,texfile,&x1,&y1,&hp1,&x2,&y2,&hp2,
			&x3,&y3,&hp3)!=11)
		{
			if (feof(fic))
			{
				// Empty constellation file
				fclose(fic);
				return;
			}

			printf("ERROR while loading art for constellation %s\n", shortname);
			exit(-1);
		}

		cons = NULL;
		cons = find_from_short_name(shortname);
		if (!cons)
		{
			printf("ERROR : Can't find constellation called : %s\n",shortname);
			exit(-1);
		}

		cons->art_tex = new s_texture(texfile);
		texSize = cons->art_tex->getSize();

		Vec3f s1 = _VouteCeleste->search(hp1)->get_earth_equ_pos();
		Vec3f s2 = _VouteCeleste->search(hp2)->get_earth_equ_pos();
		Vec3f s3 = _VouteCeleste->search(hp3)->get_earth_equ_pos();

		// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
		// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
		// We need 3 stars and the 4th point is deduced from the other to get an normal base
		// X = B inv(A)
		Vec3f s4 = s1 + (s2-s1)^(s3-s1);
		Mat4f B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
		Mat4f A(x1, texSize-y1, 0.f, 1.f, x2, texSize-y2, 0.f, 1.f,
			x3, texSize-y3, 0.f, 1.f, x1, texSize-y1, texSize, 1.f);
		Mat4f X = B * A.inverse();

		cons->art_vertex[0] = Vec3f(X*Vec3f(0,0,0));
		cons->art_vertex[1] = Vec3f(X*Vec3f(texSize,0,0));
		cons->art_vertex[2] = Vec3f(X*Vec3f(texSize,texSize,0));
		cons->art_vertex[3] = Vec3f(X*Vec3f(0,texSize,0));

    }
    fclose(fic);
}

// Draw all the constellations in the vector
void Constellation_mgr::draw(Projector* prj) const
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3fv(lines_color);
	prj->set_orthographic_projection();	// set 2D coordinate
    vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		(*iter)->draw_optim(prj);
    }
	prj->reset_perspective_projection();
}

// Draw one constellation of internationnal name abr
void Constellation_mgr::draw(Projector* prj, char abr[4]) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		if (!strcmp((*iter)->short_name,abr)) break;
	}
    (*iter)->draw(prj);
}

void Constellation_mgr::draw_art(Projector* prj) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
    glColor3f(1,1,1);
	prj->set_orthographic_projection();
    vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		(*iter)->draw_art_optim(prj);
    }
	prj->reset_perspective_projection();
	glDisable(GL_CULL_FACE);
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(Projector* prj, bool _gravity_label)
{
	Constellation::gravity_label = _gravity_label;
    glColor3fv(names_color);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    prj->set_orthographic_projection();	// set 2D coordinate
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		// Check if in the field of view
    	if ( prj->project_earth_equ_check((*iter)->XYZname, (*iter)->XYname) )
			(*iter)->draw_name(asterFont, prj);
    }
    prj->reset_perspective_projection();
}

void Constellation_mgr::draw_one_name(Projector* prj, Constellation* c, bool _gravity_label) const
{
	Constellation::gravity_label = _gravity_label;
    glColor3f(0.7,0.1,0.1);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    prj->set_orthographic_projection();	// set 2D coordinate
	// Check if in the field of view
    if ( prj->project_earth_equ_check(c->XYZname, c->XYname) )
	if (c) c->draw_name(asterFont, prj);
    prj->reset_perspective_projection();
}

Constellation* Constellation_mgr::is_star_in(const Hip_Star * s) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		// Check if the star is in one of the constellation
    	if ((*iter)->is_star_in(s)) return (*iter);
    }
	return NULL;
}

Constellation* Constellation_mgr::find_from_short_name(const string& shortname) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		// Check if the star is in one of the constellation
    	if (string((*iter)->short_name)==shortname) return (*iter);
    }
	return NULL;
}
