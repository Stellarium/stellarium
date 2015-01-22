#version 120

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
Note: This shader currently requires some #version 120 features!
*/

//macros that can be set by ShaderManager (simple true/false flags)
#define BLENDING 1
#define SHADOWS 1
#define SHADOW_FILTER 0
#define SHADOW_FILTER_HQ 0
#define MAT_DIFFUSETEX 1
#define MAT_EMISSIVETEX 1
#define MAT_SPECULAR 1
#define BUMP 0
#define HEIGHT 0
#define ALPHATEST 1
#define TORCH 1

#if SHADOW_FILTER_HQ
#define FILTER_STEPS 64
//NOTE: Intel does NOT like it for some reason if this is a const array, so we set it as uniform
uniform vec2 poissonDisk[FILTER_STEPS] = vec2[](
   vec2(-0.613392, 0.617481),
   vec2(0.170019, -0.040254),
   vec2(-0.299417, 0.791925),
   vec2(0.645680, 0.493210),
   vec2(-0.651784, 0.717887),
   vec2(0.421003, 0.027070),
   vec2(-0.817194, -0.271096),
   vec2(-0.705374, -0.668203),
   vec2(0.977050, -0.108615),
   vec2(0.063326, 0.142369),
   vec2(0.203528, 0.214331),
   vec2(-0.667531, 0.326090),
   vec2(-0.098422, -0.295755),
   vec2(-0.885922, 0.215369),
   vec2(0.566637, 0.605213),
   vec2(0.039766, -0.396100),
   vec2(0.751946, 0.453352),
   vec2(0.078707, -0.715323),
   vec2(-0.075838, -0.529344),
   vec2(0.724479, -0.580798),
   vec2(0.222999, -0.215125),
   vec2(-0.467574, -0.405438),
   vec2(-0.248268, -0.814753),
   vec2(0.354411, -0.887570),
   vec2(0.175817, 0.382366),
   vec2(0.487472, -0.063082),
   vec2(-0.084078, 0.898312),
   vec2(0.488876, -0.783441),
   vec2(0.470016, 0.217933),
   vec2(-0.696890, -0.549791),
   vec2(-0.149693, 0.605762),
   vec2(0.034211, 0.979980),
   vec2(0.503098, -0.308878),
   vec2(-0.016205, -0.872921),
   vec2(0.385784, -0.393902),
   vec2(-0.146886, -0.859249),
   vec2(0.643361, 0.164098),
   vec2(0.634388, -0.049471),
   vec2(-0.688894, 0.007843),
   vec2(0.464034, -0.188818),
   vec2(-0.440840, 0.137486),
   vec2(0.364483, 0.511704),
   vec2(0.034028, 0.325968),
   vec2(0.099094, -0.308023),
   vec2(0.693960, -0.366253),
   vec2(0.678884, -0.204688),
   vec2(0.001801, 0.780328),
   vec2(0.145177, -0.898984),
   vec2(0.062655, -0.611866),
   vec2(0.315226, -0.604297),
   vec2(-0.780145, 0.486251),
   vec2(-0.371868, 0.882138),
   vec2(0.200476, 0.494430),
   vec2(-0.494552, -0.711051),
   vec2(0.612476, 0.705252),
   vec2(-0.578845, -0.768792),
   vec2(-0.772454, -0.090976),
   vec2(0.504440, 0.372295),
   vec2(0.155736, 0.065157),
   vec2(0.391522, 0.849605),
   vec2(-0.620106, -0.328104),
   vec2(0.789239, -0.419965),
   vec2(-0.545396, 0.538133),
   vec2(-0.178564, -0.596057)
);
#elif SHADOW_FILTER
#define FILTER_STEPS 16
//NOTE: Intel does NOT like it for some reason if this is a const array, so we set it as uniform
uniform vec2 poissonDisk[FILTER_STEPS] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);
#else
#define FILTER_STEPS 0
#endif

#if MAT_DIFFUSETEX
uniform sampler2D u_texDiffuse;
#endif
#if MAT_EMISSIVETEX
uniform sampler2D u_texEmissive;
#endif
#if BUMP
uniform sampler2D u_texBump;
#endif
#if HEIGHT
uniform sampler2D u_texHeight;
#endif

//material info
uniform vec3 u_vMixAmbient; // = light ambient * mtl ambient/diffuse depending on Illum model
uniform vec3 u_vMixDiffuse; // light diffuse * mat diffuse
#if MAT_SPECULAR
uniform vec3 u_vMatSpecular;
uniform float u_vMatShininess;
#endif
uniform float u_vMatAlpha;

uniform vec3 u_vMixEmissive;

#if SHADOWS
//shadow related uniforms
uniform vec4 u_vSquaredSplits; //the frustum splits
uniform sampler2DShadow u_texShadow0;
uniform sampler2DShadow u_texShadow1;
uniform sampler2DShadow u_texShadow2;
uniform sampler2DShadow u_texShadow3;
#endif
#if ALPHATEST
uniform float u_fAlphaThresh;
#endif

#if TORCH
uniform vec3 u_vMixTorchDiffuse;
uniform float u_fTorchAttenuation;
#endif

