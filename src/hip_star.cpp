/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�eau
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
float Hip_Star::star_mag_scale = 10.f;
float Hip_Star::names_brightness = 1.f;
tone_reproductor* Hip_Star::eye = NULL;
Projector* Hip_Star::proj = NULL;
bool Hip_Star::gravity_label = false;

Hip_Star::Hip_Star()
{
}

Hip_Star::~Hip_Star()
{ 
}

void Hip_Star::get_info_string(char * s, const navigator * nav) const
{
	static char tempStr[20];
	sprintf(tempStr,"HP %d",HP);

	static char tempLy[20];
	if(Distance) sprintf(tempLy, "LY : %.1f", Distance);
	else tempLy[0] = 0; 

	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,XYZ);
	// Hip = Hipparcos, RA=Right Ascention, DE=Declinaison, Mag=Magnitude
	sprintf(s,_("Name : %s%s%s\nHip : %.4d\nRA : %s\nDE : %s\nMag : %.2f\n%s"),
		CommonName.c_str(), CommonName=="" ? "" : " ",
		SciName=="" ? tempStr : SciName.c_str(), HP, print_angle_hms(tempRA*180./M_PI).c_str(),
			print_angle_dms_stel(tempDE*180./M_PI).c_str(), Mag, tempLy);
}

void Hip_Star::get_short_info_string(char * s, const navigator * nav) const
{
	static char tempStr[20];
	sprintf(tempStr,"HP %d",HP);

	static char tempLy[20];
	if(Distance) sprintf(tempLy, "%.1f ly", Distance);
	else tempLy[0] = 0; 

	if (CommonName!="" || SciName!="") sprintf(s,_("%s: mag %.1f %s"), CommonName=="" ? SciName.c_str() : CommonName.c_str(), Mag, tempLy);
	else sprintf(s,_("%s: mag %.1f %s"), tempStr, Mag, tempLy);
}

// Read datas in binary catalog and compute x,y,z;
// The aliasing bug on some architecture has been fixed by Rainer Canavan on 26/11/2003
int Hip_Star::read(FILE * catalog)
{
	float RA=0, DE=0, xDE, xRA;
	fread(&xRA,4,1,catalog);
	LE_TO_CPU_FLOAT(RA, xRA);

	fread(&xDE,4,1,catalog);
	LE_TO_CPU_FLOAT(DE, xDE);

	// for debug printing
	//	float rao = RA;
	//  float deo = DE;
     
    RA*=M_PI/12.;     // Convert from hours to rad
    DE*=M_PI/180.;    // Convert from deg to rad

	unsigned short int mag, xmag;
	fread(&xmag,2,1,catalog);
	LE_TO_CPU_INT16(mag, xmag);

	Mag = (5. + mag) / 256.0;
	if (Mag>250) Mag = Mag - 256;

	unsigned char type;
	fread(&type,1,1,catalog);
	//	LE_TO_CPU_INT16(type, xtype);

	// Calc the Cartesian coord with RA and DE
    sphe_to_rect(RA,DE,XYZ);

    XYZ*=RADIUS_STAR;

    switch(type)
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
    

    MaxColorValue = MY_MAX(RGB[0],RGB[2]);

	// Precomputation of a term used later
	term1 = expf(-0.92103f*(Mag + 12.12331f)) * 108064.73f;

	// distance
	float LY;
	fread(&LY,4,1,catalog);
	LE_TO_CPU_FLOAT(Distance, LY);


	if (mag==0 && type==0) return 0;

	//	printf("%d\t%d\t%.4f\t%.4f\t%c\n", HP, mag, rao, deo, SpType);

    return 1;

}


void Hip_Star::draw(void)
{
    // Compute the equivalent star luminance for a 5 arc min circle and convert it
	// in function of the eye adaptation
	float rmag = eye->adapt_luminance(term1);
	rmag/=powf(proj->get_fov(),0.85f);
	rmag*=70.f;
	float cmag = 1.f;
	
    // if size of star is too small (blink) we put its size to 1.2 --> no more blink
    // And we compensate the difference of brighteness with cmag
    if (rmag<1.2f)
    {
        cmag=rmag*rmag/1.44f;
		if (rmag/star_scale<0.1f || cmag<0.1/star_mag_scale) return;
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
    cmag*=(1.-twinkle_amount*rand()/RAND_MAX);

	// Global scaling
	rmag*=star_scale;
	cmag*=star_mag_scale;

    glColor3fv(RGB*(cmag/MaxColorValue));
    glBegin(GL_QUADS );
        glTexCoord2i(0,0);    glVertex2f(XY[0]-rmag,XY[1]-rmag);	// Bottom left
        glTexCoord2i(1,0);    glVertex2f(XY[0]+rmag,XY[1]-rmag);	// Bottom right
        glTexCoord2i(1,1);    glVertex2f(XY[0]+rmag,XY[1]+rmag);	// Top right
        glTexCoord2i(0,1);    glVertex2f(XY[0]-rmag,XY[1]+rmag);	// Top left
    glEnd();
}

void Hip_Star::draw_point(void)
{
	float cmag;
	float rmag;

    // Compute the equivalent star luminance for a 5 arc min circle and convert it
	// in function of the eye adaptation
	rmag = eye->adapt_luminance(term1);
	rmag = rmag/powf(proj->get_fov(),0.85f)*50.f;

    // if size of star is too small (blink) we put its size to 1.2 --> no more blink
    // And we compensate the difference of brighteness with cmag
    cmag = rmag * rmag / 1.44f;

	if (rmag/star_scale<0.05f || cmag<0.05/star_mag_scale) return;

    // Calculation of the luminosity
    // Random coef for star twinkling
    cmag*=(1.-twinkle_amount*rand()/RAND_MAX);
	cmag*=star_mag_scale;
    glColor3fv(RGB*(cmag/MaxColorValue));

	// rms - one pixel stars
	glDisable(GL_TEXTURE_2D);
	glPointSize(0.1);
	glBegin(GL_POINTS);
	glVertex3f(XY[0],XY[1],0);
	glEnd();
	glEnable(GL_TEXTURE_2D); // required for star labels to work
}

void Hip_Star::draw_name(const s_font* star_font)
{   
    glColor3fv(RGB*0.75*names_brightness);
	gravity_label ? proj->print_gravity180(star_font, XY[0],XY[1], CommonName, 1, 6, -4) :
	star_font->print(XY[0]+6,XY[1]-4, CommonName);
}
