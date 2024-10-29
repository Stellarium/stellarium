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

#ifndef SPOLYGON_HPP
#define SPOLYGON_HPP

#include "VecMath.hpp"

#include <QVector>

class Plane;

class SPolygon
{
public:
	SPolygon();
	//! Construct a polygon from 4 corner vertices
	SPolygon(const Vec3f &c0, const Vec3f &c1, const Vec3f &c2, const Vec3f &c3);
	~SPolygon();

	enum Order
	{
		CCW = 0, CW
	};

	//! Holds all vertices of this polygon
	QVector<Vec3f> vertices;

	//! Intersect by specified plane and store the intersection points
	void intersect(const Plane &p, QVector<Vec3f> &intersectionPoints);

	//! Reverse the vertices order
	void reverseOrder();
	//! Add the vertex v to vertices if it is not already present
	void addUniqueVert(const Vec3f &v);

	void render();

private:
};

#endif
