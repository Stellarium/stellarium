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

extern s_font * constNameFont;

constellation::constellation() : Name(NULL), Inter(NULL), Asterism(NULL)
{
}

constellation::~constellation()
{   if (Asterism) delete [] Asterism;
    Asterism = NULL;
    if (Name) delete Name;
    Name = NULL;
    if (Inter) delete Inter;
    Inter = NULL;
}

char * removeEndSpaces1(char * str)
{   
	unsigned int i=0;
    while (str[i]!='\0' && str[i]!='\n' && str[i]!='\r') {i++;}
    str[i]='\0';
    i--;
    while(str[i]==' ') {str[i]='\0'; i--;}
    char * result = strdup(str);
    return result;
}

int constellation::Read(FILE *  fic, Star_mgr * _VouteCeleste)
// Lis les infos de la constellation et récupère les x,y et z des etoiles
// Le fichier des constellation doit etre fini sur une ligne vide (faire entree sur la derniere ligne quoi....)
{   char buff[400];
    unsigned int HR;

    fgets(buff,22,fic);
    buff[22]='\0';
    Name = removeEndSpaces1(buff);

    if (feof(fic)) return 0;

    fgets(Abreviation,4,fic);
    Abreviation[3]='\0';

    fgets(buff,19,fic);
    buff[19]='\0';
    Inter = removeEndSpaces1(buff);

    fscanf(fic,"%2u:",&NbSegments);

//    printf("%s %s %s %u\n",Name,Abreviation,Inter,NbSegments);

    fgets(buff,NbSegments*8+NbSegments*2,fic);
    char c = fgetc(fic);
    while (c!='\n') {c = fgetc(fic);}

    Asterism = new Star*[NbSegments*2];
    for (int i=0;i<NbSegments*2;i++)
    {
        sscanf(buff+i*5,"%4u",&HR);
        Asterism[i]=_VouteCeleste->Rechercher(HR);
		if (!Asterism[i])
		{
			printf("Error in constellation %s asterism : can't find star HR=%d\n",Inter,HR);
			exit(1);
		}
//        printf("HR = %u %x\n",HR,Asterism[i]);
    }

    Xnom=0;
    Ynom=0;
    Znom=0;
    for(int ii=0;ii<NbSegments*2;ii++)
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
{   glPushMatrix();
    for(int i=0;i<NbSegments;i++)
    {   glBegin (GL_LINES);
                glVertex3f((*Asterism[2*i]).XYZ[0],(*Asterism[2*i]).XYZ[1],(*Asterism[2*i]).XYZ[2]);
                glVertex3f((*Asterism[2*i+1]).XYZ[0],(*Asterism[2*i+1]).XYZ[1],(*Asterism[2*i+1]).XYZ[2]);
        glEnd ();
    }
    glPopMatrix();
}

// Same thing but for only one separate constellation (can be used without the class Constellation_mgr )
void constellation::DrawSeule()
{   glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(0.2,0.2,0.2);
    glPushMatrix();
    for(int i=0;i<NbSegments;i++)
    {   glBegin (GL_LINES);
                glVertex3f((*Asterism[2*i]).XYZ[0],(*Asterism[2*i]).XYZ[1],(*Asterism[2*i]).XYZ[2]);
                glVertex3f((*Asterism[2*i+1]).XYZ[0],(*Asterism[2*i+1]).XYZ[1],(*Asterism[2*i+1]).XYZ[2]);
        glEnd ();
    }
    glPopMatrix();
}

// Chek if the constellation is in the field of view and calc the x,y position if true
void constellation::ComputeName()
{   if (acos((global.XYZVision[0]*Xnom+global.XYZVision[1]*Ynom+global.XYZVision[2]*Znom)/RAYON)>((float)global.Fov+4)*PI*1.333333/360)
    {   inFov=false;
        return; // Si hors du champ
    }
    inFov=true;
    Project(Xnom,Ynom,Znom,x_c ,y_c);
    return;
}

// Draw the name
void constellation::DrawName()
{   
	if (inFov)
    {   
		constNameFont->print((int)x_c-40,(int)(global.Y_Resolution-y_c), Inter/*Name*/); //"Inter" for internationnal name
    }
}
