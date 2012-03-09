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
uniform sampler2D bmap;

uniform bool boolBump;
uniform vec4 vecColor;
uniform bool onlyColor;
 
uniform float fTransparencyThresh;
uniform float alpha;

varying vec4 SM_tex_coord; 
varying vec3 vecLight;
varying vec3 vecEye;
varying vec3 vecNormal;
  
vec4 getLighting()
{
	//Ambient part
	vec4 color = (gl_FrontLightModelProduct.sceneColor * gl_FrontMaterial.ambient) + (gl_LightSource[0].ambient * gl_FrontMaterial.ambient);
	
	//For bump mapping, the normal comes from the bump map texture lookup
	vec3 n = vec3(0.0);
	if(boolBump)
	{
		n = normalize(texture2D(bmap, gl_TexCoord[0].st).xyz * 2.0 - 1.0);
	}
	else
	{
		n = normalize(vecNormal);
	}
	
		//Traditional shadow mapping
	vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;
	float depth = texture(smap, tex_coords.xy).x;
	
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
	
	vec3 l = normalize(vecLight);
	
	//Lambert term
	float NdotL = dot(n, l);
	
	if(NdotL > 0.0)
	{
		//Diffuse part
		color += gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * NdotL * shadowFactor;
		
		//Specular part
		vec3 e = normalize(vecEye);
		vec3 r = reflect(-l, n);
		
		float spec = pow(max(0.0, dot(r, e)), gl_FrontMaterial.shininess);
		
		color += gl_LightSource[0].specular * gl_FrontMaterial.specular * spec;
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