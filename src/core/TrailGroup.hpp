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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
	TrailGroup(float atimeExtent);

	void draw(StelCore* core, StelPainter*);

	// Add 1 point to all the curves at current time and suppress too old points
	void update();

	// Set the matrix to use to post process J2000 positions before storing in the trail
	void setJ2000ToTrailNative(const Mat4d& m);

	void addObject(const StelObjectP&);
	void removeObject(const StelObjectP&);

	void setOpacity(float op) {opacity=op;}

	//! Reset all trails points
	void reset();

private:
	class Trail
	{
	public:
		Trail(const StelObjectP& obj) : stelObject(obj) {;}
		StelObjectP stelObject;
		// All previous positions
		QList<Vec3d> posHistory;
	};

	QMap<StelObjectP, Trail> allTrails;

	// Maximum time extent in days
	float timeExtent;

	QList<float> times;

	Mat4d j2000ToTrailNative;
	Mat4d j2000ToTrailNativeInverted;

	float opacity;
};

#endif // TRAILMGR_HPP
