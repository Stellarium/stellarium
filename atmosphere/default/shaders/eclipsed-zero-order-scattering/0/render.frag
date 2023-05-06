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
#line 6 0 // render.frag
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
const vec4 scatteringCrossSection_molecules=vec4(2.39459446e-30,1.71496275e-30,1.26023066e-30,9.46713716e-31);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.0350000001,0.0370000005,0.0399999991,0.0410000011);
const vec4 solarIrradianceAtTOA=vec4(1.03699994,1.24899995,1.68400002,1.97500002);
const vec4 lightPollutionRelativeRadiance=vec4(0,0,4.3e-07,1.623e-06);
const vec4 wavelengths=vec4(360,391.333344,422.666656,454);
const int wlSetIndex=0;
#endif
#line 7 0 // render.frag
#line 1 3 // calc-view-dir.h.glsl
vec3 calcViewDir();
#line 8 0 // render.frag
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
#line 9 0 // render.frag



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
#line 13 0 // render.frag
#line 1 6 // radiance-to-luminance.h.glsl
const mat4 radianceToLuminance=mat4(1.38997746,0.0419133306,6.48549128,0,  105.968178,3.00652099,500.960266,142.641495,  3776.24023,119.971481,18207.7598,6412.0166,  6910.01318,981.06665,37504.0586,26741.9102);
#line 14 0 // render.frag
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
#line 15 0 // render.frag
#line 1 8 // eclipsed-direct-irradiance.h.glsl
#ifndef INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D
#define INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D

vec4 calcEclipsedDirectGroundIrradiance(const vec3 pointOnGround, const vec3 sunDir, const vec3 moonPos);

#endif
#line 16 0 // render.frag


uniform sampler3D scatteringTextureInterpolationGuides01;
uniform sampler3D scatteringTextureInterpolationGuides02;
uniform sampler3D scatteringTexture;
uniform sampler2D eclipsedScatteringTexture;
uniform sampler3D eclipsedDoubleScatteringTexture;
uniform vec3 cameraPosition;
uniform vec3 sunDirection;
uniform vec3 moonPosition;
uniform float lightPollutionGroundLuminance;
uniform vec4 solarIrradianceFixup=vec4(1); // Used when we want to alter solar irradiance post-precomputation
uniform bool pseudoMirrorSkyBelowHorizon = false;
uniform bool useInterpolationGuides=false;
in vec3 position;
layout(location=0) out vec4 luminance;
layout(location=1) out vec4 radianceOutput;

vec4 solarRadiance()
{
    return solarIrradianceAtTOA*solarIrradianceFixup/(PI*sqr(sunAngularRadius));
}

