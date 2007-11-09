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

//kornyakov: for 3d-objects
#include <GL\glut.h>
#include "texture.h"
#include "3dsloader.h"

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
/***** MSVC Stellarium Build *****/
//Original == #if defined(__sun) || defined(__sun__)
#if defined(__sun) || defined(__sun__) || defined(__MSVC__)
/***** MSVC Stellarium Build *****/
	// in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
    double *cos_sin_rho = new double[2*(stacks+1)];
#else
    double cos_sin_rho[2*(stacks+1)];
#endif
    double *cos_sin_rho_p = cos_sin_rho;
    for (i = 0; i <= stacks; i++) {
      double rho = i * drho;
      *cos_sin_rho_p++ = cos(rho);
      *cos_sin_rho_p++ = sin(rho);
    }

    const GLfloat dtheta = 2.0 * M_PI / (GLfloat) slices;

/***** MSVC Stellarium Build *****/
//Original == double cos_sin_theta[2*(slices+1)];
#if defined(__sun) || defined(__sun__) || defined(__MSVC__)
/***** MSVC Stellarium Build *****/
	double *cos_sin_theta = new double[2*(slices+1)];
#else
    double cos_sin_theta[2*(slices+1)];
#endif

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

    //kornyakov: black planets
    float intensity = GetIntensity();

    // draw intermediate as quad strips
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
                //kornyakov: planet fading
                glColor3f(intensity*(c*diffuseLight[0] + ambientLight[0]),
                          intensity*(c*diffuseLight[1] + ambientLight[1]),
                          intensity*(c*diffuseLight[2] + ambientLight[2]));                
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
                //kornyakov: planet fading
                glColor3f(intensity*(c*diffuseLight[0] + ambientLight[0]),
                  intensity*(c*diffuseLight[1] + ambientLight[1]),
                  intensity*(c*diffuseLight[2] + ambientLight[2])); 
            }
            sVertex3(x * radius, y * radius, z * one_minus_oblateness * radius, mat);
            s += ds;
        }
        glEnd();
        t -= dt;
    }
    glPopMatrix();
    if (isLightOn) glEnable(GL_LIGHTING);

	/***** MSVC Stellarium Build *****/
	//Original == #if defined(__sun) || defined(__sun__)
	#if defined(__sun) || defined(__sun__) || defined(__MSVC__)
		delete[] cos_sin_theta;
	/***** MSVC Stellarium Build *****/
		delete[] cos_sin_rho;
	#endif
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

void CustomProjector::display(object3d_ptr shuttle, const Mat4d& mat) const
{
  float previousAmbient[4] = {0,0,0,0};
  float zero[4] = {0,0,0,0};
  float unity[4] = {1, 1, 1, 1};
  glGetMaterialfv(GL_FRONT, GL_AMBIENT, previousAmbient);
  glMaterialfv(GL_FRONT, GL_AMBIENT, unity);
  glMaterialfv(GL_FRONT,GL_EMISSION, unity);

  glBindTexture(GL_TEXTURE_2D, shuttle->id_texture); // We set the active texture 

  bool flag = glIsEnabled(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST); // We enable the depth test (also called z buffer)  
  glBegin(GL_TRIANGLES); // glBegin and glEnd delimit the vertices that define a primitive (in our case triangles)
  for (int l_index=0;l_index<shuttle->polygons_qty;l_index++)
  {
    //----------------- FIRST VERTEX -----------------
    // Texture coordinates of the first vertex
    glTexCoord2f( shuttle->mapcoord[ shuttle->polygon[l_index].a ].u,
      shuttle->mapcoord[ shuttle->polygon[l_index].a ].v);
    // Coordinates of the first vertex
    sVertex3( shuttle->vertex[ shuttle->polygon[l_index].a ].x,
      shuttle->vertex[ shuttle->polygon[l_index].a ].y,
      shuttle->vertex[ shuttle->polygon[l_index].a ].z, mat);

    //----------------- SECOND VERTEX -----------------
    // Texture coordinates of the second vertex
    glTexCoord2f( shuttle->mapcoord[ shuttle->polygon[l_index].b ].u,
      shuttle->mapcoord[ shuttle->polygon[l_index].b ].v);
    // Coordinates of the second vertex
    sVertex3( shuttle->vertex[ shuttle->polygon[l_index].b ].x,
      shuttle->vertex[ shuttle->polygon[l_index].b ].y,
      shuttle->vertex[ shuttle->polygon[l_index].b ].z, mat);

    //----------------- THIRD VERTEX -----------------
    // Texture coordinates of the third vertex
    glTexCoord2f( shuttle->mapcoord[ shuttle->polygon[l_index].c ].u,
      shuttle->mapcoord[ shuttle->polygon[l_index].c ].v);
    // Coordinates of the Third vertex
    sVertex3( shuttle->vertex[ shuttle->polygon[l_index].c ].x,
      shuttle->vertex[ shuttle->polygon[l_index].c ].y,
      shuttle->vertex[ shuttle->polygon[l_index].c ].z, mat);
  }
  glEnd();
  if (!flag) glDisable(GL_DEPTH_TEST); // We enable the depth test (also called z buffer)

  glMaterialfv(GL_FRONT, GL_AMBIENT, previousAmbient);
  glMaterialfv(GL_FRONT, GL_EMISSION, zero);
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void CustomProjector::s3dsObject(object3d_ptr shuttle, const Mat4d& mat, int orient_inside) const
{
  glPushMatrix();
  glLoadMatrixd(mat);

  CustomProjector::display(shuttle, mat);

  glPopMatrix();
}
