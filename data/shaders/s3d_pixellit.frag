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

uniform sampler2D u_texDiffuse;

//light info
uniform vec3 u_vLightDirection; //in view space, from point to light
uniform vec3 u_vLightAmbient;
uniform vec3 u_vLightDiffuse;

//material info
uniform vec3 u_vMatAmbient;
uniform vec3 u_vMatDiffuse;
uniform vec3 u_vMatSpecular;
uniform float u_vMatShininess;
uniform float u_vMatAlpha;

varying vec3 v_normal;
varying vec2 v_texcoord;
varying vec3 v_halfvec;

vec3 calcLighting(vec3 normal)
{
	vec3 ret;
	
	//ambient + small constant lighting
	vec3 Iamb = (u_vLightAmbient + vec3(0.025,0.025,0.025)) * u_vMatAmbient;
	ret+=Iamb;
	
	//basic lambert term
	float NdotL = dot(normal, u_vLightDirection);
	vec3 Idiff = u_vLightDiffuse * u_vMatDiffuse * max(0.0,NdotL);
	ret+=Idiff;
	
	if(NdotL>0.0)
	{
		//specular term according to Blinn-Phong model (light specular is assumed to be white for now)
		vec3 Ispec = u_vMatSpecular * pow( dot(normal, v_halfvec), 4 * u_vMatShininess);
		ret+=Ispec;
	}
	
	return ret;
}

void main(void)
{
	vec4 texVal = texture2D(u_texDiffuse,v_texcoord);
	gl_FragColor = vec4(calcLighting(normalize(v_normal)),u_vMatAlpha) * texVal;
}