#ifndef _FRUSTUM_HPP_
#define _FRUSTUM_HPP_

#include <vector>
#include <math.h>
#include "Plane.hpp"
#include "AABB.hpp"

class Frustum
{
public:
    enum Corner
    {
        NBL = 0, NBR, NTR, NTL,
        FBL, FBR, FTR, FTL,
        CORNERCOUNT
    };

    enum FrustumPlane
    {
        NEARP = 0, FARP,
        LEFT, RIGHT,
        BOTTOM, TOP,
        PLANECOUNT
    };

    enum
    {
        OUTSIDE, INTERSECT, INSIDE
    };

    Frustum();
    ~Frustum();

    void setCamInternals(float fov, float aspect, float zNear, float zFar)
    {
        this->fov = fov;
        this->aspect = aspect;
        this->zNear = zNear;
        this->zFar = zFar;
    }

    void calcFrustum(Vec3d p, Vec3d l, Vec3d u);
    const Vec3f& getCorner(Corner corner) const;
    const Plane& getPlane(FrustumPlane plane) const;
    int pointInFrustum(Vec3f p);
    int boxInFrustum(const AABB* bbox);

    void drawFrustum();
    void saveCorners();
    float fov;
    float aspect;
    float zNear;
    float zFar;
    Mat4d m;
    AABB* bbox;

private:

    std::vector<Vec3f> corners;
    std::vector<Vec3f> drawCorners;
    std::vector<Plane> planes;
    AABB* drawBbox;
};

#endif
