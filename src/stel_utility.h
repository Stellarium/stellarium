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

// OpenGL projections and camera setting
void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y);
void Project(float objx_i,float objy_i,float objz_i,double & x ,double & y ,double & z);
Vec3d UnProject(double x ,double y);

void setOrthographicProjection(int w, int h);
void resetPerspectiveProjection();
//void renderBitmapString(float x, float y, void *font,char *string);

// CODE BORROWED FROM LIBNOVA

#if 0

/*!
** Date
* \struct ln_date
* \brief Human readable Date and time used by libnova
*
* This is the Human readable (easy printf) date format used
* by libnova.
*/

struct ln_date
{
    int years; 		/*!< Years. All values are valid */
    int months;		/*!< Months. Valid values : 1 (January) - 12 (December) */
    int days; 		/*!< Days. Valid values 1 - 28,29,30,31 Depends on month.*/
    int hours; 		/*!< Hours. Valid values 0 - 23. */
    int minutes; 	/*!< Minutes. Valid values 0 - 59. */
    double seconds;	/*!< Seconds. Valid values 0 - 59.99999.... */
};

/*! \fn void get_date (double JD, struct ln_date * date)
* \ingroup calendar
* \brief Calculate the date from the julian day.
*/
void get_date (double JD, struct ln_date * date);

/*! \fn double get_dec_location(char * s)
* \ingroup misc
* \brief Obtains Latitude, Longitude, RA or Declination from a string.
*/
double get_dec_location(char *s);

/*! \fn char * get_humanr_location(double location)
*  \ingroup misc
*  \brief Obtains a human readable location in the form: ddºmm'ss.ss"
*/
char *get_humanr_location(double location);

#endif

#endif