void main()
{
    vec3 viewDir=calcViewDir();
    if(length(viewDir) == 0)
        discard;

    // NOTE: we simply clamp negative altitudes to zero (otherwise the model will break down). This is not
    // quite correct physically: there are places with negative elevation above sea level. But the error of
    // this approximation has the same order of magnitude as the assumption that the Earth and its atmosphere
    // are spherical.
    float altitude = max(cameraPosition.z, 0.);
    CONST vec3 oldCamPos=cameraPosition;
    // Hide the uniform with this name, thus effectively modifying it for the following code
    vec3 cameraPosition=vec3(oldCamPos.xy, altitude);

    bool lookingIntoAtmosphere=true;
    if(altitude>atmosphereHeight)
    {
        CONST vec3 p = cameraPosition - earthCenter;
        CONST float p_dot_v = dot(p, viewDir);
        CONST float p_dot_p = dot(p, p);
        CONST float squaredDistBetweenViewRayAndEarthCenter = p_dot_p - sqr(p_dot_v);
        CONST float distanceToTOA = -p_dot_v - sqrt(sqr(earthRadius+atmosphereHeight) - squaredDistBetweenViewRayAndEarthCenter);
        if(distanceToTOA>=0)
        {
            cameraPosition += viewDir*distanceToTOA;
            altitude = atmosphereHeight;
        }
        else
        {
#if 1 /*RENDERING_ANY_ZERO_SCATTERING*/
            lookingIntoAtmosphere=false;
#else
            luminance=vec4(0);
            radianceOutput=vec4(0);
            return;
#endif
        }
    }

    CONST vec3 zenith=normalize(cameraPosition-earthCenter);
    float cosViewZenithAngle=dot(zenith,viewDir);

    bool viewRayIntersectsGround=false;
    {
        CONST vec3 p = cameraPosition - earthCenter;
        CONST float p_dot_v = dot(p, viewDir);
        CONST float p_dot_p = dot(p, p);
        CONST float squaredDistBetweenViewRayAndEarthCenter = p_dot_p - sqr(p_dot_v);
        CONST float distanceToIntersection = -p_dot_v - sqrt(sqr(earthRadius) - squaredDistBetweenViewRayAndEarthCenter);
        // altitude==0 is a special case where distance to intersection calculation
        // is unreliable (has a lot of noise in its sign), so check it separately
        if(distanceToIntersection>0 || (altitude==0 && cosViewZenithAngle<0))
            viewRayIntersectsGround=true;
    }

    // Stellarium wants to display a sky-like view when ground is hidden.
    // Aside from aesthetics, this affects brightness input to Stellarium's tone mapper.
    if(pseudoMirrorSkyBelowHorizon && viewRayIntersectsGround)
    {
        CONST float horizonCZA = cosZenithAngleOfHorizon(altitude);
        CONST float viewElev  = asin(clampCosine(cosViewZenithAngle));
        CONST float horizElev = asin(clampCosine(horizonCZA));
        CONST float newViewElev = 2*horizElev - viewElev;
        CONST float viewDirXYnorm = length(viewDir-zenith*cosViewZenithAngle);
        if(viewDirXYnorm == 0)
        {
            viewDir = zenith;
        }
        else
        {
            // Remove original zenith-directed component
            viewDir -= zenith*cosViewZenithAngle;
            // Update cos(VZA). This change will affect all the subsequent code in this function.
            cosViewZenithAngle = sin(newViewElev);
            // Adjust the remaining (horizontal) components so that the final viewDir is normalized
            viewDir *= safeSqrt(1-sqr(cosViewZenithAngle))/viewDirXYnorm;
            // Add the new zenith-directed component
            viewDir += zenith*cosViewZenithAngle;
        }
        viewRayIntersectsGround = false;
    }

    CONST float cosSunZenithAngle =dot(zenith,sunDirection);
    CONST float dotViewSun=dot(viewDir,sunDirection);

    CONST vec3 sunXYunnorm=sunDirection-dot(sunDirection,zenith)*zenith;
    CONST vec3 viewXYunnorm=viewDir-dot(viewDir,zenith)*zenith;
    CONST vec3 sunXY = sunXYunnorm.x!=0 || sunXYunnorm.y!=0 || sunXYunnorm.z!=0 ? normalize(sunXYunnorm) : vec3(0);
    CONST vec3 viewXY = viewXYunnorm.x!=0 || viewXYunnorm.y!=0 || viewXYunnorm.z!=0 ? normalize(viewXYunnorm) : vec3(0);
    CONST float azimuthRelativeToSun=safeAtan(dot(cross(sunXY, viewXY), zenith), dot(sunXY, viewXY));

#if 0 /*RENDERING_ZERO_SCATTERING*/
    vec4 radiance;
    if(viewRayIntersectsGround)
    {
        // XXX: keep in sync with the same code in computeScatteringDensity(), but don't forget about
        //      the difference in the usage of viewDir vs incDir.
        CONST float distToGround = distanceToGround(cosViewZenithAngle, altitude);
        CONST vec4 transmittanceToGround=transmittance(cosViewZenithAngle, altitude, distToGround, viewRayIntersectsGround);
        CONST vec3 groundNormal = normalize(zenith*(earthRadius+altitude)+viewDir*distToGround);
        CONST vec4 groundIrradiance = irradiance(dot(groundNormal, sunDirection), 0);
        // Radiation scattered by the ground
        CONST float groundBRDF = 1/PI; // Assuming Lambertian BRDF, which is constant
        radiance = transmittanceToGround*groundAlbedo*groundIrradiance*solarIrradianceFixup*groundBRDF
                 + lightPollutionGroundLuminance*lightPollutionRelativeRadiance;
    }
    else if(dotViewSun>cos(sunAngularRadius))
    {
        if(lookingIntoAtmosphere)
            radiance=transmittanceToAtmosphereBorder(cosViewZenithAngle, altitude)*solarRadiance();
        else
            radiance=solarRadiance();
    }
    else
    {
        discard;
    }
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 1 /*RENDERING_ECLIPSED_ZERO_SCATTERING*/
    vec4 radiance;
    CONST float dotViewMoon=dot(viewDir,normalize(moonPosition-cameraPosition));
    if(viewRayIntersectsGround)
    {
        // XXX: keep in sync with the similar code in non-eclipsed zero scattering rendering.
        CONST float distToGround = distanceToGround(cosViewZenithAngle, altitude);
        CONST vec4 transmittanceToGround=transmittance(cosViewZenithAngle, altitude, distToGround, viewRayIntersectsGround);
        CONST vec3 pointOnGround = cameraPosition+viewDir*distToGround;
        CONST vec4 directGroundIrradiance = calcEclipsedDirectGroundIrradiance(pointOnGround, sunDirection, moonPosition);
        // FIXME: add first-order indirect irradiance too: it's done in non-eclipsed irradiance (in the same
        // conditions: when limited to 2 orders). This should be calculated at the same time when second order
        // is: all the infrastructure is already there.
        CONST vec4 groundIrradiance = directGroundIrradiance;
        // Radiation scattered by the ground
        CONST float groundBRDF = 1/PI; // Assuming Lambertian BRDF, which is constant
        radiance = transmittanceToGround*groundAlbedo*groundIrradiance*solarIrradianceFixup*groundBRDF
                 + lightPollutionGroundLuminance*lightPollutionRelativeRadiance;
    }
    else if(dotViewSun>cos(sunAngularRadius) && dotViewMoon<cos(moonAngularRadius(cameraPosition,moonPosition)))
    {
        if(lookingIntoAtmosphere)
            radiance=transmittanceToAtmosphereBorder(cosViewZenithAngle, altitude)*solarRadiance();
        else
            radiance=solarRadiance();
    }
    else
    {
        discard;
    }
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_ECLIPSED_SINGLE_SCATTERING_ON_THE_FLY*/
    CONST vec4 scattering=computeSingleScatteringEclipsed(cameraPosition,viewDir,sunDirection,moonPosition,
                                                          viewRayIntersectsGround);
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_ECLIPSED_SINGLE_SCATTERING_PRECOMPUTED_RADIANCE*/
    CONST vec2 texCoords = eclipseTexVarsToTexCoords(azimuthRelativeToSun, cosViewZenithAngle, altitude,
                                                     viewRayIntersectsGround, eclipsedSingleScatteringTextureSize);
    // We don't use mip mapping here, but for some reason, on my NVidia GTX 750 Ti with Linux-x86 driver 390.116 I get
    // an artifact at the point where azimuth texture coordinate changes from 1 to 0 (at azimuthRelativeToSun crossing
    // 0). This happens when I simply call texture(eclipsedScatteringTexture, texCoords) without specifying LOD.
    // Apparently, the driver uses the derivative for some reason, even though it shouldn't.
    CONST vec4 scattering = textureLod(eclipsedScatteringTexture, texCoords, 0);
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_ECLIPSED_SINGLE_SCATTERING_PRECOMPUTED_LUMINANCE*/
    CONST vec2 texCoords = eclipseTexVarsToTexCoords(azimuthRelativeToSun, cosViewZenithAngle, altitude,
                                                     viewRayIntersectsGround, eclipsedSingleScatteringTextureSize);
    // We don't use mip mapping here, but for some reason, on my NVidia GTX 750 Ti with Linux-x86 driver 390.116 I get
    // an artifact at the point where azimuth texture coordinate changes from 1 to 0 (at azimuthRelativeToSun crossing
    // 0). This happens when I simply call texture(eclipsedScatteringTexture, texCoords) without specifying LOD.
    // Apparently, the driver uses the derivative for some reason, even though it shouldn't.
    CONST vec4 scattering = textureLod(eclipsedScatteringTexture, texCoords, 0);
    luminance=scattering*currentPhaseFunction(dotViewSun);
#elif 0 /*RENDERING_ECLIPSED_DOUBLE_SCATTERING_PRECOMPUTED_RADIANCE*/
    vec4 radiance=exp(sampleEclipseDoubleScattering3DTexture(eclipsedDoubleScatteringTexture,
                                                             cosSunZenithAngle, cosViewZenithAngle, azimuthRelativeToSun,
                                                             altitude, viewRayIntersectsGround));
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_ECLIPSED_DOUBLE_SCATTERING_PRECOMPUTED_LUMINANCE*/
    luminance=exp(sampleEclipseDoubleScattering3DTexture(eclipsedDoubleScatteringTexture,
                                                         cosSunZenithAngle, cosViewZenithAngle, azimuthRelativeToSun,
                                                         altitude, viewRayIntersectsGround));
#elif 0 /*RENDERING_SINGLE_SCATTERING_ON_THE_FLY*/
    CONST vec4 scattering=computeSingleScattering(cosSunZenithAngle,cosViewZenithAngle,dotViewSun,
                                                  altitude,viewRayIntersectsGround);
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_SINGLE_SCATTERING_PRECOMPUTED_RADIANCE*/
    vec4 scattering;
    if(useInterpolationGuides)
    {
        scattering = sample3DTextureGuided(scatteringTexture, scatteringTextureInterpolationGuides01,
                                           scatteringTextureInterpolationGuides02, cosSunZenithAngle,
                                           cosViewZenithAngle, dotViewSun, altitude, viewRayIntersectsGround);
    }
    else
    {
        scattering = sample3DTexture(scatteringTexture, cosSunZenithAngle, cosViewZenithAngle,
                                     dotViewSun, altitude, viewRayIntersectsGround);
    }
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_SINGLE_SCATTERING_PRECOMPUTED_LUMINANCE*/
    vec4 scattering;
    if(useInterpolationGuides)
    {
        scattering = sample3DTextureGuided(scatteringTexture, scatteringTextureInterpolationGuides01,
                                           scatteringTextureInterpolationGuides02, cosSunZenithAngle,
                                           cosViewZenithAngle, dotViewSun, altitude, viewRayIntersectsGround);
    }
    else
    {
        scattering = sample3DTexture(scatteringTexture, cosSunZenithAngle, cosViewZenithAngle,
                                     dotViewSun, altitude, viewRayIntersectsGround);
    }
    luminance=scattering * (bool(PHASE_FUNCTION_IS_EMBEDDED) ? vec4(1) : currentPhaseFunction(dotViewSun));
#elif 0 /*RENDERING_MULTIPLE_SCATTERING_LUMINANCE*/
    luminance=sample3DTexture(scatteringTexture, cosSunZenithAngle, cosViewZenithAngle, dotViewSun, altitude, viewRayIntersectsGround);
#elif 0 /*RENDERING_MULTIPLE_SCATTERING_RADIANCE*/
    vec4 radiance=sample3DTexture(scatteringTexture, cosSunZenithAngle, cosViewZenithAngle, dotViewSun, altitude, viewRayIntersectsGround);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_LIGHT_POLLUTION_RADIANCE*/
    vec4 radiance=lightPollutionGroundLuminance*lightPollutionScattering(altitude, cosViewZenithAngle, viewRayIntersectsGround);
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif 0 /*RENDERING_LIGHT_POLLUTION_LUMINANCE*/
    luminance=lightPollutionGroundLuminance*lightPollutionScattering(altitude, cosViewZenithAngle, viewRayIntersectsGround);
#else
#error What to render?
#endif
}
