/*
 * Stellarium
 * Copyright (C) 2002-2016 Fabien Chereau and Stellarium contributors
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
  This is the vertex shader for solar system object rendering
 */

attribute highp vec3 vertex;
attribute highp vec3 unprojectedVertex;
attribute mediump vec2 texCoord;
uniform highp mat4 projectionMatrix;
uniform highp vec3 lightDirection;
uniform highp vec3 eyeDirection;
varying mediump vec2 texc;
varying highp vec3 P;
#ifdef IS_MOON
    varying highp vec3 normalX;
    varying highp vec3 normalY;
    varying highp vec3 normalZ;
#else
    varying mediump float lum_;
#endif

void main()
{
    gl_Position = projectionMatrix * vec4(vertex, 1.);
    texc = texCoord;
    highp vec3 normal = normalize(unprojectedVertex);
#ifdef IS_MOON
    normalX = normalize(cross(vec3(0,0,1), normal));
    normalY = normalize(cross(normal, normalX));
    normalZ = normal;
#else
    mediump float c = dot(lightDirection, normal);
    lum_ = clamp(c, 0.0, 1.0);
#endif

    P = unprojectedVertex;
}
