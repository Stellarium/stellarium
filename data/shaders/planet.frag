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
#ifdef IS_OBJ
    varying mediump vec3 normal; //pre-calculated normals
#endif

uniform sampler2D tex;
uniform mediump vec3 ambientLight;
uniform mediump vec3 diffuseLight;
uniform highp vec4 sunInfo;
uniform mediump float skyBrightness;

uniform int shadowCount;
uniform highp mat4 shadowData;

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
    //light and eye direction in model space
    uniform highp vec3 lightDirection;
    uniform highp vec3 eyeDirection;
#endif
#ifdef IS_MOON
    uniform sampler2D earthShadow;
    uniform sampler2D normalMap;

    varying highp vec3 normalX;
    varying highp vec3 normalY;
    varying highp vec3 normalZ;
#else
    varying mediump float lum_;
#endif

#ifdef SHADOWMAP
uniform vec2 poissonDisk[64];

lowp float offset_lookup(in highp sampler2D sTex, in vec4 loc, in vec2 offset, in float zbias)
{
    //the macro SM_SIZE is set to the shadowmap size
    const vec2 texmapscale=vec2(1.0/float(SM_SIZE));
    //"simulates" textureProjOffset for use in GLSL < 130
    float texVal = texture2DProj(sTex, vec4(loc.xy + (offset * texmapscale * loc.w), loc.z, loc.w)).r;
    //perform shadow comparison
    return texVal > (loc.z/loc.w)-zbias ? 1.0 : 0.0;
}

//basic pseudo-random number generator
mediump float random(in mediump vec4 seed4)
{
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

lowp float sampleShadowMap(in highp sampler2D sTex, in highp vec4 coord)
{
    //z-bias is modified using the angle between the surface and the light
    //gives less shadow acne
    float zbias = 0.0025 * tan(acos(dot(normal, normalize(sunInfo.xyz))));
    zbias = clamp(zbias, 0.0, 0.01);
    
    float sum = 0.0;
    for(int i=0;i<16;++i)
    {
        //choose 16 "random"  locations out of 64 using the screen-space coordinates
        int index = int(mod( int(64.0*random(vec4(gl_FragCoord.xyy,i))), 64));
        sum += offset_lookup(sTex, coord, poissonDisk[index], zbias);
    }
    
    return clamp(sum / 16.0,0.0,1.0);
}
#endif

// Calculates the Oren-Nayar reflectance with fixed sigma and E0 (https://en.wikipedia.org/wiki/Oren%E2%80%93Nayar_reflectance_model)
mediump float orenNayar(in mediump vec3 normal, in highp vec3 lightDir, in highp vec3 viewDir)
{
    // GZ next 2 dont require highp IMHO
    mediump float cosAngleLightNormal = dot(normal, lightDir);
    mediump float cosAngleEyeNormal = dot(normal, viewDir);
    //acos can be quite expensive, can we avoid it?
    mediump float angleLightNormal = acos(cosAngleLightNormal);
    mediump float angleEyeNormal = acos(cosAngleEyeNormal);
    mediump float alpha = max(angleEyeNormal, angleLightNormal);
    mediump float beta = min(angleEyeNormal, angleLightNormal);
    mediump float gamma = dot(viewDir - normal * cosAngleEyeNormal, lightDir - normal * cosAngleLightNormal);
    // GZ next 5 can be lowp instead of mediump. Roughness original 1.0, then 0.8 in 0.14.0.
    lowp float roughness = 0.9;
    lowp float roughnessSquared = roughness * roughness;
    lowp float A = 1.0 - 0.5 * (roughnessSquared / (roughnessSquared + 0.57));
    lowp float B = 0.45 * (roughnessSquared / (roughnessSquared + 0.09));
    lowp float C = sin(alpha) * tan(beta);
    // GZ final number was 2, but this causes overly bright moon. was 1.5 in 0.14.0.
    return max(0.0, cosAngleLightNormal) * (A + B * max(0.0, gamma) * C) * 1.85;
}

void main()
{
    mediump float final_illumination = 1.0;
#if defined(IS_OBJ) || defined(IS_MOON)
    mediump float lum = 1.;
#else
    mediump float lum = lum_;
#endif
#ifdef RINGS_SUPPORT
    if(isRing)
        lum=1.0;
#endif
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
#endif
#if defined(IS_OBJ) || defined(IS_MOON)
    // Use an Oren-Nayar model for rough surfaces
    // Ref: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Oren-Nayar
    lum = orenNayar(normal, lightDirection, eyeDirection);
#endif
//Reduce lum if sky is bright, to avoid burnt-out look in daylight sky.
    lum *= (1.0-0.4*skyBrightness);
#ifdef SHADOWMAP
    //use shadowmapping
    float shadow = sampleShadowMap(shadowTex, shadowCoord);
    lum*=shadow;
#endif
    mediump vec4 litColor = vec4(lum * final_illumination * diffuseLight + ambientLight, 1.0);
#ifdef IS_MOON
    if(final_illumination < 0.99)
    {
        lowp vec4 shadowColor = texture2D(earthShadow, vec2(final_illumination, 0.5));
        gl_FragColor = mix(texture2D(tex, texc) * litColor, shadowColor, shadowColor.a);
    }
    else
#endif
    {
        gl_FragColor = texture2D(tex, texc) * litColor;
        /*
        #ifdef SHADOWMAP
        vec2 col = vec2(shadow,shadow);
        if(shadow>=1.0)
            col = shadowCoord.xy;
        gl_FragColor = vec4(shadow,shadow,shadow,1.0);
        #endif
        */
    }
}
