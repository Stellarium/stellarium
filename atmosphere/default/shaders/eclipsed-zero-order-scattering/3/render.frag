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
const vec4 scatteringCrossSection_molecules=vec4(1.37066344e-31,1.16012782e-31,9.88507054e-32,8.47480528e-32);
const vec4 scatteringCrossSection_aerosols=vec4(4.29679994e-14,4.29679994e-14,4.29679994e-14,4.29679994e-14);
const vec4 groundAlbedo=vec4(0.367000014,0.467999995,0.48300001,0.490999997);
const vec4 solarIrradianceAtTOA=vec4(1.30900002,1.23000002,1.14199996,1.06200004);
const vec4 lightPollutionRelativeRadiance=vec4(3.80500001e-06,4.31499984e-06,4.95599988e-06,3.00800002e-05);
const vec4 wavelengths=vec4(736,767.333313,798.666687,830);
const int wlSetIndex=3;
#endif
#line 5 0 // render.frag
#line 1 2 // calc-view-dir.h.glsl
vec3 calcViewDir();
#line 6 0 // render.frag
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
#line 7 0 // render.frag



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
#line 11 0 // render.frag
#line 1 5 // radiance-to-luminance.h.glsl
const mat4 radianceToLuminance=mat4(19.8756237,7.17745161,0,0.0937032327,  2.13895917,0.772417068,0,0.0149318045,  0.241072536,0.0870557055,0,0,  0.0133876652,0.00483453181,0,0);
#line 12 0 // render.frag
#line 1 6 // texture-sampling-functions.h.glsl
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
#line 13 0 // render.frag
#line 1 7 // eclipsed-direct-irradiance.h.glsl
#ifndef INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D
#define INCLUDE_ONCE_8E9C53B3_83A0_4DD7_9106_32A13BD8935D

vec4 calcEclipsedDirectGroundIrradiance(const vec3 pointOnGround, const vec3 sunDir, const vec3 moonPos);

