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

#include "s_utility.h"
#include "stellarium.h"
#include "MathOps.h"

float RA_en_Rad(int RAh, int RAm, float RAs)
{   return (float)RAh*2*PI/24+(float)RAm*(2*PI/24)/60+RAs*((2*PI/24)/60)/60;
}

float RA_en_Rad(int RAh, float RAm)
{   return (float)RAh*2*PI/24+RAm*(2*PI/24)/60;
}

float DE_en_Rad(bool DEsign,int Deg, int Arcmin, int Arcsec)
{   float tps=(float)Deg*PI/180+(float)Arcmin*(PI/180)/60+(float)Arcsec*((PI/180)/60)/60;
    if (DEsign) return -1*tps; else return tps;
} 

float DE_en_Rad(int Deg, float Arcmin)
{   return (float)Deg*PI/180+Arcmin*(PI/180)/60;
} 

void RA_en_hms(int & RAh, int & RAm, float & RAs, float RA)
{   RAh=(int)(RA*12/PI);
    RAm=(int)(RA*720/PI-RAh*60);
    RAs=RA*43200/PI-RAm*60-RAh*3600;
}

void Equ_to_altAz(vec3_t &leVect, float raZen, float deZen)
{   leVect.RotateY(raZen);
    leVect.RotateX(Astro::PI_OVER_TWO-deZen);
}

void AltAz_to_equ(vec3_t &leVect, float raZen, float deZen)
{   leVect.RotateX(deZen - Astro::PI_OVER_TWO);
    leVect.RotateY(-raZen);
}

// Calc the x,y coord on the screen with the X,Y and Z pos
void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y)
{   double z;
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    gluProject(objx_i,objy_i,objz_i,M,P,V,&x,&y,&z);
}

// For text printing
void setOrthographicProjection(int w, int h)
{   glMatrixMode(GL_PROJECTION);    // switch to projection mode
    glPushMatrix();                 // save previous matrix which contains the settings for the perspective projection
    glLoadIdentity();               // reset matrix
    gluOrtho2D(0, w, 0, h);         // set a 2D orthographic projection
    glScalef(1, -1, 1);             // invert the y axis, down is positive
    glTranslatef(0, -h, 0);         // mover the origin from the bottom left corner to the upper left corner
    glMatrixMode(GL_MODELVIEW);
}

void resetPerspectiveProjection() 
{   glMatrixMode(GL_PROJECTION);    // set the current matrix to GL_PROJECTION
    glPopMatrix();                  // restore previous settings
    glMatrixMode(GL_MODELVIEW);     // get back to GL_MODELVIEW matrix
}

void renderBitmapString(float x, float y, void *font,char *string)
{   char *c;
    glRasterPos2f(x, y);            // set position to start drawing fonts
    for (c=string; *c != '\0'; c++) // loop all the characters in the string
    {   glutBitmapCharacter(font, *c);
    }
}

void RADE_to_XYZ(double RA, double DE, vec3_t &XYZ)
{   const double cosLat = cos( DE );
    XYZ[0] = sin( RA ) * cosLat;
    XYZ[1] = sin( DE );
    XYZ[2] = cos( RA ) * cosLat;
}

void XYZ_to_RADE(double &RA, double &DE, vec3_t XYZ)
{   float xz_dist = (float)sqrt(XYZ[0]*XYZ[0]+XYZ[2]*XYZ[2]);
    DE = atan2(XYZ[1],xz_dist);
    RA = atan2(XYZ[0],XYZ[2]);
    RA=AstroOps::normalizeRadians(RA);
}
