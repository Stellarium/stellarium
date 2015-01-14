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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
 
/*
This is the fragment shader for vertex lighting, which does not need to do many things
*/
 
#version 110

#define BLENDING 1
#define MAT_DIFFUSETEX 1
#define MAT_SPECULAR 1
#define ALPHATEST 1

#if MAT_DIFFUSETEX
uniform sampler2D u_texDiffuse;
#endif

#if ALPHATEST
uniform float u_fAlphaThresh;
#endif

uniform float u_vMatAlpha;

varying vec2 v_texcoord;
varying vec3 v_texillumination;
#if MAT_SPECULAR
varying vec3 v_specillumination;
#endif

void main(void)
{
#if MAT_DIFFUSETEX
	vec4 texVal = texture2D(u_texDiffuse,v_texcoord);
	
	#if ALPHATEST
	if(texVal.a < u_fAlphaThresh)
		discard;
	#endif
	
	vec3 color = v_texillumination * texVal.rgb;
	#if MAT_SPECULAR
	color += v_specillumination;
	#endif

	#if BLENDING
	gl_FragColor = vec4(color, texVal.a * u_vMatAlpha);
	#else
	gl_FragColor = vec4(color, 1.0);
	#endif
#else
	vec3 color = v_texillumination;
	#if MAT_SPECULAR
	color += v_specillumination;
	#endif
	
	gl_FragColor = vec4(color,u_vMatAlpha); //u_vMatAlpha is automatically set to 1.0 if blending is disabled
#endif
}