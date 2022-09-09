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
#line 5 0 // compute-eclipsed-double-scattering.frag
#line 1 2 // phase-functions.h.glsl
vec4 phaseFunction_molecules(float dotViewSun);
vec4 phaseFunction_aerosols(float dotViewSun);
vec4 currentPhaseFunction(float dotViewSun);
#line 6 0 // compute-eclipsed-double-scattering.frag
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
#line 7 0 // compute-eclipsed-double-scattering.frag
#line 1 4 // texture-coordinates.h.glsl
#ifndef INCLUDE_ONCE_72E237D7_42B6_462B_90E4_73AB6B6E4DE4
#define INCLUDE_ONCE_72E237D7_42B6_462B_90E4_73AB6B6E4DE4

float texCoordToUnitRange(const float texCoord, const float texSize);
float unitRangeToTexCoord(const float u, const float texSize);
struct TransmittanceTexVars
{
    float cosViewZenithAngle;
    float altitude;
};
TransmittanceTexVars transmittanceTexCoordToTexVars(const vec2 texCoord);
vec2 transmittanceTexVarsToTexCoord(const float cosVZA, float altitude);
struct IrradianceTexVars
{
    float cosSunZenithAngle;
    float altitude;
};
IrradianceTexVars irradianceTexCoordToTexVars(const vec2 texCoord);
vec2 irradianceTexVarsToTexCoord(const float cosSunZenithAngle, const float altitude);

struct ScatteringTexVars
{
    float cosSunZenithAngle;
    float cosViewZenithAngle;
    float dotViewSun;
    float altitude;
    bool viewRayIntersectsGround;
};
ScatteringTexVars scatteringTexIndicesToTexVars(const vec3 texIndices);
vec4 sample4DTexture(const sampler3D tex, const float cosSunZenithAngle, const float cosViewZenithAngle,
                     const float dotViewSun, const float altitude, const bool viewRayIntersectsGround);
vec4 sample3DTexture(const sampler3D tex, const float cosSunZenithAngle, const float cosViewZenithAngle,
                     const float dotViewSun, const float altitude, const bool viewRayIntersectsGround);
vec4 sample3DTextureGuided(const sampler3D tex,
                           const sampler3D interpolationGuides01Tex, const sampler3D interpolationGuides02Tex,
                           const float cosSunZenithAngle, const float cosViewZenithAngle,
                           const float dotViewSun, const float altitude, const bool viewRayIntersectsGround);

struct EclipseScatteringTexVars
{
    float azimuthRelativeToSun;
    float cosViewZenithAngle;
    bool viewRayIntersectsGround;
};
EclipseScatteringTexVars eclipseTexCoordsToTexVars(const vec2 texCoords, const float altitude);
vec2 eclipseTexVarsToTexCoords(const float azimuthRelativeToSun, const float cosViewZenithAngle,
                               const float altitude, const bool viewRayIntersectsGround,
                               const vec2 texSize);

vec4 sampleEclipseDoubleScattering3DTexture(const sampler3D tex, const float cosSunZenithAngle,
                                            const float cosViewZenithAngle, const float azimuthRelativeToSun,
                                            const float altitude, const bool viewRayIntersectsGround);

struct LightPollution2DCoords
{
    float cosViewZenithAngle;
    float altitude;
};
struct LightPollutionTexVars
{
    float altitude;
    float cosViewZenithAngle;
    bool viewRayIntersectsGround;
};
LightPollutionTexVars scatteringTexIndicesToLightPollutionTexVars(const vec2 texIndices);
vec2 lightPollutionTexVarsToTexCoords(const float altitude, const float cosViewZenithAngle, const bool viewRayIntersectsGround);

#endif
#line 8 0 // compute-eclipsed-double-scattering.frag
#line 1 5 // radiance-to-luminance.h.glsl
const mat4 radianceToLuminance=mat4(1.38997746,0.0419133306,6.48549128,0,  105.968163,3.00652099,500.960266,142.641495,  3776.24023,119.971481,18207.7598,6412.0166,  6910.01318,981.06665,37504.0586,26741.9102);
#line 9 0 // compute-eclipsed-double-scattering.frag
#line 1 6 // eclipsed-direct-irradiance.h.glsl
#ifndef INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D
#define INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D

vec4 calcEclipsedDirectGroundIrradiance(const vec3 pointOnGround, const vec3 sunDir, const vec3 moonPos);

#endif
#line 10 0 // compute-eclipsed-double-scattering.frag
#line 1 7 // texture-sampling-functions.h.glsl
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
#line 11 0 // compute-eclipsed-double-scattering.frag
#line 1 8 // single-scattering-eclipsed.h.glsl
#ifndef INCLUDE_ONCE_050024D2_BD56_434E_8D40_2055DA0B78EC
#define INCLUDE_ONCE_050024D2_BD56_434E_8D40_2055DA0B78EC

