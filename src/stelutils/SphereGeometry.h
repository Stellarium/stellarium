#ifndef SPHEREGEOMETRY_H_
#define SPHEREGEOMETRY_H_

#include "vecmath.h"
#include <vector>

using namespace std;

//! all x with x*n-d >= 0
struct HalfSpace
{
	Vec3d n;
	double d;
	bool inside(const Vec3d &x) const {return (x*n>=d);}
	bool operator==(const HalfSpace& other) const {return (n==other.n && d==other.d);}
};

// Several HalfSpaces defining a convex region
class Convex
{
public:
	Convex() {;}
	Convex(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2,const Vec3d &e3);
	bool operator==(const Convex& other) const {return (halfSpaces==other.halfSpaces);}
	size_t getNbHalfSpace(void) const {return halfSpaces.size();}
	const HalfSpace& operator[](size_t x) const {return halfSpaces[x];}
	void addHalfSpace(const HalfSpace& h) {halfSpaces.push_back(h);}
	//! Return true if the vector is inside the convex
	bool inside(const Vec3d& v) const;
	//! Return true if the vector is inside the convex, but don't check for half space with index idx
	bool insideButOne(const Vec3d& v, unsigned int idx) const;
protected:
	std::vector<HalfSpace> halfSpaces;
};

// When the halfSpaces are planes (d=0) the Convex is a polygon
class ConvexPolygon : public Convex
{
public:
	ConvexPolygon() {;}
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2,const Vec3d &e3);
	bool operator==(const ConvexPolygon& other) const {return (vertex==other.vertex);}
	const Vec3d& getVertex(int i) const {return vertex[i];}
	//! Return true if the two convex intersect
	bool intersect(const ConvexPolygon& c) const;
	bool intersect(const Vec3d& v) const {return inside(v);}
	
	void getBoundingLonLat(double result[4]) const;
	
private:
	std::vector<Vec3d> vertex;
};

#endif /*SPHEREGEOMETRY_H_*/
