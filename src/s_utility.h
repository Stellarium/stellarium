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

#if defined( MACOSX )
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <math.h>
#include "vector.h"

float RA_en_Rad(int RAh, int RAm, float RAs);
float RA_en_Rad(int RAh, float RAm);
float DE_en_Rad(bool DEsign,int Dec, int Arcmin, int Arcsec);
float DE_en_Rad(int Dec, float Arcmin);
void RA_en_hms(int & RAh, int & RAm, float & RAs, float RA);

void RADE_to_XYZ(double RA, double DE, vec3_t &XYZ);
void XYZ_to_RADE(double &RA, double &DE, vec3_t XYZ);

void Equ_to_altAz(vec3_t &leVect, float raZen, float deZen);
void AltAz_to_equ(vec3_t &leVect, float raZen, float deZen);

void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y);

void setOrthographicProjection(int w, int h);
void resetPerspectiveProjection();
void renderBitmapString(float x, float y, void *font,char *string);

#endif
