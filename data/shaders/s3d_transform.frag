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
This fragment shader is only used if the material is alpha-tested to allow for better shadows.
*/

#define ALPHATEST 1
 
#if ALPHATEST
uniform lowp float u_fAlphaThresh;
uniform sampler2D u_texDiffuse;
#endif

varying mediump vec2 v_texcoord;

void main(void)
{
#if ALPHATEST
	if(texture2D(u_texDiffuse,v_texcoord).a < u_fAlphaThresh)
		discard;
#endif
}