/*
 * Stellarium
 * Copyright (C) 2010 Fabien Chereau
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

#ifndef TRAILGROUP_HPP
#define TRAILGROUP_HPP

#include "VecMath.hpp"
#include "StelCore.hpp"
#include "StelObjectType.hpp"


class TrailGroup
{
public:
	TrailGroup(float atimeExtent);

	~TrailGroup();

	void draw(StelCore* core, class StelRenderer* renderer);

	// Add 1 point to all the curves at current time and suppress too old points
	void update();

	// Set the matrix to use to post process J2000 positions before storing in the trail
	void setJ2000ToTrailNative(const Mat4d& m);

	void addObject(const StelObjectP&, const Vec3f* col=NULL);

	void setOpacity(float op) 
	{
		opacity = op;
	}

	//! Reset all trails points
	void reset();

private:
	//! Vertex with a position and a color.
	struct ColoredVertex
	{
		Vec3f position;
		Vec4f color;

		ColoredVertex(){}

		VERTEX_ATTRIBUTES(Vec3f Position, Vec4f Color);
	};

	struct Trail
	{
		Trail(const StelObjectP& obj, const Vec3f& col) 
			: stelObject(obj), color(col), vertexBuffer(NULL) 
		{
		}

		StelObjectP stelObject;

		// Using QVector instead of QList.
		// QList is an array of pointers to elements, which are 8 byte on 64-bit,
		// while Vec3f is 12 byte, so not much space saved (or time, at popFront).
		// At the same time, it has a bad cache performance that slows down drawing.

		// All previous positions
		QVector<Vec3f> posHistory;
		Vec3f color;
		StelVertexBuffer<ColoredVertex>* vertexBuffer;
	};

	QList<Trail> allTrails;

	// Maximum time extent in days
	float timeExtent;

	QList<float> times;

	Mat4d j2000ToTrailNative;
	Mat4d j2000ToTrailNativeInverted;

	float opacity;

	//! Last time for which we have positions in history.
	//!
	//! We only add new positions to history after a long enough interval (one minute).
	double lastTimeInHistory;
};

#endif // TRAILMGR_HPP
