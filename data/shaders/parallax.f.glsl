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
uniform sampler2D bmap;
uniform sampler2D hmap;

uniform bool boolBump;
uniform bool boolHeight;
uniform vec4 vecColor;
uniform bool onlyColor;
 
uniform float fTransparencyThresh;
uniform float alpha;
uniform int iIllum;
 
varying vec3 vecLight;
varying vec3 vecEye;
varying vec3 vecNormal;
varying vec4 vecPos;
varying vec3 vecHalf;

uniform float s;
  
vec4 getLighting()
{
	vec2 hTexCoord = gl_TexCoord[0].st;
	vec3 v = normalize(vecEye);
		
	if(boolHeight)
	{
		float height = texture2D(hmap, hTexCoord).r;
		//*scale +bias
		height = height * s - 0.5*s;
		
		hTexCoord = hTexCoord + (height * v.xy);
	}

	//For bump mapping, the normal comes from the bump map texture lookup
	vec3 n = normalize(vecNormal);
	if(boolBump)
	{
		n = normalize(texture2D(bmap, hTexCoord).xyz * 2.0 - 1.0);
	}
	
	vec3 l = normalize(vecLight);
	
	//Lambert term
	float NdotL = dot(n, l);
	vec4 color = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * max(0.0, NdotL);
	
	//Add ambient
	if(iIllum > 0)
	{
		//Ka * Ia
		color += (gl_FrontLightModelProduct.sceneColor * gl_FrontMaterial.ambient) + (gl_LightSource[0].ambient * gl_FrontMaterial.ambient);		
		
		//Add reflect
		if(iIllum == 2)
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
	else
	{
		//Add the lightsources ambient at least
		color += gl_LightSource[0].ambient;
	}
	
	return color * texture2D(tex, hTexCoord);
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
		color *= vecColor;
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