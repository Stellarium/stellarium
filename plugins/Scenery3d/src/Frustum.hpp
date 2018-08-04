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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <vector>
#include "Plane.hpp"
#include "GeomMath.hpp"

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
	const Vec3f &getCorner(Corner corner) const;
	const Plane &getPlane(FrustumPlane plane) const;
	int pointInFrustum(const Vec3f &p);
	int boxInFrustum(const AABBox &bbox);

	void drawFrustum() const;
	void saveDrawingCorners();
	void resetCorners();
	float fov;
	float aspect;
	float zNear;
	float zFar;
	Mat4d m;
	AABBox bbox;

	std::vector<Vec3f> drawCorners;
	AABBox drawBbox;

	std::vector<Vec3f> corners;
	std::vector<Plane*> planes;

private:


};

#endif
