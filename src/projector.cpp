/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#include <iostream>
#include <cstdio>
#include "projector.h"


Projector::Projector(int _screenW, int _screenH, double _fov, double _min_fov, double _max_fov) :
	min_fov(_min_fov), max_fov(_max_fov), zNear(0.1), zFar(10000), flag_auto_zoom(0), hoffset(0), voffset(0)
{
	fov = _fov;
	if (_fov>max_fov) fov = max_fov;
	if (_fov<min_fov) fov = min_fov;
	set_viewport(_screenW, _screenH, 0, 0);
	set_fov(_fov);
	set_screen_size(_screenW,_screenH);
	maximize_viewport();
}

Projector::~Projector()
{
}

void Projector::set_fov(double f)
{
	fov = f;
	if (f>max_fov) fov = max_fov;
	if (f<min_fov) fov = min_fov;
	init_project_matrix();
}


void Projector::set_viewport_offset(int _xoff, int _yoff)
{
  hoffset = _xoff;
  voffset = _yoff;
}

void Projector:: get_viewport_offset(int &_xoff, int &_yoff)
{
  _xoff = hoffset;
  _yoff = voffset;

}


void Projector::set_square_viewport(void)
{
	glDisable(GL_STENCIL_TEST);
	int mind = MY_MIN(screenW,screenH);
	set_viewport(hoffset + (screenW-mind)/2, voffset + (screenH-mind)/2, mind, mind);
	viewport_type = SQUARE;
}

void Projector::set_disk_viewport(void)
{
  set_square_viewport();

  // NOTE - no longer use stencil buffer (needed for other purposes)

  glClear(GL_COLOR_BUFFER_BIT);

  viewport_type = DISK;
}

void Projector::set_viewport_type(VIEWPORT_TYPE t)
{
	switch (t)
	{
		case SQUARE :
			set_square_viewport(); break;
		case DISK :
			set_disk_viewport(); break;
		case MAXIMIZED :
		default :
			maximize_viewport(); break;
	}
}

// Fill with black around the circle
void Projector::draw_viewport_shape(void)
{
	if (viewport_type != DISK) return;

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	set_2Dfullscreen_projection();
	glTranslatef(hoffset+screenW/2,voffset+screenH/2,0.f);
	GLUquadricObj * p = gluNewQuadric();
	gluDisk(p, MY_MIN(screenW,screenH)/2, screenW+screenH, 256, 1);  // should always cover whole screen
	gluDeleteQuadric(p);
	restore_from_2Dfullscreen_projection();
}

void Projector::set_viewport(int x, int y, int w, int h)
{
	glDisable(GL_STENCIL_TEST);
	vec_viewport[0] = x;
	vec_viewport[1] = y;
	vec_viewport[2] = w;
	vec_viewport[3] = h;
	glViewport(x, y, w, h);
	ratio = (float)h/w;
	init_project_matrix();
}

void Projector::set_screen_size(int w, int h)
{
	screenW = w;
	screenH = h;
}

void Projector::set_clipping_planes(double znear, double zfar)
{
	zNear = znear;
	zFar = zfar;
	init_project_matrix();
}

void Projector::change_fov(double deltaFov)
{
  // if we are zooming in or out
  if (deltaFov) set_fov(fov+deltaFov);
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of gluPerspective
void Projector::init_project_matrix(void)
{
	double f = 1./tan(fov*M_PI/360.);
	mat_projection = Mat4d(	f*ratio, 0., 0., 0.,
							0., f, 0., 0.,
							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
}

void Projector::update_openGL(void) const
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
	glViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], vec_viewport[3]);
}

// Set the standard modelview matrices used for projection
void Projector::set_modelview_matrices(	const Mat4d& _mat_earth_equ_to_eye,
					const Mat4d& _mat_helio_to_eye,
					const Mat4d& _mat_local_to_eye,
					const Mat4d& _mat_prec_earth_equ_to_eye)
{
	mat_earth_equ_to_eye = _mat_earth_equ_to_eye;
	mat_prec_earth_equ_to_eye = _mat_prec_earth_equ_to_eye;
	mat_helio_to_eye = _mat_helio_to_eye;
	mat_local_to_eye = _mat_local_to_eye;

	inv_mat_earth_equ_to_eye = (mat_projection*mat_earth_equ_to_eye).inverse();
	inv_mat_helio_to_eye = (mat_projection*mat_helio_to_eye).inverse();
	inv_mat_local_to_eye = (mat_projection*mat_local_to_eye).inverse();	
}

