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
 
#ifndef SPHEREGEOMETRY_HPP_
#define SPHEREGEOMETRY_HPP_
 
/*
    In this file we define different geometrical shapes :
    - HalfSpace
    - Convex
    - Polygon
    - ConvexPolygon
    - Disk
    
    We also define two functions
    - contains(x, y)
    - intersect(x, y) = intersect(y, x)
*/

#include <vector>
#include <algorithm>
#include <functional>

#include "vecmath.h"

namespace StelGeom
{

// Just used to resolved some compile time type embiguities
template <class T1, class T2>
static bool containsT(const T1& o1, const T2& o2)
{
    return contains(o1, o2);
}


/****************
Now the geometrical objects
*****************/

template<class T>
bool intersect(const Vec3d& v, const T& o)
{ return contains(o, v); }

template<class T>
bool intersect(const T& o, const Vec3d& v)
{ return contains(o, v); }

//! HalfSpace
struct HalfSpace
{
    HalfSpace() : d(0) {}
    HalfSpace(const Vec3d& n_) : n(n_), d(0) {}
    HalfSpace(const Vec3d& n_, double d_) : n(n_), d(d_) {}
    HalfSpace(const HalfSpace& other) : n(other.n), d(other.d) {}
	bool contains(const Vec3d &v) const {return (v*n>=d);}
	bool operator==(const HalfSpace& other) const {return (n==other.n && d==other.d);}
	
	Vec3d n;
	double d;
};

//! Polygon class
//! This is a set of points
class Polygon : public std::vector<Vec3d>
{
public:
    //! Default contructor
    Polygon(int size = 0) : std::vector<Vec3d>(size) {}
    //! Special constructor for 3 points polygon
    Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2);
    //! Special constructor for 4 points polygon
    Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3);
};

// template<class T>
// bool contains(const T& o, const Polygon& p)
// {
// 	for (Polygon::const_iterator vertex=p.begin();vertex!=p.end();++vertex)
// 	{
// 		if (!contains(o, *vertex))
// 			return false;
// 	}
// 	return true;
// }

// template<class T>
// bool intersect(const T& o, const Polygon& p)
// {
// 	for (Polygon::const_iterator iter=p.begin();iter!=p.end();++iter)
// 	{
// 		if (containsT(o, *iter))
// 			return true;
// 	}
// 	return false;
// }

template<class T>
bool intersect(const Polygon& p, const T& o)
{ return intersect(o, p); }



//! Several HalfSpaces defining a convex region
//! Damn X11! Convex is #defined as an int in X11/X.h: (#define Convex 2)
//! So we need to use another name...
class ConvexS : public std::vector<HalfSpace>
{
public:
    //! copy constructor
    ConvexS(const ConvexS& c) : std::vector<HalfSpace>(c) {}
    //! Default constructor
    ConvexS(int size = 0) : std::vector<HalfSpace>(size) {}
    //! Special constructor for 3 halfspaces convex
    ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2);
    //! Special constructor for 4 halfspaces convex
    ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3);
	
	//! Tell whether the points of the passed Polygon are all outside of at least one HalfSpace
	bool areAllPointsOutsideOneSide(const Polygon& poly) const
	{
		for (const_iterator iter=begin();iter!=end();++iter)
		{
			bool allOutside = true;
			for (Polygon::const_iterator v=poly.begin();v!=poly.end()&& allOutside==true;++v)
			{
				allOutside = allOutside && !iter->contains(*v);
			}
			if (allOutside)
				return true;
		}
		return false;
	}
};

// template <class T>
// bool contains(const ConvexS& c, const T& o)
// {
// 	for (ConvexS::const_iterator iter=c.begin();iter!=c.end();++iter)
// 	{
// 		if (!contains(*iter, o))
// 			return false;
// 	}
// 	return true;
// }


//! ConvexPolygon class
//! It stores both informations :
//! The halfpaces and the points
class ConvexPolygon : public ConvexS, public Polygon
{
public:

    //! Default constructor
    ConvexPolygon() {}

