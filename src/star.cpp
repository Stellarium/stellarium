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

#include "star.h"
#include "s_font.h"

extern unsigned int starTextureId;
extern s_font * starFont;

static unsigned int HRpasse=0;
static float coef;
static float cmag;
static float rmag;



Star::Star() : CommonName(NULL), ShortCommonName(NULL), haveShortCommonName(false)
{
}

Star::~Star()
{   
    if (CommonName) free(CommonName);
    if (ShortCommonName) free(ShortCommonName);
}

int Star::Read(FILE * catalog)
// Lis les infos de l'étoile dans le fichier et calcule x,y et z;
{  
    char strTmp[20];
    char ligne[50];
    
    fgets(ligne,45,catalog);

    strncpy(strTmp,ligne,4);    // HR number
    strTmp[4]='\0';
    HR = atoi(strTmp);

    strncpy(Name,ligne+4,10);   // Name (Flamsteed)
    Name[10]='\0';

    strncpy(strTmp,ligne+14,6); // SAO number
    strTmp[6]='\0';
    SAO = atoi(strTmp);

    strncpy(strTmp,ligne+20,7); // RaRad
    strTmp[7]='\0';
    RaRad=atof(strTmp);

    strncpy(strTmp,ligne+27,8); // DecRad
    strTmp[7]='\0';
    DecRad=atof(strTmp);

    strncpy(strTmp,ligne+35,5); // Mag
    Mag=atof(strTmp);

    SpType=ligne[40];           // SpType

    // Calc the Cartesian coord with RA and DE
    RADE_to_XYZ(RaRad,DecRad,XYZ);
    XYZ*=RAYON;


    switch (SpType)             // Color depending on the specrtal type
    {   
        case 'O':   RGB[0]=0.8f;  RGB[1]=1.0f; RGB[2]=1.3f;  break;
        case 'B':   RGB[0]=0.9f;  RGB[1]=1.0f; RGB[2]=1.2f;  break;
        case 'A':   RGB[0]=0.95f; RGB[1]=1.0f; RGB[2]=1.15f; break;
        case 'F':   RGB[0]=1.05f; RGB[1]=1.0f; RGB[2]=1.05f; break;
        case 'G':   RGB[0]=1.3f;  RGB[1]=1.0f; RGB[2]=0.9f;  break;
        case 'K':   RGB[0]=1.15f; RGB[1]=0.95f;RGB[2]=0.8f;  break;
        case 'M':
        case 'C':   RGB[0]=1.3f;  RGB[1]=0.85f;RGB[2]=0.6f;  break;
        case 'S':   RGB[0]=1.5f;  RGB[1]=0.8f; RGB[2]=0.2f;  break;
    }

    // First part of the calculation of the demi-size of the star texture
    float L=pow(100,-Mag/4.);
    MaxColorValue=myMax(RGB[0],RGB[2]);
    rmag_t = sqrt(L/(pow(L,0.46666)+7.079))*sqrt(MaxColorValue)*1200.;

    return feof(catalog);
}




void Star::Draw(void)
{
    // Check if in the field of view, if not return
    if (XY[0]<0 || XY[0]>global.X_Resolution || XY[1]<0 || XY[1]>global.Y_Resolution) return;
    
    // Random coef for star twinkling
    coef=(double)rand()/RAND_MAX*global.StarTwinkleAmount/10;

    // Calculation of the demi-size of the star texture

    rmag = rmag_t/pow(global.Fov,0.85);

    cmag = 1;
    
    // if size of star is too small (blink) we put it to 1 --> no more blink
    // And we compensate the difference of brighteness with cmag
    if (rmag<1.2) 
    {   
        if (rmag<0.5) return;
        cmag=pow(rmag,2)/1.44;
        rmag=1.2;
    }

    if (rmag>4) 
    {   
        rmag=4;
    }

    // Calculation of the luminosity
    cmag*=(1-coef);
    rmag*=global.StarScale/3;
    glColor3fv(RGB/(MaxColorValue)*cmag);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(XY[0],global.Y_Resolution-XY[1],0);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f(-rmag,-rmag,0);      //Bas Gauche
        glTexCoord2f(1.0f,0.0f);    glVertex3f(rmag,-rmag,0);       //Bas Droite
        glTexCoord2f(1.0f,1.0f);    glVertex3f(rmag,rmag,0);        //Haut Droit
        glTexCoord2f(0.0f,1.0f);    glVertex3f(-rmag,rmag,0);       //Haut Gauche
    glEnd ();
    glPopMatrix();
}

void Star::DrawName(void)
{   
    glColor3fv(RGB/2.5);
	starFont->print(XY[0]+6,global.Y_Resolution-XY[1]+6, ShortCommonName); //"Inter" for internationnal name
}
