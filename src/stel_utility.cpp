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

#include "malloc.h"
#include "stel_utility.h"
#include "stellarium.h"

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


void sphe_to_rect(double lng, double lat, Vec3d * v)
{
	const double cosLat = cos(lat);
    (*v)[0] = cos(lng) * cosLat;
    (*v)[1] = sin(lng) * cosLat;
	(*v)[2] = sin(lat);
}

void sphe_to_rect(double lng, double lat, double r, Vec3d *v)
{
	const double cosLat = cos(lat);
    (*v)[0] = cos(lng) * cosLat;
    (*v)[1] = sin(lng) * cosLat;
	(*v)[2] = sin(lat);
	(*v)*=r;
}

void sphe_to_rect(float lng, float lat, Vec3f * v)
{
	const double cosLat = cos(lat);
    (*v)[0] = cos(lng) * cosLat;
    (*v)[1] = sin(lng) * cosLat;
	(*v)[2] = sin(lat);
}

void rect_to_sphe(double *lng, double *lat, const Vec3d *v)
{
	double r = sqrt((*v)[0]*(*v)[0]+(*v)[1]*(*v)[1]+(*v)[2]*(*v)[2]);
    *lat = asin((*v)[2]/r);
    *lng = atan2((*v)[1],(*v)[0]);
}

void rect_to_sphe(float *lng, float *lat, const Vec3f *v)
{
	double r = sqrt((*v)[0]*(*v)[0]+(*v)[1]*(*v)[1]+(*v)[2]*(*v)[2]);
    *lat = asin((*v)[2]/r);
    *lng = atan2((*v)[1],(*v)[0]);
}


// Calc the x,y coord on the screen with the X,Y and Z pos
void Project(float objx,float objy,float objz,double & x ,double & y)
{
	double z;
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    gluProject(objx,objy,objz,M,P,V,&x,&y,&z);
}

// Calc the x,y coord on the screen with the X,Y and Z pos
void Project(float objx,float objy,float objz,double & x ,double & y, double & z)
{
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    gluProject(objx,objy,objz,M,P,V,&x,&y,&z);
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
    glPushMatrix(); 
    glLoadIdentity();
}

void resetPerspectiveProjection() 
{   
    glMatrixMode(GL_PROJECTION);    // set the current matrix to GL_PROJECTION
    glPopMatrix();                  // restore previous settings
    glMatrixMode(GL_MODELVIEW);     // get back to GL_MODELVIEW matrix
    glPopMatrix(); 
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




/* local types and macros */
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define iswhite(c)  ((c)== ' ' || (c)=='\t')

/*
[]------------------------------------------------------------------------[]
|  trim() & strip()                                                        |
|                                                                          |
|  strips trailing whitespaces from buf.                                   |
|                                                                          |
[]------------------------------------------------------------------------[]
*/
static char *trim(char *x)
{
    char *y;

    if(!x)
        return(x);
    y = x + strlen(x)-1;
    while (y >= x && iswhite(*y))
        *y-- = 0; /* skip white space */
    return x;
}


/*
[]------------------------------------------------------------------------[]
|                                                                          |
|   skipwhite()                                                            |
|   salta espacios en blanco                                               |
|                                                                          |
[]------------------------------------------------------------------------[]
*/
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
double get_dec_angle(char *s)
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
char * print_angle_dms(double location)
{
    static char buf[16];
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
    sec = 60.0 * (modf(location, &deg));
    if (sec <= 0.0)
        sec *= -1;
    sec = 60.0 * (modf(sec, &min));
    sprintf(buf,"%+.2dº%.2d'%.2f\"",(int)deg, (int) min, sec);
    return buf;
}

char * print_angle_dms_stel(double location)
{
    static char buf[16];
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
    sec = 60.0 * (modf(location, &deg));
    if (sec <= 0.0)
        sec *= -1;
    sec = 60.0 * (modf(sec, &min));
    sprintf(buf,"%+.2d\6%.2d'%.2f\"",(int)deg, (int) min, sec);
    return buf;
}

/* Obtains a human readable angle in the form: hhhmmmss.sss" */
char * print_angle_hms(double angle)
{
    static char buf[16];
    double hr = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
	while (angle<0) angle+=360;
	angle/=15.;
    min = 60.0 * (modf(angle, &hr));
    sec = 60.0 * (modf(min, &min));
    sprintf(buf,"%.2dh%.2dm%.2fs",(int)hr, (int) min, sec);
    return buf;
}
