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

uniform sampler2D tex;
uniform sampler2D bmap;

uniform bool boolBump;
uniform vec4 vecColor;
uniform bool onlyColor;

uniform float fTransparencyThresh;

varying vec3 vecLight;
varying vec3 vecHalf;

void main(void)
{
	vec4 texColor = texture(tex, gl_TexCoord[0].st);
	
	if(texColor.a < fTransparencyThresh)
		discard;
		
	vec3 n = normalize(texture(bmap, gl_TexCoord[0].st).xyz * 2.0 - 1.0);
	vec3 l = normalize(vecLight);
	vec3 h = normalize(vecHalf);
	
	float NdotL = max(0.0, dot(n, l));
	float NdotH = max(0.0, dot(n, h));
	float p = (NdotL == 0.0) ? 0.0 : pow(gl_FrontMaterial.shininess, NdotH);
	
	vec4 ambient = gl_FrontLightProduct[0].ambient;
    vec4 diffuse = gl_FrontLightProduct[0].diffuse * NdotL;
	vec4 specular = gl_FrontLightProduct[0].specular * p;
	
	vec4 color;
	if(onlyColor) color = vecColor * (gl_LightSource[0].ambient + diffuse + specular);
	else color = texColor * (gl_LightSource[0].ambient + diffuse + specular);
    
    gl_FragColor = color;
}