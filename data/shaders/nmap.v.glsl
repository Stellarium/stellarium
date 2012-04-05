/*
 * Stellarium
 * Copyright (C) 2011 Eleni Maria Stea
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

attribute vec3 tang;
attribute vec3 pvec;

uniform vec3 lpos;
uniform float t;
uniform vec3 cvel;

varying vec3 var_ldir;
varying vec3 var_vdir;
varying vec3 var_norm;

varying vec3 pos;

void main()
{
	gl_Position = gl_ProjectionMatrix * vec4(pvec.x, pvec.y, pvec.z, 1.0);

	vec3 vpos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 normal = normalize(gl_NormalMatrix * gl_Normal);
	vec3 tangent = normalize(gl_NormalMatrix * tang);
	vec3 binormal = cross(normal, tangent);

	vec3 ldir = normalize(lpos - vpos);

	gl_TexCoord[0] = gl_MultiTexCoord0;

	mat3 tbnv = mat3(tangent.x, binormal.x, normal.x,
			tangent.y, binormal.y, normal.y,
			tangent.z, binormal.z, normal.z);

	var_ldir = tbnv * ldir;
	var_vdir = -(tbnv * pvec);
	vec3 position = gl_Vertex.xyz;
	
	float theta = mod(t * cvel.x, 2.0 * 3.14159265);

	pos.x = position.x * cos(theta) + position.y * sin(theta);
	pos.y = -position.x * sin(theta) + position.y * cos(theta);
	pos.z = position.z;

	var_norm = normal;
}
