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

attribute highp vec4 vertex; // vertex projected by CPU
attribute mediump vec2 texCoord;
attribute highp vec4 unprojectedVertex; //original vertex coordinate (in km for OBJ models, in AU otherwise)
#ifdef IS_OBJ
    attribute mediump vec3 normalIn; // OBJs have pre-calculated normals
#endif

uniform highp mat4 projectionMatrix;
#ifdef SHADOWMAP
uniform highp mat4 shadowMatrix;
varying highp vec4 shadowCoord;
#endif

varying mediump vec2 texc; //texture coord
varying highp vec3 P; //original unprojected position (in AU)

#ifdef IS_MOON
    //Luna uses normal mapping
    varying highp vec3 normalX;
    varying highp vec3 normalY;
    varying highp vec3 normalZ;
#else
    varying mediump vec3 normalVS;
    //normal objects use gouraud shading
    //good enough for our spheres
    uniform highp vec3 lightDirection;
    varying mediump float lambertIllum;
#endif

void main()
{
    gl_Position = projectionMatrix * vertex;
    texc = texCoord;
    
#ifdef SHADOWMAP
    shadowCoord = shadowMatrix * unprojectedVertex; //uses the km data for better accuracy
#endif
#ifdef IS_OBJ
    //OBJ uses imported normals
    //assume it is pre-normalized by the importer
    normalVS = normalIn;
    
    //The unprojectedVertex here is in km, so we have to scale to AU
    P = unprojectedVertex.xyz / 149597870.691;
#else
    //unprojectedVertex is already in AU
    P = unprojectedVertex.xyz;
    //other objects use the spherical normals
    highp vec3 normal = normalize(unprojectedVertex.xyz);
    #ifdef IS_MOON
        if (abs(normal.z)==1.0) // avoid invalid pole anomaly
        {
            normalX=vec3(0);
            normalY=vec3(0);
        }
        else
        {
            normalX = normalize(cross(vec3(0,0,1), normal));
            normalY = normalize(cross(normal, normalX));
        }
        normalZ = normal;
    #else
        normalVS = normal;
        //simple Lambert illumination
        mediump float c = dot(lightDirection, normal);
        lambertIllum = clamp(c, 0.0, 1.0);
    #endif
#endif
}
