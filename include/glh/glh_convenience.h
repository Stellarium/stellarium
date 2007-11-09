/*
    glh - is a platform-indepenedent C++ OpenGL helper library 


    Copyright (c) 2000 Cass Everitt
	Copyright (c) 2000 NVIDIA Corporation
    All rights reserved.

    Redistribution and use in source and binary forms, with or
	without modification, are permitted provided that the following
	conditions are met:

     * Redistributions of source code must retain the above
	   copyright notice, this list of conditions and the following
	   disclaimer.

     * Redistributions in binary form must reproduce the above
	   copyright notice, this list of conditions and the following
	   disclaimer in the documentation and/or other materials
	   provided with the distribution.

     * The names of contributors to this software may not be used
	   to endorse or promote products derived from this software
	   without specific prior written permission. 

       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
	   POSSIBILITY OF SUCH DAMAGE. 


    Cass Everitt - cass@r3.nu
*/

#ifndef GLH_CONVENIENCE_H
#define GLH_CONVENIENCE_H

// Convenience methods for using glh_linear objects
// with opengl...



// debugging hack...
#include <iostream>

using namespace std;

#ifdef MACOS
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <glh/glh_linear.h>
#include <glh/glh_extensions.h>


namespace glh
{

  // matrix helpers

  inline matrix4f get_matrix(GLenum matrix) 
  {
	GLfloat m[16];
	glGetFloatv(matrix, m);
	return matrix4f(m);
  }

  // transform helpers

  inline void glh_rotate(const quaternionf & r)
  {
	float angle;
	vec3f axis;
	r.get_value(axis, angle);
	glRotatef(to_degrees(angle), axis.v[0], axis.v[1], axis.v[2]);
  }

  // inverse of camera_lookat
  inline matrix4f object_lookat(const vec3f & from, const vec3f & to, const vec3f & Up)
  {
	  vec3f look = to - from;
	  look.normalize();
	  vec3f up(Up);
	  up -= look * look.dot(up);
	  up.normalize();
	  
	  quaternionf r(vec3f(0,0,-1), vec3f(0,1,0), look, up);
	  matrix4f m;
	  r.get_value(m);
	  m.set_translate(from);
	  return m;
  }


  // inverse of object_lookat
  inline matrix4f camera_lookat(const vec3f & eye, const vec3f & lookpoint, const vec3f & Up)
  {
	  vec3f look = lookpoint - eye;
	  look.normalize();
	  vec3f up(Up);
	  up -= look * look.dot(up);
	  up.normalize();

	  matrix4f t;
	  t.set_translate(-eye);

	  quaternionf r(vec3f(0,0,-1), vec3f(0,1,0), look, up);
	  r.invert();
	  matrix4f rm;
	  r.get_value(rm);
	  return rm*t;	  
  }


  inline matrix4f frustum(float left, float right,
				   float bottom, float top,
				   float zNear, float zFar)
  {
	matrix4f m;
	m.make_identity();

	m(0,0) = (2*zNear) / (right - left);
	m(0,2) = (right + left) / (right - left);
	
	m(1,1) = (2*zNear) / (top - bottom);
	m(1,2) = (top + bottom) / (top - bottom);
	
	m(2,2) = -(zFar + zNear) / (zFar - zNear);
	m(2,3) = -2*zFar*zNear / (zFar - zNear);
   
	m(3,2) = -1;
	m(3,3) = 0;

	return m;
  }

  inline matrix4f frustum_inverse(float left, float right,
						   float bottom, float top,
						   float zNear, float zFar)
  {
	matrix4f m;
	m.make_identity();

	m(0,0) = (right - left) / (2 * zNear);
	m(0,3) = (right + left) / (2 * zNear);
	
	m(1,1) = (top - bottom) / (2 * zNear);
	m(1,3) = (top + bottom) / (2 * zNear);

	m(2,2) = 0;
	m(2,3) = -1;
	
	m(3,2) = -(zFar - zNear) / (2 * zFar * zNear);
	m(3,3) = (zFar + zNear) / (2 * zFar * zNear);

	return m;
  }

  inline matrix4f perspective(float fovy, float aspect, float zNear, float zFar)
  {
	double tangent = tan(to_radians(fovy/2.0f));
	float y = (float)tangent * zNear;
	float x = aspect * y;
	return frustum(-x, x, -y, y, zNear, zFar);
  }

  inline matrix4f perspective_inverse(float fovy, float aspect, float zNear, float zFar)
  {
	double tangent = tan(to_radians(fovy/2.0f));
	float y = (float)tangent * zNear;
	float x = aspect * y;
	return frustum_inverse(-x, x, -y, y, zNear, zFar);
  }



  // are these names ok?

  inline void set_texgen_planes(GLenum plane_type, const matrix4f & m)
  {
	  GLenum coord[] = {GL_S, GL_T, GL_R, GL_Q };
	  for(int i = 0; i < 4; i++)
          {
          vec4f row;
          m.get_row(i,row);
		  glTexGenfv(coord[i], plane_type, row.v);
          }
  }

  // handy for register combiners
  inline vec3f range_compress(const vec3f & v)
  { vec3f vret(v); vret *= .5f; vret += .5f; return vret; }
  
  inline vec3f range_uncompress(const vec3f & v)
  { vec3f vret(v); vret -= .5f; vret *= 2.f; return vret; }

} // namespace glh

#endif
