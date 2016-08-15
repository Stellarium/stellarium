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
const vec2 poissonDisk[64] = vec2[64]( 
   vec2(-0.610470, -0.702763),
   vec2( 0.609267,  0.765488),
   vec2(-0.817537, -0.412950),
   vec2( 0.777710, -0.446717),
   vec2(-0.668764, -0.524195),
   vec2( 0.425181,  0.797780),
   vec2(-0.766728, -0.065185),
   vec2( 0.266692,  0.917346),
   vec2(-0.578028, -0.268598),
   vec2( 0.963767,  0.079058),
   vec2(-0.968971, -0.039291),
   vec2( 0.174263, -0.141862),
   vec2(-0.348933, -0.505110),
   vec2( 0.837686, -0.083142),
   vec2(-0.462722, -0.072878),
   vec2( 0.701887, -0.281632),
   vec2(-0.377209, -0.247278),
   vec2( 0.765589,  0.642157),
   vec2(-0.678950,  0.128138),
   vec2( 0.418512, -0.186050),
   vec2(-0.442419,  0.242444),
   vec2( 0.442748, -0.456745),
   vec2(-0.196461,  0.084314),
   vec2( 0.536558, -0.770240),
   vec2(-0.190154, -0.268138),
   vec2( 0.643032, -0.584872),
   vec2(-0.160193, -0.457076),
   vec2( 0.089220,  0.855679),
   vec2(-0.200650, -0.639838),
   vec2( 0.220825,  0.710969),
   vec2(-0.330313, -0.812004),
   vec2(-0.046886,  0.721859),
   vec2( 0.070102, -0.703208),
   vec2(-0.161384,  0.952897),
   vec2( 0.034711, -0.432054),
   vec2(-0.508314,  0.638471),
   vec2(-0.026992, -0.163261),
   vec2( 0.702982,  0.089288),
   vec2(-0.004114, -0.901428),
   vec2( 0.656819,  0.387131),
   vec2(-0.844164,  0.526829),
   vec2( 0.843124,  0.220030),
   vec2(-0.802066,  0.294509),
   vec2( 0.863563,  0.399832),
   vec2( 0.268762, -0.576295),
   vec2( 0.465623,  0.517930),
   vec2( 0.340116, -0.747385),
   vec2( 0.223493,  0.516709),
   vec2( 0.240980, -0.942373),
   vec2(-0.689804,  0.649927),
   vec2( 0.272309, -0.297217),
   vec2( 0.378957,  0.162593),
   vec2( 0.061461,  0.067313),
   vec2( 0.536957,  0.249192),
   vec2(-0.252331,  0.265096),
   vec2( 0.587532, -0.055223),
   vec2( 0.034467,  0.289122),
   vec2( 0.215271,  0.278700),
   vec2(-0.278059,  0.615201),
   vec2(-0.369530,  0.791952),
   vec2(-0.026918,  0.542170),
   vec2( 0.274033,  0.010652),
   vec2(-0.561495,  0.396310),
   vec2(-0.367752,  0.454260)
);

lowp float offset_lookup(in highp sampler2D sTex, in vec4 loc, in vec2 offset, in float zbias)
{
    //the macro SM_SIZE is set to the shadowmap size
    const vec2 texmapscale=vec2(1.0/SM_SIZE, 1.0/SM_SIZE);
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
        int index = int(64.0*random(vec4(gl_FragCoord.xyy,i))) % 64;
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