// Update auto_zoom if activated
void Projector::update_auto_zoom(int delta_time)
{
	if (flag_auto_zoom)
	{
		// Use a smooth function
		double c;

		if( zoom_move.start > zoom_move.aim )
		{
			// slow down as approach final view
			c = 1 - (1-zoom_move.coef)*(1-zoom_move.coef)*(1-zoom_move.coef);
		}
		else
		{
			// speed up as leave zoom target
			c = (zoom_move.coef)*(zoom_move.coef)*(zoom_move.coef);
		}

		set_fov(zoom_move.start + (zoom_move.aim - zoom_move.start) * c);
		zoom_move.coef+=zoom_move.speed*delta_time;
		if (zoom_move.coef>=1.)
		{
			flag_auto_zoom = 0;
			set_fov(zoom_move.aim);
		}
	}
	/*
    if (flag_auto_zoom)
    {
		// Use a smooth function
		float smooth = 3.f;
		double c = atanf(smooth * 2.*zoom_move.coef-smooth)/atanf(smooth)/2+0.5;
		set_fov(zoom_move.start + (zoom_move.aim - zoom_move.start) * c);
        zoom_move.coef+=zoom_move.speed*delta_time;
        if (zoom_move.coef>=1.)
        {
			flag_auto_zoom = 0;
            set_fov(zoom_move.aim);
        }
    }*/
}

// Zoom to the given field of view
void Projector::zoom_to(double aim_fov, float move_duration)
{
	zoom_move.aim=aim_fov;
    zoom_move.start=fov;
    zoom_move.speed=1.f/(move_duration*1000);
    zoom_move.coef=0.;
    flag_auto_zoom = true;
}

// Set the drawing mode in 2D for drawing in the full screen
// Use reset_perspective_projection() to restore previous projection mode
void Projector::set_2Dfullscreen_projection(void) const
{
	glViewport(0, 0, screenW, screenH);
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
    glPushMatrix();						// store previous matrix
    glLoadIdentity();
    gluOrtho2D(	0, screenW,
				0, screenH);			// set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode

    glPushMatrix();
    glLoadIdentity();
}

// Reset the previous projection mode after a call to set_orthographic_projection()
void Projector::restore_from_2Dfullscreen_projection(void) const
{
    glMatrixMode(GL_PROJECTION);		// Restore previous matrix
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
	glViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], vec_viewport[3]);
    glPopMatrix();
}

// Set the drawing mode in 2D. Use reset_perspective_projection() to reset
// previous projection mode
void Projector::set_orthographic_projection(void) const
{
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
    glPushMatrix();						// store previous matrix
    glLoadIdentity();
    gluOrtho2D(	vec_viewport[0], vec_viewport[0] + vec_viewport[2],
				vec_viewport[1], vec_viewport[1] + vec_viewport[3]);	// set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode
    glPushMatrix();
    glLoadIdentity();
}

// Reset the previous projection mode after a call to set_orthographic_projection()
void Projector::reset_perspective_projection(void) const
{
    glMatrixMode(GL_PROJECTION);		// Restore previous matrix
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}



// Reimplementation of gluSphere : glu is overrided for non standard projection
void Projector::sSphere(GLdouble radius, GLint slices, GLint stacks, const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);
	GLUquadricObj * p = gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	if (orient_inside) gluQuadricOrientation(p, GLU_INSIDE);
	gluSphere(p, radius, slices, stacks);
	gluDeleteQuadric(p);
	glPopMatrix();
}

