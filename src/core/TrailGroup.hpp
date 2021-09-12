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

class StelPainter;

class TrailGroup
{
public:
	TrailGroup(float atimeExtent, int maxPoints);

	void draw(StelCore* core, StelPainter*);

	// Add 1 point to all the curves at current time and remove too old points
	void update();

	void addObject(const StelObjectP&, const Vec3f* col=Q_NULLPTR);

	void setOpacity(float op) {opacity=op;}

	//! Reset (clear) all trails points, and set maximum trail length to keep up framerate
	void reset(int maxPoints);

private:
	class Trail
	{
	public:
		Trail(const StelObjectP& obj, const Vec3f& col) : stelObject(obj), color(col) {;}
		StelObjectP stelObject;
		// All previous positions
		QList<Vec3d> posHistory;
		Vec3f color;
	};

	StelCore *core;
	QList<Trail> allTrails;

	// Maximum time extent in days
	float timeExtent;
	int maxPoints; //!< Limitation to avoid fps breakdown.

	QList<float> times;

	Mat4d j2000ToTrailNative;
	Mat4d j2000ToTrailNativeInverted;

	float opacity;
};

#endif // TRAILMGR_HPP
