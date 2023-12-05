#version 150

/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
 
/*
This is a geometry-shader (using 3.2 core functionality, #version 150) based acceleration of cubemap rendering.
For vertex-based lighting, this is pretty simple: the lighting is performed in the
 Vertex shader in a common view space
*/

#define MAT_SPECULAR 1

layout(triangles) in;
layout(triangle_strip,max_vertices = 18) out;

in vec2 v_texcoordGS[];
in vec3 v_texilluminationGS[];
#if MAT_SPECULAR
in vec3 v_specilluminationGS[];
#endif

out vec2 v_texcoord;
out vec3 v_texillumination;
#if MAT_SPECULAR
out vec3 v_specillumination;
#endif

uniform mat4 u_mCubeMVP[6];

void main(void)
{
	//iterate over cubemap faces
	for(gl_Layer=0; gl_Layer<6;++gl_Layer)
	{
		//iterate over triangle vertices
		for(int vtx = 0;vtx<3;++vtx)
		{
			//calc new position in current cubemap face
			gl_Position = u_mCubeMVP[gl_Layer] * gl_in[vtx].gl_Position;
			//pass on other varyings
			v_texcoord = v_texcoordGS[vtx];
			v_texillumination = v_texilluminationGS[vtx];
			#if MAT_SPECULAR
			v_specillumination = v_specilluminationGS[vtx];
			#endif
			EmitVertex();
		}
		EndPrimitive();
	}
}