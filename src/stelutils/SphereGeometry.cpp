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
    push_back(e1^e0);
    push_back(e2^e1);
	push_back(e0^e2);
	
	// Warning: vectors not normalized while they should be
	// In this case it works because d==0 for each HalfSpace
}

//! Special constructor for 4 halfspaces convex
ConvexS::ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
{
    reserve(4);
	const double d = e3*((e2-e3)^(e1-e3));
	if (d > 0)
	{
		push_back(e1^e0);
		push_back(e2^e1);
		push_back(e3^e2);
		push_back(e0^e3);
		
		// Warning: vectors not normalized while they should be
		// In this case it works because d==0 for each HalfSpace
	}
	else
	{
		push_back((e2-e3)^(e1-e3));
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
// TODO Optimize using add oc formulas from http://en.wikipedia.org/wiki/Solid_angle
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
	if (ConvexS::size()==1)
	{
		// Handle special case for > 180 degree polygons
		return asConvex()[0].n;
	}
	
	Vec3d barycenter;
	for (unsigned int i=0;i<Polygon::size();++i)
	{
		barycenter += Polygon::operator[](i);
	}
	barycenter.normalize();
	return barycenter;
}

namespace StelGeom
{

//! Compute the intersection of the planes defined by the 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 HalfSpaces don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined
bool planeIntersect2(const HalfSpace& h1, const HalfSpace& h2, Vec3d& p1, Vec3d& p2)
{
	const Vec3d& n1 = h1.n;
	const Vec3d& n2 = h2.n;
	const double& d1 = h1.d;
	const double& d2 = h2.d;
	const double& a1 = n1[0];
	const double& b1 = n1[1];
	const double& c1 = n1[2];
	const double& a2 = n2[0];
	const double& b2 = n2[1];
	const double& c2 = n2[2];
		
	// Compute the parametric equation of the line at the intersection of the 2 planes
	Vec3d u = n1^n2;
	if (u==Vec3d(0,0,0))
	{
		// The planes are parallel
		return false;
	}
	
	// u gives the direction of the line, still need to find a suitable start point p0
	u.normalize();
	int maxI = (fabs(u[0])>fabs(u[1])) ? (fabs(u[0])>fabs(u[2]) ? 0 : 2) : (fabs(u[1])>fabs(u[2]) ? 1 : 2);
	Vec3d p0;
	if (maxI==0)
	{
		const double denom = c1*b2-c2*b1;
		p0[1] = (d2*c1-d1*c2)/denom;
		p0[2] = (d1*b2-d2*b1)/denom;
	}
	else if (maxI==1)
	{
		const double denom = a1*c2-a2*c1;
		p0[0]=(c1*d2-c2*d1)/denom;
		p0[2]=(a2*d1-d2*d1)/denom;
	}
	else if (maxI==2)
	{
		const double denom = a1*b2-a2*b1;
		p0[0]=(b1*d2-b2*d1)/denom;
		p0[1]=(a2*d1-a1*d2)/denom;
	}
	
	// The points are on the unit sphere x^2+y^2+z^2=1, replace y and z and get something of the form ax^2+b*x+c=0
	const double a = 1.;
	const double b = 2.*p0*u;
	const double c = p0.lengthSquared()-1.;
	
	// If discriminant <=0, zero or 1 real solution
	const double D = b*b-4.*a*c;
	if (D<=0)
		return false;
	
	const double t1 = (-b+std::sqrt(D))/(2.*a);
	const double t2 = (-b-std::sqrt(D))/(2.*a);
	p1 = p0+u*t1;
	p2 = p0+u*t2;
	
	return true;
}
}
