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


Constellation_mgr::Constellation_mgr() : asterFont(NULL)
{
}

// Destructor
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
	unsigned int x1, y1, x2, y2, hp1, hp2;
	int texSize;
    while(!feof(fic))
    {
        if (fscanf(fic,"%s %s %u %u %u %u %u %u\n",shortname,texfile,&x1,&y1,&hp1,&x2,&y2,&hp2)!=8)
		{
			printf("ERROR while loading art for constellation %s\n", shortname);
			exit(-1);
		}

    }
    fclose(fic);

	// TODO Solve a 12 variables system to get the transfo matrix from texture coord to
	// 3d position...
	s_texture * art_tex = new s_texture(texfile);
	texSize = art_tex->getSize();
	Vec3f pos_star1 = _VouteCeleste->search(hp1)->get_earth_equ_pos();
	Vec3f pos_star2 = _VouteCeleste->search(hp2)->get_earth_equ_pos();


}

// Draw all the constellations in the vector
void Constellation_mgr::draw(Projector* prj)
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
	prj->set_orthographic_projection();	// set 2D coordinate
    vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		(*iter)->draw(prj);
    }
	prj->reset_perspective_projection();
}

// Draw one constellation of internationnal name abr
void Constellation_mgr::draw(Projector* prj, char abr[4])
{
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		if (!strcmp((*iter)->short_name,abr)) break;
	}
    (*iter)->draw_alone(prj);
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(Projector* prj, bool _gravity_label)
{
	Constellation::gravity_label = _gravity_label;
    glColor3f(0.7,0.1,0.1);
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
