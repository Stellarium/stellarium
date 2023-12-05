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

/**
  * This implementation is based on Stingl's Robust Hard Shadows. */

#ifndef POLYHEDRON_HPP
#define POLYHEDRON_HPP

#include "Frustum.hpp"
#include "Plane.hpp"
#include "SPolygon.hpp"

class Polyhedron
{
public:
	Polyhedron();
	~Polyhedron();

	//! Vector holding all polygons of this polyhedron
	QVector<SPolygon> polygons;

	//! Adds a frustum to this polyhedron
	void add(const Frustum& f);
	//! Adds a polygon to this polyhedron
	void add(const SPolygon& p);
	//! Adds a polygon to this polyhedron
	void add(const QVector<Vec3f> &verts, const Vec3f &normal);

	//! Intersect this polyhedron with the specified bounding box
	void intersect(const AABBox &bb);
	//! Intersect this polyhedron with the specified plane
	void intersect(const Plane &p);
	//! Extrude each point of this polyhedron towards direction until we hit the bounding box
	void extrude(const Vec3f &dir, const AABBox &bb);
	//! Clear up
	void clear();
	//! Returns the unique vertices count
	int getVertCount() const;
	//! Returns the unique vertices
	const QVector<Vec3f> &getVerts() const;
	//! Makes the unique vertices vector
	void makeUniqueVerts();

	void render() const;

	//! This is used for debugging of the crop matrix
	//! It contains the world-space representation of the orthographic projection used for shadowmapping.
	//Box debugBox;
private:
	//! Vector holding all unique vertices of this polyhedron
	QVector<Vec3f> uniqueVerts;
	//! Adds the vertex if it's unique
	void addUniqueVert(const Vec3f &v);
	//! Intersect for extrude()
	static void intersect(const Line &l, const Vec3f &min, const Vec3f &max, QVector<Vec3f> &vertices);
	//! Clip for extrude()
	static bool clip(float p, float q, float &u1, float &u2);
};

#endif
