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
	vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {
		delete (*iter);
    }
    if (asterFont) delete asterFont;
    asterFont = NULL;
}

// Load from file
void Constellation_mgr::Load(char * font_fileName, char * fileName, Hip_Star_mgr * _VouteCeleste)
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

    constellation * cons = NULL;
    while(!feof(fic))
    {
        cons = new constellation;
        if (cons && cons->Read(fic, _VouteCeleste))
        {
            Liste.push_back(cons);
        }
        else
        {
        	if (cons) delete cons;
        }
    }
    fclose(fic);
}

// Draw all the constellations in the vector
void Constellation_mgr::Draw()
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
    vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {
		(**iter).Draw();
    }
}

// Draw one constellation of internationnal name Abr
void Constellation_mgr::Draw(char Abr[4])
{
	vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {if (!strcmp((**iter).Abreviation,Abr)) break;}
    (**iter).DrawSeule();
}

// Draw the names of all the constellations
void Constellation_mgr::DrawName(draw_utility * du)
{
    float coef;
    coef=rand();
    coef/=RAND_MAX;
    glColor3f(0.020*coef+0.7,0.020*coef+0.1,0.03*coef+0.1);
    //du->set_orthographic_projection();
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
	vector<constellation *>::iterator iter;

    double z;
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

    du->set_orthographic_projection();	// set 2D coordinate

    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {
		gluProject( (**iter).Xnom, (**iter).Ynom, (**iter).Znom, M,P,V, &(**iter).XYnom[0], &(**iter).XYnom[1], &z);
		// Check if in the field of view
    	if ( z > 1 || (**iter).XYnom[0]<0. || (**iter).XYnom[1]<0. ||
			(**iter).XYnom[0]>du->screenW || (**iter).XYnom[1]>du->screenH ) continue;
		(**iter).DrawName(asterFont);
    }

    du->reset_perspective_projection();
}
