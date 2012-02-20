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
uniform sampler2D bmap;

uniform bool boolBump;
uniform vec4 vecColor;
uniform bool onlyColor;

uniform float fTransparencyThresh;

varying vec4 SM_tex_coord;
varying vec3 vecLight;
varying vec3 vecPosition;
varying vec3 vecNormal;

vec4 getBumpedDiffuse()
{
	//In-shader tangent/binormal calculation by kvark (http://stackoverflow.com/a/5261402)
	//Derived position
	vec3 p_dx = dFdx(vecPosition);
	vec3 p_dy = dFdy(vecPosition);
	//Compute derivations of the texture coordinate
	//Derivations give a flat TBN basis for each point of a polygon.
	//In order to get a smooth one we have to re-orthogonalize it based on a given (smooth) vertex normal. 
	vec2 tc_dx = dFdx(gl_TexCoord[0].st);
	vec2 tc_dy = dFdy(gl_TexCoord[0].st);
	// compute initial tangent and bi-tangent
	vec3 t = normalize(tc_dy.y * p_dx - tc_dx.y * p_dy );
	vec3 b = normalize(tc_dy.x * p_dx - tc_dx.x * p_dy ); // sign inversion
	// get new tangent from a given mesh normal
	vec3 n = normalize(vecNormal);
	vec3 x = cross(n, t);
	t = cross(x, n);
	t = normalize(t);
	// get updated bi-tangent
	x = cross(b, n);
	b = cross(n, x);
	b = normalize(b);
	
	//TBN space matrix
	mat3 tbnv = mat3(t.x, b.x, n.x,
					 t.y, b.y, n.y,
					 t.z, b.z, n.z);

	//Bringing the light into TBN space
	vec3 tbnvLight = tbnv * vecLight;

	//Normal per pixel - map to 0..1 range
	vec3 normal = texture(bmap, gl_TexCoord[0].st).xyz * 2.0 - 1.0;
	normal = normalize(normal);
	
	vec3 ldir = normalize(tbnvLight);
	
	return gl_LightSource[0].diffuse * max(0.0, dot(normal, ldir));
}

vec4 getDiffuse()
{
	vec3 normal = normalize(vecNormal);
    vec3 light = normalize(vecLight);
	
	return gl_LightSource[0].diffuse * max(0.0, dot(normal, light));
}

vec4 getDiffuseLighting()
{
	if(boolBump)
	{
		return getBumpedDiffuse();
	} 
	else
	{
		return getDiffuse();
	}	
}

void main(void)
{
	vec4 diffuse = getDiffuseLighting();
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