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

#include "stel_utility.h"
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
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.;
}

double dms_to_rad(int d, int m, double s)
{
	double t = (double)M_PI/180.*d+(double)M_PI/10800.*m+s*M_PI/648000.;
	return fabs(t)*d/abs(d);
}

double hms_to_rad(unsigned int h, double m)
{
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.;
}

double dms_to_rad(int d, double m)
{
	double t = (double)M_PI/180.*d+(double)M_PI/10800.*m;
	return fabs(t)*d/abs(d);
}
/*
void rad_to_hms(int * h, int * m, double * s, double r)
{
	*h = (unsigned int)(r*12./PI);
    *m = (unsigned int)(r*720./PI-h*60.);
    *s = r*43200./PI-m*60.-h*3600.;
}*/

void sphe_to_rect(double lng, double lat, Vec3d * v)
{
	const double cosLat = cos(lat);
    (*v)[0] = sin(lng) * cosLat;
    (*v)[1] = sin(lat);
    (*v)[2] = cos(lng) * cosLat;
}

void sphe_to_rect(double lng, double lat, double r, Vec3d *v)
{
	const double cosLat = cos(lat);
    (*v)[0] = sin(lng) * cosLat;
    (*v)[1] = sin(lat);
    (*v)[2] = cos(lng) * cosLat;
	v->normalize();
	(*v)*=r;
}

void sphe_to_rect(float lng, float lat, Vec3f * v)
{
	const double cosLat = cos(lat);
    (*v)[0] = sin(lng) * cosLat;
    (*v)[1] = sin(lat);
    (*v)[2] = cos(lng) * cosLat;
}

