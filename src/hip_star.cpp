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
float HipStar::twinkle_amount = 10.f;
float HipStar::star_scale = 10.f;
float HipStar::star_mag_scale = 10.f;
float HipStar::names_brightness = 1.f;
ToneReproductor* HipStar::eye = NULL;
Projector* HipStar::proj = NULL;
bool HipStar::gravity_label = false;
int HipStar::nameFormat = 0;
s_font *HipStar::starFont = NULL;

Vec3f HipStar::circle_color = Vec3f(0.f,0.f,0.f);
Vec3f HipStar::label_color = Vec3f(.8f,.8f,.8f);
Vec3f HipStar::ChartColors[20] = 
{
	Vec3f(0.25,0.60,1.00) /* A+*/,	Vec3f(0.73,0.13,0.59) /* B+*/,	Vec3f(1.00,0.50,0.00) /* C-*/,
	Vec3f(0.00,0.00,0.00) /* Dx*/,	Vec3f(0.00,0.00,0.00) /* Ex*/,	Vec3f(0.15,0.85,0.00) /* F+*/,
	Vec3f(1.00,1.00,0.25) /* G+*/,	Vec3f(0.00,0.00,0.00) /* Hx*/,	Vec3f(0.00,0.00,0.00) /* Ix*/,
	Vec3f(0.00,0.00,0.00) /* Jx*/,	Vec3f(1.00,0.65,0.25) /* K+*/,	Vec3f(0.00,0.00,0.00) /* Lx*/,
	Vec3f(1.00,0.00,0.00) /* M */,	Vec3f(1.00,0.00,0.00) /* N+*/,	Vec3f(0.73,0.13,0.59) /* O+*/,
	Vec3f(0.00,0.00,0.00) /* Px*/,	Vec3f(0.00,0.00,0.00) /* Qx*/,	Vec3f(1.00,0.00,0.00) /* R+*/,
	Vec3f(0.00,0.00,0.00) /* S+*/,	Vec3f(0.50,0.50,0.50) /* Defualt */
};

HipStar::HipStar() :
	HP(0),
	HD(0),
	SAO(0),
	doubleStar(false),
	variableStar(false)
{
	CommonName = "";
	SciName = "";
	ShortSciName = "";
	OrigSciName = "";
}

HipStar::~HipStar()
{ 
}

string HipStar::get_info_string(const Navigator * nav) const
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
		if (HP) oss << "HP " << HP;
#ifdef DATA_FILES_USE_SAO
		else oss << "SAO" << SAO;
#endif
	}
	if (doubleStar) oss << "(Dbl)";
	else if (variableStar) oss << "(Var)";
	oss << endl;
	
	oss << "Cat : HP:";
	if (HP > 0)	oss << HP; else oss << "-";

#ifdef DATA_FILES_USE_SAO
	oss << "  HD:";
	if (HD > 0)	oss << HD; else oss << "-";

	oss << "  SAO:";
	if (SAO > 0)	oss << SAO; else oss << "-";
#endif
	oss << endl;

	oss << "RA : " << StelUtility::printAngleHMS(tempRA) << endl;
	oss << "DE : " << StelUtility::printAngleDMS(tempDE) << endl;

	oss << "Spectral : " << SpType << endl;
	oss.setf(ios::fixed);
	oss.precision(2);
	oss << "Magnitude : " << Mag << endl;
	oss << "Distance : ";
	
	oss.precision(1);
	if(Distance) oss << Distance; else oss << "-";
	oss << " Light Years" << endl;

	// calculate alt az
	Vec3d local_pos = nav->earth_equ_to_local(equatorial_pos);
	rect_to_sphe(&tempRA,&tempDE,local_pos);
	tempRA = 3*M_PI - tempRA;  // N is zero, E is 90 degrees
	if(tempRA > M_PI*2) tempRA -= M_PI*2;

	oss << "Az  : " << StelUtility::printAngleDMS(tempRA) << endl;
	oss << "Alt : " << StelUtility::printAngleDMS(tempDE) << endl;

	
	return oss.str();
}

string HipStar::get_short_info_string(const Navigator * nav) const
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
int HipStar::read(FILE * catalog)
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

	setColor(SpType); // Color depending on the spectral type

	// distance
	float LY;
	fread(&LY,4,1,catalog);
	LE_TO_CPU_FLOAT(Distance, LY);

	if (mag==0 && type==0) return 0;

	//	printf("%d\t%d\t%.4f\t%.4f\t%c\n", HP, mag, rao, deo, SpType);

    return 1;

}

void HipStar::draw(void)
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

void HipStar::draw_point(void)
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

bool HipStar::draw_name(void)
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
	
	if (draw_mode == DM_NORMAL) glColor3fv(RGB*0.75*names_brightness);
	else glColor3fv(label_color);

	gravity_label ? proj->print_gravity180(starFont, XY[0],XY[1], starname, 1, 6, -4) :
	starFont->print(XY[0]+6,XY[1]-4, starname);
	
	return true;
}

