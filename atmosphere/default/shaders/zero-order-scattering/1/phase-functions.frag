#version 330
#line 1 1 // version.h.glsl
#ifndef INCLUDE_ONCE_EF4160B0_E881_42C8_BB48_A408AF2E4354
#define INCLUDE_ONCE_EF4160B0_E881_42C8_BB48_A408AF2E4354

#extension GL_ARB_shading_language_420pack : enable
#ifdef GL_ARB_shading_language_420pack
# define CONST const
#else
# define CONST
#endif

#endif
#line 3 0 // phase-functions.frag
#line 1 2 // const.h.glsl
#ifndef INCLUDE_ONCE_2B59AE86_E78B_4D75_ACDF_5DA644F8E9A3
#define INCLUDE_ONCE_2B59AE86_E78B_4D75_ACDF_5DA644F8E9A3
const float earthRadius=6.371e+06; // must be in meters
const float atmosphereHeight=120000; // must be in meters

const vec3 earthCenter=vec3(0,0,-earthRadius);

const float dobsonUnit = 2.687e20; // molecules/m^2
const float PI=3.1415926535897932;
const float km=1000;
#define sqr(x) ((x)*(x))

uniform float sunAngularRadius=0.00459925318;
const float moonRadius=1737100;
const vec4 scatteringTextureSize=vec4(128,8,32,32);
const vec2 irradianceTextureSize=vec2(64,16);
const vec2 transmittanceTextureSize=vec2(256,64);
const vec2 eclipsedSingleScatteringTextureSize=vec2(32,128);
const vec2 lightPollutionTextureSize=vec2(128,64);
const int radialIntegrationPoints=50;
const int angularIntegrationPoints=512;
const int lightPollutionAngularIntegrationPoints=200;
const int eclipseAngularIntegrationPoints=512;
const int numTransmittanceIntegrationPoints=500;
const vec4 scatteringCrossSection_molecules=vec4(7.24905737e-31,5.64417092e-31,4.45984673e-31,3.57049192e-31);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.0430000015,0.0670000017,0.107000001,0.0900000036);
const vec4 solarIrradianceAtTOA=vec4(1.96800005,1.87699997,1.85399997,1.81799996);
const vec4 lightPollutionRelativeRadiance=vec4(2.15e-06,1.11400004e-06,3.85800013e-06,2.40999998e-05);
const vec4 wavelengths=vec4(485.333344,516.666687,548,579.333313);
const int wlSetIndex=1;
#endif
#line 4 0 // phase-functions.frag

vec4 phaseFunction_molecules(float dotViewSun)
{
        return vec4(3./(16*PI)*(1+sqr(dotViewSun)));
}
vec4 phaseFunction_aerosols(float dotViewSun)
{
        CONST float g=0.76;
        CONST float g2=g*g;
        CONST float k = 3/(8*PI)*(1-g2)/(2+g2);
        return vec4(k * (1+sqr(dotViewSun)) / pow(1+g2 - 2*g*dotViewSun, 1.5) + 1/((1-dotViewSun)*600+0.05))*0.904;
}
vec4 currentPhaseFunction(float dotViewSun) { return vec4(3.4028235e38); }
