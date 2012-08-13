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

uniform vec4 colors[4] = vec4[4](vec4(0.8, 0.2, 0.2, 1.0),  //Red
								 vec4(0.2, 0.8, 0.2, 1.0),  //Green
								 vec4(0.2, 0.2, 0.8, 1.0),  //Blue
								 vec4(0.7, 0.7, 0.7, 1.0));
								 
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
		  if(texture2D(smap_0, sm_coord_c.xy + poissonDisk[i]/700.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(0) * visibility;
	
	/*
		vec4 sm_coord_c = texmat_0*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		
		float shadow = texture2D(smap_0, sm_coord_c.xy).x;
		float s = (shadow < sm_coord_c.z) ? 0.0 : 1.0;

		shadow_c = debugColor(0) * s;
	*/
    }
    else if(dist < vecSplits.y)
    {
		vec4 sm_coord_c = texmat_1*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_1, sm_coord_c.xy + poissonDisk[i]/700.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(1) * visibility;
	/*
		vec4 sm_coord_c = texmat_1*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		
		float shadow = texture2D(smap_1, sm_coord_c.xy).x;
		float s = (shadow < sm_coord_c.z) ? 0.0 : 1.0;

		shadow_c = debugColor(1) * s;
	*/	
    }
    else if(dist < vecSplits.z)
    {
		vec4 sm_coord_c = texmat_2*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_2, sm_coord_c.xy + poissonDisk[i]/700.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(2) * visibility;
	/*
		vec4 sm_coord_c = texmat_2*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		
		float shadow = texture2D(smap_2, sm_coord_c.xy).x;
		float s = (shadow < sm_coord_c.z) ? 0.0 : 1.0;

		shadow_c = debugColor(2) * s;
	*/
    }
	else if(dist < vecSplits.w)
	{
		vec4 sm_coord_c = texmat_3*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		float visibility=1.0;

		for (int i=0;i<16;i++)
		{
		  if(texture2D(smap_3, sm_coord_c.xy + poissonDisk[i]/700.0).z  <  sm_coord_c.z-bias)
		  {
			visibility-=0.05;
		  }
		}
		
		shadow_c = debugColor(3) * visibility;
	/*
		vec4 sm_coord_c = texmat_3*vecPos;
		sm_coord_c.xyz = sm_coord_c.xyz/sm_coord_c.w;
		
		float shadow = texture2D(smap_3, sm_coord_c.xy).x;
		float s = (shadow < sm_coord_c.z) ? 0.0 : 1.0;

		shadow_c = debugColor(3) * s;	
	*/
	}
	
    return shadow_c;
}
 
vec4 getLighting()
{
	vec4 shadow = getShadow();

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
				vec3 e = normalize(vecEye);
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