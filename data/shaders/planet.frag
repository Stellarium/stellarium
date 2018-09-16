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
  This is the fragment shader for solar system object rendering
 */

varying mediump vec2 texc; //texture coord
varying highp vec3 P; //original vertex pos in model space

uniform sampler2D tex;
uniform mediump vec3 ambientLight;
uniform mediump vec3 diffuseLight;
uniform highp vec4 sunInfo;
uniform mediump float skyBrightness;

uniform int shadowCount;
uniform highp mat4 shadowData;

//x = scaling, y = exponential falloff
uniform mediump vec2 outgasParameters;
//eye direction in model space, pre-normalized
uniform highp vec3 eyeDirection;

#ifdef RINGS_SUPPORT
uniform bool ring;
uniform highp float outerRadius;
uniform highp float innerRadius;
uniform sampler2D ringS;
uniform bool isRing;
#endif

#ifdef SHADOWMAP
uniform highp sampler2D shadowTex;
varying highp vec4 shadowCoord;
#endif

#if defined(IS_OBJ) || defined(IS_MOON)
    #define OREN_NAYAR 1
    //light direction in model space, pre-normalized
    uniform highp vec3 lightDirection;  
    //x = A, y = B, z = scaling factor (rho/pi * E0)
    uniform mediump vec3 orenNayarParameters;
#endif
#ifdef IS_MOON
    uniform sampler2D earthShadow;
    uniform sampler2D normalMap;

    varying highp vec3 normalX;
    varying highp vec3 normalY;
    varying highp vec3 normalZ;
#else
    varying mediump float lambertIllum;
    varying mediump vec3 normalVS; //pre-calculated normals or spherical normals in model space
#endif

#ifdef SHADOWMAP
uniform highp vec2 poissonDisk[64];

lowp float offset_lookup(in highp sampler2D sTex, in highp vec4 loc, in highp vec2 offset, in highp float zbias)
{
    //the macro SM_SIZE is set to the shadowmap size
    const mediump vec2 texmapscale=vec2(1.0/float(SM_SIZE));
    //"simulates" textureProjOffset for use in GLSL < 130
    highp vec4 coords = vec4(loc.xy + (offset * texmapscale * loc.w), loc.z, loc.w);
    
    //for some reason, when not adding a LOD bias, the result is wrong in my VM (Ubuntu 16.04)
    //even if the texture has NO mipmaps and the lookup filter is GL_NEAREST???
    //I'm 99% certain this is some bug in the GL driver, 
    //because adding ANY bias here makes it work correctly
    //It should have no effect on platforms which don't have this bug
    highp float texVal = texture2DProj(sTex, coords, -1000.0).r;
    //perform shadow comparison
    return texVal > (loc.z-zbias)/loc.w ? 1.0 : 0.0;
}

