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

#include "Plane.hpp"

Plane::Plane(): distance(0.0f), sDistance(0.0f)
{
}

Plane::Plane(Vec3f &v1, Vec3f &v2, Vec3f &v3): Plane(v1, v2, v3, SPolygon::CCW)
{
	distance=0.0f;
	sDistance=0.0f;
}

Plane::Plane(const Vec4f &e): sDistance(0.0f)
{
	Vec3f n = Vec3f(e.v[0], e.v[1], e.v[2]);
	n.normalize();

	normal = n;
	distance = e.v[3];
}

Plane::Plane(const Vec3f &v1, const Vec3f &v2, const Vec3f &v3, SPolygon::Order o): sDistance(0.0f)
{
	Vec3f edge1 = v2-v1;
	Vec3f edge2 = v3-v1;

	switch(o)
	{
		case SPolygon::CCW:
			normal = edge1^edge2;
			break;

		case SPolygon::CW:
			normal = edge2^edge1;
			break;
	}

	normal.normalize();
	distance = v1.dot(normal);
}

Plane::~Plane() {}

void Plane::setPoints(const Vec3f &v1,const Vec3f &v2,const Vec3f &v3, SPolygon::Order o)
{
	Vec3f edge1 = v2-v1;
	Vec3f edge2 = v3-v1;

	switch(o)
	{
		case SPolygon::CCW:
			normal = edge1^edge2;
			break;

		case SPolygon::CW:
			normal = edge2^edge1;
			break;
	}

	normal.normalize();
	distance = v1.dot(normal);
}

float Plane::calcDistance(const Vec3f p) const
{
	return p.dot(normal) - distance;
}

bool Plane::isBehind(const Vec3f &p) const
{
	bool result = false;

	if(calcDistance(p) < 0.0f)
		result = true;

	return result;
}

void Plane::saveValues()
{
	sNormal = normal;
	sP = p;
	sDistance = distance;
}

void Plane::resetValues()
{
	sNormal = Vec3f(0.0f);
	sP = Vec3f(0.0f);
	sDistance = 0.0f;
}

bool Plane::intersect(const Line &l, float &val) const
{
	float dp = normal.dot(l.direction);
	float eps = std::numeric_limits<float>::epsilon();

	if(std::abs(dp) < eps)
	{
		val = 0.0f;
		return false;
	}

	val = (distance - normal.dot(l.startPoint))/dp;

	//Intersection outside
	if(val < -eps || val > (1.0f + eps))
	{
		val = 0.0f;
		return false;
	}

	return true;
}
