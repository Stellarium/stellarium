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
void Constellation_mgr::load(char * font_fileName, char * fileName, Hip_Star_mgr * _VouteCeleste)
{
	printf("Loading constellation data...\n");

    asterFont = new s_font(12.,"spacefont", font_fileName); // load Font
    if (!asterFont)
    {
		printf("Can't create asterFont\n");
        exit(-1);
    }

	FILE * fic = fopen(fileName,"r");
    if (!fic)
    {
		printf("Can't open %s\n",fileName);
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
void Constellation_mgr::draw(draw_utility * du, navigator* nav)
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
	du->set_orthographic_projection();	// set 2D coordinate
    vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		(*iter)->draw(nav);
    }
	du->reset_perspective_projection();
}

// Draw one constellation of internationnal name abr
void Constellation_mgr::draw(draw_utility * du, navigator* nav, char abr[4])
{
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		if (!strcmp((*iter)->short_name,abr)) break;
	}
    (*iter)->draw_alone(du, nav);
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(draw_utility * du, navigator* nav)
{
    glColor3f(0.7,0.1,0.1);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    du->set_orthographic_projection();	// set 2D coordinate

	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		// Check if in the field of view
    	if ( nav->project_earth_equ_to_screen_check((*iter)->XYZname, (*iter)->XYname) )
			(*iter)->draw_name(asterFont);
    }

    du->reset_perspective_projection();
}
