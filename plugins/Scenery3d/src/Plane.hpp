#ifndef _PLANE_HPP_
#define _PLANE_HPP_

#include "VecMath.hpp"

class Plane
{
public:
    Plane();
    Plane(Vec3f& v1, Vec3f& v2, Vec3f& v3);
    ~Plane();

    Vec3f normal, sNormal;
    Vec3f p, sP;
    float distance, sDistance;

    void setPoints(Vec3f& v1, Vec3f& v2, Vec3f& v3);
    float calcDistance(Vec3f p) const;
    bool isBehind(Vec3f p);
    void saveValues();
};

#endif