//basic pseudo-random number generator
mediump float random(in mediump vec4 seed4)
{
    highp float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

lowp float sampleShadowMap(in highp sampler2D sTex, in highp vec4 coord, in highp float zbias)
{   
    //uncomment for a single sample
    //return offset_lookup(sTex,coord, vec2(0.0),zbias);
 
    // for some reason > 5 samples do not seem to work on my Ubuntu VM 
    // (no matter if ES2 or GL 2.1)
    // everything gets shadowed, but no errors?!
    // so to be sure we just fix the sample count at 4 for now, 
    // even though 16 would look quite a lot better
    const int SAMPLE_COUNT = 4;
    mediump float sum = 0.0;
    for(int i=0;i<SAMPLE_COUNT;++i)
    {
        //choose "random" locations out of 64 using the screen-space coordinates
        //for ES2, we have to use mod(float, float), mod(int, int) is not defined
        int index = int(mod( 64.0*random(vec4(gl_FragCoord.xyy,i)), 64.0));
        sum += offset_lookup(sTex, coord, poissonDisk[index], zbias);
    }
    
    return clamp(sum / float(SAMPLE_COUNT),0.0,1.0);
}
#endif

#ifdef OREN_NAYAR
// Calculates the Oren-Nayar reflectance (https://en.wikipedia.org/wiki/Oren%E2%80%93Nayar_reflectance_model)
// the scale parameter is actually rho/pi * E_0 here
// A and B are precalculated on the CPU side
mediump float orenNayar(in mediump vec3 normal, in highp vec3 lightDir, in highp vec3 viewDir, in mediump float A, in mediump float B, in mediump float scale)
{
    // GZ next 2 dont require highp IMHO
    mediump float cosAngleLightNormal = dot(normal, lightDir);  //cos theta_i
    mediump float cosAngleEyeNormal = dot(normal, viewDir); //cos theta_r
    //acos can be quite expensive, can we avoid it?
    mediump float angleLightNormal = acos(cosAngleLightNormal); //theta_i
    mediump float angleEyeNormal = acos(cosAngleEyeNormal); //theta_r
    mediump float alpha = max(angleEyeNormal, angleLightNormal); //alpha = max(theta_i, theta_r)
    mediump float beta = min(angleEyeNormal, angleLightNormal); //beta = min(theta_i, theta_r)
    mediump float gamma = dot(viewDir - normal * cosAngleEyeNormal, lightDir - normal * cosAngleLightNormal);
    mediump float C = sin(alpha) * tan(beta);
    return max(0.0, cosAngleLightNormal) * (A + B * max(0.0, gamma) * C) * scale;
}
#endif

// calculate pseudo-outgassing effect, inspired by MeshLab's "electronic microscope" shader
lowp float outgasFactor(in mediump vec3 normal, in highp vec3 lightDir, in mediump float falloff)
{
    mediump float opac = dot(normal,lightDir);
    opac = abs(opac);
    opac = 1.0 - pow(opac, falloff);
    return opac;
}

void main()
{
    mediump float final_illumination = 1.0;
#ifdef OREN_NAYAR
    mediump float lum = 1.;
#else
    mediump float lum = lambertIllum;
#endif
#ifdef RINGS_SUPPORT
    if(isRing)
        lum=1.0;
#endif
    //shadow calculation
    if(lum > 0.0)
    {
        highp vec3 sunPosition = sunInfo.xyz;
#ifdef RINGS_SUPPORT
        if(ring && !isRing)
        {
            highp vec3 ray = normalize(sunPosition);
            highp float u = - P.z / ray.z;
            if(u > 0.0 && u < 1e10)
            {
                mediump float ring_radius = length(P + u * ray);
                if(ring_radius > innerRadius && ring_radius < outerRadius)
                {
                    ring_radius = (ring_radius - innerRadius) / (outerRadius - innerRadius);
                    lowp float ringAlpha = texture2D(ringS, vec2(ring_radius, 0.5)).w;
                    final_illumination = 1.0 - ringAlpha;
                }
            }
        }
#endif

        highp float sunRadius = sunInfo.w;
        highp float L = length(sunPosition - P);
        highp float R = asin(sunRadius / L);
        for (int i = 0; i < 4; ++i)
        {
            if (shadowCount>i)
            {
                highp vec3 satellitePosition = shadowData[i].xyz;
                highp float satelliteRadius = shadowData[i].w;
                highp float l = length(satellitePosition - P);
                highp float r = asin(satelliteRadius / l);
                highp float d = acos(min(1.0, dot(normalize(sunPosition - P), normalize(satellitePosition - P))));

                mediump float illumination = 1.0;
                if(d >= R + r)
                {
                    // distance too far
                    illumination = 1.0;
                }
                else if(r >= R + d)
                {
                    // umbra
#ifdef IS_MOON
                    illumination = d / (r - R) * 0.6;
#else
                    illumination = 0.0;
#endif
                }
                else if(d + r <= R)
                {
                    // penumbra completely inside
                    illumination = 1.0 - r * r / (R * R);
                }
                else
                {
                    // penumbra partially inside
#ifdef IS_MOON
                    illumination = ((d - abs(R-r)) / (R + r - abs(R-r))) * 0.4 + 0.6;
#else
                    mediump float x = (R * R + d * d - r * r) / (2.0 * d);
                    mediump float alpha = acos(x / R);
                    mediump float beta = acos((d - x) / r);
                    mediump float AR = R * R * (alpha - 0.5 * sin(2.0 * alpha));
                    mediump float Ar = r * r * (beta - 0.5 * sin(2.0 * beta));
                    mediump float AS = R * R * 2.0 * 1.57079633;
                    illumination = 1.0 - (AR + Ar) / AS;
#endif
                }
                final_illumination = min(illumination, final_illumination);
            }
        }
    }

#ifdef IS_MOON
    mediump vec3 normal = texture2D(normalMap, texc).rgb-vec3(0.5, 0.5, 0);
    normal = normalize(normalX*normal.x+normalY*normal.y+normalZ*normal.z);
    // normal now contains the real surface normal taking normal map into account
#else
    // important to normalize here again
    mediump vec3 normal = normalize(normalVS);
#endif
#ifdef OREN_NAYAR
    // Use an Oren-Nayar model for rough surfaces
    // Ref: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Oren-Nayar
    lum = orenNayar(normal, lightDirection, eyeDirection, orenNayarParameters.x, orenNayarParameters.y, orenNayarParameters.z);
#endif
    //calculate pseudo-outgassing/rim-lighting effect
    lowp float outgas = 0.0;
    if(outgasParameters.x > 0.0)
    {
        outgas = outgasParameters.x * outgasFactor(normal, eyeDirection, outgasParameters.y);
    }
//Reduce lum if sky is bright, to avoid burnt-out look in daylight sky.
    lum *= (1.0-0.4*skyBrightness);
#ifdef SHADOWMAP
    //use shadowmapping
    //z-bias is modified using the angle between the surface and the light
    //gives less shadow acne
    highp float NdotL = clamp(dot(normal, lightDirection), 0.0, 1.0);
    highp float zbias = 0.01 * tan(acos(NdotL));
    zbias = clamp(zbias, 0.0, 0.03);
    lowp float shadow = sampleShadowMap(shadowTex, shadowCoord, zbias);
    lum*=shadow;
#endif

    //final lighting color
    mediump vec4 litColor = vec4(lum * final_illumination * diffuseLight + ambientLight, 1.0);

    //apply texture-colored rimlight
    //litColor.xyz = clamp( litColor.xyz + vec3(outgas), 0.0, 1.0);

    lowp vec4 texColor = texture2D(tex, texc);
    mediump vec4 finalColor = texColor;
#ifdef IS_MOON
    if(final_illumination < 0.99)
    {
        lowp vec4 shadowColor = texture2D(earthShadow, vec2(final_illumination, 0.5));
        finalColor = mix(finalColor * litColor, shadowColor, clamp(shadowColor.a, 0.0, 0.7)); // clamp alpha to allow some maria detail.
    }
    else
#endif
    {
        finalColor *= litColor;
    }

    //apply white rimlight
    finalColor.xyz = clamp( finalColor.xyz + vec3(outgas), 0.0, 1.0);

    gl_FragColor = finalColor;
    //to debug texture issues, uncomment and reload shader
    //gl_FragColor = texColor;
}