void HipStar::draw_chart(void)
{
	float r = Mag*-0.81+8.85;

	// multiplier = 1 at fov = 35, and 1/2 at fov=180
	r = r *((.5-1)/(180-35)*proj->get_fov()+(1-35*((.5-1)/(180-35))));

	if (r < 2.4) return;
	
    glColor3fv(ChartColors[ChartColorIndex]);

//	glBindTexture (GL_TEXTURE_2D, idcTex);
   	glBegin(GL_QUADS);
        glTexCoord2i(0,0);    glVertex2f(XY[0]-r,XY[1]-r);	// Bottom left
   	    glTexCoord2i(1,0);    glVertex2f(XY[0]+r,XY[1]-r);	// Bottom right
       	glTexCoord2i(1,1);    glVertex2f(XY[0]+r,XY[1]+r);	// Top right
        glTexCoord2i(0,1);    glVertex2f(XY[0]-r,XY[1]+r);	// Top left
    glEnd();

   	glColor3fv(circle_color);
	glCircle(XY,r);
	if (variableStar)
		glCircle(XY,(r-2));
	else if (doubleStar)
	{
		bool lastState = glIsEnabled(GL_TEXTURE_2D);

		glDisable(GL_TEXTURE_2D);
		glLineWidth(1.f);
			
		glBegin(GL_LINE_LOOP);
			glVertex3f(XY[0] - r - 4, XY[1],0.f);
			glVertex3f(XY[0] - r, XY[1],0.f);
		glEnd();
			
		glBegin(GL_LINE_LOOP);
			glVertex3f(XY[0] + r, XY[1],0.f);
			glVertex3f(XY[0] + r+4, XY[1],0.f);
		glEnd();
			
		if (lastState) glEnable(GL_TEXTURE_2D);
		glLineWidth(1.0f);
	}
}

bool HipStar::readSAO(char *record, float maxmag)
{
	int RAh, RAm, DEd, DEm;
	float RAs, DEs;
	double DE, RA;
	int num;
	
	// filter the magnitude
	sscanf(&record[103],"%f",&Mag);
	if (Mag < -1) Mag = 14; 			// trap -9.99's
	if (Mag > maxmag) return false;

	sscanf(&record[0],"%d",&SAO);
	sscanf(&record[11],"%d",&num);
	if (num > 0) HD = num;
	
	sscanf(&record[37],"%d %d %f",&RAh, &RAm, &RAs);
	sscanf(&record[52],"%d %d %f",&DEd, &DEm, &DEs);
	RA = (double)RAh+(float)RAm/60.+(float)RAs/3600.;
	DE = (double)DEd+(float)DEm/60.+(float)DEs/3600.;
	if (record[51] == '-') DE *= -1.;

	RA*=M_PI/12.;     // Convert from hours to rad
	DE*=M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
    sphe_to_rect(RA,DE,XYZ);
	XYZ *= RADIUS_STAR;

	SpType = record[143];
	setColor(SpType); // Color depending on the spectral type

	Distance = 0; // not in file (no parallex)

	return true;
}

bool HipStar::readHP(char *record, float maxmag)
{
	int RAh, RAm, DEd, DEm;
	float RAs, DEs;
	double DE, RA;
	int num;
	float par;
	
	sscanf(&record[2],"%d",&HP);

	// filter the magnitude
	sscanf(&record[351],"%f",&Mag);
	if (Mag < -2) Mag = 14;				// trap -9.99's, sirius = -1.03!
	if (Mag > maxmag)
		return false;

	sscanf(&record[465],"%d",&num);
	if (num > 0) HD = num;

	sscanf(&record[14],"%d %d %f",&RAh, &RAm, &RAs);
	sscanf(&record[28],"%d %d %f",&DEd, &DEm, &DEs);
	RA = (double)RAh+(float)RAm/60.+(float)RAs/3600.;
	DE = (double)DEd+(float)DEm/60.+(float)DEs/3600.;
	if (record[27] == '-') DE *= -1.;
	
	RA*=M_PI/12.;     // Convert from hours to rad
	DE*=M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
    sphe_to_rect(RA,DE,XYZ);
	XYZ *= RADIUS_STAR;

	SpType = record[516];
	setColor(SpType); // Color depending on the spectral type

	// Determine the distance from the parallax
	sscanf(&record[87],"%f",&par);
	if (par <= 0.001)
		Distance = 0;
	else
	{
		Distance = 1.f*3261.64/par;
		if (Distance < 0) Distance = 0;
	}

	return true;
}

// Color depending on the spectral type

void HipStar::setColor(char sp)
{
	// Normal star colors
    switch (sp)             // Color depending on the spectral type
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

	// Precomputation of a term used later
	term1 = expf(-0.92103f*(Mag + 12.12331f)) * 108064.73f;

    MaxColorValue = MY_MAX(RGB[0],RGB[2]);

	// chart star color
    switch (sp)             // Color depending on the spectral type
    {
        case 'O':   ChartColorIndex = 14;  break;
        case 'B':	ChartColorIndex = 1; break;
        case 'A': 	ChartColorIndex = 0; break;
        case 'F':	ChartColorIndex = 5; break;
        case 'G':	ChartColorIndex = 6; break;
        case 'K':	ChartColorIndex = 10; break;
        case 'M':	ChartColorIndex = 12; break;
        case 'C':	ChartColorIndex = 2; break;
        case 'R':	ChartColorIndex = 17; break;
        case 'N':	ChartColorIndex = 13; break;
		case 'S':	ChartColorIndex = 18; break;
        default :	ChartColorIndex = 20; break;
    }
}
