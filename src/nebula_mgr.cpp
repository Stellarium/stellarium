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

// class used to manage groups of Nebulas

#include "nebula_mgr.h"
#include "stellarium.h"
#include "s_texture.h"
#include "s_font.h"
#include "navigator.h"

#define RADIUS_NEB 28.

s_font * nebulaFont;

Nebula_mgr::Nebula_mgr()
{
	Selectionnee = NULL;
}

Nebula_mgr::~Nebula_mgr()
{   vector<Nebula *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   delete (*iter);
    }
    delete Nebula::texCircle;
}

// read from stream
int Nebula_mgr::Read(char * font_fileName, char * fileName)
{
	FILE * fic;
	fic = fopen(fileName,"r");
    if (!fic)
	{
		printf("Can't open %s\n",fileName);
		exit(1);
	}

    printf("Loading nebulas data...\n");
    Nebula * e = NULL;
    for(;;)
    {   
		e = new Nebula;
		if (e==NULL) {printf("ERROR, CAN'T CREATE A NEBUKA OBJECT\n"); exit(1);}
        int temp = e->Read(fic);
        if (temp==0) // eof
        {
			if (e) delete e;
			e = NULL;
            break;
        }
        if (temp==1 || temp==2) Liste.push_back(e);
	}
	fclose(fic);

    nebulaFont=new s_font(12.,"spacefont", font_fileName); // load Font
    if (!nebulaFont)
    {
	    printf("Can't create nebulaFont\n");
        exit(1);
    }

	return 0;
}

int Nebula_mgr::ReadTexture()
{   
	Nebula::texCircle=new s_texture("neb");   // Load circle texture
	if (!Nebula::texCircle) exit(1);
    vector<Nebula *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   
		if ((*iter)->ReadTexture()==0) return 0;
    }
    return 1;
}

// Draw all the Nebulaes
void Nebula_mgr::Draw(int names_ON, draw_utility * du)
{   glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

    vector<Nebula *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   
        // project in 2D to check if the nebula is in screen
	    gluProject((**iter).XYZ[0],(**iter).XYZ[1],(**iter).XYZ[2],M,P,V,&((**iter).XY[0]),
			&((**iter).XY[1]),&((**iter).XY[2]));
        if ((**iter).XY[2]<1 && (**iter).XY[0]>0 && (**iter).XY[0]<du->screenW &&
			(**iter).XY[1]>0 && (**iter).XY[1]<du->screenH)
	        (**iter).Draw();
    }
    if (names_ON)
    {
        du->set_orthographic_projection();
        for(iter=Liste.begin();iter!=Liste.end();iter++)
        { 
            // Draw the name
	        if ((**iter).XY[2]<1 && (**iter).XY[0]>0 && (**iter).XY[0]<du->screenW &&
				(**iter).XY[1]>0 && (**iter).XY[1]<du->screenH)
            {
                (**iter).DrawName();
                (**iter).DrawCircle(du);
            }
        }
        du->reset_perspective_projection();
    }
}

// Look for a nebulae by XYZ coords
stel_object * Nebula_mgr::search(vec3_t Pos)
{
	Pos.normalize();
    vector<Nebula *>::iterator iter;
    Nebula * plusProche=NULL;
    float anglePlusProche=0.;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   if ((**iter).XYZ[0]*Pos[0]+(**iter).XYZ[1]*Pos[1]+(**iter).XYZ[2]*Pos[2]>anglePlusProche)
        {   anglePlusProche=(**iter).XYZ[0]*Pos[0]+(**iter).XYZ[1]*Pos[1]+(**iter).XYZ[2]*Pos[2];
            plusProche=(*iter);
        }
    }
    if (anglePlusProche>RADIUS_NEB*0.999)
    {
        return plusProche;
    }
    else return NULL;
}

