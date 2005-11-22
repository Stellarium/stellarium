/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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
#include "s_gui.h"  //circle

#define RADIUS_STAR 1.

// Init Static variables
float Hip_Star::twinkle_amount = 10.f;
float Hip_Star::star_scale = 10.f;
float Hip_Star::star_mag_scale = 10.f;
float Hip_Star::names_brightness = 1.f;
tone_reproductor* Hip_Star::eye = NULL;
Projector* Hip_Star::proj = NULL;
bool Hip_Star::gravity_label = false;
int Hip_Star::nameFormat = 0;
s_font *Hip_Star::starFont = NULL;

Vec3f Hip_Star::circle_color = Vec3f(0.f,0.f,0.f);
Vec3f Hip_Star::label_color = Vec3f(.8f,.8f,.8f);

Hip_Star::Hip_Star() :
	HP(0),
	doubleStar(false),
	variableStar(false)
{
	CommonName = "";
	SciName = "";
	ShortSciName = "";
	OrigSciName = "";
}

Hip_Star::~Hip_Star()
{ 
}

string Hip_Star::get_info_string(const navigator * nav) const
{
	float tempDE, tempRA;

	Vec3d equatorial_pos = nav->prec_earth_equ_to_earth_equ(XYZ);
	rect_to_sphe(&tempRA,&tempDE,equatorial_pos);

	ostringstream oss;
	if (CommonName!="" || SciName!="")
	{
		oss << "Name : " << CommonName << string(CommonName == "" ? "" : " ");
		oss << string (SciName=="" ? "" : SciName);
			//translateGreek(SciName,false)); 
	}
	else 
	{
		oss << "HP " << HP << endl;
	}
	oss << endl;
	
	oss << "Cat : HP:";
	if (HP > 0)	oss << HP; else oss << "-";

	oss << endl;

	oss << "RA : " << print_angle_hms(tempRA*180./M_PI) << endl;
	oss << "DE : " << print_angle_dms_stel(tempDE*180./M_PI) << endl;

	oss << "Spectral : " << SpType << endl;
	oss.setf(ios::fixed);
	oss.precision(2);
	oss << "Magnitude : " << Mag << endl;
	oss << "Distance : ";
	
	oss.precision(1);
	if(Distance) oss << Distance; else oss << "-";
	oss << " Light Years" << endl;
	
	return oss.str();
}

string Hip_Star::get_short_info_string(const navigator * nav) const
{
	ostringstream oss;
	if (CommonName!="" || SciName!="")
	{
		if (CommonName == "") oss << SciName; else oss << CommonName; 
	}
	else 
		oss << "HP " << HP;

	oss.setf(ios::fixed);
	oss.precision(1);
	oss << ": mag " << Mag;
	if(Distance) oss << Distance << "ly";

	return oss.str();
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

bool Hip_Star::draw_name(void)
{   
	string starname;
	
	if (nameFormat == 0) // commonname
	{
		if (CommonName == "") return false;
		starname = CommonName;
	}
	else // nameFormat 1 & 2
	{
		if (SciName == "") return false;
		// format 1 only has the star scientific name
		// format 2 also has the constellation letters.
		if (nameFormat == 1)
			starname = ShortSciName;
		else
			starname = SciName;
//		starname = translateGreek(SciName, (nameFormat == 1));
		
		if (CommonName != "")
		{
			ostringstream oss;
			oss << starname << "(" + CommonName + ")";
			starname = oss.str();
		}
	}
    glColor3fv(RGB*0.75*names_brightness);
	gravity_label ? proj->print_gravity180(starFont, XY[0],XY[1], starname, 1, 6, -4) :
	starFont->print(XY[0]+6,XY[1]-4, starname);
	
	return true;
}
