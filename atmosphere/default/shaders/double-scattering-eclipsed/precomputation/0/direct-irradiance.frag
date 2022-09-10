#version 330
#extension GL_ARB_shading_language_420pack : require

#line 1 1 // const.h.glsl
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
const vec4 scatteringCrossSection_molecules=vec4(2.39459446e-30,1.71496275e-30,1.26023066e-30,9.46713716e-31);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.0350000001,0.0370000005,0.0399999991,0.0410000011);
const vec4 solarIrradianceAtTOA=vec4(1.03699994,1.24899995,1.68400002,1.97500002);
const vec4 lightPollutionRelativeRadiance=vec4(0,0,4.3e-07,1.623e-06);
const vec4 wavelengths=vec4(360,391.333344,422.666656,454);
const int wlSetIndex=0;
#endif
#line 5 0 // direct-irradiance.frag
#line 1 2 // texture-sampling-functions.h.glsl
#ifndef INCLUDE_ONCE_AF5AE9F4_8A9A_4521_838A_F8281B8FEB53
#define INCLUDE_ONCE_AF5AE9F4_8A9A_4521_838A_F8281B8FEB53
vec4 transmittanceToAtmosphereBorder(const float cosViewZenithAngle, const float altitude);
vec4 transmittance(const float cosViewZenithAngle, const float altitude, const float dist,
                   const bool viewRayIntersectsGround);
vec4 irradiance(const float cosSunZenithAngle, const float altitude);
vec4 scattering(const float cosSunZenithAngle, const float cosViewZenithAngle,
                const float dotViewSun, const float altitude, const bool viewRayIntersectsGround,
                const int scatteringOrder);
vec4 lightPollutionScattering(const float altitude, const float cosViewZenithAngle, const bool viewRayIntersectsGround);
#endif
#line 6 0 // direct-irradiance.frag

vec4 computeDirectGroundIrradiance(const float cosSunZenithAngle, const float altitude)
{
    // Several approximations are used:
    // * Radiance is assumed independent of position on the solar disk.
    // * Transmittance is assumed to not change much over the solar disk (i.e. we approximate it with a constant).
    // * Instead of true integration of the cosine factor in the integrand from the definition of irradiance we use its
    //   value in the center of the solar disk, assuming it to be a kind of "average".
    // * When the Sun is partially behind the astronomical horizon, we approximate the radiative view factor (i.e.
    //   cosine factor integrated over the solar disk) with a simple quadratic spline.
    const float averageCosFactor = cosSunZenithAngle < -sunAngularRadius ? 0
                                      : cosSunZenithAngle > sunAngularRadius ? cosSunZenithAngle
                                      : sqr(cosSunZenithAngle+sunAngularRadius)/(4*sunAngularRadius);
    return solarIrradianceAtTOA * transmittanceToAtmosphereBorder(cosSunZenithAngle, altitude) * averageCosFactor;
}
