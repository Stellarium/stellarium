#ifndef _TRIPLET_HPP_
#define _TRIPLET_HPP_

//! A simple struct holding a triplet of values of some type.
//!
//! Safer and more readable than using fixed-size arrays and passing them as pointers.
//!
//! Used e.g. for triangles. 
template<class T>
struct Triplet
{
	//! First, second, and third element of the triplet.
	T a, b, c;

	//! Default constructor for collections.
	Triplet(){}

	//! Construct a triplet from three values.
	Triplet(const T& a, const T& b, const T& c) : a(a), b(b), c(c){}
};

#endif // _TRIPLET_HPP_
