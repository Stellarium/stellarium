/*
 * Stellarium
 * Copyright (C) 2011-2012 Andrei Borza (from Scenery3d plug-in)
 * Copyright (C) 2016 Florian Schaukowitsch
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

//! @file
//! This header contains useful classes for common geometric operations that are
//! useful for 3D rendering, such as AABB, and other vector math helpers.

#ifndef GEOMMATH_HPP
#define GEOMMATH_HPP

#include "VecMath.hpp"

//! An axis-aligned bounding-box class
class AABBox
{
public:
	//! Identifies a corner of the AABB
	enum Corner
	{
		MinMinMin = 0, MaxMinMin, MaxMaxMin, MinMaxMin,
		MinMinMax, MaxMinMax, MaxMaxMax, MinMaxMax,
		CORNERCOUNT
	};

	//! Identifies a face of the AABB
	enum Face
	{
		Front = 0, Back, Bottom, Top, Left, Right,
		FACECOUNT
	};

	//! Creates an AABBox with minimum vertex set to infinity and maximum vertex set to -infinity
	AABBox();
	//! Creates an AABBox with the specified minimum and maximum extents.
	AABBox(const Vec3f& min, const Vec3f& max);

	//! Updates the bounding box to include the specified vertex.
	void expand(const Vec3f& vec);

	//! Updates the bounding box to include the specified other AABB
	void expand(const AABBox& box);

	//! Resets minimum to infinity and maximum to -infinity
	//! (equivalent to creating a new AABBox with the default constructor)
	void reset();

	//! Returns true when each component of the minimum extents is smaller than the
	//! corresponding component of the maximum extents. This allows to detect empty
	//! (or freshly created) boxes with negative volume, or AABBs with only a single vertex
	//! (where the minimum and maximum extents are the same).
	bool isValid() const;

	//! Returns the volume of the bounding box.
	float getVolume() const;

	//! Returns the coordinates of the specified corner of the AABB.
	Vec3f getCorner(AABBox::Corner corner) const;

	//! Return the plane equation in the
	//! [general form](https://en.wikipedia.org/wiki/Plane_(geometry)#Point-normal_form_and_general_form_of_the_equation_of_a_plane)
	//! for the specified face of the AABB as Vec4f
	Vec4f getPlane(AABBox::Face p) const;

	//! Returns a Vec3f that for each component either:
	//! - returns the corresponding component of the minimum extents of the AABB if
	//!   the component of the \p normal parameter is less than zero,
	//! - or the component of the maximum extents of the AABBox otherwise.
	Vec3f positiveVertex(Vec3f& normal) const;
	//! Returns a Vec3f that for each component either:
	//! - returns the corresponding component of the maximum extents of the AABB if
	//!   the component of the \p normal parameter is less than zero,
	//! - or the component of the minimum extents of the AABBox otherwise.
	Vec3f negativeVertex(Vec3f& normal) const;

	//! The minimal extents of the box.
	Vec3f min;
	//! The maximal extents of the box.
	Vec3f max;
};

//! A simple line class, identified by a point and a direction vector.
class Line
{
public:
	//! Constructs a line given a start point and a direction vector
	Line(const Vec3f &p, const Vec3f &dir);

	Vec3f startPoint;
	//! Equals startPoint + direction
	Vec3f endPoint;
	Vec3f direction;

	//! Gets the point that lies on the line according to the
	//! equation \p startPoint + \p val * \p direction
	Vec3f getPoint(float val) const;
};

#endif
