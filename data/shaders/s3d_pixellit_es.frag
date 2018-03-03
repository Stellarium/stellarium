/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014, 2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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
This is a shader for phong/per-pixel lighting with shadowing and bumpmapping.
This is a reduced version of the shader for OpenGL ES2, without shadow filtering/PCSS 
*/

//macros that can be set by ShaderManager (simple true/false flags)
#define BLENDING 1
#define SHADOWS 1
#define SINGLE_SHADOW_FRUSTUM 1
#define MAT_DIFFUSETEX 1
#define MAT_EMISSIVETEX 1
#define MAT_SPECULAR 1
#define BUMP 1
#define HEIGHT 1
#define ALPHATEST 1
#define TORCH 1

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
uniform mediump vec3 u_vMixAmbient; // = light ambient * mtl ambient/diffuse depending on Illum model
uniform mediump vec3 u_vMixDiffuse; // light diffuse * mat diffuse
#if MAT_SPECULAR
uniform mediump vec3 u_vMixSpecular;
uniform mediump float u_vMatShininess;
#endif
uniform mediump float u_vMatAlpha;

uniform mediump vec3 u_vMixEmissive;

#if SHADOWS
//in a later version, this may become configurable
#if SINGLE_SHADOW_FRUSTUM
#define FRUSTUM_SPLITS 1
#else
#define FRUSTUM_SPLITS 4
#endif
//shadow related uniforms
uniform mediump vec4 u_vSplits; //the frustum splits

//Basic opengl ES2 does not have shadow samplers, so we compare ourselves
#define SHADOWSAMPLER sampler2D

//for some reason, Intel does absolutely not like it if the shadowmaps are passed as an array
//nothing is drawn, but no error is shown ...
//therefore, use 4 ugly uniforms
uniform mediump SHADOWSAMPLER u_texShadow0;
#if !SINGLE_SHADOW_FRUSTUM
uniform mediump SHADOWSAMPLER u_texShadow1;
uniform mediump SHADOWSAMPLER u_texShadow2;
uniform mediump SHADOWSAMPLER u_texShadow3;
#endif

#endif //SHADOWS

#if ALPHATEST
uniform lowp float u_fAlphaThresh;
#endif

#if TORCH
uniform mediump vec3 u_vMixTorchDiffuse;
uniform mediump float u_fTorchAttenuation;
#endif

varying mediump vec3 v_normal;
varying mediump vec2 v_texcoord;
varying mediump vec3 v_lightVec; //light vector, in VIEW or TBN space according to bump settings
varying mediump vec3 v_viewPos; //position of fragment in view space

#if SHADOWS
//varying arrays seem to cause some problems, so we use 4 vecs for now...
varying mediump vec4 v_shadowCoord0;
#if !SINGLE_SHADOW_FRUSTUM
varying mediump vec4 v_shadowCoord1;
varying mediump vec4 v_shadowCoord2;
varying mediump vec4 v_shadowCoord3;
#endif
#endif

#if SHADOWS

lowp float sampleShadow(in mediump SHADOWSAMPLER tex, in mediump vec4 coord)
{
	mediump vec3 texC = coord.xyz / coord.w;
	//no filtering performed, just return the sampled tex
	return (texture2D(tex,texC.xy).r > texC.z ? 1.0 : 0.0);
}

lowp float getShadow()
{
	//simplification of the smap.f.glsl shader
	//IMPORTANT: use clip coords here, not distance to camera
	mediump float dist = gl_FragCoord.z;
		
	//check in which split the fragment falls
	//I tried using indices to simplify the code a bit, but this lead to very strange artifacts...
	//If all calculations are correct, this should be 1cm
	if(dist < u_vSplits.x)
	{
		return sampleShadow(u_texShadow0,v_shadowCoord0);
	}
	#if !SINGLE_SHADOW_FRUSTUM
	else if(dist < u_vSplits.y)
	{
		return sampleShadow(u_texShadow1,v_shadowCoord1);
	}
	else if(dist < u_vSplits.z)
	{
		return sampleShadow(u_texShadow2,v_shadowCoord2);
	}
	else if(dist < u_vSplits.w)
	{
		return sampleShadow(u_texShadow3,v_shadowCoord3);
	}
	#endif
	
	return 1.0;
}
#endif

