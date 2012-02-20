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
uniform sampler2D smap;

uniform vec4 vecColor;
uniform bool onlyColor;

uniform float fTransparencyThresh;

varying vec4 SM_tex_coord;
varying vec3 vecLight;
varying vec3 vecNormal;

void main(void)
{
	vec3 normal = normalize(vecNormal);
	vec3 light = normalize(vecLight);
	
	vec4 diffuse = gl_LightSource[0].diffuse * max(0.0, dot(normal, light));
	
	vec4 texColor = texture(tex, gl_TexCoord[0].st);
	
	if(texColor.a < fTransparencyThresh)
		discard;
	
	vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;
	float depth = texture(smap, tex_coords.xy).x;
	
	float factor;
	if(depth > (tex_coords.z + 0.00001))
	{
		//In light!
		factor = 1.0f;
	}
	else
	{
		factor = 0.3f;
	}
	
	vec4 color;
	if(onlyColor) color = vecColor * (gl_LightSource[0].ambient + diffuse*factor);
	else color = texColor * (gl_LightSource[0].ambient + diffuse*factor);
	
	gl_FragColor = vec4(color.xyz, 1.0);
}