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
#include "stellarium.h"
#include "navigator.h"
#include "stellastro.h"
#include "stel_utility.h"

#define RADIUS_STAR 1.

// Init Static variables
float Hip_Star::twinkle_amount = 10.f;
float Hip_Star::star_scale = 10.f;
tone_reproductor* Hip_Star::eye = NULL;
Projector* Hip_Star::proj = NULL;

Hip_Star::Hip_Star() : 
	CommonName(NULL),
    Name(NULL)
{
}

Hip_Star::~Hip_Star()
{   
	if (Name) delete Name;
	if (CommonName) delete CommonName;
}

void Hip_Star::get_info_string(char * s, const navigator * nav) const
{
	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,XYZ);
	sprintf(s,"Name :%s %s\nHip : %.4d\nRA : %s\nDE : %s\nMag : %.2f",
		CommonName==NULL ? "-" : CommonName,
		Name==NULL ? "-" : Name, HP, print_angle_hms(tempRA*180./M_PI),
			print_angle_dms_stel(tempDE*180./M_PI), Mag);
}

int Hip_Star::read(FILE * catalog)
// Read datas in binary catalog and compute x,y,z;
{  
    float RA=0, DE=0;
    fread((char*)&RA,4,1,catalog);
    LE_TO_CPU_FLOAT(RA, RA);
    
    fread((char*)&DE,4,1,catalog);
    LE_TO_CPU_FLOAT(DE, DE);
    
    RA*=M_PI/12.;     // Convert from hours to rad
    DE*=M_PI/180.;    // Convert from deg to rad

    unsigned short int mag;
    fread((char*)&mag,2,1,catalog);
    LE_TO_CPU_INT16(mag, mag);

    Mag = (5. + mag) / 256.0;
    if (Mag>250) Mag = Mag - 256;

    unsigned short int type;
    fread((char*)&type,2,1,catalog);
	LE_TO_CPU_INT16(type, type);

    // Calc the Cartesian coord with RA and DE
    sphe_to_rect(RA,DE,XYZ);

    XYZ*=RADIUS_STAR;

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
    

    MaxColorValue = myMax(RGB[0],RGB[2]);

	// Precomputation of a term used later
	term1 = expf(-0.92103f*(Mag + 12.12331f)) * 108064.73f;

	if (mag==0 && type==0)
	{
		return 0;
	}
    return 1;

}


void Hip_Star::draw(void)
{
	static float cmag;
	static float rmag;

    // Compute the equivalent star luminance for a 5 arc min circle and convert it
	// in function of the eye adaptation
	rmag = eye->adapt_luminance(term1);
	rmag = rmag/powf(proj->get_fov(),0.85f)*50.f;

    cmag = 1.f;

    // if size of star is too small (blink) we put its size to 1.2 --> no more blink
    // And we compensate the difference of brighteness with cmag
    if (rmag<1.2f)
    {
        if (rmag<0.3f) return;
        cmag=rmag*rmag/1.44f;
        rmag=1.2f;
    }
	else
    {
		if (rmag>5.f)
    	{
        	rmag=5.f;
    	}
	}

    // Calculation of the luminosity
    // Random coef for star twinkling
    cmag*=(1.-rand()/RAND_MAX*twinkle_amount);
	// Global scaling
	rmag*=star_scale;

    glColor3fv(RGB*(cmag/MaxColorValue));
    glBegin(GL_QUADS );
        glTexCoord2i(0,0);    glVertex2f(XY[0]-rmag,XY[1]-rmag);	// Bottom left
        glTexCoord2i(1,0);    glVertex2f(XY[0]+rmag,XY[1]-rmag);	// Bottom right
        glTexCoord2i(1,1);    glVertex2f(XY[0]+rmag,XY[1]+rmag);	// Top right
        glTexCoord2i(0,1);    glVertex2f(XY[0]-rmag,XY[1]+rmag);	// Top left
    glEnd();
}

void Hip_Star::draw_name(const s_font* star_font)
{   
    glColor3fv(RGB*(1./2.5));
	star_font->print(XY[0]+6,XY[1]-4, CommonName);
}
