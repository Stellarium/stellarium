/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chéreau
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

#include "navigator.h"
#include "stellarium.h"
#include "stel_utility.h"
#include "stel_object.h"
#include "stellastro.h"
#include "solarsystem.h"

observator_pos::observator_pos() : planet(3), longitude(0.), latitude(0.), time_zone(0), altitude(0)
{
	name = strdup("Anonymous_Location");
}

observator_pos::~observator_pos()
{
	if (name) delete name;
}

void observator_pos::save(FILE * f)
{
	// Set the position values in config file format
	char * tempLatitude=strdup(print_angle_dms(latitude));
	char * tempLongitude=strdup(print_angle_dms(longitude));
	fprintf(f,"%s %s %d %d %s\n",tempLongitude, tempLatitude, altitude, time_zone, name);
	printf("SAVE location : %s %s %d %d %s\n",tempLongitude, tempLatitude, altitude, time_zone, name);
	if (tempLatitude) delete tempLatitude;
	if (tempLongitude) delete tempLongitude;
}

void observator_pos::load(FILE * f)
{
	char tempLatitude[20], tempLongitude[20], tempName[100];
	tempName[0]=0;
	fscanf(f,"%s %s %d %d %s\n",tempLongitude, tempLatitude, &altitude, &time_zone, tempName);
	name=strdup(tempName);
	printf("LOAD location : %s %s %d %d %s\n",tempLongitude, tempLatitude, altitude, time_zone, name);
    // set the read latitude and longitude
    longitude=get_dec_angle(tempLongitude);
    latitude=get_dec_angle(tempLatitude);
}


navigator::navigator() : fov(60.), deltaFov(0.), deltaAlt(0.), deltaAz(0.), move_speed(0.001),
	flag_traking(0), flag_lock_equ_pos(0), flag_auto_move(0), time_speed(JD_SECOND), JDay(0.)
{
	local_vision=Vec3d(1.,0.,0.);
	equ_vision=Vec3d(1.,0.,0.);
}

navigator::~navigator()
{
}

// Load the position info in the file name given
void navigator::load_position(char * fileName)
{
	FILE * f = NULL;
	f=fopen(fileName,"rt");
	if (!f)
	{
        printf("ERROR %s NOT FOUND\n",fileName);
        exit(-1);
	}
	printf("Loading location file... (%s)\n",fileName);
	position.load(f);

	fclose(f);
}

