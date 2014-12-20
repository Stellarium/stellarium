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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
 
/*
This is a shader for phong/per-pixel lighting.
*/
 
#version 120

//matrices
uniform mat4 u_mModelView;
uniform mat4 u_mProjection;
uniform mat3 u_mNormal;

uniform vec3 u_vLightDirection; //in view space, from point to light

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec3 v_normal; //normal in view space
varying vec2 v_texcoord;
varying vec3 v_halfvec; //halfway vector between viewer and light-source, for blinn phong (view space)

void main(void)
{
	//transform normal
	v_normal = u_mNormal * a_normal;
	
	//pass on tex coord
	v_texcoord = a_texcoord;
	
	//calc halfway vector
	vec4 viewPos = u_mModelView * a_vertex;
	v_halfvec = normalize(-normalize(viewPos.xyz) + u_vLightDirection);
	
	//calc final position
	gl_Position = u_mProjection * viewPos;
}