#endif
#line 14 0 // render.frag


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
    return solarIrradianceAtTOA/(PI*sqr(sunAngularRadius));
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
    const vec3 oldCamPos=cameraPosition;
    // Hide the uniform with this name, thus effectively modifying it for the following code
    vec3 cameraPosition=vec3(oldCamPos.xy, altitude);

    bool lookingIntoAtmosphere=true;
    if(altitude>atmosphereHeight)
    {
        const vec3 p = cameraPosition - earthCenter;
        const float p_dot_v = dot(p, viewDir);
        const float p_dot_p = dot(p, p);
        const float squaredDistBetweenViewRayAndEarthCenter = p_dot_p - sqr(p_dot_v);
        const float distanceToTOA = -p_dot_v - sqrt(sqr(earthRadius+atmosphereHeight) - squaredDistBetweenViewRayAndEarthCenter);
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

    const vec3 zenith=normalize(cameraPosition-earthCenter);
    float cosViewZenithAngle=dot(zenith,viewDir);

    bool viewRayIntersectsGround=false;
    {
        const vec3 p = cameraPosition - earthCenter;
        const float p_dot_v = dot(p, viewDir);
        const float p_dot_p = dot(p, p);
        const float squaredDistBetweenViewRayAndEarthCenter = p_dot_p - sqr(p_dot_v);
        const float distanceToIntersection = -p_dot_v - sqrt(sqr(earthRadius) - squaredDistBetweenViewRayAndEarthCenter);
        // altitude==0 is a special case where distance to intersection calculation
        // is unreliable (has a lot of noise in its sign), so check it separately
        if(distanceToIntersection>0 || (altitude==0 && cosViewZenithAngle<0))
            viewRayIntersectsGround=true;
    }

    // Stellarium wants to display a sky-like view when ground is hidden.
    // Aside from aesthetics, this affects brightness input to Stellarium's tone mapper.
    if(pseudoMirrorSkyBelowHorizon && viewRayIntersectsGround)
    {
        const float horizonCZA = cosZenithAngleOfHorizon(altitude);
        const float viewElev  = asin(clampCosine(cosViewZenithAngle));
        const float horizElev = asin(clampCosine(horizonCZA));
        const float newViewElev = 2*horizElev - viewElev;
        const float viewDirXYnorm = length(viewDir-zenith*cosViewZenithAngle);
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

    const float cosSunZenithAngle =dot(zenith,sunDirection);
    const float dotViewSun=dot(viewDir,sunDirection);

    const vec3 sunXYunnorm=sunDirection-dot(sunDirection,zenith)*zenith;
    const vec3 viewXYunnorm=viewDir-dot(viewDir,zenith)*zenith;
    const vec3 sunXY = sunXYunnorm.x!=0 || sunXYunnorm.y!=0 || sunXYunnorm.z!=0 ? normalize(sunXYunnorm) : vec3(0);
    const vec3 viewXY = viewXYunnorm.x!=0 || viewXYunnorm.y!=0 || viewXYunnorm.z!=0 ? normalize(viewXYunnorm) : vec3(0);
    const float azimuthRelativeToSun=safeAtan(dot(cross(sunXY, viewXY), zenith), dot(sunXY, viewXY));

#if RENDERING_ZERO_SCATTERING
    vec4 radiance;
    if(viewRayIntersectsGround)
    {
        // XXX: keep in sync with the same code in computeScatteringDensity(), but don't forget about
        //      the difference in the usage of viewDir vs incDir.
        const float distToGround = distanceToGround(cosViewZenithAngle, altitude);
        const vec4 transmittanceToGround=transmittance(cosViewZenithAngle, altitude, distToGround, viewRayIntersectsGround);
        const vec3 groundNormal = normalize(zenith*(earthRadius+altitude)+viewDir*distToGround);
        const vec4 groundIrradiance = irradiance(dot(groundNormal, sunDirection), 0);
        // Radiation scattered by the ground
        const float groundBRDF = 1/PI; // Assuming Lambertian BRDF, which is constant
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
    const float dotViewMoon=dot(viewDir,normalize(moonPosition-cameraPosition));
    if(viewRayIntersectsGround)
    {
        // XXX: keep in sync with the similar code in non-eclipsed zero scattering rendering.
        const float distToGround = distanceToGround(cosViewZenithAngle, altitude);
        const vec4 transmittanceToGround=transmittance(cosViewZenithAngle, altitude, distToGround, viewRayIntersectsGround);
        const vec3 pointOnGround = cameraPosition+viewDir*distToGround;
        const vec4 directGroundIrradiance = calcEclipsedDirectGroundIrradiance(pointOnGround, sunDirection, moonPosition);
        // FIXME: add first-order indirect irradiance too: it's done in non-eclipsed irradiance (in the same
        // conditions: when limited to 2 orders). This should be calculated at the same time when second order
        // is: all the infrastructure is already there.
        const vec4 groundIrradiance = directGroundIrradiance;
        // Radiation scattered by the ground
        const float groundBRDF = 1/PI; // Assuming Lambertian BRDF, which is constant
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
#elif RENDERING_ECLIPSED_SINGLE_SCATTERING_ON_THE_FLY
    const vec4 scattering=computeSingleScatteringEclipsed(cameraPosition,viewDir,sunDirection,moonPosition,
                                                          viewRayIntersectsGround);
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif RENDERING_ECLIPSED_SINGLE_SCATTERING_PRECOMPUTED_RADIANCE
    const vec2 texCoords = eclipseTexVarsToTexCoords(azimuthRelativeToSun, cosViewZenithAngle, altitude,
                                                     viewRayIntersectsGround, eclipsedSingleScatteringTextureSize);
    // We don't use mip mapping here, but for some reason, on my NVidia GTX 750 Ti with Linux-x86 driver 390.116 I get
    // an artifact at the point where azimuth texture coordinate changes from 1 to 0 (at azimuthRelativeToSun crossing
    // 0). This happens when I simply call texture(eclipsedScatteringTexture, texCoords) without specifying LOD.
    // Apparently, the driver uses the derivative for some reason, even though it shouldn't.
    const vec4 scattering = textureLod(eclipsedScatteringTexture, texCoords, 0);
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif RENDERING_ECLIPSED_SINGLE_SCATTERING_PRECOMPUTED_LUMINANCE
    const vec2 texCoords = eclipseTexVarsToTexCoords(azimuthRelativeToSun, cosViewZenithAngle, altitude,
                                                     viewRayIntersectsGround, eclipsedSingleScatteringTextureSize);
    // We don't use mip mapping here, but for some reason, on my NVidia GTX 750 Ti with Linux-x86 driver 390.116 I get
    // an artifact at the point where azimuth texture coordinate changes from 1 to 0 (at azimuthRelativeToSun crossing
    // 0). This happens when I simply call texture(eclipsedScatteringTexture, texCoords) without specifying LOD.
    // Apparently, the driver uses the derivative for some reason, even though it shouldn't.
    const vec4 scattering = textureLod(eclipsedScatteringTexture, texCoords, 0);
    luminance=scattering*currentPhaseFunction(dotViewSun);
#elif RENDERING_ECLIPSED_DOUBLE_SCATTERING_PRECOMPUTED_RADIANCE
    vec4 radiance=exp(sampleEclipseDoubleScattering3DTexture(eclipsedDoubleScatteringTexture,
                                                             cosSunZenithAngle, cosViewZenithAngle, azimuthRelativeToSun,
                                                             altitude, viewRayIntersectsGround));
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif RENDERING_ECLIPSED_DOUBLE_SCATTERING_PRECOMPUTED_LUMINANCE
    luminance=exp(sampleEclipseDoubleScattering3DTexture(eclipsedDoubleScatteringTexture,
                                                         cosSunZenithAngle, cosViewZenithAngle, azimuthRelativeToSun,
                                                         altitude, viewRayIntersectsGround));
#elif RENDERING_SINGLE_SCATTERING_ON_THE_FLY
    const vec4 scattering=computeSingleScattering(cosSunZenithAngle,cosViewZenithAngle,dotViewSun,
                                                  altitude,viewRayIntersectsGround);
    vec4 radiance=scattering*currentPhaseFunction(dotViewSun);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif RENDERING_SINGLE_SCATTERING_PRECOMPUTED_RADIANCE
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
#elif RENDERING_SINGLE_SCATTERING_PRECOMPUTED_LUMINANCE
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
#elif RENDERING_MULTIPLE_SCATTERING_LUMINANCE
    luminance=sample3DTexture(scatteringTexture, cosSunZenithAngle, cosViewZenithAngle, dotViewSun, altitude, viewRayIntersectsGround);
#elif RENDERING_MULTIPLE_SCATTERING_RADIANCE
    vec4 radiance=sample3DTexture(scatteringTexture, cosSunZenithAngle, cosViewZenithAngle, dotViewSun, altitude, viewRayIntersectsGround);
    radiance*=solarIrradianceFixup;
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif RENDERING_LIGHT_POLLUTION_RADIANCE
    vec4 radiance=lightPollutionGroundLuminance*lightPollutionScattering(altitude, cosViewZenithAngle, viewRayIntersectsGround);
    luminance=radianceToLuminance*radiance;
    radianceOutput=radiance;
#elif RENDERING_LIGHT_POLLUTION_LUMINANCE
    luminance=lightPollutionGroundLuminance*lightPollutionScattering(altitude, cosViewZenithAngle, viewRayIntersectsGround);
#else
#error What to render?
#endif
}
