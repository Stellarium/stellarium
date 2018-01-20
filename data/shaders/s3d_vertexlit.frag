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
This is the fragment shader for vertex lighting, which does not need to do many things
*/

#define BLENDING 1
#define MAT_DIFFUSETEX 1
#define MAT_EMISSIVETEX 1
#define MAT_SPECULAR 1
#define ALPHATEST 1

#if MAT_DIFFUSETEX
uniform sampler2D u_texDiffuse;
#endif
#if MAT_EMISSIVETEX
uniform sampler2D u_texEmissive;
#endif

#if ALPHATEST
uniform lowp float u_fAlphaThresh;
#endif

uniform mediump vec3 u_vMixEmissive; //material emissive modulated by light angle
uniform mediump float u_vMatAlpha;

varying mediump vec2 v_texcoord;
varying mediump vec3 v_texillumination;
#if MAT_SPECULAR
varying mediump vec3 v_specillumination;
#endif

void main(void)
{
#if MAT_DIFFUSETEX
	lowp vec4 texVal = texture2D(u_texDiffuse,v_texcoord);
	
	#if ALPHATEST
	if(texVal.a < u_fAlphaThresh)
		discard;
	#endif
	
	lowp vec3 color = v_texillumination * texVal.rgb;
#else
	lowp vec3 color = v_texillumination;
#endif
	
	#if MAT_SPECULAR
	color += v_specillumination;
	#endif
	
	#if MAT_EMISSIVETEX
	color += u_vMixEmissive * texture2D(u_texEmissive,v_texcoord).rgb;
	#else
	color += u_vMixEmissive;
	#endif

#if MAT_DIFFUSETEX
	#if BLENDING
	gl_FragColor = vec4(color, texVal.a * u_vMatAlpha);
	#else
	gl_FragColor = vec4(color, 1.0);
	#endif
#else	
	gl_FragColor = vec4(color,u_vMatAlpha); //u_vMatAlpha is automatically set to 1.0 if blending is disabled
#endif
}