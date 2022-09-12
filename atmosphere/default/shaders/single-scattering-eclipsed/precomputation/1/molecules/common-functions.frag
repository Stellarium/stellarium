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
#line 3 0 // common-functions.frag
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
#line 4 0 // common-functions.frag

bool dbgDataPresent=false;
bool debugDataPresent()
{
    return dbgDataPresent;
}
vec4 dbgData=vec4(0);
vec4 debugData()
{
    return dbgData;
}
void setDebugData(float a)
{
    dbgDataPresent=true;
    dbgData=vec4(a,0,0,0);
}
void setDebugData(float a,float b)
{
    dbgDataPresent=true;
    dbgData=vec4(a,b,0,0);
}
void setDebugData(float a,float b, float c)
{
    dbgDataPresent=true;
    dbgData=vec4(a,b,c,0);
}
void setDebugData(float a,float b, float c, float d)
{
    dbgDataPresent=true;
    dbgData=vec4(a,b,c,d);
}

void swap(inout float x, inout float y)
{
    CONST float t = x;
    x = y;
    y = t;
}

// Assumes that if its argument is negative, it's due to rounding errors and
// should instead be zero.
float safeSqrt(const float x)
{
    return sqrt(max(x,0.));
}

float safeAtan(const float y, const float x)
{
    CONST float a = atan(y,x);
    return x==0 && y==0 ? 0 : a;
}

float clampCosine(const float x)
{
    return clamp(x, -1., 1.);
}

// Fixup for possible rounding errors resulting in altitude being outside of theoretical bounds
float clampAltitude(const float altitude)
{
    return clamp(altitude, 0., atmosphereHeight);
}

// Fixup for possible rounding errors resulting in distance being outside of theoretical bounds
float clampDistance(const float d)
{
    return max(d, 0.);
}

vec3 normalToEarth(vec3 point)
{
    return normalize(point-earthCenter);
}

float pointAltitude(vec3 point)
{
    return length(point-earthCenter)-earthRadius;
}

float distanceToAtmosphereBorder(const float cosZenithAngle, const float observerAltitude)
{
    CONST float Robs=earthRadius+observerAltitude;
    CONST float Ratm=earthRadius+atmosphereHeight;
    CONST float discriminant=sqr(Ratm)-sqr(Robs)*(1-sqr(cosZenithAngle));
    return clampDistance(safeSqrt(discriminant)-Robs*cosZenithAngle);
}

float distanceToGround(const float cosZenithAngle, const float observerAltitude)
{
    CONST float Robs=earthRadius+observerAltitude;
    CONST float discriminant=sqr(earthRadius)-sqr(Robs)*(1-sqr(cosZenithAngle));
    return clampDistance(-safeSqrt(discriminant)-Robs*cosZenithAngle);
}

float cosZenithAngleOfHorizon(const float altitude)
{
    CONST float R=earthRadius;
    CONST float h=max(0.,altitude); // negative values would result in sqrt(-|x|)
    return -sqrt(2*h*R+sqr(h))/(R+h);
}

bool rayIntersectsGround(const float cosViewZenithAngle, const float observerAltitude)
{
    return cosViewZenithAngle<cosZenithAngleOfHorizon(observerAltitude);
}

float distanceToNearestAtmosphereBoundary(const float cosZenithAngle, const float observerAltitude,
                                          const bool viewRayIntersectsGround)
{
    return viewRayIntersectsGround ? distanceToGround(cosZenithAngle, observerAltitude)
                                   : distanceToAtmosphereBorder(cosZenithAngle, observerAltitude);
}

