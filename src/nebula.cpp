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

extern s_font * nebulaFont;

Nebula::Nebula()
{   
	incLum = rand()%100;
	nebTexture = NULL;
	matTransfo = NULL;
}

Nebula::~Nebula()
{   if (matTransfo) delete matTransfo;
    if (nebTexture) delete nebTexture;
}

int Nebula::Read(FILE * catalogue)
// Lis les infos de la nébuleuse dans le fichier et calcule x,y et z;
{   int rahr;
    float ramin;
    int dedeg;
    int demin;
    matTransfo=new float[16];   // Used to store the precalc transfos matrix used to draw the star

    if (fscanf(catalogue,"%u %u %s %s %d %f %d %d %f %f %f %s\n",&NGC,&Messier, Type, Constellation, &rahr, &ramin,&dedeg,&demin,&Mag,&Taille,&Rotation, Name)<12) return 0;//printf("Chargement de M%u %dh%fmin %ddeg%dmin Vmag=%f Taille=%f Rot:%f\n",Messier,rahr,ramin,dedeg,demin,Mag,Taille,Rotation);

    // Replace the "_" with " "
    char * cc;
    cc=strchr(Name,'_');
    while(cc!=NULL)
    {   (*cc)=' ';
        cc=strchr(Name,'_');
    }
    
    // Calc the RA and DE from the datas
    RaRad=RA_en_Rad(rahr,ramin);
    DecRad=DE_en_Rad(dedeg,demin);

    // Calc the Cartesian coord with RA and DE
    RADE_to_XYZ(RaRad,DecRad,XYZ);
    XYZ*=RAYON;

    matTransfo=new float[16];   // Used to store the precalc transfos matrix used to draw the star

    glLoadIdentity();           // Init the current matrix

    // Precalcul de la matrice de Positionnement/Rotation pour le dessin
    glTranslatef(XYZ[0],XYZ[1],XYZ[2]);
    glRotatef(RaRad*180/PI,0,1,0);
    glRotatef(DecRad*180/PI,-1,0,0);
    glRotatef(Rotation,0,0,1);
    glGetFloatv(GL_MODELVIEW_MATRIX , matTransfo);  // Store the matrix

    RayonPrecalc=RAYON*sin(Taille/2/60*PI/180);
    //printf("rayon : %f\n",RayonPrecalc);

    if (Messier>0)      //Load texture for Messiers
    {   int temp = ReadTexture();
        return temp;
    }
    return 0;
}


int Nebula::ReadTexture()
{   char nomFic[128];
    sprintf(nomFic,"m%d",Messier);
    nebTexture=new s_texture(nomFic);
    return 1;
}

void Nebula::Draw()
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glPushMatrix();
    glMultMatrixf(matTransfo);          // recuperation de la matrice calculé au chargement

    float cmag=(1-Mag/12)*2;
    glColor3f(cmag,cmag,cmag);
    glBindTexture (GL_TEXTURE_2D, nebTexture->getID());
    
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              //Bas Droite
        glVertex3f(RayonPrecalc,-RayonPrecalc,0.0f);
        glTexCoord2i(0,0);              //Bas Gauche
        glVertex3f(-RayonPrecalc,-RayonPrecalc,0.0f);
        glTexCoord2i(1,1);              //Haut Droit
        glVertex3f(RayonPrecalc,RayonPrecalc,0.0f);
        glTexCoord2i(0,1);              //Haut Gauche
        glVertex3f(-RayonPrecalc,RayonPrecalc,0.0f);
    glEnd ();

    glPopMatrix();  
}

void Nebula::DrawCircle(void)
{   if (global.Fov<sqrt(Taille)*2) return;
    incLum++;
    glColor3f(sqrt(global.Fov)/10*(0.4+0.2*sin(incLum/10)),sqrt(global.Fov)/10*(0.4+0.2*sin(incLum/10)),0.1);
    glBindTexture (GL_TEXTURE_2D, Nebula::texCircle->getID()); 
    glPushMatrix();
    glTranslatef(XY[0],global.Y_Resolution-XY[1],0);
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              //Bas Droite
        glVertex3f(4,-4,0.0f);
        glTexCoord2i(0,0);              //Bas Gauche
        glVertex3f(-4,-4,0.0f);
        glTexCoord2i(1,1);              //Haut Droit
        glVertex3f(4,4,0.0f);
        glTexCoord2i(0,1);              //Haut Gauche
        glVertex3f(-4,4,0.0f);
    glEnd ();
    glPopMatrix(); 
}

void Nebula::DrawName(void)
{   
    glColor3f(0.4,0.3,0.5);
	nebulaFont->print(XY[0]+3,global.Y_Resolution-XY[1]+3, Name); //"Inter" for internationnal name
}

s_texture * Nebula::texCircle=NULL;
