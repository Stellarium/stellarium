#ifndef _AABB_HPP_
#define _AABB_HPP_

#include "VecMath.hpp"
#include <vector>
#include "QtOpenGL"

class AABB
{
public:
    enum Corner
    {
        MinMinMin = 0, MaxMinMin, MaxMaxMin, MinMaxMin,
        MinMinMax, MaxMinMax, MaxMaxMax, MinMaxMax,
        CORNERCOUNT
    };

    Vec3f min, max;

    AABB(Vec3f min, Vec3f max);
    ~AABB();

    Vec3f getCorner(Corner corner) const;

    //! Used for frustum culling
    Vec3f positiveVertex(Vec3f& normal) const;
    Vec3f negativeVertex(Vec3f& normal) const;

    void render();
};

#endif
