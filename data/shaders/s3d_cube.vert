/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
 
/*
This is a shader that applies cubemapping to pre-transformed geometry
*/
 
//matrices
uniform mat4 u_mProjection;

//Note: they are both vec3 here!
attribute vec3 a_vertex;
attribute vec3 a_texcoord;

varying highp vec3 v_texcoord;

void main(void)
{
	v_texcoord = a_texcoord;
	gl_Position = u_mProjection * vec4(a_vertex,1.0);
}