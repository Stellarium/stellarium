/*
 * Stellarium
 * Copyright (C) 2007 Guillaume Chereau
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

#include "SphereGeometry.hpp"
#include <QDebug>

using namespace StelGeom;

//! Special constructor for 3 halfspaces convex
ConvexS::ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2)
{
    reserve(3);
    push_back(e0^e1);
    push_back(e1^e2);
    push_back(e2^e0);
	
	// Warning: vectors not normalized while they should be
	// In this case it works because d==0 for each HalfSpace
}

//! Special constructor for 4 halfspaces convex
ConvexS::ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
{
    reserve(4);
	const double d = e0*((e1-e0)^(e2-e0));
	if (d > 0)
	{
		push_back(e0^e1);
		push_back(e1^e2);
		push_back(e2^e3);
		push_back(e3^e0);
		
		// Warning: vectors not normalized while they should be
		// In this case it works because d==0 for each HalfSpace
	}
	else
	{
		push_back((e1-e0)^(e2-e0));
		(*begin()).d = d;
		(*begin()).n.normalize();
	}
}



//! Special constructor for 3 points polygon
Polygon::Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2)
{
    reserve(3);
    push_back(e0);
    push_back(e1);
    push_back(e2);
}

//! Special constructor for 4 points polygon
Polygon::Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
{
    reserve(4);
    push_back(e0);
    push_back(e1);
    push_back(e2);
    push_back(e3);
}

//! Return the convex polygon area in steradians
double ConvexPolygon::getArea() const
{
	// Use Girard's theorem
	double angleSum=0.;
	const ConvexS& cvx = asConvex();
	const int size = cvx.size();
			
	if (size==1)
	{
		// Handle special case for > 180 degree polygons
		return cvx[0].getArea();
	}

	// Sum the angles at each corner of the polygon
	// (the non cartesian angle is found from the plan normals)
	for (int i=0;i<size-1;++i)
	{
		angleSum += M_PI-cvx[i].n.angle(cvx[i+1].n);
	}
	// Last angle
	angleSum += M_PI-cvx[size-1].n.angle(cvx[0].n);
	return angleSum - M_PI*(size-2);
}
	
//! Return the convex polygon barycenter
// TODO this code is quite wrong but good for triangles
Vec3d ConvexPolygon::getBarycenter() const
{
	Vec3d barycenter;
	for (unsigned int i=0;i<Polygon::size();++i)
	{
		barycenter += Polygon::operator[](i);
	}
	barycenter.normalize();
	return barycenter;
}

	
/*

void ConvexPolygon::getBoundingLonLat(double result[4]) const
{
	assert(vertex.size()==4);
	const Vec3d oneZ(Vec3d(0,0,1));
	
	// Get the bounding meridian and parallel for the viewport
	bool insideUp = inside(Vec3d(0,0,1));
	bool insideDown = inside(Vec3d(0,0,-1));


	result[2] = MY_MIN(MY_MIN(vertex[0].latitude(), vertex[1].latitude()), MY_MIN(vertex[2].latitude(), vertex[3].latitude()));
	result[3] = MY_MAX(MY_MAX(vertex[0].latitude(), vertex[1].latitude()), MY_MAX(vertex[2].latitude(), vertex[3].latitude()));
	if (result[2]<-M_PI/2)
		result[2] = -M_PI/2;
	if (result[3]>M_PI/2)
		result[3] = M_PI/2;

	if (insideUp) result[3] = M_PI/2;
	if (insideDown) result[2] = -M_PI/2;

	// Look for long bounds
	if (insideUp || insideDown)
	{
		result[0] = 0.;
		result[1] = 2.*M_PI;
	}
	else
	{
		result[0] = MY_MIN(MY_MIN(vertex[0].longitude(), vertex[1].longitude()), MY_MIN(vertex[2].longitude(), vertex[3].longitude()));
		result[1] = MY_MAX(MY_MAX(vertex[0].longitude(), vertex[1].longitude()), MY_MAX(vertex[2].longitude(), vertex[3].longitude()));
	}
}
*/