void calcLighting(in mediump vec3 normal,in mediump vec3 eye,out mediump vec3 texCol,out mediump vec3 specCol)
{
	mediump vec3 L = v_lightVec; //no normalize here, or it may cause divide by zero

	//basic lambert term
	mediump float NdotL = clamp(dot(normal, L),0.0,1.0);
#if BUMP
	//use the original NdotL (which equals L.z because original normal is (0,0,1) in TBN space!) to modify the result
	//this hides incorrect illumination on the backside of objects
	mediump float origNormalFactor = 1.0 - 1.0 / (1.0 + 100.0 * max(L.z,0.0));
	NdotL *=  origNormalFactor;
#endif
	mediump vec3 Idiff = u_vMixDiffuse * NdotL;

	
#if SHADOWS
	lowp float shd = getShadow();
	Idiff *= shd;
#endif
	
#if TORCH
	//calculate additional diffuse, modeled by a point light centered on the cam pos
	mediump float camDistSq = dot(v_viewPos,v_viewPos);
	mediump float att = max(0.0, 1.0 - camDistSq * u_fTorchAttenuation);
	att *= att;
	
	Idiff += att * u_vMixTorchDiffuse * max(0.0, dot(normal,eye));
#endif
	
	#if MAT_SPECULAR
	if(NdotL>0.001)
	{
		//calculate phong reflection vector
		mediump vec3 R = reflect(-L,normal);
		
		mediump float RdotE = dot(normalize(R), eye);
		
		if(RdotE>0.0)
		{
			//specular term according to Phong model (light specular is assumed to be white for now)
			specCol = u_vMixSpecular * pow( RdotE, u_vMatShininess);
			#if SHADOWS
			specCol *= shd;
			#endif
			#if BUMP
			specCol *= origNormalFactor;
			#endif
		}
		else
		{
			specCol = vec3(0,0,0);
		}
	}
	else
	{
		specCol = vec3(0,0,0);
	}
	#else
	specCol = vec3(0,0,0);
	#endif
		
	texCol = u_vMixAmbient + Idiff;
}

#if BUMP
mediump vec3 getBumpNormal(mediump vec2 texCoords)
{
	return texture2D(u_texBump, texCoords).xyz * 2.0 - 1.0;
}
#endif

#if HEIGHT
const mediump float heightScale = 0.015; //const for now
#endif

void main(void)
{
#if MAT_DIFFUSETEX
	lowp vec4 texVal = texture2D(u_texDiffuse,v_texcoord);
	#if ALPHATEST
	//check if alpha lies below threshold
	if(texVal.a < u_fAlphaThresh)
		discard;
	#endif
#endif

	mediump vec2 texCoords = v_texcoord;
	mediump vec3 eye = normalize(-v_viewPos);
	
	#if HEIGHT
	//pertube texture coords with heightmap
	mediump float height = texture2D(u_texHeight, texCoords).r;
	//*scale +bias
	height = height * heightScale - 0.5*heightScale;
		
	texCoords = texCoords + (height * eye.xz);
	#endif
	
	#if BUMP
	mediump vec3 normal = getBumpNormal(texCoords);
	#else
	mediump vec3 normal = v_normal;
	#endif

	mediump vec3 texCol,specCol; //texCol gets multiplied with diffuse map, specCol not
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
	//gl_FragColor = vec4(texCol * texVal.rgb + specCol, 1.0);
	gl_FragColor = vec4(texCol * texVal.rgb + specCol, 1.0);
	#endif
#else
	gl_FragColor = vec4(texCol + specCol,u_vMatAlpha); //u_vMatAlpha is automatically set to 1.0 if blending is disabled
#endif
}