vec4 computeSingleScatteringEclipsed(const vec3 camera, const vec3 viewDir, const vec3 sunDir, const vec3 moonDir,
                                     const bool viewRayIntersectsGround);

#endif
#line 12 0 // compute-eclipsed-double-scattering.frag
#line 1 9 // total-scattering-coefficient.h.glsl
vec4 totalScatteringCoefficient(float altitude, float dotViewInc);
#line 13 0 // compute-eclipsed-double-scattering.frag

in vec3 position;
out vec4 partialRadiance;

vec4 computeDoubleScatteringEclipsedDensitySample(const int directionIndex, const vec3 cameraViewDir, const vec3 scatterer,
                                                  const vec3 sunDir, const vec3 moonPos)
{
    const vec3 zenith=vec3(0,0,1);
    const float altitude=pointAltitude(scatterer);
    // XXX: Might be a good idea to increase sampling density near horizon and decrease near zenith&nadir.
    // XXX: Also sampling should be more dense near the light source, since there often is a strong forward
    //       scattering peak like that of Mie phase functions.
    // TODO:At the very least, the phase functions should be lowpass-filtered to avoid aliasing, before
    //       sampling them here.

    // Instead of iterating over all directions, we compute only one sample, for only one direction, to
    // facilitate parallelization. The summation will be done after this parallel computation of the samples.

    const float dSolidAngle = sphereIntegrationSolidAngleDifferential(eclipseAngularIntegrationPoints);
    // Direction to the source of incident ray
    const vec3 incDir = sphereIntegrationSampleDir(directionIndex, eclipseAngularIntegrationPoints);

    // NOTE: we don't recalculate sunDir as we do in computeScatteringDensity(), because it would also require
    // at least recalculating the position of the Moon. Instead we take into account scatterer's position to
    // calculate zenith direction and the direction to the incident ray.
    const vec3 zenithAtScattererPos=normalize(scatterer-earthCenter);
    const float cosIncZenithAngle=dot(incDir, zenithAtScattererPos);
    const bool incRayIntersectsGround=rayIntersectsGround(cosIncZenithAngle, altitude);

    float distToGround=0;
    vec4 transmittanceToGround=vec4(0);
    if(incRayIntersectsGround)
    {
        distToGround = distanceToGround(cosIncZenithAngle, altitude);
        transmittanceToGround = transmittance(cosIncZenithAngle, altitude, distToGround, incRayIntersectsGround);
    }

    vec4 incidentRadiance = vec4(0);
    // XXX: keep this ground-scattered radiation logic in sync with that in computeScatteringDensity().
    {
        // The point where incident light originates on the ground, with current incDir
        const vec3 pointOnGround = scatterer+incDir*distToGround;
        const vec4 groundIrradiance = calcEclipsedDirectGroundIrradiance(pointOnGround, sunDir, moonPos);
        // Radiation scattered by the ground
        const float groundBRDF = 1/PI; // Assuming Lambertian BRDF, which is constant
        incidentRadiance += transmittanceToGround*groundAlbedo*groundIrradiance*groundBRDF;
    }
    // Radiation scattered by the atmosphere
    incidentRadiance+=computeSingleScatteringEclipsed(scatterer,incDir,sunDir,moonPos,incRayIntersectsGround);

    const float dotViewInc = dot(cameraViewDir, incDir);
    return dSolidAngle * incidentRadiance * totalScatteringCoefficient(altitude, dotViewInc);
}

uniform float cameraAltitude;
uniform vec3 cameraViewDir;
uniform float sunZenithAngle;
uniform vec3 moonPositionRelativeToSunAzimuth;

void main()
{
    const vec3 sunDir=vec3(sin(sunZenithAngle), 0, cos(sunZenithAngle));
    const vec3 cameraPos=vec3(0,0,cameraAltitude);
    const bool viewRayIntersectsGround=rayIntersectsGround(cameraViewDir.z, cameraAltitude);

    const float radialIntegrInterval=distanceToNearestAtmosphereBoundary(cameraViewDir.z, cameraAltitude,
                                                                         viewRayIntersectsGround);

    const int directionIndex=int(gl_FragCoord.x);
    const float radialDistIndex=gl_FragCoord.y;

    // Using midpoint rule for quadrature
    const float dl=radialIntegrInterval/radialIntegrationPoints;
    const float dist=(radialDistIndex+0.5)*dl;
    const vec4 scDensity=computeDoubleScatteringEclipsedDensitySample(directionIndex, cameraViewDir, cameraPos+cameraViewDir*dist,
                                                                      sunDir, moonPositionRelativeToSunAzimuth);
    const vec4 xmittance=transmittance(cameraViewDir.z, cameraAltitude, dist, viewRayIntersectsGround);
    partialRadiance = scDensity*xmittance*dl;

}
