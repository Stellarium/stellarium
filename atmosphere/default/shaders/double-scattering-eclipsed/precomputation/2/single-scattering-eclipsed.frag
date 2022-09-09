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
const vec4 scatteringCrossSection_molecules=vec4(2.89217878e-31,2.36756505e-31,1.95668875e-31,1.63119977e-31);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.0700000003,0.057,0.0469999984,0.137999997);
const vec4 solarIrradianceAtTOA=vec4(1.72300005,1.60399997,1.51600003,1.40799999);
const vec4 lightPollutionRelativeRadiance=vec4(3.35000004e-05,1.33100002e-05,9.49999958e-06,4.30399996e-06);
const vec4 wavelengths=vec4(610.666687,642,673.333313,704.666687);
const int wlSetIndex=2;
#endif
#line 5 0 // single-scattering-eclipsed.frag
#line 1 2 // densities.h.glsl
float scattererNumberDensity_molecules(float altitude);
float scattererNumberDensity_aerosols(float altitude);
float absorberNumberDensity_ozone(float altitude);
vec4 scatteringCrossSection();
float scattererDensity(float altitude);
#line 6 0 // single-scattering-eclipsed.frag
#line 1 3 // common-functions.h.glsl
#ifndef INCLUDE_ONCE_B0879E51_5608_481B_9832_C7D601BD6AB1
#define INCLUDE_ONCE_B0879E51_5608_481B_9832_C7D601BD6AB1
float distanceToAtmosphereBorder(const float cosZenithAngle, const float observerAltitude);
float distanceToNearestAtmosphereBoundary(const float cosZenithAngle, const float observerAltitude,
                                          const bool viewRayIntersectsGround);
float distanceToGround(const float cosZenithAngle, const float observerAltitude);
float cosZenithAngleOfHorizon(const float altitude);
bool rayIntersectsGround(const float cosViewZenithAngle, const float observerAltitude);
float safeSqrt(const float x);
float safeAtan(const float y, const float x);
float clampCosine(const float x);
float clampDistance(const float x);
float clampAltitude(const float altitude);
vec3 normalToEarth(vec3 point);
float pointAltitude(vec3 point);
vec4 rayleighPhaseFunction(float dotViewSun);
float sunVisibility(const float cosSunZenithAngle, float altitude);
float moonAngularRadius(const vec3 cameraPosition, const vec3 moonPosition);
float sunVisibilityDueToMoon(const vec3 camera, const vec3 sunDir, const vec3 moonDir);
vec3 sphereIntegrationSampleDir(const int index, const int pointCountOnSphere);
float sphereIntegrationSolidAngleDifferential(const int pointCountOnSphere);

void swap(inout float x, inout float y);

bool debugDataPresent();
vec4 debugData();
void setDebugData(float a);
void setDebugData(float a,float b);
void setDebugData(float a,float b,float c);
void setDebugData(float a,float b,float c,float d);
#endif
#line 7 0 // single-scattering-eclipsed.frag
#line 1 4 // texture-sampling-functions.h.glsl
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
#line 8 0 // single-scattering-eclipsed.frag
#line 1 5 // phase-functions.h.glsl
vec4 phaseFunction_molecules(float dotViewSun);
vec4 phaseFunction_aerosols(float dotViewSun);
vec4 currentPhaseFunction(float dotViewSun);
#line 9 0 // single-scattering-eclipsed.frag

float cosZenithAngle(vec3 origin, vec3 direction)
{
    return dot(direction, normalToEarth(origin));
}

// This function omits phase function and solar irradiance: these are to be applied somewhere in the calling code.
vec4 computeSingleScatteringIntegrandEclipsed(const float cosSunZenithAngle, const float cosViewZenithAngle,
                                              const float dotViewSun, const float altitude,
                                              const float dist, const bool viewRayIntersectsGround,
                                              const vec3 scatterer, const vec3 sunDir, const vec3 moonPos)
{
    const float r=earthRadius+altitude;
    // Clamping only guards against rounding errors here, we don't try to handle here the case when the
    // endpoint of the view ray intentionally appears in outer space.
    const float altAtDist=clampAltitude(sqrt(sqr(dist)+sqr(r)+2*r*dist*cosViewZenithAngle)-earthRadius);
    const float cosSunZenithAngleAtDist=clampCosine((r*cosSunZenithAngle+dist*dotViewSun)/(earthRadius+altAtDist));

    const vec4 xmittance=transmittance(cosViewZenithAngle, altitude, dist, viewRayIntersectsGround)
                                                    *
                         transmittanceToAtmosphereBorder(cosSunZenithAngleAtDist, altAtDist)
                                                    *
                                    sunVisibilityDueToMoon(scatterer,sunDir,moonPos)
                                                    *
                            // FIXME: this ignores orientation of the crescent of eclipsed Sun WRT horizon
                                    sunVisibility(cosSunZenithAngleAtDist, altAtDist);
#if 1 /*ALL_SCATTERERS_AT_ONCE_WITH_PHASE_FUNCTION*/
    vec4 totalScatteringCoefficient=vec4(0);
    totalScatteringCoefficient +=  scattererNumberDensity_molecules(altAtDist) * scatteringCrossSection_molecules * phaseFunction_molecules(dotViewSun);
    totalScatteringCoefficient +=  scattererNumberDensity_aerosols(altAtDist) * scatteringCrossSection_aerosols * phaseFunction_aerosols(dotViewSun);

    return xmittance * totalScatteringCoefficient;
#else
    return xmittance * scattererDensity(altAtDist);
#endif
}

vec4 computeSingleScatteringEclipsed(const vec3 camera, const vec3 viewDir, const vec3 sunDir, const vec3 moonPos,
                                     const bool viewRayIntersectsGround)
{
    const float cosViewZenithAngle=cosZenithAngle(camera,viewDir);
    const float cosSunZenithAngle=cosZenithAngle(camera,sunDir);
    const float altitude=pointAltitude(camera);
    const float dotViewSun=dot(viewDir,sunDir);
    const float integrInterval=distanceToNearestAtmosphereBoundary(cosViewZenithAngle, altitude,
                                                                   viewRayIntersectsGround);

    // Using the midpoint rule for quadrature
    vec4 spectrum=vec4(0);
    const float dl=integrInterval/radialIntegrationPoints;
    for(int n=0; n<radialIntegrationPoints; ++n)
    {
        const float dist=(n+0.5)*dl;
        spectrum += computeSingleScatteringIntegrandEclipsed(cosSunZenithAngle, cosViewZenithAngle, dotViewSun,
                                                             altitude, dist, viewRayIntersectsGround,
                                                             camera+viewDir*dist, sunDir, moonPos);
    }

    spectrum *= dl*solarIrradianceAtTOA
#if 1 /*ALL_SCATTERERS_AT_ONCE_WITH_PHASE_FUNCTION*/
                                // the multiplier is already included
#else
                                        * scatteringCrossSection()
#endif
        ;

    return spectrum;
}
