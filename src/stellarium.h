/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#ifndef _STELLARIUM_H_
#define _STELLARIUM_H_

#include <config.h>

#if defined( WIN32 ) || defined ( __MWERKS__ ) || defined( _MSC_VER ) || defined( MINGW32 )
#  ifndef WIN32
#     define WIN32
#  endif
#  include <windows.h>
#endif

#include "SDL.h"
#include "SDL_opengl.h"

#define APP_NAME "Stellarium "VERSION

#ifndef HAVE_ACOSF
# define acosf(x) (float)(acos(x))
#endif
#ifndef HAVE_ASINF
# define asinf(x) (float)(asin(x))
#endif
#ifndef HAVE_ATAN2F
# define atan2f(x) (float)(atan2(x))
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


#define AU 149597870.691
#define MY_MAX(a,b) (((a)>(b))?(a):(b))
#define MY_MIN(a,b) (((a)<(b))?(a):(b))

#endif /*_STELLARIUM_H_*/
