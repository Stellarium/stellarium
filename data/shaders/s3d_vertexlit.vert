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
 
#version 110

#define MAT_AMBIENT 1
#define MAT_SPECULAR 1
#define GEOMETRY_SHADER 1

//matrices
#if ! GEOMETRY_SHADER
uniform mat4 u_mMVP;
#endif
uniform mat4 u_mModelView;
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
#if MAT_SPECULAR
uniform vec3 u_vMatSpecular;
uniform float u_vMatShininess;
#endif

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

#if GEOMETRY_SHADER
#define VAR_TEXCOORD v_texcoordGS
#define VAR_TEXILLUMINATION v_texilluminationGS
#define VAR_SPECILLUMINATION v_specilluminationGS
#else
#define VAR_TEXCOORD v_texcoord
#define VAR_TEXILLUMINATION v_texillumination
#define VAR_SPECILLUMINATION v_specillumination
#endif

varying vec2 VAR_TEXCOORD;
varying vec3 VAR_TEXILLUMINATION;
#if MAT_SPECULAR
varying vec3 VAR_SPECILLUMINATION;
#endif

void calcLighting(vec3 normal, vec3 viewPos, out vec3 texIll, out vec3 specIll)
{
#if MAT_AMBIENT
	//ambient + small constant lighting
	vec3 Iamb = (u_vLightAmbient + vec3(0.025,0.025,0.025)) * u_vMatAmbient;
#else
	//The diffuse color is the ambient color - same as glColorMaterial with GL_AMBIENT_AND_DIFFUSE
	vec3 Iamb = (u_vLightAmbient + vec3(0.025,0.025,0.025)) * u_vMatDiffuse;
#endif
	
	//basic lambert term
	float NdotL = dot(normal, u_vLightDirectionView);
	vec3 Idiff = u_vLightDiffuse * u_vMatDiffuse * max(0.0,NdotL);
	
#if MAT_SPECULAR
	vec3 eye = normalize(-viewPos);
	
	specIll = vec3(0,0,0);
	if(NdotL>0.0)
	{
		//calculate phong reflection vector
		vec3 R = reflect(-u_vLightDirectionView,normal);
		
		float RdotE = dot(normalize(R), eye);
		
		if(RdotE>0.0)
		{
			//specular term according to Phong model (light specular is assumed to be white for now)
			specIll = u_vMatSpecular * pow( RdotE, u_vMatShininess);
		}
	}
#endif
	
	texIll = Iamb + Idiff;
}

void main(void)
{
	//transform normal
	vec3 normal = u_mNormal * a_normal;
	
	vec4 viewPos = u_mModelView * a_vertex;

	vec3 texIll,specIll;
	calcLighting(normal,viewPos.xyz,texIll,specIll);
	
	VAR_TEXILLUMINATION = texIll;
	#if MAT_SPECULAR
	VAR_SPECILLUMINATION = specIll;
	#endif
	
	VAR_TEXCOORD = a_texcoord;
	
	#if GEOMETRY_SHADER
	gl_Position = a_vertex; //pass on unchanged
	#else
	gl_Position = u_mMVP * a_vertex;
	#endif
}