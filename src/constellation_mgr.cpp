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
void Constellation_mgr::load(const string& font_fileName, const string& fileName, Hip_Star_mgr * _VouteCeleste)
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
