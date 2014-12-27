// Solve.hpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// 2008-06-28 - reformatted to comply with Stellarium's coding
//              style -MNG


#ifndef _SOLVE_HPP_
#define _SOLVE_HPP_

#include <utility>

// Solve a function using the bisection method.  Returns a pair
// with the solution as the first element and the error as the second.
template<class T, class F> std::pair<T, T> solveBisection(F f,
                                                          T lower, T upper,
                                                          T err,
                                                          int maxIter = 100)
{
	T x = 0.0;

	for (int i = 0; i < maxIter; i++)
	{
		x = (lower + upper) * (T) 0.5;
		if (upper - lower < 2 * err)
			break;

		T y = f(x);
		if (y < 0)
			lower = x;
		else
			upper = x;
	}

	return std::make_pair(x, (upper - lower) / 2);
}


// Solve using iteration; terminate when error is below err or the maximum
// number of iterations is reached.
template<class T, class F> std::pair<T, T> solveIteration(F f,
                                                          T x0,
                                                          T err,
                                                          int maxIter = 100)
{
	T x = 0;
	T x2 = x0;

	for (int i = 0; i < maxIter; i++)
	{
		x = x2;
		x2 = f(x);
		if (abs(x2 - x) < err)
			return std::make_pair(x2, x2 - x);
	}

	return std::make_pair(x2, x2 - x);
}


// Solve using iteration method and a fixed number of steps.
template<class T, class F> std::pair<T, T> solveIteration_fixed(F f,
                                                                 T x0,
                                                                 int maxIter)
{
	T x = 0;
	T x2 = x0;

	for (int i = 0; i < maxIter; i++)
	{
		x = x2;
		x2 = f(x);
	}

	return std::make_pair(x2, x2 - x);
}

#endif // _SOLVE_HPP_

