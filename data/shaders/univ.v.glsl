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

//Texture Bias Matrix for Shadow mapping
uniform mat4 tex_mat;

uniform mat3 NormalMatrix;

varying vec4 SM_tex_coord;
varying vec3 vecLight;
varying vec3 vecPosition;
varying vec3 vecNormal;

void main(void)
{
	//Shadow texture coords in projected light space
	SM_tex_coord = tex_mat * gl_Vertex;
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	
	vec3 lightDir = normalize(gl_LightSource[0].position.xyz);
	
	vecLight = normalize(NormalMatrix * lightDir);
	vecPosition = gl_Vertex.xyz;
	vecNormal = gl_Normal;
}