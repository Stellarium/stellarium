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

#include "nebula.h"
#include "s_texture.h"
#include "stellarium.h"
#include "s_font.h"
#include "navigator.h"

#define RADIUS_NEB 28.

s_texture * Nebula::texCircle = NULL;

Nebula::Nebula()
{   
	incLum = rand()%100;
	nebTexture = NULL;
	matTransfo = NULL;
}

Nebula::~Nebula()
{
	if (matTransfo) delete matTransfo;
    if (nebTexture) delete nebTexture;
}

void Nebula::get_info_string(char * s, navigator * nav) const
{
	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,&XYZ);
	sprintf(s,"Name : %s (NGC %u)\nRA : %s\nDE : %s\nMag : %.2f", Name, NGC, print_angle_hms(tempRA*180./M_PI), print_angle_dms_stel(tempDE*180./M_PI), Mag);
}

int Nebula::Read(FILE * catalogue)
// Lis les infos de la nébuleuse dans le fichier et calcule x,y et z;
{
	int rahr;
    float ramin;
    int dedeg;
    int demin;
    matTransfo=new float[16];   // Used to store the precalc transfos matrix used to draw the star

	char texName[255];

    if (fscanf(catalogue,"%u %u %s %s %d %f %d %d %f %f %f %s %s\n",
		&NGC,&Messier, Type, Constellation, &rahr, &ramin,&dedeg,&demin,
		&Mag,&Taille,&Rotation, Name, texName)!=13)
	{
		return 0;
	}

    // Replace the "_" with " "
    char * cc;
    cc=strchr(Name,'_');
    while(cc!=NULL)
    {
		(*cc)=' ';
        cc=strchr(Name,'_');
    }
    
    // Calc the RA and DE from the datas
    RaRad=hms_to_rad(rahr, (double)ramin);
    DecRad=dms_to_rad(dedeg, (double)demin);

    // Calc the Cartesian coord with RA and DE
    sphe_to_rect(RaRad,DecRad,&XYZ);
    XYZ*=RADIUS_NEB;

    matTransfo=new float[16];   // Used to store the precalc transfos matrix

    glLoadIdentity();           // Init the current matrix

    // Precomputation of the rotation/translation matrix
    glTranslatef(XYZ[0],XYZ[1],XYZ[2]);
    glRotatef(RaRad*180./M_PI,0,0,1);
    glRotatef(DecRad*180./M_PI,0,-1,0);
    glRotatef(Rotation,1,0,0);
    glGetFloatv(GL_MODELVIEW_MATRIX , matTransfo);  // Store the matrix

    RayonPrecalc=RADIUS_NEB*sin(Taille/2/60*M_PI/180);
    //printf("rayon : %f\n",RayonPrecalc);

    nebTexture=new s_texture(texName);

    return 1;
}

void Nebula::Draw()
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glPushMatrix();
    glMultMatrixf(matTransfo);          // reload the matrix precomputed while loading

    float cmag=(1-Mag/12)*2;
    glColor3f(cmag,cmag,cmag);
    glBindTexture (GL_TEXTURE_2D, nebTexture->getID());
    
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
        glVertex3f(0.0f,-RayonPrecalc,-RayonPrecalc);
        glTexCoord2i(0,0);              // Bottom Left
        glVertex3f(0.0f, RayonPrecalc,-RayonPrecalc);
        glTexCoord2i(1,1);              // Top Right
        glVertex3f(0.0f,-RayonPrecalc, RayonPrecalc);
        glTexCoord2i(0,1);              // Top Left
        glVertex3f(0.0f, RayonPrecalc, RayonPrecalc);
    glEnd ();

    glPopMatrix();  
}

void Nebula::DrawCircle(draw_utility * du)
{
	if (du->fov<sqrt(Taille)*2) return;
    incLum++;
    glColor3f(sqrt(du->fov)/10*(0.4+0.2*sin(incLum/10)),
		 sqrt(du->fov)/10*(0.4+0.2*sin(incLum/10)),0.1);
    glBindTexture (GL_TEXTURE_2D, Nebula::texCircle->getID()); 
    glPushMatrix();
    glTranslatef(XY[0], XY[1],0);
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
        glVertex3f(4,-4,0.0f);
        glTexCoord2i(0,0);              // Bottom Left
        glVertex3f(-4,-4,0.0f);
        glTexCoord2i(1,1);              // Top Right
        glVertex3f(4,4,0.0f);
        glTexCoord2i(0,1);              // Top Left
        glVertex3f(-4,4,0.0f);
    glEnd ();
    glPopMatrix(); 
}

// Return the radius of a circle containing the object on screen
float Nebula::get_on_screen_size(navigator * nav, draw_utility * du)
{
	return Taille/60./du->fov*du->screenH;
}

void Nebula::DrawName(s_font* nebulaFont)
{   
    glColor3f(0.4,0.3,0.5);
	nebulaFont->print(XY[0]+3,XY[1]+3, Name); //"Inter" for internationnal name
}