// Draw a half sphere
void Projector::sHalfSphere(GLdouble radius, GLint slices, GLint stacks,
	const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	GLfloat rho, drho, theta, dtheta;
	GLfloat x, y, z;
	GLfloat s, t, ds, dt;
	GLint i, j, imin, imax;
	GLfloat nsign;

	if (orient_inside) nsign = -1.0;
	else nsign = 1.0;

	drho = M_PI / (GLfloat) stacks;
	dtheta = 2.0 * M_PI / (GLfloat) slices;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
	ds = 1.0 / slices;
	dt = 1.0 / stacks;
	t = 1.0;			// because loop now runs from 0
	imin = 0;
	imax = stacks;

	// draw intermediate stacks as quad strips
	for (i = imin; i < imax/2; i++)
	{
		rho = i * drho;
		glBegin(GL_QUAD_STRIP);
		s = 0.0;
		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0 : j * dtheta;
			x = -sin(theta) * sin(rho);
			y = cos(theta) * sin(rho);
			z = nsign * cos(rho);
			glNormal3f(x * nsign, y * nsign, z * nsign);
			glTexCoord2f(s, t);
			sVertex3(x * radius, y * radius, z * radius, mat);
			x = -sin(theta) * sin(rho + drho);
			y = cos(theta) * sin(rho + drho);
			z = nsign * cos(rho + drho);
			glNormal3f(x * nsign, y * nsign, z * nsign);
			glTexCoord2f(s, t - dt);
			s += ds;
			sVertex3(x * radius, y * radius, z * radius, mat);
		}
		glEnd();
		t -= dt;
	}
	glPopMatrix();
}

// Draw a disk with a special texturing mode having texture center at disk center
void Projector::sDisk(GLdouble radius, GLint slices, GLint stacks,
	const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	GLfloat r, dr, theta, dtheta;
	GLfloat x, y;
	GLint j;
	GLfloat nsign;

	if (orient_inside) nsign = -1.0;
	else nsign = 1.0;

	dr = radius / (GLfloat) stacks;
	dtheta = 2.0 * M_PI / (GLfloat) slices;

	// draw intermediate stacks as quad strips
	for (r = 0; r < radius; r+=dr)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0 : j * dtheta;
			x = r*cos(theta);
			y = r*sin(theta);
			glNormal3f(0, 0, nsign);
			glTexCoord2f(0.5+x/2/radius, 0.5+y/2/radius);
			sVertex3(x, y, 0, mat);
			x = (r+dr)*cos(theta);
			y = (r+dr)*sin(theta);
			glNormal3f(0, 0, nsign);
			glTexCoord2f(0.5+x/2/radius, 0.5+y/2/radius);
			sVertex3(x, y, 0, mat);
		}
		glEnd();
	}
	glPopMatrix();
}

inline void sSphereMapTexCoord(double rho, double theta, double texture_fov)
{
	if (rho>texture_fov/2.)
	{
		rho=texture_fov/2.;
	}
	glTexCoord2f(0.5f + rho/texture_fov * cosf(theta), 0.5f + rho/texture_fov * sinf(theta));
}

inline void sSphereMapTexCoordFast(float rho, float costheta, float sintheta, float texture_fov)
{
	if (rho>texture_fov/2.)
	{
		rho=texture_fov/2.;
	}
	glTexCoord2f(0.5f + rho/texture_fov * costheta, 0.5f + rho/texture_fov * sintheta);
}