void rect_to_sphe(double *lng, double *lat, const Vec3d *v)
{
	double xz_dist = sqrt((*v)[0]*(*v)[0]+(*v)[2]*(*v)[2]);
    *lat = atan2((*v)[1],xz_dist);
    *lng = atan2((*v)[0],(*v)[2]);
    *lng = range_radians(*lng);
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


// Convert x,y screen pos in 3D vector
Vec3d UnProject(double x ,double y)
{
	GLdouble M[16];
    GLdouble P[16];
    GLdouble objx[1];
    GLdouble objy[1];
    GLdouble objz[1];
    GLint V[4];

    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

	gluUnProject(x,(double)global.Y_Resolution-y,1.,M,P,V,objx,objy,objz);
	return Vec3d(*objx,*objy,*objz);
}



#if 0
// BORROWED FROM libnova GPL library


/* local types and macros */
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define iswhite(c)  ((c)== ' ' || (c)=='\t')

// strips trailing whitespaces from buf.
static char *trim(char *x)
{
    char *y;

    if(!x)
        return(x);
    y = x + strlen(x)-1;
    while (y >= x && isspace(*y))
        *y-- = 0; /* skip white space */
    return x;
}


// salta espacios en blanco                                               |
static void skipwhite(char **s)
{
   while(iswhite(**s))
        (*s)++;
}


/*! \fn double get_dec_location(char * s)
* \param s Location string
* \return angle in degrees
*
* Obtains Latitude, Longitude, RA or Declination from a string.
*
*  If the last char is N/S doesn't accept more than 90 degrees.
*  If it is E/W doesn't accept more than 180 degrees.
*  If they are hours don't accept more than 24:00
*
*  Any position can be expressed as follows:
*  (please use a 8 bits charset if you want
*  to view the degrees separator char '0xba')
*
*  42.30.35,53
*  90º0'0,01 W
*  42º30'35.53 N
*  42º30'35.53S
*  42º30'N
*  - 42.30.35.53
*   42:30:35.53 S
*  + 42.30.35.53
*  +42º30 35,53
*   23h36'45,0
*
*
*  42:30:35.53 S = -42º30'35.53"
*  + 42 30.35.53 S the same previous position, the plus (+) sign is
*  considered like an error, the last 'S' has precedence over the sign
*
*  90º0'0,01 N ERROR: +- 90º0'00.00" latitude limit
*
*/
double get_dec_location(char *s)
{

	char *ptr, *dec, *hh;
	BOOL negative = FALSE;
	char delim1[] = " :.,;ºDdHhMm'\n\t";
	char delim2[] = " NSEWnsew\"\n\t";
	int dghh = 0, minutes = 0;
	double seconds = 0.0, pos;
        short count;

	enum _type{
    	    HOURS, DEGREES, LAT, LONG
	}type;

	if (s == NULL || !*s)
		return(-0.0);
	count = strlen(s) + 1;
	if ((ptr = (char *) alloca(count)) == NULL)
		return (-0.0);
	memcpy(ptr, s, count);
	trim(ptr);
	skipwhite(&ptr);

        /* the last letter has precedence over the sign */
	if (strpbrk(ptr,"SsWw") != NULL)
		negative = TRUE;

	if (*ptr == '+' || *ptr == '-')
		negative = (char) (*ptr++ == '-' ? TRUE : negative);
	skipwhite(&ptr);
	if ((hh = strpbrk(ptr,"Hh")) != NULL && hh < ptr + 3)
            type = HOURS;
        else
            if (strpbrk(ptr,"SsNn") != NULL)
		type = LAT;
	    else
 	        type = DEGREES; /* unspecified, the caller must control it */

	if ((ptr = strtok(ptr,delim1)) != NULL)
		dghh = atoi (ptr);
	else
		return (-0.0);

	if ((ptr = strtok(NULL,delim1)) != NULL) {
		minutes = atoi (ptr);
		if (minutes > 59)
		    return (-0.0);
	}else
		return (-0.0);

	if ((ptr = strtok(NULL,delim2)) != NULL) {
		if ((dec = strchr(ptr,',')) != NULL)
			*dec = '.';
		seconds = strtod (ptr, NULL);
		if (seconds > 59)
			return (-0.0);
	}

	if ((ptr = strtok(NULL," \n\t")) != NULL) {
		skipwhite(&ptr);
		if (*ptr == 'S' || *ptr == 'W' || *ptr == 's' || *ptr == 'W')
			    negative = TRUE;
	}
        pos = dghh + minutes /60.0 + seconds / 3600.0;

	if (type == HOURS && pos > 24.0)
		return (-0.0);
	if (type == LAT && pos > 90.0)
		return (-0.0);
	else
	    if (pos > 180.0)
		return (-0.0);

	if (negative)
		pos = 0.0 - pos;

	return (pos);

}


/*! \fn char * get_humanr_location(double location)
* \param location Location angle in degress
* \return Angle string
*
* Obtains a human readable location in the form: ddºmm'ss.ss"
*/
char *get_humanr_location(double location)
{
    static char buf[16];
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
    sec = 60.0 * (modf(location, &deg));
    if (sec < 0.0)
        sec *= -1;
    sec = 60.0 * (modf(sec, &min));
    sprintf(buf,"%+dº%d'%.2f\"",(int)deg, (int) min, sec);
    return buf;
}

/*! \fn void get_date (double JD, struct ln_date * date)
* \param JD Julian day
* \param date Pointer to new calendar date.
*
* Calculate the date from the Julian day
*/
void get_date (double JD, struct ln_date * date)
{
   int A,a,B,C,D,E;
   double F,Z;

   JD += 0.5;
   Z = (int) JD;
   F = JD - Z;

   if (Z < 2299161)
   {
       A = Z;
   }
   else
   {
       a = (int) ((Z - 1867216.25) / 36524.25);
       A = Z + 1 + a - (int)(a / 4);
   }

   B = A + 1524;
   C = (int) ((B - 122.1) / 365.25);
   D = (int) (365.25 * C);
   E = (int) ((B - D) / 30.6001);

   /* get the hms */
   date->hours = F * 24;
   F -= (double)date->hours / 24;
   date->minutes = F * 1440;
   F -= (double)date->minutes / 1440;
   date->seconds = F * 86400;

   /* get the day */
   date->days = B - D - (int)(30.6001 * E);

   /* get the month */
   if (E < 14)
   {
       date->months = E - 1;
   }
   else
   {
       date->months = E - 13;
   }

   /* get the year */
   if (date->months > 2)
   {
       date->years = C - 4716;
   }
   else
   {
       date->years = C - 4715;
   }
}
#endif
