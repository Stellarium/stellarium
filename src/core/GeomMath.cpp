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

#include "GeomMath.hpp"


AABBox::AABBox()
{
	*this = AABBox(Vec3f(std::numeric_limits<float>::infinity()),Vec3f(-std::numeric_limits<float>::infinity()));
}

AABBox::AABBox(const Vec3f& min, const Vec3f& max)
{
	this->min = min;
	this->max = max;
}

void AABBox::expand(const Vec3f &vec)
{
	min = Vec3f(	std::min(vec.v[0], min.v[0]),
			std::min(vec.v[1], min.v[1]),
			std::min(vec.v[2], min.v[2]));
	max = Vec3f(	std::max(vec.v[0], max.v[0]),
			std::max(vec.v[1], max.v[1]),
			std::max(vec.v[2], max.v[2]));
}

void AABBox::expand(const AABBox &box)
{
	expand(box.min);
	expand(box.max);
}

void AABBox::reset()
{
	//! use the constructor to avoid duplication
	*this = AABBox();
}

bool AABBox::isValid() const
{
	return (	min[0] < max[0] &&
			min[1] < max[1] &&
			min[2] < max[2]);
}

float AABBox::getVolume() const
{
	Vec3f diff = max - min;
	return diff[0]*diff[1]*diff[2];
}

Vec3f AABBox::getCorner(AABBox::Corner corner) const
{
	switch(corner)
	{
		case MinMinMin:
			return min;
		case MaxMinMin:
			return Vec3f(max.v[0], min.v[1], min.v[2]);
		case MaxMaxMin:
			return Vec3f(max.v[0], max.v[1], min.v[2]);
		case MinMaxMin:
			return Vec3f(min.v[0], max.v[1], min.v[2]);
		case MinMinMax:
			return Vec3f(min.v[0], min.v[1], max.v[2]);
		case MaxMinMax:
			return Vec3f(max.v[0], min.v[1], max.v[2]);
		case MaxMaxMax:
			return max;
		case MinMaxMax:
			return Vec3f(min.v[0], max.v[1], max.v[2]);
		default:
			//! Invalid case
			//! use a sNaN to detect bugs
			return Vec3f(std::numeric_limits<float>::signaling_NaN());
	}
}

Vec4f AABBox::getPlane(AABBox::Face p) const
{
	switch(p)
	{
		case Front:
			return Vec4f(0.0f, -1.0f, 0.0f, -min.v[1]);
		case Back:
			return Vec4f(0.0f, 1.0f, 0.0f, max.v[1]);
		case Bottom:
			return Vec4f(0.0f, 0.0f, -1.0f, -min.v[2]);
		case Top:
			return Vec4f(0.0f, 0.0f, 1.0f, max.v[2]);
		case Left:
			return Vec4f(-1.0f, 0.0f, 0.0f, -min.v[0]);
		case Right:
			return Vec4f(1.0f, 0.0f, 0.0f, max.v[0]);
		default:
			//! Invalid case
			//! use a sNaN to detect bugs
			return Vec4f(std::numeric_limits<float>::signaling_NaN());
	}
}

Vec3f AABBox::positiveVertex(Vec3f& normal) const
{
	Vec3f out = min;

	if(normal.v[0] >= 0.0f)
		out.v[0] = max.v[0];
	if(normal.v[1] >= 0.0f)
		out.v[1] = max.v[1];
	if(normal.v[2] >= 0.0f)
		out.v[2] = max.v[2];

	return out;
}

Vec3f AABBox::negativeVertex(Vec3f& normal) const
{
	Vec3f out = max;

	if(normal.v[0] >= 0.0f)
		out.v[0] = min.v[0];
	if(normal.v[1] >= 0.0f)
		out.v[1] = min.v[1];
	if(normal.v[2] >= 0.0f)
		out.v[2] = min.v[2];

	return out;
}

Line::Line(const Vec3f &p, const Vec3f &dir)
{
	startPoint = p;
	direction = dir;
	endPoint = startPoint + direction;
}

Vec3f Line::getPoint(float val) const
{
	return startPoint + (val*direction);
}