float sunVisibility(const float cosSunZenithAngle, float altitude)
{
    if(altitude<0) altitude=0;
    CONST float sinHorizonZenithAngle = earthRadius/(earthRadius+altitude);
    CONST float cosHorizonZenithAngle = -sqrt(1-sqr(sinHorizonZenithAngle));
    /* Approximating visible fraction of solar disk by smoothstep between the position of the Sun
     * touching the horizon by its upper part and the position with lower part touching the horizon.
     * The calculation assumes that solar angular radius is small and thus approximately equals its sine.
     * For details, see Bruneton's explanation before GetTransmittanceToSun() in the updated
     * Precomputed Atmospheric Scattering demo.
     */
     return smoothstep(-sinHorizonZenithAngle*sunAngularRadius,
                        sinHorizonZenithAngle*sunAngularRadius,
                        cosSunZenithAngle-cosHorizonZenithAngle);
}

/*
   R1,R2 - radii of the circles
   d - distance between centers of the circles
   returns area of intersection of these circles
 */
float circlesIntersectionArea(float R1, float R2, float d)
{
    if(d+min(R1,R2)<max(R1,R2)) return PI*sqr(min(R1,R2));
    if(d>=R1+R2) return 0.;

    // Return area of the lens with radii R1 and R2 and offset d
    return sqr(R1)*acos(clamp( (sqr(d)+sqr(R1)-sqr(R2))/(2*d*R1) ,-1.,1.)) +
           sqr(R2)*acos(clamp( (sqr(d)+sqr(R2)-sqr(R1))/(2*d*R2) ,-1.,1.)) -
           0.5*sqrt(max( (-d+R1+R2)*(d+R1-R2)*(d-R1+R2)*(d+R1+R2) ,0.));
}

float angleBetween(const vec3 a, const vec3 b)
{
    CONST float d=dot(a,b);
    CONST float c=length(cross(a,b));
    // To avoid loss of precision, don't use dot product near the singularity
    // of acos, and cross product near the singularity of asin
    if(abs(d) < abs(c))
        return acos(d/(length(a)*length(b)));
    CONST float smallerAngle = asin(c/(length(a)*length(b)));
    if(d<0) return PI-smallerAngle;
    return smallerAngle;
}

float angleBetweenSunAndMoon(const vec3 camera, const vec3 sunDir, const vec3 moonPos)
{
    return angleBetween(sunDir, moonPos-camera);
}

float moonAngularRadius(const vec3 cameraPosition, const vec3 moonPosition)
{
    return moonRadius/length(moonPosition-cameraPosition);
}

float visibleSolidAngleOfSun(const vec3 camera, const vec3 sunDir, const vec3 moonPos)
{
    CONST float Rs=sunAngularRadius;
    CONST float Rm=moonAngularRadius(camera,moonPos);
    float visibleSolidAngle=PI*sqr(Rs);

    CONST float dSM=angleBetweenSunAndMoon(camera,sunDir,moonPos);
    if(dSM<Rs+Rm)
    {
        visibleSolidAngle -= circlesIntersectionArea(Rm,Rs,dSM);
    }

    return visibleSolidAngle;
}

float sunVisibilityDueToMoon(const vec3 camera, const vec3 sunDir, const vec3 moonPos)
{
    return visibleSolidAngleOfSun(camera,sunDir,moonPos)/(PI*sqr(sunAngularRadius));
}

float sphereIntegrationSolidAngleDifferential(const int pointCountOnSphere)
{
    return 4*PI/pointCountOnSphere;
}
vec3 sphereIntegrationSampleDir(const int index, const int pointCountOnSphere)
{
    CONST float goldenRatio=1.6180339887499;
    // The range of n is 0.5, 1.5, ..., pointCountOnSphere-0.5
    CONST float n=index+0.5;
    // Explanation of the Fibonacci grid generation can be seen at https://stackoverflow.com/a/44164075/673852
    CONST float zenithAngle=acos(clamp(1-(2.*n)/pointCountOnSphere, -1.,1.));
    CONST float azimuth=n*(2*PI*goldenRatio);
    return vec3(cos(azimuth)*sin(zenithAngle),
                sin(azimuth)*sin(zenithAngle),
                cos(zenithAngle));
}
