/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza
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

#ifndef PLANE_HPP
#define PLANE_HPP

#include "GeomMath.hpp"
#include "SPolygon.hpp"

class Plane
{
public:
	Plane();
	Plane(Vec3f &v1, Vec3f &v2, Vec3f &v3);
	Plane(const Vec4f &e);
	Plane(const Vec3f &v1, const Vec3f &v2, const Vec3f &v3, SPolygon::Order o);
	~Plane();

	Vec3f normal, sNormal;
	Vec3f p, sP;
	float distance, sDistance;

	void setPoints(const Vec3f &v1, const Vec3f &v2, const Vec3f &v3, SPolygon::Order o = SPolygon::CCW);
	float calcDistance(const Vec3f p) const;
	bool isBehind(const Vec3f& p) const;
	void saveValues();
	void resetValues();

	bool intersect(const Line &l, float &val) const;
};

#endif
