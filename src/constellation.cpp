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
#include "navigator.h"

#define RADIUS_CONST 8.

constellation::constellation() : Name(NULL), Inter(NULL), Asterism(NULL)
{
}

constellation::~constellation()
{   
	if (Asterism) delete Asterism;
    Asterism = NULL;
    if (Name) delete Name;
    Name = NULL;
    if (Inter) delete Inter;
    Inter = NULL;
}

int constellation::Read(FILE *  fic, Hip_Star_mgr * _VouteCeleste)
// Read constellation datas and grab cartesian positions of stars
{   
	char buff1[40];
	char buff2[40];
    unsigned int HP;

	fscanf(fic,"%s %s %s %u ",Abreviation,buff1,buff2,&NbSegments);

	Name=strdup(buff1);
	Inter=strdup(buff2);

    Asterism = new Hip_Star*[NbSegments*2];
    for (unsigned int i=0;i<NbSegments*2;i++)
    {
        fscanf(fic,"%u",&HP);
        Asterism[i]=_VouteCeleste->search(HP);
		if (!Asterism[i])
		{
			printf("Error in constellation %s asterism : can't find star HP=%d\n",Inter,HP);
		}
    }

	fscanf(fic,"\n");

    Xnom=0;
    Ynom=0;
    Znom=0;
    for(unsigned int ii=0;ii<NbSegments*2;ii++)
    {   Xnom+=(*Asterism[ii]).XYZ[0];
        Ynom+=(*Asterism[ii]).XYZ[1];
        Znom+=(*Asterism[ii]).XYZ[2];
    }
    Xnom/=NbSegments*2;
    Ynom/=NbSegments*2;
    Znom/=NbSegments*2;
    
    return 1;
}

// Draw the lines for the constellation using the coords of the stars (optimized for use with the class Constellation_mgr only)
void constellation::Draw()
{
    for(unsigned int i=0;i<NbSegments;++i)
    {
		glBegin (GL_LINES);
			glVertex3f((*Asterism[2*i]).XYZ[0],(*Asterism[2*i]).XYZ[1],(*Asterism[2*i]).XYZ[2]);
			glVertex3f((*Asterism[2*i+1]).XYZ[0],(*Asterism[2*i+1]).XYZ[1],(*Asterism[2*i+1]).XYZ[2]);
        glEnd ();
    }
}

// Same thing but for only one separate constellation (can be used without the class Constellation_mgr )
void constellation::DrawSeule()
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
    glPushMatrix();
    for(unsigned int i=0;i<NbSegments;i++)
    {
		glBegin (GL_LINES);
                glVertex3f((*Asterism[2*i]).XYZ[0],(*Asterism[2*i]).XYZ[1],(*Asterism[2*i]).XYZ[2]);
                glVertex3f((*Asterism[2*i+1]).XYZ[0],(*Asterism[2*i+1]).XYZ[1],(*Asterism[2*i+1]).XYZ[2]);
        glEnd ();
    }
    glPopMatrix();
}

// Draw the name
void constellation::DrawName(s_font * constfont)
{
	constfont->print((int)XYnom[0]-40,(int)XYnom[1], Inter/*Name*/); //"Inter" for internationnal name
}
