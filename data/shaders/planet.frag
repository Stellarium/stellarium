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

VARYING mediump vec2 texc; //texture coord
VARYING highp vec3 P; //original vertex pos in model space

const highp float PI = 3.14159265;

uniform sampler2D tex;
uniform mediump vec2 poleLat; //latitudes of pole caps, in terms of texture coordinate. x>0...north, y<1...south. 
uniform mediump vec3 ambientLight; // Must be in linear sRGB, without OETF application
uniform mediump vec3 diffuseLight; // Must be in linear sRGB, without OETF application
uniform highp vec4 sunInfo;
uniform mediump float skyBrightness;
uniform bool hasAtmosphere;

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
VARYING highp vec4 shadowCoord;
#endif

//light direction in model space, pre-normalized
uniform highp vec3 lightDirection;
#if defined(IS_OBJ) || defined(IS_MOON)
    #define OREN_NAYAR 1
    //x = A, y = B, z = scaling factor (rho/pi * E0), w roughness
    uniform mediump vec4 orenNayarParameters;
#endif
#ifdef IS_MOON
    uniform sampler2D earthShadow;
    uniform mediump float eclipsePush;
    uniform sampler2D normalMap;
    uniform sampler2D horizonMap;

    VARYING highp vec3 normalX;
    VARYING highp vec3 normalY;
    VARYING highp vec3 normalZ;
#else
    VARYING mediump vec3 normalVS; //pre-calculated normals or spherical normals in model space
#endif

