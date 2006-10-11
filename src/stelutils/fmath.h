/*
 * Copyright (C) 2004 Fabien Chereau
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

// Redefine the single precision math functions if not defined
// This must be used in conjunction with the autoconf macro :
// AC_CHECK_FUNCS(sinf cosf tanf asinf acosf atanf expf logf log10f atan2f sqrtf)

#ifndef _FMATH_H_
#define _FMATH_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_POW10
# define pow10(x) pow(10,(x))
#endif

// math functions for SUN C++
#ifdef __SUNPRO_CC
#include <math.h>
#include <sunmath.h>
#elif defined(__sun) || defined(__sun__)
#include <math.h>
static inline float logf (float x) { return log (x); }
static inline float log10f (float x) { return log10 (x); }
static inline float sqrtf (float x) { return sqrt (x); }
static inline float floorf (float x) { return floor (x); }
static inline float powf (float x, float y) { return pow (x,y); }
static inline float cosf (float x) { return cos (x); }
static inline float acosf (float x) { return acos (x); }
static inline float sinf (float x) { return sin (x); }
static inline float asinf (float x) { return asin (x); }
static inline float atan2f (float x, float y) { return atan2 (x,y); }
static inline float tanf (float x) { return tan (x); }
static inline float atanf (float x) { return atan (x); }
static inline float ceilf (float x) { return ceil (x); }
static inline float expf (float x) { return exp (x); }
#elif !defined(WIN32)
#ifndef HAVE_ACOSF
# define acosf(x) (float)(acos(x))
#endif
#ifndef HAVE_ASINF
# define asinf(x) (float)(asin(x))
#endif
#ifndef HAVE_ATAN2F
# define atan2f(x, y) (float)(atan2((x),(y)))
#endif
#ifndef HAVE_ATANF
# define atanf(x) (float)(atan(x))
#endif
#ifndef HAVE_COSF
# define cosf(x) (float)(cos(x))
#endif
#ifndef HAVE_EXPF
# define expf(x) (float)(exp(x))
#endif
#ifndef HAVE_POWF
# define powf(x, y) (float)(pow((x),(y)))
#endif
#ifndef HAVE_LOG10F
# define log10f(x) (float)(log10(x))
#endif
#ifndef HAVE_LOGF
# define logf(x) (float)(log(x))
#endif
#ifndef HAVE_SINF
# define sinf(x) (float)(sin(x))
#endif
#ifndef HAVE_SQRTF
# define sqrtf(x) (float)(sqrt(x))
#endif

#endif /* CYGWIN */

#endif /*_FMATH_H_*/
