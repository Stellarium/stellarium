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
#line 3 0 // compute-eclipsed-double-scattering.frag
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
#line 4 0 // compute-eclipsed-double-scattering.frag
#line 1 3 // phase-functions.h.glsl
vec4 phaseFunction_molecules(float dotViewSun);
vec4 phaseFunction_aerosols(float dotViewSun);
vec4 currentPhaseFunction(float dotViewSun);
#line 5 0 // compute-eclipsed-double-scattering.frag
#line 1 4 // common-functions.h.glsl
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
#line 6 0 // compute-eclipsed-double-scattering.frag
#line 1 5 // texture-coordinates.h.glsl
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
#line 7 0 // compute-eclipsed-double-scattering.frag
#line 1 6 // radiance-to-luminance.h.glsl
const mat4 radianceToLuminance=mat4(1196.74268,3672.67432,12945.498,45528.7344,  835.02478,13767.5205,2120.37939,51317.4727,  8632.59277,21193.416,222.095078,27381.1602,  19410,18758.1953,35.7829933,6712.12158);
#line 8 0 // compute-eclipsed-double-scattering.frag
#line 1 7 // eclipsed-direct-irradiance.h.glsl
#ifndef INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D
#define INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D

vec4 calcEclipsedDirectGroundIrradiance(const vec3 pointOnGround, const vec3 sunDir, const vec3 moonPos);

#endif
#line 9 0 // compute-eclipsed-double-scattering.frag
#line 1 8 // texture-sampling-functions.h.glsl
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
#line 10 0 // compute-eclipsed-double-scattering.frag
#line 1 9 // single-scattering-eclipsed.h.glsl
#ifndef INCLUDE_ONCE_050024D2_BD56_434E_8D40_2055DA0B78EC
#define INCLUDE_ONCE_050024D2_BD56_434E_8D40_2055DA0B78EC

vec4 computeSingleScatteringEclipsed(const vec3 camera, const vec3 viewDir, const vec3 sunDir, const vec3 moonDir,
                                     const bool viewRayIntersectsGround);

#endif
#line 11 0 // compute-eclipsed-double-scattering.frag
#line 1 10 // total-scattering-coefficient.h.glsl
vec4 totalScatteringCoefficient(float altitude, float dotViewInc);
#line 12 0 // compute-eclipsed-double-scattering.frag

in vec3 position;
out vec4 partialRadiance;

vec4 computeDoubleScatteringEclipsedDensitySample(const int directionIndex, const vec3 cameraViewDir, const vec3 scatterer,
                                                  const vec3 sunDir, const vec3 moonPos)
{
    CONST vec3 zenith=vec3(0,0,1);
    CONST float altitude=pointAltitude(scatterer);
    // XXX: Might be a good idea to increase sampling density near horizon and decrease near zenith&nadir.
    // XXX: Also sampling should be more dense near the light source, since there often is a strong forward
    //       scattering peak like that of Mie phase functions.
    // TODO:At the very least, the phase functions should be lowpass-filtered to avoid aliasing, before
    //       sampling them here.

    // Instead of iterating over all directions, we compute only one sample, for only one direction, to
    // facilitate parallelization. The summation will be done after this parallel computation of the samples.

    CONST float dSolidAngle = sphereIntegrationSolidAngleDifferential(eclipseAngularIntegrationPoints);
    // Direction to the source of incident ray
    CONST vec3 incDir = sphereIntegrationSampleDir(directionIndex, eclipseAngularIntegrationPoints);

    // NOTE: we don't recalculate sunDir as we do in computeScatteringDensity(), because it would also require
    // at least recalculating the position of the Moon. Instead we take into account scatterer's position to
    // calculate zenith direction and the direction to the incident ray.
    CONST vec3 zenithAtScattererPos=normalize(scatterer-earthCenter);
    CONST float cosIncZenithAngle=dot(incDir, zenithAtScattererPos);
    CONST bool incRayIntersectsGround=rayIntersectsGround(cosIncZenithAngle, altitude);

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
        CONST vec3 pointOnGround = scatterer+incDir*distToGround;
        CONST vec4 groundIrradiance = calcEclipsedDirectGroundIrradiance(pointOnGround, sunDir, moonPos);
        // Radiation scattered by the ground
        CONST float groundBRDF = 1/PI; // Assuming Lambertian BRDF, which is constant
        incidentRadiance += transmittanceToGround*groundAlbedo*groundIrradiance*groundBRDF;
    }
    // Radiation scattered by the atmosphere
    incidentRadiance+=computeSingleScatteringEclipsed(scatterer,incDir,sunDir,moonPos,incRayIntersectsGround);

    CONST float dotViewInc = dot(cameraViewDir, incDir);
    return dSolidAngle * incidentRadiance * totalScatteringCoefficient(altitude, dotViewInc);
}

uniform float cameraAltitude;
uniform vec3 cameraViewDir;
uniform float sunZenithAngle;
uniform vec3 moonPositionRelativeToSunAzimuth;

void main()
{
    CONST vec3 sunDir=vec3(sin(sunZenithAngle), 0, cos(sunZenithAngle));
    CONST vec3 cameraPos=vec3(0,0,cameraAltitude);
    CONST bool viewRayIntersectsGround=rayIntersectsGround(cameraViewDir.z, cameraAltitude);

    CONST float radialIntegrInterval=distanceToNearestAtmosphereBoundary(cameraViewDir.z, cameraAltitude,
                                                                         viewRayIntersectsGround);

    CONST int directionIndex=int(gl_FragCoord.x);
    CONST float radialDistIndex=gl_FragCoord.y;

    // Using midpoint rule for quadrature
    CONST float dl=radialIntegrInterval/radialIntegrationPoints;
    CONST float dist=(radialDistIndex+0.5)*dl;
    CONST vec4 scDensity=computeDoubleScatteringEclipsedDensitySample(directionIndex, cameraViewDir, cameraPos+cameraViewDir*dist,
                                                                      sunDir, moonPositionRelativeToSunAzimuth);
    CONST vec4 xmittance=transmittance(cameraViewDir.z, cameraAltitude, dist, viewRayIntersectsGround);
    partialRadiance = scDensity*xmittance*dl;

}
