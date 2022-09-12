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
#line 3 0 // texture-sampling-functions.frag
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
const vec4 scatteringCrossSection_molecules=vec4(2.89217878e-31,2.36756505e-31,1.95668875e-31,1.63119977e-31);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.0700000003,0.057,0.0469999984,0.137999997);
const vec4 solarIrradianceAtTOA=vec4(1.72300005,1.60399997,1.51600003,1.40799999);
const vec4 lightPollutionRelativeRadiance=vec4(3.35000004e-05,1.33100002e-05,9.49999958e-06,4.30399996e-06);
const vec4 wavelengths=vec4(610.666687,642,673.333313,704.666687);
const int wlSetIndex=2;
#endif
#line 4 0 // texture-sampling-functions.frag
#line 1 3 // phase-functions.h.glsl
vec4 phaseFunction_molecules(float dotViewSun);
vec4 phaseFunction_aerosols(float dotViewSun);
vec4 currentPhaseFunction(float dotViewSun);
#line 5 0 // texture-sampling-functions.frag
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
#line 6 0 // texture-sampling-functions.frag
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
#line 7 0 // texture-sampling-functions.frag

uniform sampler2D transmittanceTexture;
uniform sampler2D irradianceTexture;

uniform sampler3D firstScatteringTexture;
uniform sampler3D multipleScatteringTexture;

uniform sampler2D lightPollutionScatteringTexture;

vec4 irradiance(const float cosSunZenithAngle, const float altitude)
{
    CONST vec2 texCoords=irradianceTexVarsToTexCoord(cosSunZenithAngle, altitude);
    return texture(irradianceTexture, texCoords);
}

vec4 opticalDepthToAtmosphereBorder(const float cosViewZenithAngle, const float altitude)
{
    CONST vec2 texCoords=transmittanceTexVarsToTexCoord(cosViewZenithAngle, altitude);
    // We don't use mip mapping here, but for some reason, on my NVidia GTX 750 Ti with Linux-x86 driver 390.116 I get
    // an artifact when looking into nadir from TOA at some values of texture sizes (in particular, size of
    // transmittance texture for altitude being 4096). This happens when I simply call texture(eclipsedScatteringTexture,
    // texCoords) without specifying LOD.
    // Apparently, the driver uses the derivative for some reason, even though it shouldn't.
    return textureLod(transmittanceTexture, texCoords, 0);
}

vec4 transmittanceToAtmosphereBorder(const float cosViewZenithAngle, const float altitude)
{
    return exp(-opticalDepthToAtmosphereBorder(cosViewZenithAngle,altitude));
}

// Assumes that the endpoint of view ray doesn't intentionally exit atmosphere.
vec4 transmittance(const float cosViewZenithAngle, const float altitude, const float dist,
                   const bool viewRayIntersectsGround)
{
    CONST float r=earthRadius+altitude;
    // Clamping only guards against rounding errors here, we don't try to handle view ray endpoint
    // in space here.
    CONST float altAtDist=clampAltitude(sqrt(sqr(dist)+sqr(r)+2*r*dist*cosViewZenithAngle)-earthRadius);
    CONST float cosViewZenithAngleAtDist=clampCosine((r*cosViewZenithAngle+dist)/(earthRadius+altAtDist));

    vec4 depth;
    if(viewRayIntersectsGround)
    {
        depth=opticalDepthToAtmosphereBorder(-cosViewZenithAngleAtDist, altAtDist) -
              opticalDepthToAtmosphereBorder(-cosViewZenithAngle, altitude);
    }
    else
    {
        depth=opticalDepthToAtmosphereBorder(cosViewZenithAngle, altitude) -
              opticalDepthToAtmosphereBorder(cosViewZenithAngleAtDist, altAtDist);
    }
    return exp(-depth);
}

vec4 calcFirstScattering(const float cosSunZenithAngle, const float cosViewZenithAngle,
                         const float dotViewSun, const float altitude, const bool viewRayIntersectsGround)
{
    CONST vec4 scattering = sample4DTexture(firstScatteringTexture, cosSunZenithAngle, cosViewZenithAngle,
                                          dotViewSun, altitude, viewRayIntersectsGround);
    return scattering*currentPhaseFunction(dotViewSun);
}

vec4 scattering(const float cosSunZenithAngle, const float cosViewZenithAngle,
                const float dotViewSun, const float altitude, const bool viewRayIntersectsGround,
                const int scatteringOrder)
{
    if(scatteringOrder==1)
        return calcFirstScattering(cosSunZenithAngle, cosViewZenithAngle,
                                   dotViewSun, altitude, viewRayIntersectsGround);
    else
        return sample4DTexture(multipleScatteringTexture, cosSunZenithAngle, cosViewZenithAngle,
                               dotViewSun, altitude, viewRayIntersectsGround);
}

vec4 lightPollutionScattering(const float altitude, const float cosViewZenithAngle, const bool viewRayIntersectsGround)
{
    CONST vec2 coords = lightPollutionTexVarsToTexCoords(altitude, cosViewZenithAngle, viewRayIntersectsGround);
    return texture(lightPollutionScatteringTexture, coords);
}
