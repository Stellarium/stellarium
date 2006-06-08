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

#include <cstdio>
#include <iostream>
#include "custom_projector.h"


CustomProjector::CustomProjector(const Vec4i& viewport, double _fov)
                :Projector(viewport, _fov)
{
	mat_projection.set(1., 0., 0., 0.,
							0., 1., 0., 0.,
							0., 0., -1, 0.,
							0., 0., 0., 1.);				
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of glOrtho
void CustomProjector::init_project_matrix(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
}

// Override glVertex3f
// Here is the main trick for texturing in fisheye mode : The trick is to compute the
// new coordinate in orthographic projection which will simulate the fisheye projection.
void CustomProjector::sVertex3(double x, double y, double z, const Mat4d& mat) const
{
	Vec3d win;
	Vec3d v(x,y,z);
	project_custom(v, win, mat);

	// Can be optimized by avoiding matrix inversion if it's always the same
	gluUnProject(win[0],win[1],win[2],mat,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
	glVertex3dv(v);
}

void CustomProjector::sSphere(GLdouble radius, GLdouble one_minus_oblateness,
                              GLint slices, GLint stacks,
                              const Mat4d& mat, int orient_inside) const
{
    glPushMatrix();
    glLoadMatrixd(mat);

      // It is really good for performance to have Vec4f,Vec3f objects
      // static rather than on the stack. But why?
      // Is the constructor/destructor so expensive?
    static Vec4f lightPos4;
    static Vec3f lightPos3;
    GLboolean isLightOn;
    static Vec3f transNorm;
    float c;

    static Vec4f ambientLight;
    static Vec4f diffuseLight;

    glGetBooleanv(GL_LIGHTING, &isLightOn);

    if (isLightOn)
    {
        glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos4);
        lightPos3 = lightPos4;
        lightPos3 -= mat * Vec3d(0.,0.,0.); // -posCenterEye
        lightPos3.normalize();
        glGetLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
        glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
        glDisable(GL_LIGHTING);
    }

    GLfloat x, y, z;
    GLfloat s, t;
    GLint i, j;
    GLfloat nsign;

    if (orient_inside) {
      nsign = -1.0;
      t=0.0; // from inside texture is reversed
    } else {
      nsign = 1.0;
      t=1.0;
    }

    const GLfloat drho = M_PI / (GLfloat) stacks;
    double cos_sin_rho[2*(stacks+1)];
    double *cos_sin_rho_p = cos_sin_rho;
    for (i = 0; i <= stacks; i++) {
      double rho = i * drho;
      *cos_sin_rho_p++ = cos(rho);
      *cos_sin_rho_p++ = sin(rho);
    }

    const GLfloat dtheta = 2.0 * M_PI / (GLfloat) slices;
    double cos_sin_theta[2*(slices+1)];
    double *cos_sin_theta_p = cos_sin_theta;
    for (i = 0; i <= slices; i++) {
      double theta = (i == slices) ? 0.0 : i * dtheta;
      *cos_sin_theta_p++ = cos(theta);
      *cos_sin_theta_p++ = sin(theta);
    }

    // texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
    // t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
    // cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
    const GLfloat ds = 1.0 / slices;
    const GLfloat dt = nsign / stacks; // from inside texture is reversed


    // draw intermediate  as quad strips
    for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks;
         i++,cos_sin_rho_p+=2)
    {
        glBegin(GL_QUAD_STRIP);
        s = 0.0;
        for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;
             j++,cos_sin_theta_p+=2)
        {
            x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
            y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
            z = nsign * cos_sin_rho_p[0];
            glTexCoord2f(s, t);
            if (isLightOn)
            {
                transNorm = mat.multiplyWithoutTranslation(
                                  Vec3d(x * one_minus_oblateness * nsign,
                                        y * one_minus_oblateness * nsign,
                                        z * nsign));
                c = lightPos3.dot(transNorm);
                if (c<0) c=0;
                glColor3f(c*diffuseLight[0] + ambientLight[0],
                    c*diffuseLight[1] + ambientLight[1],
                    c*diffuseLight[2] + ambientLight[2]);
            }
            sVertex3(x * radius, y * radius, z * one_minus_oblateness * radius, mat);
            x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
            y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
            z = nsign * cos_sin_rho_p[2];
            glTexCoord2f(s, t - dt);
            if (isLightOn)
            {
                transNorm = mat.multiplyWithoutTranslation(
                                  Vec3d(x * one_minus_oblateness * nsign,
                                        y * one_minus_oblateness * nsign,
                                        z * nsign));
                c = lightPos3.dot(transNorm);
                if (c<0) c=0;
                glColor3f(c*diffuseLight[0] + ambientLight[0],
                    c*diffuseLight[1] + ambientLight[1],
                    c*diffuseLight[2] + ambientLight[2]);
            }
            sVertex3(x * radius, y * radius, z * one_minus_oblateness * radius, mat);
            s += ds;
        }
        glEnd();
        t -= dt;
    }
    glPopMatrix();
    if (isLightOn) glEnable(GL_LIGHTING);
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void CustomProjector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	static GLdouble da, r, dz;
	static GLfloat z, nsign;
	static GLint i, j;

	nsign = 1.0;
	if (orient_inside) glCullFace(GL_FRONT);
	//nsign = -1.0;
	//else nsign = 1.0;

	da = 2.0 * M_PI / slices;
	dz = height / stacks;

	GLfloat ds = 1.0 / slices;
	GLfloat dt = 1.0 / stacks;
	GLfloat t = 0.0;
	z = 0.0;
	r = radius;
	for (j = 0; j < stacks; j++)
	{
	GLfloat s = 0.0;
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= slices; i++)
	{
		GLfloat x, y;
		if (i == slices)
		{
			x = sinf(0.0);
			y = cosf(0.0);
		}
		else
		{
			x = sinf(i * da);
			y = cosf(i * da);
		}
		glNormal3f(x * nsign, y * nsign, 0);
		glTexCoord2f(s, t);
		sVertex3(x * r, y * r, z, mat);
		glNormal3f(x * nsign, y * nsign, 0);
		glTexCoord2f(s, t + dt);
		sVertex3(x * r, y * r, z + dz, mat);
		s += ds;
	}			/* for slices */
	glEnd();
	t += dt;
	z += dz;
	}				/* for stacks */

	glPopMatrix();
	if (orient_inside) glCullFace(GL_BACK);
}
