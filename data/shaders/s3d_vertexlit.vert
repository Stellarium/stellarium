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

#define MAT_AMBIENT 1
#define GEOMETRY_SHADER 1

//matrices
#if ! GEOMETRY_SHADER
uniform mat4 u_mMVP;
#endif
uniform mat3 u_mNormal;

//light info
uniform vec3 u_vLightDirectionView; //in view space
uniform vec3 u_vLightAmbient;
uniform vec3 u_vLightDiffuse;

//material info
#if MAT_AMBIENT
uniform vec3 u_vMatAmbient;
#endif
uniform vec3 u_vMatDiffuse;
uniform float u_vMatAlpha;

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

#if GEOMETRY_SHADER
#define VAR_TEXCOORD v_texcoordGS
#define VAR_ILLUMINATION v_illuminationGS
#else
#define VAR_TEXCOORD v_texcoord
#define VAR_ILLUMINATION v_illumination
#endif

varying vec2 VAR_TEXCOORD;
varying vec4 VAR_ILLUMINATION;

vec3 calcLighting(vec3 normal)
{
#if MAT_AMBIENT
	//ambient + small constant lighting
	vec3 Iamb = (u_vLightAmbient + vec3(0.025,0.025,0.025)) * u_vMatAmbient;
#else
	//Add the lightsources ambient at least
	vec3 Iamb = u_vLightAmbient + vec3(0.025,0.025,0.025);
#endif
	
	//basic lambert term
	float NdotL = dot(normal, u_vLightDirectionView);
	vec3 Idiff = u_vLightDiffuse * u_vMatDiffuse * max(0.0,NdotL);
	
	return Iamb + Idiff;
}

void main(void)
{
	//transform normal
	vec3 normal = u_mNormal * a_normal;
	
	VAR_ILLUMINATION = vec4(calcLighting(normal), u_vMatAlpha);	
	VAR_TEXCOORD = a_texcoord;
	#if GEOMETRY_SHADER
	gl_Position = a_vertex; //pass on unchanged
	#else
	gl_Position = u_mMVP * a_vertex;
	#endif
}