void Projector::sSphere_map(GLdouble radius, GLint slices, GLint stacks, const Mat4d& mat, double texture_fov, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	GLfloat rho, drho, theta, dtheta;
	GLfloat x, y, z;
	GLint i, j, imin, imax;
	GLfloat nsign;

	if (orient_inside) nsign = -1.0;
	else nsign = 1.0;

	drho = M_PI / (GLfloat) stacks;
	dtheta = 2.0 * M_PI / (GLfloat) slices;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
	imin = 0;

#ifdef NVIDIA
	imax = stacks/1.8;
#else
	imax = stacks;
#endif

	static float sinrho;
	static float cosrho;
	static float sintheta;
	static float costheta;
	static float sinrho_plus_drho;
	static float cosrho_plus_drho;

	// draw intermediate stacks as quad strips
	if (nsign==1)
	{
		for (i = imin; i < imax; ++i)
		{
			rho = drho * i;
			sinrho = sinf(rho);
			cosrho = cosf(rho);
			sinrho_plus_drho = sinf(rho + drho);
			cosrho_plus_drho = cosf(rho + drho);

			glBegin(GL_QUAD_STRIP);
			for (j = 0; j <= slices; ++j)
			{
				theta = (j == slices) ? 0.0 : dtheta * j;
				sintheta = sinf(theta);
				costheta = cosf(theta);

				x = -sintheta * sinrho;
				y = costheta * sinrho;
				z = cosrho;
				glNormal3f(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho, costheta, sintheta, texture_fov);
				sVertex3(x * radius, y * radius, z * radius, mat);

				x = -sintheta * sinrho_plus_drho;
				y = costheta * sinrho_plus_drho;
				z = cosrho_plus_drho;
				glNormal3f(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho + drho, costheta, sintheta, texture_fov);
				sVertex3(x * radius, y * radius, z * radius, mat);
			}
			glEnd();
		}
	}
	else
	{
		for (i = imin; i < imax; ++i)
		{
			rho = drho * i;
			sinrho = sinf(rho);
			cosrho = cosf(rho);
			sinrho_plus_drho = sinf(rho + drho);
			cosrho_plus_drho = cosf(rho + drho);

			glBegin(GL_QUAD_STRIP);
			for (j = 0; j <= slices; ++j)
			{
				theta = (j == slices) ? 0.0 : dtheta * j;
				sintheta = sinf(theta);
				costheta = cosf(theta);

				x = -sintheta * sinrho_plus_drho;
				y = costheta * sinrho_plus_drho;
				z = cosrho_plus_drho;
				glNormal3f(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho + drho, costheta, -sintheta, texture_fov);
				sVertex3(x * radius, y * radius, z * radius, mat);

				x = -sintheta * sinrho;
				y = costheta * sinrho;
				z = cosrho;
				glNormal3f(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho, costheta, -sintheta, texture_fov);
				sVertex3(x * radius, y * radius, z * radius, mat);
			}
			glEnd();
		}
	}
	glPopMatrix();
}


// Reimplementation of gluCylinder : glu is overrided for non standard projection
void Projector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);
	GLUquadricObj * p = gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	if (orient_inside)
	{
		glCullFace(GL_FRONT);
	}
	gluCylinder(p, radius, radius, height, slices, stacks);
	gluDeleteQuadric(p);
	glPopMatrix();
	if (orient_inside)
	{
		glCullFace(GL_BACK);
	}
}

void Projector::print_gravity180(const s_font* font, float x, float y, const string& str,
				 bool speed_optimize, float xshift, float yshift) const
{
	static float dx, dy, d, theta, psi;
	dx = x - (vec_viewport[0] + vec_viewport[2]/2);
	dy = y - (vec_viewport[1] + vec_viewport[3]/2);
	d = sqrt(dx*dx + dy*dy);

	// If the text is too far away to be visible in the screen return
	if (d>MY_MAX(vec_viewport[3], vec_viewport[2])*2) return;


	theta = M_PI + atan2f(dx, dy - 1);
	psi = atan2f((float)font->getStrLen(str)/str.length(),d + 1) * 180./M_PI;

	if (psi>5) psi = 5;
	set_orthographic_projection();
	glTranslatef(x,y,0);
	glRotatef(theta*180./M_PI,0,0,-1);
	glTranslatef(xshift, -yshift, 0);
	glScalef(1, -1, 1);
	
    glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	for (unsigned int i=0;i<str.length();++i)
	{

	  if( !speed_optimize ) {
	    font->print_char_outlined(str[i]);
	  } else {
	    font->print_char(str[i]);
	  }

		if (str[i]!=16 && str[i]!=17 &&str[i]!=18) {

		  if( !speed_optimize ) {
		    psi = atan2f((float)font->getStrLen(str.substr(i,1)),d) * 180./M_PI;
		    if (psi>5) psi = 5;
		  }
		  glRotatef(psi,0,0,-1);
		  
		}


	}
	reset_perspective_projection();
}

