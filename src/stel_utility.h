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

#ifndef _S_UTILITY_H_
#define _S_UTILITY_H_

// Utilities

#include <math.h>
#include "stellarium.h"
#include "vecmath.h"


// Convenient class which groups commonly used variables and functions
class draw_utility
{
public:
	draw_utility();
	virtual ~draw_utility();

	double fov;
	int screenW;
	int screenH;
	void set_params(double _fov, int _screenW, int _screenH);

	// OpenGL projections and camera setting
	void project(float objx_i,float objy_i,float objz_i,double & x ,double & y ,double & z);
	Vec3d unproject(double x ,double y);

	void set_orthographic_projection(void);
	void reset_perspective_projection(void);

private:
	// Used to store openGL matrices and vectors
	GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
};



// Angles and coordinate conversions
double hms_to_rad(unsigned int h, unsigned int m, double s);
double dms_to_rad(int d, int m, double s);

double hms_to_rad(unsigned int h, double m);
double dms_to_rad(int d, double m);

//void rad_to_hms(unsigned int *h, unsigned int *m, double *s, double r);
void sphe_to_rect(double lng, double lat, Vec3d *v);
void sphe_to_rect(double lng, double lat, double r, Vec3d *v);
void sphe_to_rect(float lng, float lat, Vec3f *v);
void rect_to_sphe(double *lng, double *lat, const Vec3d * v);
void rect_to_sphe(float *lng, float *lat, const Vec3f * v);

/* Obtains Latitude, Longitude, RA or Declination from a string. */
double get_dec_angle(char *s);

/* Obtains a human readable angle in the form: ddºmm'ss.ss" */
char * print_angle_dms(double location);

/* Obtains a human readable angle in the form: dd\6mm'ss.ss" */
char * print_angle_dms_stel(double location);

/* Obtains a human readable angle in the form: hhhmmmss.sss" */
char * print_angle_hms(double location);

#endif
