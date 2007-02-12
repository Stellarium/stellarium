#include "SphereGeometry.h"
#include "StelUtils.hpp"

// Create a Convex from 4 vectors
Convex::Convex(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2,const Vec3d &e3) : halfSpaces(4)
{
  halfSpaces[0].d = e0*((e1-e0)^(e2-e0));
  if (halfSpaces[0].d > 0)
  {
    halfSpaces[0].n = e0^e1;
    halfSpaces[1].n = e1^e2;
    halfSpaces[2].n = e2^e3;
    halfSpaces[3].n = e3^e0;
    halfSpaces[0].d = 0.0;
    halfSpaces[1].d = 0.0;
    halfSpaces[2].d = 0.0;
    halfSpaces[3].d = 0.0;
  }
  else
  {
  	halfSpaces.pop_back();
  	halfSpaces.pop_back();
  	halfSpaces.pop_back();
    halfSpaces[0].n = (e1-e0)^(e2-e0);
  }
}

//! Return true if the vector is inside the convex
bool Convex::inside(const Vec3d& v) const
{
	for (vector<HalfSpace>::const_iterator iter=halfSpaces.begin();iter!=halfSpaces.end();++iter)
	{
		if (!iter->inside(v))
			return false;
	}
	return true;
}


// Create a ConvexPolygon from 4 vectors
ConvexPolygon::ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2,const Vec3d &e3) : 
	Convex(e0, e1, e2, e3), vertex(4)
{
  vertex[0] = e0;
  vertex[1] = e1;
  vertex[2] = e2;
  vertex[3] = e3;
}

// Return true if the two convex intersect
bool ConvexPolygon::intersect(const ConvexPolygon& c) const
{
	vector<HalfSpace>::const_iterator iter=halfSpaces.begin();
	for (;iter!=halfSpaces.end();++iter)
	{
		vector<Vec3d>::const_iterator iterv=c.vertex.begin();
		for (;iterv!=c.vertex.end();++iterv)
		{
			if (iter->inside(*iterv))
				break;
		}
		// If there is one side for which all the vertex of the other polygon 
		// are outside, then the polygons don't intersect! 
		if (iterv==c.vertex.end())
			return false;
	}
	return true;
}
