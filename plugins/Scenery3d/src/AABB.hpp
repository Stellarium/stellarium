#ifndef _AABB_HPP_
#define _AABB_HPP_

#include <vector>
#include "VecMath.hpp"
#include "Util.hpp"

//! A simple "box" class with 8 arbitrary vertices
class Box
{
public:
	Box();

	Vec3f vertices[8];

	//! Transforms the vertices
	void transform(const QMatrix4x4 &tf);
	//! Renders the box
	void render() const;
};

//! An axis-aligned bounding-box class
class AABB
{
public:
    enum Corner
    {
        MinMinMin = 0, MaxMinMin, MaxMaxMin, MinMaxMin,
        MinMinMax, MaxMinMax, MaxMaxMax, MinMaxMax,
        CORNERCOUNT
    };

    enum Plane
    {
        Front = 0, Back, Bottom, Top, Left, Right,
        PLANECOUNT
    };

    Vec3f min, max;

    //! Creates an AABB with minimum vertex set to infinity and maximum vertex set to -infinity
    AABB();
    AABB(Vec3f min, Vec3f max);
    ~AABB();

    //! Resets minimum to infinity and maximum to -infinity
    void reset();
    //! Resets minimum and maximum to zero vectors
    void resetToZero();

    //! Updates the bounding box to include the specified vertex.
    void expand(const Vec3f& vec);

    Vec3f getCorner(Corner corner) const;

    //! Used for frustum culling
    Vec3f positiveVertex(Vec3f& normal) const;
    Vec3f negativeVertex(Vec3f& normal) const;

    void render() const;

    //! Return the plane equation for specified plane as Vec4f
    Vec4f getEquation(AABB::Plane p) const;
    //! Returns a box object that represents the AABB.
    Box toBox();
};

#endif
