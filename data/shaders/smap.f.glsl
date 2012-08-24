/*
 * Stellarium Scenery3d Plug-in
 *
 /*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza
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

#version 120
 
uniform sampler2D tex;
uniform sampler2D smap_0;
uniform sampler2D smap_1;
uniform sampler2D smap_2;
uniform sampler2D smap_3;

uniform mat4 texmat_0;
uniform mat4 texmat_1;
uniform mat4 texmat_2;
uniform mat4 texmat_3;

uniform vec4 vecSplits;
uniform vec4 vecColor;
uniform bool onlyColor;

uniform float fTransparencyThresh;
uniform float alpha;
uniform int iIllum;
uniform bool boolDebug;

uniform int frustumSplits;

varying vec3 vecLight;
varying vec3 vecEye;
varying vec3 vecNormal;
varying vec4 vecEyeView;
varying vec4 vecPos;

uniform vec4 colors[4] = vec4[4](vec4(1.0, 0.1, 0.0, 1.0),  //Red
								 vec4(0.0, 1.0, 0.5, 1.0),  //Green
								 vec4(0.25, 0.41, 0.88, 1.0),  //Blue
								 vec4(0.29, 0.0, 0.51, 1.0)); //Orange


								 
vec2 poissonDisk[16] = vec2[]( 
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




/* alternative, even smoother, from http://www.geeks3d.com/20100628/3d-programming-ready-to-use-64-sample-poisson-disc/
vec2 poissonDisk[64] = vec2[]( 
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
 */


vec4 debugColor(int i)
{
	if(boolDebug)
	{
		return colors[i];
	}
	
	return vec4(1.0, 1.0, 1.0, 1.0);
}								 
 
vec4 getShadow()
{
	float bias = 0.00005;
	float dist = dot(vecEyeView.xyz, vecEyeView.xyz);
	
    vec4 shadow_c = vec4(1.0, 1.0, 1.0, 1.0);

    if(dist < vecSplits.x)
    {
		vec4 sm_coord_c = texmat_0*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_0, sm_coord_c.xy + poissonDisk[i]/1500.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(0) * visibility;

    }
    else if(dist < vecSplits.y)
    {
		vec4 sm_coord_c = texmat_1*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_1, sm_coord_c.xy + poissonDisk[i]/1500.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(1) * visibility;
    }
    else if(dist < vecSplits.z)
    {
		vec4 sm_coord_c = texmat_2*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_2, sm_coord_c.xy + poissonDisk[i]/1500.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(2) * visibility;
    }
	else if(dist < vecSplits.w)
	{
		vec4 sm_coord_c = texmat_3*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_3, sm_coord_c.xy + poissonDisk[i]/1500.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(3) * visibility;
	}
	
    return shadow_c;
}
 
vec4 getLighting()
{
	vec4 shadow = getShadow();
	vec3 v = normalize(vecEye);

	//Lambert Illumination
	vec3 n = normalize(vecNormal);
	vec3 l = normalize(vecLight);
		
	//Lambert term
	float NdotL = dot(n, l);
	
	vec4 color = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * max(0.0, NdotL) * shadow;	
		
	//Add ambient
	if(iIllum > 0)
	{
		//Ka * Ia
		color += (gl_FrontLightModelProduct.sceneColor * gl_FrontMaterial.ambient) + (gl_LightSource[0].ambient * gl_FrontMaterial.ambient);
		
		//Highlight
		if(iIllum == 2 && length(shadow) > 0)
		{
			//Reflection term
			if(NdotL > 0.0)
			{		
				vec3 e = normalize(v);
				vec3 r = normalize(-reflect(l,n)); 
				float RdotE = max(0.0, dot(r, e));
					
				if (RdotE > 0.0)
				{
					float spec = pow(RdotE, gl_FrontMaterial.shininess);		
					color += gl_LightSource[0].specular * gl_FrontMaterial.specular * spec;
				}
			}
		}
	}
	else
	{
		//Add the lightsources ambient at least
		color += gl_LightSource[0].ambient;
	}
	
	return color;
} 
  
void main(void)
{
	vec4 texel = texture2D(tex, gl_TexCoord[0].st);
	if(texel.a < fTransparencyThresh)
		discard;
	
	//Get shading
    vec4 color = getLighting();
	
	//Color only mode?
	if(onlyColor)
	{
		color = vecColor * color;
	}
	else
	{
		color = texel * color;
	}	

	if(iIllum == 9)
	{
		gl_FragColor = vec4(color.xyz, alpha);
	}
	else
	{
		gl_FragColor = vec4(color.xyz, 1.0);
	}
}