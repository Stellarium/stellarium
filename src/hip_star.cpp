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

#include "hip_star.h"
#include "bytes.h"

extern unsigned int starTextureId;
extern s_font * starFont;

static float coef;
static float cmag;
static float rmag;

Hip_Star::Hip_Star() : 
	CommonName(NULL),
    Name(NULL)
{
}

Hip_Star::~Hip_Star()
{   
	if(Name) delete Name;
	if(CommonName) delete CommonName;
}

int Hip_Star::Read(FILE * catalog)
// Read datas in binary catalog and compute x,y,z;
{  
    char err;
    float RA=0, DE=0, paralax=0;
    char errParalax=0;
    
    HP=0;
    fread((char*)&HP,4,1,catalog);
    LE_TO_CPU_INT32(HP,HP);
    
    int HD=0;
    fread((char*)&HD,4,1,catalog);
    LE_TO_CPU_INT32(HD,HD);
    
    fread((char*)&RA,4,1,catalog);
    LE_TO_CPU_FLOAT(RA, RA);
    
    fread((char*)&DE,4,1,catalog);
    LE_TO_CPU_FLOAT(DE, DE);
    
    RA/=12./PI;     // Convert from hours to rad
    
    DE/=180./PI;    // Convert from deg to rad 
    
    fread((char*)&paralax,4,1,catalog);
    LE_TO_CPU_FLOAT(paralax, paralax);
    
    unsigned short int mag;
    fread((char*)&mag,2,1,catalog);
    LE_TO_CPU_INT16(mag, mag);
    Mag = ((float)mag + 5) / 256.0;
    if (Mag>250) Mag = Mag - 256;

    unsigned short int type;
    fread((char*)&type,2,1,catalog);
	LE_TO_CPU_INT16(type, type);
	 
    fread((char*)&errParalax,1,1,catalog);
	
    //printf("HP=%u RA=%f DE=%f parax=%f mag=%f type=%u err=%d\n",HP,RA,DE,paralax, Mag,(unsigned int) type, (int)errParalax);
	SpType = 'O';

    // Calc the Cartesian coord with RA and DE
    RADE_to_XYZ((double)RA,(double)DE,XYZ);
    XYZ*=RAYON;


    switch(type >> 8 & 0xf)
    {
        case 0 : SpType = 'O'; break;
        case 1 : SpType = 'B'; break;
        case 2 : SpType = 'A'; break;
        case 3 : SpType = 'F'; break;
        case 4 : SpType = 'G'; break;
        case 5 : SpType = 'K'; break;
        case 6 : SpType = 'M'; break;
        case 7 : SpType = 'R'; break;
        case 8 : SpType = 'S'; break;
        case 9 : SpType = 'N'; break;
        case 10: SpType = 'W'; break; /* WC */
        case 11: SpType = 'X'; break; /* WN */
        case 12: SpType = '?'; break;
        default: SpType = '?';
    }
    

    switch (SpType)             // Color depending on the spectral type
    {   
        case 'O':   RGB[0]=0.8f;  RGB[1]=1.0f; RGB[2]=1.3f;  break;
        case 'B':   RGB[0]=0.9f;  RGB[1]=1.0f; RGB[2]=1.2f;  break;
        case 'A':   RGB[0]=0.95f; RGB[1]=1.0f; RGB[2]=1.15f; break;
        case 'F':   RGB[0]=1.05f; RGB[1]=1.0f; RGB[2]=1.05f; break;
        case 'G':   RGB[0]=1.3f;  RGB[1]=1.0f; RGB[2]=0.9f;  break;
        case 'K':   RGB[0]=1.15f; RGB[1]=0.95f;RGB[2]=0.8f;  break;
        case 'M':   RGB[0]=1.15f; RGB[1]=0.85f;RGB[2]=0.8f;  break;
        case 'C':   RGB[0]=1.3f;  RGB[1]=0.85f;RGB[2]=0.6f;  break;
        case 'R':
        case 'N':
        case 'S':   RGB[0]=1.5f;  RGB[1]=0.8f; RGB[2]=0.2f;  break;
        default :   RGB[0]=1.0f;  RGB[1]=1.0f; RGB[2]=1.0f;
    }
    
    // First part of the calculation of the demi-size of the star texture
    float L=pow(100,-Mag/4.1);
    MaxColorValue=myMax(RGB[0],RGB[2]);
    rmag_t = sqrt(L/(pow(L,0.46666)+7.079))*sqrt(MaxColorValue)*1200.;

    return 1;
}


void Hip_Star::Draw(void)
{
    // Check if in the field of view, if not return
    if ( XY[0]<0 || XY[0]>global.X_Resolution || 
         XY[1]<0 || XY[1]>global.Y_Resolution ) 
        return;
    // Random coef for star twinkling
    coef=(float)rand()/RAND_MAX*global.StarTwinkleAmount/10;

    // Calculation of the demi-size of the star texture
    rmag = rmag_t/pow(global.Fov,0.85);

    cmag = 1.;
    
    // if size of star is too small (blink) we put it to 1 --> no more blink
    // And we compensate the difference of brighteness with cmag
    if (rmag<1.2) 
    {   
        if (rmag<0.5) return;
        cmag=pow(rmag,2)/1.44;
        rmag=1.2;
    }

    if (rmag>6) 
    {   
        rmag=6.;
    }

    // Calculation of the luminosity
    cmag*=(1.-coef);
    rmag*=global.StarScale/3;
    glColor3fv(RGB/MaxColorValue*cmag);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(XY[0],global.Y_Resolution-XY[1],0);
    glBegin(GL_QUADS );
        glTexCoord2i(0,0);    glVertex3f(-rmag,-rmag,0.f);      //Bas Gauche
        glTexCoord2i(1,0);    glVertex3f(rmag,-rmag,0.f);       //Bas Droite
        glTexCoord2i(1,1);    glVertex3f(rmag,rmag,0.f);        //Haut Droit
        glTexCoord2i(0,1);    glVertex3f(-rmag,rmag,0.f);       //Haut Gauche
    glEnd ();
    glPopMatrix();
}

void Hip_Star::DrawName(void)
{   
    glColor3fv(RGB/2.5);
	starFont->print(XY[0]+6,global.Y_Resolution-XY[1]+6, CommonName);
}