varying vec3 v_normal;
varying vec2 v_texcoord;
varying vec3 v_lightVec; //light vector, in VIEW or TBN space according to bump settings
varying vec3 v_viewPos; //position of fragment in view space

#if SHADOWS
varying vec4 v_shadowCoord0;
varying vec4 v_shadowCoord1;
varying vec4 v_shadowCoord2;
varying vec4 v_shadowCoord3;
#endif

#if SHADOWS
float sampleShadow(in sampler2DShadow tex, in vec4 coord)
{
	#if FILTER_STEPS
	// a filter is defined
	float sum =0.0;
	for(int i=0;i<FILTER_STEPS;++i)
	{
		//TODO this should be texture size dependent!
		sum+=shadow2DProj(tex,vec4(coord.xy + poissonDisk[i]/700.0, coord.z, coord.w)).x;
	}
	return sum / FILTER_STEPS;
	#else
	//no filtering performed, just return the sampled tex
	return shadow2DProj(tex,coord).x;
	#endif
}

float getShadow()
{
	//simplification of the smap.f.glsl shader
	float dist = dot(v_viewPos,v_viewPos); //squared length of view vec
	
	//check in which split the fragment falls
	if(dist < u_vSquaredSplits.x)
	{
		return sampleShadow(u_texShadow0,v_shadowCoord0);
	}
	else if(dist < u_vSquaredSplits.y)
	{
		return sampleShadow(u_texShadow1,v_shadowCoord1);
	}
	else if(dist < u_vSquaredSplits.z)
	{
		return sampleShadow(u_texShadow2,v_shadowCoord2);
	}
	else if(dist < u_vSquaredSplits.w)
	{
		return sampleShadow(u_texShadow3,v_shadowCoord3);
	}
	
	return 1.0;
}
#endif

void calcLighting(in vec3 normal,in vec3 eye,out vec3 texCol,out vec3 specCol)
{
	vec3 L = v_lightVec; //no normalize here, or it may cause divide by zero
	
	//basic lambert term
	float NdotL = dot(normal, L);
	vec3 Idiff = u_vMixDiffuse * max(0.0,NdotL);
	
#if SHADOWS
	float shd = getShadow();
	Idiff *= shd;
#endif
	
#if TORCH
	//calculate additional diffuse, modeled by a point light centered on the cam pos
	float camDistSq = dot(v_viewPos,v_viewPos);
	float att = max(0.0, 1.0 - camDistSq * u_fTorchAttenuation);
	att *= att;
	
	Idiff += att * u_vMixTorchDiffuse * max(0.0, dot(normal,eye));
#endif
	
	#if MAT_SPECULAR
	if(NdotL>0.0)
	{
		//calculate phong reflection vector
		vec3 R = reflect(-L,normal);
		
		float RdotE = dot(normalize(R), eye);
		
		if(RdotE>0.0)
		{
			//specular term according to Phong model (light specular is assumed to be white for now)
			specCol = u_vMatSpecular * pow( RdotE, u_vMatShininess);
			#if SHADOWS
			specCol *= shd;
			#endif
		}
	}
	#else
	specCol = vec3(0,0,0);
	#endif
	
	
	texCol = u_vMixAmbient + Idiff;
}

#if BUMP
vec3 getBumpNormal(vec2 texCoords)
{
	return texture2D(u_texBump, texCoords).xyz * 2.0 - 1.0;
}
#endif

#if HEIGHT
const float heightScale = 0.015f; //const for now
#endif

void main(void)
{
#if MAT_DIFFUSETEX
	vec4 texVal = texture2D(u_texDiffuse,v_texcoord);
	#if ALPHATEST
	//check if alpha lies below threshold
	if(texVal.a < u_fAlphaThresh)
		discard;
	#endif
#endif

	vec2 texCoords = v_texcoord;
	vec3 eye = normalize(-v_viewPos);
	
	#if HEIGHT
	//pertube texture coords with heightmap
	float height = texture2D(u_texHeight, texCoords).r;
	//*scale +bias
	height = height * heightScale - 0.5*heightScale;
		
	texCoords = texCoords + (height * eye.xz);
	#endif
	
	#if BUMP
	vec3 normal = getBumpNormal(texCoords);
	#else
	vec3 normal = v_normal;
	#endif

	vec3 texCol,specCol; //texCol gets multiplied with diffuse map, specCol not
	calcLighting(normalize(normal),eye,texCol,specCol);
	
	#if MAT_EMISSIVETEX
	//use existing specCol to avoid using another vec3
	specCol += u_vMixEmissive * texture2D(u_texEmissive,v_texcoord).rgb;
	#else
	specCol += u_vMixEmissive;
	#endif
	
#if MAT_DIFFUSETEX
	#if BLENDING
	gl_FragColor = vec4(texCol * texVal.rgb + specCol,u_vMatAlpha * texVal.a);
    #else
	gl_FragColor = vec4(texCol * texVal.rgb + specCol, 1.0);
	#endif
#else
	gl_FragColor = vec4(texCol + specCol,u_vMatAlpha); //u_vMatAlpha is automatically set to 1.0 if blending is disabled
#endif
}