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
#include "vecmath.h"

double hms_to_rad(unsigned int h, unsigned int m, double s);
double dms_to_rad(int sign, int d, int m, double s);
void rad_to_hms(unsigned int *h, unsigned int *m, double *s, double r);
void sphe_to_rect(double lng, double lat, Vec3d *v);
void sphe_to_rect(float lng, float lat, Vec3f *v);
void rect_to_sphe(double *lng, double *lat, const Vec3d * v);

void Equ_to_altAz(vec3_t &leVect, float raZen, float deZen);
void AltAz_to_equ(vec3_t &leVect, float raZen, float deZen);
void Equ_to_altAz(Vec3d &leVect, double raZen, double deZen);
void AltAz_to_equ(Vec3d &leVect, double raZen, double deZen);

void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y);
void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y ,double & z);

void setOrthographicProjection(int w, int h);
void resetPerspectiveProjection();
//void renderBitmapString(float x, float y, void *font,char *string);

#endif