    //! Special constructor for 3 points
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2):
	    ConvexS(e0, e1, e2), Polygon(e0, e1, e2)
	{}
	
	//! Special constructor for 4 points
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3):
	    ConvexS(e0, e1, e2, e3), Polygon(e0, e1, e2, e3)
	{}
	
	bool operator==(const ConvexPolygon& other) const {
	    return ((const Polygon&)(*this)==(const Polygon&)other);
	}
	
	//! By default the [] operator return the vertexes
	const Vec3d& operator[](const Polygon::size_type& i)const {
	    return Polygon::operator[](i);
	}
	
	//! By default the [] operator return the vertexes
	Vec3d& operator[](Polygon::size_type& i) {
	    return Polygon::operator[](i);
	}
	
	//! Cast to Polygon in case of ambiguity
	Polygon& asPolygon() {return static_cast<Polygon&>(*this);}
	
	//! Same with const
	const Polygon& asPolygon() const {return static_cast<const Polygon&>(*this);}
	
	//! Cast to Convex in case of ambiguity
	ConvexS& asConvex() {return static_cast<ConvexS&>(*this);}
	
	//! Same with const
	const ConvexS& asConvex() const {return static_cast<const ConvexS&>(*this);}
};


//! We rewrite the intersect for ConvexPolygon
inline bool intersect(const ConvexPolygon& cp1, const ConvexPolygon& cp2)
{
	//std::cerr << "intersect(const ConvexPolygon& cp1, const ConvexPolygon& cp2)" << std::endl;
	const ConvexS& c1 = cp1;
	const ConvexS& c2 = cp2;
	return !c1.areAllPointsOutsideOneSide(cp2) && !c2.areAllPointsOutsideOneSide(cp1);
}

struct Disk : HalfSpace
{
    //! constructor
    // @param a is the disk radius in radian
    Disk(const Vec3d& n, double a) : HalfSpace(n, std::cos(a/2))
    {} 
};

//! We rewrite the intersect for ConvexPolygon
inline bool intersect(const Disk& cp1, const ConvexPolygon& cp2)
{
	assert(0);
	// TODO
	return false;
}

//! We rewrite the intersect for ConvexPolygon
inline bool intersect(const ConvexS& cp1, const ConvexPolygon& cp2)
{
	assert(0);
	// TODO
	return false;
}

// special for ConvexPolygon
// inline bool intersect(const Disk& d, const ConvexPolygon& p)
// {
// 	return intersect(p, d.n) || 
// 	       intersect(d, static_cast<const Polygon&>(p));
// }
// 
// special for ConvexPolygon
inline bool intersect(const ConvexPolygon& p, const Disk& d)
{
	return intersect(d, p);
}


template<class S1, class S2>
class Difference
{
public:
    Difference(const S1& s1_, const S2& s2_) : s1(s1_), s2(s2_) 
    {}
    
    S1 s1;
    S2 s2;
};

template<class S1, class S2, class S>
inline bool intersect(const Difference<S1, S2>& d, const S& s)
{
    return !contains(d.s2, s) && intersect(d.s1, s);
}

template<class S1, class S2>
bool intersect(const Difference<S1, S2>& d, const Vec3d& v)
{ return !contains(d.s2, v) && intersect(d.s1, v); }

template<class S1, class S2, class S>
inline bool contains(const Difference<S1, S2>&d, const S& s)
{
    return !intersect(d.s2, s) && contains(d.s1, s);
}



inline bool contains(const HalfSpace& h,  const Vec3d& v)
{
	return h.contains(v);
}

inline bool contains(const HalfSpace& h, const Polygon& poly)
{
	for (Polygon::const_iterator iter=poly.begin();iter!=poly.end();++iter)
	{
		if (!h.contains(*iter))
			return false;
	}
	return true;
}

inline bool contains(const ConvexPolygon& c, const Vec3d& v)
{
	const ConvexS& conv = c;
	for (ConvexS::const_iterator iter=conv.begin();iter!=conv.end();++iter)
	{
		if (!iter->contains(v))
			return false;
	}
	return true;
}

inline bool contains(const ConvexPolygon& cp1, const ConvexPolygon& cp2)
{
	const Polygon& poly = cp2;
	for (Polygon::const_iterator iter=poly.begin();iter!=poly.end();++iter)
	{
		if (!contains(cp1, *iter))
			return false;
	}
	return true;
}

}	// namespace StelGeom

#endif /*SPHEREGEOMETRY_HPP_*/
