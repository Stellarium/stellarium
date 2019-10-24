/************************************************************************

Copyright (c) 2007 Johannes Gajdosik

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

****************************************************************/

#ifndef CALC_INTERPOLATED_ELEMENTS_H
#define CALC_INTERPOLATED_ELEMENTS_H

extern
void CalcInterpolatedElements(const double t,double elem[],
                              const int dim,
                              void (*calc_func)(const double t,double elem[],
                                                void *user),
                              const double delta_t,
                              double *t0,double e0[],
                              double *t1,double e1[],
                              double *t2,double e2[],
                              void *user);

/*
Simple interpolation routine with external cache.
The cache consists of 3 sets of values:
  e0[0..dim-1] are the cached values at time *t0,
  e1[0..dim-1] are the cached values at time *t1,
  e2[0..dim-1] are the cached values at time *t2
delta_t is the time step: *t2-*t1 = *t1-*t0 = delta_t,
(*calc_func)(t,elem,user) calculates the values elem[0..dim-1] at time t,
t is the input parameter, elem[0..dim-1] are the output values, user is the
user supplied data.

The user must supply *t0,*t1,*t2,e0,e1,e2.
The initial values must be *t0 = *t1 = *t2 = -1e100,
the initial values of e0,e1,e2 can be undefined.
The values of *t0,*t1,*t2,e0,e1,e2 belong to this function,
the user must never change them.

The user must always supply the same delta_t
for one set of (*t0,*t1,*t2,e0,e1,e2),
and of course the same dim and calc_func.

The user argument is passed to the calc_func callback.
*/

#endif // CALC_INTERPOLATED_ELEMENTS_H
