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

s_font * constNameFont;

Constellation_mgr::Constellation_mgr()
{	constNameFont = NULL;
}

// Destructor
Constellation_mgr::~Constellation_mgr()
{   vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   delete (*iter);
    }
    if (constNameFont) delete constNameFont;
    constNameFont = NULL;
}

// Load from file
void Constellation_mgr::Load(char * fileName, Star_mgr * _VouteCeleste)
{   constNameFont = new s_font(0.013*global.X_Resolution,"spacefont", DATA_DIR "/spacefont.txt"); // load Font
    if (!constNameFont)
    {   printf("Can't create constNameFont\n");
        exit(1);
    }
	FILE * fic = fopen(fileName,"r");
    if (!fic)
    {   printf("Can't open %s\n",fileName);
        exit(1);
    }
    Constellation_mgr::Read(fic,_VouteCeleste);
    fclose(fic);
}

// Read the dat and store in the vector of "constellation *"
int Constellation_mgr::Read(FILE * fic, Star_mgr * _VouteCeleste)
{   printf("Loading constellation data...\n");
    constellation * cons = NULL;
    while(!feof(fic))
    {   //printf("va creer constellation\n");
        cons = new constellation;
        //printf("constellation cree : %x\n",cons);
        if (cons && cons->Read(fic, _VouteCeleste))
        {   //printf("constellation lue\n");
            Liste.push_back(cons);
            //printf("constellation ajoutee\n");
        }
        else
        {   if (cons) delete cons;
        }
        //printf("eof = %d\n",feof(fic));
    }
    return 0;
}

// Draw all the constellations in the vector
void Constellation_mgr::Draw()
{   glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
    vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   (**iter).Draw();
    }
}

// Draw one constellation of internationnal name Abr
void Constellation_mgr::Draw(char Abr[4])
{   vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {if (!strcmp((**iter).Abreviation,Abr)) break;}
    (**iter).DrawSeule();
}

// Draw the names of all the constellations
void Constellation_mgr::DrawName()
{
    vector<constellation *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   (**iter).ComputeName();
    }

    float coef;
    coef=rand();
    coef/=RAND_MAX;
    glColor3f(0.020*coef+0.7,0.020*coef+0.1,0.03*coef+0.1);
    setOrthographicProjection(global.X_Resolution, global.Y_Resolution);
    glPushMatrix();
    glLoadIdentity();
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   (**iter).DrawName();
    }
    glPopMatrix();
    resetPerspectiveProjection();
}
