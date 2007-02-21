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
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "vecmath.h"

using namespace boost::lambda;

/******************
First some internal convenient functions
*******************/

//! Return true if the predicate if true for any of the elements of c
template<class C, class P>
static bool any(const C& c, P pred)
{
    return std::find_if(c.begin(), c.end(), pred) != c.end();
}

//! Return true if the predicate if true for all of the elements of c
template<class C, class P>
static bool all(const C& c, P pred)
{
    return std::find_if(c.begin(), c.end(), !pred) == c.end();
}


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
    HalfSpace(const HalfSpace& other) : n(other.n), d(0) {}
    
	Vec3d n;
	double d;
	bool contains(const Vec3d &v) const {return (v*n>=d);}
	bool operator==(const HalfSpace& other) const {return (n==other.n && d==other.d);}
};

inline bool contains(const HalfSpace& h,  const Vec3d& v)
{
    return h.contains(v);
}


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

template<class T>
bool contains(const T& o, const Polygon& p)
{
    return ::all(p, bind(&containsT<T, Vec3d>,o, _1));
}

template<class T>
bool intersect(const T& o, const Polygon& p)
{
//	for (Polygon::const_iterator iter=p.begin();iter!=p.end();++iter)
//	{
//		if (containsT(o, *iter))
//			return true;
//	}
//	return false;
    return ::any(p, bind(&containsT<T, Vec3d>,o, _1));
}

template<class T>
bool intersect(const Polygon& p, const T& o)
{ return intersect(o, p); }



//! Several HalfSpaces defining a convex region
class Convex : public std::vector<HalfSpace>
{
public:
    //! Default contructor
    Convex(int size = 0) : std::vector<HalfSpace>(size) {}
    //! Special constructor for 3 halfspaces convex
    Convex(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2);
    //! Special constructor for 4 halfspaces convex
    Convex(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3);
};

template <class T>
bool contains(const Convex& c, const T& o)
{
	for (Convex::const_iterator iter=c.begin();iter!=c.end();++iter)
	{
		if (!contains(*iter, o))
			return false;
	}
	return true;
// Should be equivalent but in debug one is much much more fast than the other
//    return ::all(c, bind(
//                    static_cast<bool(*)(const HalfSpace&, const T&)>(&contains),
//                    _1, o));
}



//! ConvexPoygon class
//! It stores both informations :
//! The halfpaces and the points
class ConvexPolygon : public Convex, public Polygon
{
public:

    //! Default constructor
    ConvexPolygon() {}

    //! Special constructor for 3 points
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2):
	    Convex(e0, e1, e2), Polygon(e0, e1, e2)
	{}
	
	//! Special constructor for 4 points
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3):
	    Convex(e0, e1, e2, e3), Polygon(e0, e1, e2, e3)
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
	Convex& asConvex() {return static_cast<Convex&>(*this);}
	
	//! Same with const
	const Convex& asConvex() const {return static_cast<const Convex&>(*this);}
};

//! we rewrite the intersect for ConvexPolygon
inline bool intersect(const ConvexPolygon& c1, const ConvexPolygon& c2)
{
    const Polygon& p1 = c1;
    const Polygon& p2 = c2;
    return intersect(c1, p2) || intersect(c2, p1);
}



struct Disk
{
    Vec3d norm;
    float angle;
    
    //! constructor
    Disk(const Vec3d& n, float a) :
        norm(n), angle(a)
    { 
    } 
    
    bool contains(const Vec3d& v) const
    { return norm.angle(v) <= angle; }
};


inline bool contains(const Disk& d,  const Vec3d& v)
{
    return d.contains(v);
}

inline bool intersect(const Disk& d,  const Vec3d& v)
{
    return d.contains(v);
}


// special for ConvexPolygon
inline bool intersect(const Disk& d, const ConvexPolygon& p)
{
	return intersect(p, d.norm) || 
	       intersect(d, static_cast<const Polygon&>(p));
}

// special for ConvexPolygon
inline bool intersect(const ConvexPolygon& p, const Disk& d)
{
	return intersect(d, p);
}

#endif /*SPHEREGEOMETRY_HPP_*/
