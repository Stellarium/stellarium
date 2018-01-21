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
This is a shader for basic vertex lighting. This should be the minimum quality supported.
*/

#define MAT_SPECULAR 1
#define GEOMETRY_SHADER 1
#define TORCH 1

//matrices
#if ! GEOMETRY_SHADER
uniform mat4 u_mMVP;
#endif
uniform mat4 u_mModelView;
uniform mat3 u_mNormal;

//light info
uniform mediump vec3 u_vLightDirectionView; //in view space

//material info
uniform mediump vec3 u_vMixAmbient; // = light ambient * mtl ambient/diffuse depending on Illum model
uniform mediump vec3 u_vMixDiffuse; // light diffuse * mat diffuse
#if MAT_SPECULAR
uniform mediump vec3 u_vMixSpecular;
uniform mediump float u_vMatShininess;
#endif

#if TORCH
uniform mediump vec3 u_vMixTorchDiffuse;
uniform mediump float u_fTorchAttenuation;
#endif

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute mediump vec2 a_texcoord;

#if GEOMETRY_SHADER
#define VAR_TEXCOORD v_texcoordGS
#define VAR_TEXILLUMINATION v_texilluminationGS
#define VAR_SPECILLUMINATION v_specilluminationGS
#else
#define VAR_TEXCOORD v_texcoord
#define VAR_TEXILLUMINATION v_texillumination
#define VAR_SPECILLUMINATION v_specillumination
#endif

varying mediump vec2 VAR_TEXCOORD;
varying mediump vec3 VAR_TEXILLUMINATION;
#if MAT_SPECULAR
varying mediump vec3 VAR_SPECILLUMINATION;
#endif

void calcLighting(vec3 normal, vec3 viewPos, out vec3 texIll, out vec3 specIll)
{
	//basic lambert term
	float NdotL = dot(normal, u_vLightDirectionView);
	vec3 Idiff = u_vMixDiffuse * max(0.0,NdotL);
	
#if MAT_SPECULAR || TORCH
	vec3 eye = normalize(-viewPos);
#endif

#if TORCH
	//calculate additional diffuse, modeled by a point light centered on the cam pos
	float camDistSq = dot(viewPos,viewPos);
	float att = max(0.0, 1.0 - camDistSq * u_fTorchAttenuation);
	att *= att;
	
	Idiff += att * u_vMixTorchDiffuse * max(0.0, dot(normal,eye));
#endif
	
	specIll = vec3(0,0,0);
	
#if MAT_SPECULAR
	if(NdotL>0.0)
	{
		//calculate phong reflection vector
		vec3 R = reflect(-u_vLightDirectionView,normal);
		
		float RdotE = dot(normalize(R), eye);
		
		if(RdotE>0.0)
		{
			//specular term according to Phong model (light specular is assumed to be white for now)
			specIll = u_vMixSpecular * pow( RdotE, u_vMatShininess);
		}
	}
#endif
	
	texIll = u_vMixAmbient + Idiff;
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