const highp float M_PI=3.1415926535897932384626433832795;

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
    highp float texVal = texture2DProj_3(sTex, coords, -1000.0).r;
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
mediump float orenNayar(in mediump vec3 normal, in highp vec3 lightDir, in highp vec3 viewDir, in mediump float A, in mediump float B, in mediump float scale, in mediump float roughSq)
{
    mediump float cosAngleLightNormal = dot(normal, lightDir);  //cos theta_i
    mediump float cosAngleEyeNormal = dot(normal, viewDir); //cos theta_r
    if(cosAngleLightNormal < 0.) return 0.;
    //acos can be quite expensive, can we avoid it?
    mediump float angleLightNormal = acos(cosAngleLightNormal); //theta_i
    mediump float angleEyeNormal = acos(cosAngleEyeNormal); //theta_r
    mediump float alpha = max(angleEyeNormal, angleLightNormal); //alpha = max(theta_i, theta_r)
    mediump float beta = min(angleEyeNormal, angleLightNormal); //beta = min(theta_i, theta_r)
    mediump float gamma = dot(viewDir - normal * cosAngleEyeNormal, lightDir - normal * cosAngleLightNormal); // cos(phi_r-phi_i)
    mediump float C = sin(alpha) * tan(beta);
    mediump float ON = A + B * max(0.0, gamma) * C; // Qualitative model done.
    // Now add third term:
    mediump float C3_2 = (4.0*alpha*beta)/(M_PI*M_PI);
    mediump float C3=0.125*roughSq/(roughSq+0.09)*C3_2*C3_2;
    mediump float third=(1.0-abs(gamma))*C3*tan(0.5*(alpha+beta));
    ON += third;
    // Add the intereflection term:
    mediump float betaterm = 2.0*beta/M_PI;
    mediump float ONir = (1.0-gamma*betaterm*betaterm)*0.17*roughSq/(roughSq+0.13);
    ON += ONir;
    return ON * cosAngleLightNormal * scale;
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
#ifndef IS_MOON
    if(sunInfo.w==0.)
    {
        // We are drawing the Sun
        vec4 texColor = texture2D(tex, texc);
        texColor.rgb = srgbToLinear(texColor.rgb * sunInfo.rgb);
        // Reference: chapter 14.7 "Limb Darkening" in "Allen’s Astrophysical Quantities",
        //            A.N.Cox (ed.), 4th edition, New York: Springer-Verlag, 2002.
        //            DOI 10.1007/978-1-4612-1186-0
        // The values for u2 and v2 for wavelengths 400nm-800nm were taken, linearly
        // interpolated, and integrated against CIE 1931 color matching functions.
        // The results were transformed from XYZ to linear sRGB color space.
        // We call the results for u2 "a1", and for v2 "a2".
        const vec3 a2 = vec3(-0.226988526315793, -0.232934589453355, -0.153026433664999);
        const vec3 a1 = vec3(0.848380336865573, 0.937696820066542, 0.981762186155682);
        const vec3 a0 = vec3(1) - a1 - a2;
        float cosTheta = dot(eyeDirection, normalize(normalVS));
        cosTheta = max(0., cosTheta); // Rounding errors sometimes lead to negative value
        float cosTheta2 = cosTheta*cosTheta;
        vec3 limbDarkeningCoef = a0 + a1*cosTheta + a2*cosTheta2;
        vec3 color = texColor.rgb * limbDarkeningCoef;
        FRAG_COLOR = vec4(linearToSRGB(color), texColor.a);
        return;
    }
#endif
    mediump float final_illumination = 1.0;
#ifdef OREN_NAYAR
    mediump float lum = 1.;
#else
    //simple Lambert illumination
    mediump float c = dot(lightDirection, normalize(normalVS));
    mediump float lum = clamp(c, 0.0, 1.0);
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
                mediump float s = (ring_radius - innerRadius) / (outerRadius - innerRadius);
                lowp float ringAlpha = texture2D(ringS, vec2(s, 0.5)).w;

                // FIXME: this compensates for too much transparency in the texture (see e.g. Saturn)
                ringAlpha = pow(ringAlpha, 1./2.2);

                if(ring_radius > innerRadius && ring_radius < outerRadius)
                    final_illumination = 1.0 - ringAlpha;
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
                // The min(1.0, .) here is required to avoid spurious bright pixels close to the shadow center.
                highp float d = acos(min(1.0, dot(normalize(sunPosition - P), normalize(satellitePosition - P))));

                mediump float illumination = 1.0;
                if(d >= R + r) // distance too far
                {
                    // illumination = 1.0; // NOP
                }
                else if( d <= r - R ) // fully inside umbra
                {
#ifdef IS_MOON
                    illumination = (d / (r - R)) * 0.594; // prepare texture coordinate. 0.6=umbra edge. Smaller number->larger shadow.
#else
                    illumination = 0.0;
#endif
                }
                else if(d <= R - r)
                {
                    // penumbra completely inside (Annular, or a moon transit in front of the Sun.)
                    illumination = 1.0 - r * r / (R * R);
                }
                else // penumbra: partially inside
                {
#ifdef IS_MOON
                    //illumination = ((d - abs(R-r)) / (R + r - abs(R-r))) * 0.4 + 0.6;
                    illumination = ((d - r + R) / (2.0 * R )) * 0.406 + 0.594;
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

    mediump float horizonShadowCoefficient = 1.;
    {
        // Check whether the fragment is in the shadow of surrounding mountains or the horizon
        mediump vec3 lonDir = normalX;
        mediump vec3 northDir = normalY;
        mediump vec3 zenith = normalZ;
        mediump float sunAzimuth = atan(dot(lightDirection,lonDir), dot(lightDirection,northDir));
        mediump float sinSunElevation = dot(zenith, lightDirection);
        mediump vec4 horizonElevSample = (texture2D(horizonMap, texc) - 0.5) * 2.;
        mediump vec4 sinHorizElevs = sign(horizonElevSample) * horizonElevSample * horizonElevSample;
        mediump float sinHorizElevLeft, sinHorizElevRight;
        mediump float alpha;
        if(sunAzimuth >= PI/2.)
        {
            // Sun is between East and South
            sinHorizElevLeft = sinHorizElevs[1];
            sinHorizElevRight = sinHorizElevs[2];
            alpha = (sunAzimuth - PI/2.) / (PI/2.);
        }
        else if(sunAzimuth >= 0.)
        {
            // Sun is between North and East
            sinHorizElevLeft = sinHorizElevs[0];
            sinHorizElevRight = sinHorizElevs[1];
            alpha = sunAzimuth / (PI/2.);
        }
        else if(sunAzimuth <= -PI/2.)
        {
            // Sun is between South and West
            sinHorizElevLeft = sinHorizElevs[2];
            sinHorizElevRight = sinHorizElevs[3];
            alpha = (sunAzimuth + PI) / (PI/2.);
        }
        else
        {
            // Sun is between West and North
            sinHorizElevLeft = sinHorizElevs[3];
            sinHorizElevRight = sinHorizElevs[0];
            alpha = (sunAzimuth + PI/2.) / (PI/2.);
        }
        mediump float horizElevLeft = asin(sinHorizElevLeft);
        mediump float horizElevRight = asin(sinHorizElevRight);
        mediump float horizElev = horizElevLeft + (horizElevRight-horizElevLeft)*alpha;
        if(sinSunElevation < sin(horizElev))
            horizonShadowCoefficient = 0.;
    }
#else
    // important to normalize here again
    mediump vec3 normal = normalize(normalVS);
#endif
#ifdef OREN_NAYAR
    // Use an Oren-Nayar model for rough surfaces
    // Ref: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Oren-Nayar
    lum = orenNayar(normal, lightDirection, eyeDirection, orenNayarParameters.x, orenNayarParameters.y, orenNayarParameters.z, orenNayarParameters.w);
#endif
#ifdef IS_MOON
    lum *= horizonShadowCoefficient;
#endif
    //calculate pseudo-outgassing/rim-lighting effect
    lowp float outgas = 0.0;
    if(outgasParameters.x > 0.0)
    {
        outgas = outgasParameters.x * outgasFactor(normal, eyeDirection, outgasParameters.y);
    }
    //Reduce lum if sky is bright, to avoid burnt-out look in daylight sky.
    //lum *= (1.0-0.4*skyBrightness);
    lum *= clamp((0.9-0.05*log(skyBrightness)), 0.1, 0.9);
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

    if(hasAtmosphere)
    {
        // Planets with atmosphere don't have such a sharp terminator as we get with
        // Oren-Nayar model. The following is a hack to make the terminator smoother.
        // TODO: replace it with the correct BRDF (possibly different for different planets).
        lum *= lum;
    }
    //final lighting color
    mediump vec4 litColor = vec4(lum * final_illumination * diffuseLight + ambientLight, 1.0);

    //apply texture-colored rimlight
    //litColor.xyz = clamp( litColor.xyz + vec3(outgas), 0.0, 1.0);

    lowp vec4 texColor;
#ifdef RINGS_SUPPORT
    if(isRing)
    {
        float radius = length(texc);
        float s = (radius - innerRadius) / (outerRadius - innerRadius);
        vec2 texCoord = vec2(s, 0.5);
        texColor = texture2D(tex, texCoord);
        // Guard against poor quality mipmap filtering (e.g. Mesa with NPOT textures).
        if(radius > outerRadius)
            texColor = vec4(0);
    }
    else
#endif
    {
        texColor = texture2D(tex, texc);
    }

    texColor.rgb = srgbToLinear(texColor.rgb);

    mediump vec4 finalColor = texColor;
    // apply (currently only Martian) pole caps. texc.t=0 at south pole, 1 at north pole. 
    if (texc.t>poleLat.x-0.01+0.001*sin(texc.s*18.*M_PI)) {	// North pole near t=1
        mediump float mixfactor=1.;
        if (texc.t<poleLat.x+0.01+0.001*sin(texc.s*18.*M_PI))
            mixfactor=(texc.t-poleLat.x+0.01-0.001*sin(texc.s*18.*M_PI))/0.02;
        //finalColor.xyz=mix(vec3(1., 1., 1.), finalColor.xyz, 1.-mixfactor); 
        finalColor.xyz=mix(vec3(1., 1., 1.), finalColor.xyz, smoothstep(0., 1., 1.-mixfactor)); 
    }
    if (texc.t<poleLat.y+0.01+0.001*sin(texc.s*18.*M_PI)) {	// South pole near texc.t~0
        mediump float mixfactor=1.;
        if (texc.t>poleLat.y-0.01+0.001*sin(texc.s*18.*M_PI))
            mixfactor=(poleLat.y+0.01-texc.t-0.001*sin(texc.s*18.*M_PI))/0.02;
        //finalColor.xyz=mix(vec3(1., 1., 1.), finalColor.xyz, 1.-mixfactor); 
        finalColor.xyz=mix(vec3(1., 1., 1.), finalColor.xyz, smoothstep(0., 1., 1.-mixfactor)); 
    }
    finalColor *= litColor;
#ifdef IS_MOON
    if(final_illumination < 0.9999)
    {
        lowp vec4 shadowColor = texture2D(earthShadow, vec2(final_illumination, 0.5));
        // FIXME: this should be calculated properly in linear space as
        // extinction of sunlight, and with subsequent tone mapping.
        // Current implementation is a legacy from older times.
        lowp vec4 color = vec4(linearToSRGB(finalColor.rgb), finalColor.a);
        lowp float alpha = clamp(shadowColor.a, 0.0, 0.7); // clamp alpha to allow some maria detail
        finalColor = eclipsePush * (1.0-0.75*shadowColor.a) * mix(color, shadowColor, alpha);
        finalColor.rgb = srgbToLinear(finalColor.rgb);
    }
#endif

    //apply white rimlight
    finalColor.xyz = clamp( finalColor.xyz + vec3(outgas), 0.0, 1.0);

    FRAG_COLOR = vec4(linearToSRGB(finalColor.rgb), finalColor.a);
    //to debug texture issues, uncomment and reload shader
    //FRAG_COLOR = texColor;
}
