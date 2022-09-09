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
const vec4 scatteringCrossSection_molecules=vec4(7.24905737e-31,5.64417092e-31,4.45984673e-31,3.57049192e-31);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.0430000015,0.0670000017,0.107000001,0.0900000036);
const vec4 solarIrradianceAtTOA=vec4(1.96800005,1.87699997,1.85399997,1.81799996);
const vec4 lightPollutionRelativeRadiance=vec4(2.15e-06,1.11400004e-06,3.85800013e-06,2.40999998e-05);
const vec4 wavelengths=vec4(485.333344,516.666687,548,579.333313);
const int wlSetIndex=1;
#endif
#line 5 0 // densities.frag
float scattererNumberDensity_molecules(float altitude)
{
        const float rayleighScaleHeight=8*km;
        return 3.08458e25*exp(-1/rayleighScaleHeight * altitude);
}
float scattererNumberDensity_aerosols(float altitude)
{
        const float mieScaleHeight=1.2*km;
        return 1.03333e8*exp(-1/mieScaleHeight*altitude);
}
float absorberNumberDensity_ozone(float altitude)
{
        const float totalOzoneAmount=370*dobsonUnit;

        float density;

        // A fit to AFGL atmospheric constituent profile. U.S. standard atmosphere 1976. (AFGL-TR-86-0110)
        // Reference was taken from data supplied with libRadtran.
        if(altitude < 8*km)
            density = 7.2402403521159135e-6 - 1.206527437798165e-7/km * altitude;
        else if(altitude < 21.5*km)
            density = -0.000020590185333577628 + 3.3581504669318765e-6/km * altitude;
        else if(altitude < 39*km)
            density = 0.00010542813563268143 - 2.50316678731273e-6/km * altitude;
        else
            density = 0.04298160111157969 * exp(-0.2208669270720561/km * altitude);

        density *= totalOzoneAmount;

        return density;
}
float scattererDensity(float alt) { return scattererNumberDensity_aerosols(alt); }
vec4 scatteringCrossSection() { return vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14); }
