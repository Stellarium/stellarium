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
This is a shader for basic vertex lighting. This should be the minimum quality supported.
*/
 
#version 120

//matrices
uniform mat4 u_mModelView;
uniform mat4 u_mProjection;
uniform mat3 u_mNormal;

//light info
uniform vec3 u_vLightDirection; //in view space
uniform vec3 u_vLightAmbient;
uniform vec3 u_vLightDiffuse;

//material info
uniform vec3 u_vMatAmbient;
uniform vec3 u_vMatDiffuse;
uniform float u_vMatAlpha;

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec2 v_texcoord;
varying vec4 v_illumination;

vec3 calcLighting(vec3 normal)
{
	//ambient + small constant lighting
	vec3 Iamb = (u_vLightAmbient + vec3(0.025,0.025,0.025)) * u_vMatAmbient;
	
	//basic lambert term
	float NdotL = dot(normal, u_vLightDirection);
	vec3 Idiff = u_vLightDiffuse * u_vMatDiffuse * max(0.0,NdotL);
	
	return Iamb + Idiff;
}

void main(void)
{
	//transform normal
	vec3 normal = u_mNormal * a_normal;
	
	v_illumination = vec4(calcLighting(normal), u_vMatAlpha);	
	v_texcoord = a_texcoord;
	gl_Position = u_mProjection * u_mModelView * a_vertex;
}