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

For documentation see the header file.

****************************************************************/

#include "calc_interpolated_elements.h"

void CalcInterpolatedElements(const double t,double elem[],
                              const int dim,
                              void (*calc_func)(const double t,double elem[]),
                              const double delta_t,
                              double *t0,double e0[],
                              double *t1,double e1[],
                              double *t2,double e2[]) {
/*
printf("CalcInterpolatedElements: %12.9f %12.9f %12.9f %12.9f\n",t,*t0,*t1,*t2);
*/
  int i;
  if (*t1 < -1e99) { /* *t1 uninitialized */
    *t0 = -1e100;
    *t2 = -1e100;
    *t1 = t;
    (*calc_func)(*t1,e1);
    for (i=0;i<dim;i++) elem[i] = e1[i];
    return;
  }
  if (t <= *t1) {
    if (*t1 - delta_t <= t) { /* interpolate */
      if (*t0 < -1e99) {
        *t0 = *t1 - delta_t;
        (*calc_func)(*t0,e0);
      }
    } else if (*t1 - 2.0*delta_t <= t) { /* interpolate */
      if (*t0 < -1e99) {
        *t0 = *t1 - delta_t;
        (*calc_func)(*t0,e0);
      }
      *t2 = *t1;*t1 = *t0;
      for (i=0;i<dim;i++) {e2[i] = e1[i];e1[i] = e0[i];}
      *t0 = *t1 - delta_t;
      (*calc_func)(*t0,e0);
    } else {
      *t0 = -1e100;
      *t2 = -1e100;
      *t1 = t;
      (*calc_func)(*t1,e1);
      for (i=0;i<dim;i++) elem[i] = e1[i];
      return;
    }
    {
      const double f0 = (*t1 - t);
      const double f1 = (t - *t0);
      const double fact = 1.0 / delta_t;
      for (i=0;i<dim;i++) elem[i] = fact * (e0[i]*f0 + e1[i]*f1);
    }
  } else {
    if (*t1 + delta_t >= t) { /* interpolate */
      if (*t2 < -1e99) {
        *t2 = *t1 + delta_t;
        (*calc_func)(*t2,e2);
      }
    } else if (*t1 + 2.0*delta_t >= t) { /* interpolate */
      if (*t2 < -1e99) {
        *t2 = *t1 + delta_t;
        (*calc_func)(*t2,e2);
      }
      *t0 = *t1;*t1 = *t2;
      for (i=0;i<dim;i++) {e0[i] = e1[i];e1[i] = e2[i];}
      *t2 = *t1 + delta_t;
      (*calc_func)(*t2,e2);
    } else {
      *t0 = -1e100;
      *t2 = -1e100;
      *t1 = t;
      (*calc_func)(*t1,e1);
      for (i=0;i<dim;i++) elem[i] = e1[i];
      return;
    }
    {
      const double f1 = (*t2 - t);
      const double f2 = (t - *t1);
      const double fact = 1.0 / delta_t;
      for (i=0;i<dim;i++) elem[i] = fact * (e1[i]*f1 + e2[i]*f2);
    }
  }
}

