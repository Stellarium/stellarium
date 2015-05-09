/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _AABB_HPP_
#define _AABB_HPP_

#include <vector>
#include "VecMath.hpp"

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
