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

/* puts a large angle in the correct range 0 - 2PI radians */
double range_radians (double angle)
{
    double temp;

    if (angle >= 0.0 && angle < (2.0 * M_PI))
    	return(angle);

	temp = (int)(angle / (M_PI * 2.0));

	if ( angle < 0.0 )
		temp --;
	temp *= (M_PI * 2.0);
	return angle - temp;
}

double hms_to_rad(unsigned int h, unsigned int m, double s)
{
	return (double)PI/24.*h*2.+(double)PI/12.*m/60.+s*PI/43200.;
}

double dms_to_rad(int d, int m, double s)
{
	return (double)PI/180.*d+(double)PI/10800.*m+s*PI/648000.;
}

void rad_to_hms(unsigned int * h, unsigned int * m, double * s, double r)
{
	*h = (int)(r*12./PI);
    *m = (int)(r*720./PI-h*60.);
    *s = r*43200./PI-m*60.-h*3600.;
}

void sphe_to_rect(double lng, double lat, const Vec3d * v)
{
	const double cosLat = cos(lat);
    v[0] = sin(lng) * cosLat;
    v[1] = sin(lat);
    v[2] = cos(lng) * cosLat;
}

void sphe_to_rect(float lng, float lat, Vec3f &v)
{
	const double cosLat = cos(lat);
    v[0] = sin(lng) * cosLat;
    v[1] = sin(lat);
    v[2] = cos(lng) * cosLat;
}

void rect_to_sphe(double &lng, double &lat, Vec3d *v)
{
	double xz_dist = sqrt(v[0]*v[0]+v[2]*v[2]);
    lat = atan2(v[1],xz_dist);
    lng = atan2(v[0],v[2]);
    lng = range_radians(lng);
}


void Equ_to_altAz(vec3_t &leVect, float raZen, float deZen)
{
	leVect.RotateY(raZen);
    leVect.RotateX(Astro::PI_OVER_TWO-deZen);
}

void AltAz_to_equ(vec3_t &leVect, float raZen, float deZen)
{
	leVect.RotateX(deZen - Astro::PI_OVER_TWO);
    leVect.RotateY(-raZen);
}

void Equ_to_altAz(Vec3d &leVect, double raZen, double deZen)
{
	leVect.RotateY(raZen);
    leVect.RotateX(Astro::PI_OVER_TWO-deZen);
}

void AltAz_to_equ(Vec3d &leVect, double raZen, double deZen)
{
	leVect.RotateX(deZen - Astro::PI_OVER_TWO);
    leVect.RotateY(-raZen);
}

// Calc the x,y coord on the screen with the X,Y and Z pos
void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y)
{
	double z;
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    gluProject(objx_i,objy_i,objz_i,M,P,V,&x,&y,&z);
}

// Calc the x,y coord on the screen with the X,Y and Z pos
void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y, double & z)
{
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


