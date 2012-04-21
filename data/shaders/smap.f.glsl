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

#version 110
 
uniform sampler2D tex;
uniform sampler2D smap;

uniform vec4 vecColor;
uniform bool onlyColor;
 
uniform float fTransparencyThresh;
uniform float fShininess;
uniform float alpha;

varying vec4 SM_tex_coord;
varying vec3 vecLight;
varying vec3 vecEye;
varying vec3 vecNormal;
  
vec4 getLighting()
{
	//Ambient part
	vec4 color = (gl_FrontLightModelProduct.sceneColor * gl_FrontMaterial.ambient) + (gl_LightSource[0].ambient * gl_FrontMaterial.ambient);
	
	vec3 n = normalize(vecNormal);
	vec3 l = normalize(vecLight);
	
	//Traditional shadow mapping
	vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;
	float depth = texture2D(smap, tex_coords.xy).x;
	
	float shadowFactor;
	if(depth > (tex_coords.z + 0.00001))
	{
		//In light!
		shadowFactor = 1.0;
	}
	else
	{
		shadowFactor = 0.3;
	}
	
	//Lambert term
	float NdotL = dot(n, l);
	color += gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * max(0.0, NdotL) * shadowFactor;
	
	//Reflection term
	if(NdotL > 0.0)
	{		
		vec3 e = normalize(vecEye);
		vec3 r = normalize(-reflect(l,n));  
		
		//Hack, it seems that 0.0f is not sent correctly into the shader on nvidia cards
		if(fShininess > 0.0)
		{
			float spec = pow(max(0.0, dot(r, e)), fShininess);		
			color += gl_LightSource[0].specular * gl_FrontMaterial.specular * spec;
		}
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

	//Set fragment color, alpha comes from MTL file
	gl_FragColor = vec4(color.xyz, alpha);
}