// Save the position info in the file name given
void navigator::save_position(char * fileName)
{
	FILE * f = NULL;
	f=fopen(fileName,"wt");
	if (!f)
	{
        printf("ERROR %s NOT FOUND\n",fileName);
        exit(-1);
	}
	position.save(f);
	fclose(f);
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of gluPerspective
void navigator::init_project_matrix(int w, int h, double zNear, double zFar)
{
	glMatrixMode(GL_PROJECTION);
	double f = 1./tan(fov*M_PI/360.);
	mat_projection = Mat4d(	f*h/w, 0., 0., 0.,
							0., f, 0., 0.,
							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);

	// Update the "projection on screen" matrices
	mat_earth_equ_to_screen = /*mat_projection*/lookAt()*mat_earth_equ_to_local;
	glGetIntegerv(GL_VIEWPORT,vect_viewport);
}

bool navigator::project_earth_equ_to_screen_check(const Vec3f& v, Vec3d& win)
{
	return project_earth_equ_to_screen(v, win) &&
	win[1]>vect_viewport[1] &&
	win[1]<vect_viewport[3] &&
	win[0]>vect_viewport[0] &&
	win[0]<vect_viewport[2];
}

bool navigator::project_earth_equ_to_screen(const Vec3f& v, Vec3d& win)
{
    gluProject(v[0],v[1],v[2],mat_earth_equ_to_screen,mat_projection,vect_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
//	static Vec4d u;
//	u[0] = v[0];
//	u[1] = v[1];
//	u[2] = v[2];
//	u[3] = 1.;

	//u.normalize();
	// Transform with projection and modelview matrices
//	u = mat_earth_equ_to_screen*u;

	// Convert in screen coordinate
//	win[0] = (1. + u[0]) * w / 2.;
//	win[1] = (1. + u[1]) * h / 2.;
//	win[2] = (1. + u[2]) / 2.;
}

bool navigator::project_earth_equ_to_screen(const Vec3d& v, Vec3d& win)
{
    gluProject(v[0],v[1],v[2],mat_earth_equ_to_screen,mat_projection,vect_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
	/*
	static Vec4d u;
	u[0] = v[0];
	u[1] = v[1];
	u[2] = v[2];
	u[3] = 1.;

	// Transform with projection and modelview matrices
	u = mat_earth_equ_to_screen*u;
	double u3r = 1./u[3];
	u[0]*=u3r;
	u[1]*=u3r;
	u[2]*=u3r;

	// Convert in screen coordinate
	win[0] = (1. + u[0]) * vect_viewport[2] / 2.;
	win[1] = (1. + u[1]) * vect_viewport[3] / 2.;
	win[2] = (1. + u[2]) / 2.;
	*/
}

void navigator::turn_right(int s)
{
	if (s)
	{
		deltaAz = 1;
		flag_traking = 0;
	}
	else deltaAz = 0;
}

void navigator::turn_left(int s)
{
	if (s)
	{
		deltaAz = -1;
		flag_traking = 0;
	}
	else deltaAz = 0;
}

void navigator::turn_up(int s)
{
	if (s)
	{
		deltaAlt = 1;
		flag_traking = 0;
	}
	else deltaAlt = 0;
}

void navigator::turn_down(int s)
{
	if (s)
	{
		deltaAlt = -1;
		flag_traking = 0;
	}
	else deltaAlt = 0;
}

void navigator::zoom_in(int s)
{
	deltaFov = -1*(s!=0);
}

void navigator::zoom_out(int s)
{
	deltaFov = (s!=0);
}


void navigator::update_vision_vector(int delta_time, stel_object* selected)
{
    if (flag_auto_move)
    {
		equ_vision = move.aim*move.coef;
        Vec3d temp = move.start*(1.0-move.coef);
        equ_vision+=temp;
        equ_vision.normalize();
        move.coef+=move.speed*delta_time;
        if (move.coef>=1.)
        {
			flag_auto_move=0;
            equ_vision=move.aim;
        }
		// Recalc local vision vector
		local_vision=earth_equ_to_local(&equ_vision);
    }
	else
	{
    	if (flag_traking && selected) // Equatorial vision vector locked on selected object
		{
			equ_vision=selected->get_earth_equ_pos(this);
			// Recalc local vision vector
			local_vision=earth_equ_to_local(&equ_vision);
		}
		else
		{
			if (flag_lock_equ_pos) // Equatorial vision vector locked
			{
				// Recalc local vision vector
				local_vision=earth_equ_to_local(&equ_vision);
			}
			else // Local vision vector locked
			{
				// Recalc equatorial vision vector
				equ_vision=local_to_earth_equ(&local_vision);
			}
		}
	}

	// Update now the deplacement
	update_move(delta_time);

}


void navigator::set_local_vision(Vec3d _pos)
{
	local_vision = _pos;
	equ_vision=local_to_earth_equ(&local_vision);
}

// Increment/decrement smoothly the vision field and position
void navigator::update_move(int delta_time)
{
	// the more it is zoomed, the more the mooving speed is low (in angle)
    double depl=move_speed*delta_time*fov;
    if (deltaAz<0)
    {
		deltaAz = -depl/30;
    }
    else
    {
		if (deltaAz>0)
        {
			deltaAz = (depl/30);
        }
    }
    if (deltaAlt<0)
    {
		deltaAlt = -depl/30;
    }
    else
    {
		if (deltaAlt>0)
        {
			deltaAlt = depl/30;
        }
    }
    if (deltaFov<0)
    {
		deltaFov = -depl*5;
    }
    else
    {
		if (deltaFov>0)
        {
			deltaFov = depl*5;
        }
    }

    // if we are zooming in or out
    if (deltaFov)
    {
		if (fov+deltaFov>0.001 && fov+deltaFov<100.)  fov+=deltaFov;
		if (fov+deltaFov>100) fov=100.;
		if (fov+deltaFov<0.001) fov=0.001;
    }

	double azVision, altVision;
	rect_to_sphe(&azVision,&altVision,&local_vision);

    // if we are mooving in the Azimuthal angle (left/right)
    if (deltaAz) azVision-=deltaAz;
    if (deltaAlt)
    {
		if (altVision+deltaAlt <= M_PI_2 && altVision+deltaAlt >= -M_PI_2) altVision+=deltaAlt;
    }

    // recalc all the position variables
	if (deltaFov || deltaAz || deltaAlt)
	{
    	sphe_to_rect(azVision, altVision, &local_vision);

    	// Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
    	equ_vision=local_to_earth_equ(&local_vision);
	}

}

// *********************************************************************
// Increment time
void navigator::update_time(int delta_time)
{
	JDay+=time_speed*(double)delta_time/1000.;
}


// The non optimized (more clear version is available on the CVS : before date 25/07/2003)
void navigator::update_transform_matrices(Vec3d earth_ecliptic_pos)
{
	mat_local_to_earth_equ = 	Mat4d::zrotation((get_apparent_sidereal_time(JDay)+position.longitude)*M_PI/180.) *
						Mat4d::yrotation((90.-position.latitude)*M_PI/180.);

	mat_earth_equ_to_local = mat_local_to_earth_equ.transpose();

	// These two next have to take into account the position of the observer on the earth
	mat_helio_to_earth_equ =  Mat4d::xrotation(get_mean_obliquity(JDay)*M_PI/180.) *
						Mat4d::translation(-earth_ecliptic_pos);

	Mat4d tmp = Mat4d::xrotation(-23.438855*M_PI/180.) *
				Mat4d::zrotation((position.longitude+get_mean_sidereal_time(JDay))*M_PI/180.) *
				Mat4d::yrotation((90.-position.latitude)*M_PI/180.);

	mat_local_to_helio = 	Mat4d::translation(earth_ecliptic_pos) *
							tmp *
							Mat4d::translation(Vec3d(0.,0., 6378.1/AU+(double)position.altitude/AU/1000));

	mat_helio_to_local = 	Mat4d::translation(Vec3d(0.,0.,-6378.1/AU-(double)position.altitude/AU/1000)) *
							tmp.transpose() *
							Mat4d::translation(-earth_ecliptic_pos);
}


// Home made gluLookAt(0., 0., 0.,local_vision[0],local_vision[1],local_vision[2],0.,0.,1.);
// to keep a better precision to prevent the shaking bug..
Mat4d navigator::lookAt(void)
{
	Vec3d f(local_vision);
	f.normalize();
	Vec3d s(f[1],-f[0],0.);
	Vec3d u(s^f);
	s.normalize();
	u.normalize();

	return Mat4d(s[0],u[0],-f[0],0.,
				s[1],u[1],-f[1],0.,
				s[2],u[2],-f[2],0.,
				0.,0.,0.,1.);
}

// Place openGL in earth equatorial coordinates
void navigator::switch_to_earth_equatorial(void)
{
	glLoadMatrixd(lookAt()*mat_earth_equ_to_local);
}

// Place openGL in heliocentric coordinates
void navigator::switch_to_heliocentric(void)
{
	glLoadMatrixd(lookAt()*mat_helio_to_local);
}

// Return the matrix which place openGL in heliocentric coordinates
// Function used to overide standard openGL transformation while planet drawing to prevent
// the boring shaking bug..
Mat4d navigator::get_switch_to_heliocentric_mat(void)
{
	return lookAt()*mat_helio_to_local;
}

// Place openGL in local viewer coordinates (Usually somewhere on earth viewing in a specific direction)
void navigator::switch_to_local(void)
{
	glLoadMatrixd(lookAt());
}

// Return the observer heliocentric position
Vec3d navigator::get_observer_helio_pos(void)
{
	Vec3d v(0.,0.,0.);
	return mat_local_to_helio*v;
}

// Transform vector from local coordinate to equatorial
Vec3d navigator::local_to_earth_equ(Vec3d * v)
{
	return mat_local_to_earth_equ*(*v);
}

// Transform vector from equatorial coordinate to local
Vec3d navigator::earth_equ_to_local(Vec3d * v)
{
	return mat_earth_equ_to_local*(*v);
}

// Transform vector from heliocentric coordinate to local
Vec3d navigator::helio_to_local(Vec3d* v)
{
	return mat_helio_to_local*(*v);
}

// Transform vector from heliocentric coordinate to local
Vec3d navigator::helio_to_earth_equ(Vec3d* v)
{
	return mat_helio_to_earth_equ*(*v);
}

// Transform vector from heliocentric coordinate to false equatorial : equatorial
// coordinate but centered on the observer position
Vec3d navigator::helio_to_earth_pos_equ(Vec3d* v)
{
	return mat_local_to_earth_equ*mat_helio_to_local*(*v);
}

// Move to the given equatorial position
void navigator::move_to(Vec3d _aim)
{
	move.aim=_aim;
    move.aim.normalize();
    move.aim*=2.;
    move.start=equ_vision;
    move.start.normalize();
    move.speed=0.001;
    move.coef=0.;
    flag_auto_move = true;
}



draw_utility::draw_utility() : fov(0.), screenW(0), screenH(0)
{
}

draw_utility::~draw_utility()
{
}

void draw_utility::set_params(double _fov, int _screenW, int _screenH)
{
	fov=_fov;
	screenW=_screenW;
	screenH=_screenH;
}

// Calc the x,y coord on the screen with the X,Y and Z pos
void draw_utility::project(float objx, float objy, float objz, double &x, double &y, double &z)
{
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    gluProject(objx,objy,objz,M,P,V,&x,&y,&z);
}

// Convert x,y screen pos into 3D vector
Vec3d draw_utility::unproject(double x ,double y)
{
    GLdouble objx[1];
    GLdouble objy[1];
    GLdouble objz[1];

    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

	gluUnProject(x,y,1.,M,P,V,objx,objy,objz);
	return Vec3d(objx[0],objy[0],objz[0]);
}

// Set the drawing mode in 2D. Use reset_perspective_projection() to reset
// previous projection mode
void draw_utility::set_orthographic_projection(void)
{
	glMatrixMode(GL_PROJECTION);	// switch to projection mode
    glPushMatrix();					// save previous matrix
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);	// set a 2D orthographic projection
    //glScalef(1, -1, 1);			// invert the y axis, down is positive
    //glTranslatef(0, -h, 0);		// move the origin from the bottom left corner to the upper left corner

	glMatrixMode(GL_MODELVIEW);		// Init the modeling matrice
    glPushMatrix();
    glLoadIdentity();
}

// Reset the previous projection mode after a call to set_orthographic_projection()
void draw_utility::reset_perspective_projection(void)
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
