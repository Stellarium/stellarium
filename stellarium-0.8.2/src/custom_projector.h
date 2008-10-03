/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _CUSTOM_PROJECTOR_H_
#define _CUSTOM_PROJECTOR_H_

#include "projector.h"
#include "planet.h"
//serkin:
#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <SDL_image.h>
#include <stdio.h>

#define	TEX_XSIZE	1024
#define	TEX_YSIZE	1024

struct _player_texture
{
  int valid; // was the loading attempt successful ? 
  SDL_Surface *bitmap;
  GLuint tex_id;	//OpenGL texture ID. It is likes DWORD (double machine word)
  float scale_x, scale_y; // scale the texcoords, as OpenGL thinks in TEX_XSIZE and TEX_YSIZE
};

typedef struct _player_texture Player_texture;  

// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class CustomProjector : public Projector
{
protected:
    CustomProjector(const Vec4i& viewport, double _fov = 175.);
private:
	// Reimplementation of gluSphere : glu is overrided for non standard projection
	void sSphere(GLdouble radius, GLdouble one_minus_oblateness,
		GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

  // Reimplementation of gluCylinder : glu is overrided for non standard projection
 // void display(object3d_ptr shuttle, const Mat4d& mat) const;
 // void s3dsObject(object3d_ptr object, const Mat4d& mat, int orient_inside = 0) const;
 // serkin: new methods
	void drawObject(Object3DS* obj, const Mat4d& mat);
	void ms3dsObject(Object3DS* obj, const Mat4d& mat, int orient_inside = 0);
	void render_node(Lib3dsNode *node, Lib3dsFile* file, const Mat4d& mat, double scale, char* tm);

	// Override glVertex3f and glVertex3d
	void sVertex3(double x, double y, double z, const Mat4d& mat) const;

	const Vec3d convert_pos(const Vec3d& v, const Mat4d& mat) const;
protected:
	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	void init_project_matrix(void);
};

#endif
