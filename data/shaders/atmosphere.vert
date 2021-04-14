/*
 * Stellarium
 * Copyright (C) 2002-2018 Fabien Chereau and Stellarium contributors
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

// Variables for the color computation
uniform highp vec3 sunPos;
uniform mediump float term_x, Ax, Bx, Cx, Dx, Ex;
uniform mediump float term_y, Ay, By, Cy, Dy, Ey;

// The current projection matrix
uniform mediump mat4 projectionMatrix;

// Contains the 2d position of the point on the screen (before multiplication by the projection matrix)
attribute mediump vec2 skyVertex;

// Contains the r,g,b,Y (luminosity) components.
attribute highp vec4 skyColor;

// The output variable passed to the fragment shader
varying mediump vec3 resultSkyColor;

vec3 xyYToRGB(highp float x, highp float y, highp float Y);

void main()
{
	gl_Position = projectionMatrix*vec4(skyVertex, 0., 1.);

	///////////////////////////////////////////////////////////////////////////
	// First compute the xy color component
	// skyColor contains the unprojected vertex position in r,g,b
	// + the Y (luminance) component of the color in the alpha channel
	highp vec3 unprojectedVertex = skyColor.xyz;
	highp float Y = skyColor.w;
	highp float x,y;
	if (Y>0.01)
	{
		highp float cosDistSunq = dot(sunPos,unprojectedVertex);
		highp float distSun=acos(cosDistSunq);
		highp float oneOverCosZenithAngle = (unprojectedVertex.z==0.) ? 9999999999999. : 1. / unprojectedVertex.z;

		cosDistSunq*=cosDistSunq;
		x = term_x * (1. + Ax * exp(Bx*oneOverCosZenithAngle))* (1. + Cx * exp(Dx*distSun) + Ex * cosDistSunq);
		y = term_y * (1. + Ay * exp(By*oneOverCosZenithAngle))* (1. + Cy * exp(Dy*distSun) + Ey * cosDistSunq);
		if (x < 0. || y < 0.)
		{
			x = 0.25;
			y = 0.25;
		}
	}
	else
	{
		x = 0.25;
		y = 0.25;
	}

	resultSkyColor=xyYToRGB(x,y,Y);
}
