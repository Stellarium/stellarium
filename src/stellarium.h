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

#if defined ( __MWERKS__ ) || defined( _MSC_VER )
#   define NATIVE_WIN32_COMPILER 1
#endif

#if defined( WIN32 ) || \
	defined ( NATIVE_WIN32_COMPILER )
#  ifndef WIN32
#     define WIN32
#  endif
#  include <windows.h>
#endif 

#ifdef __MINGW32__
#endif

#if defined( NATIVE_WIN32_COMPILER )
#   define HAVE_STRICMP
#endif

#include <config.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <string.h>
#include <ctype.h> 

#ifdef MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif


#include "stella_types.h"

#define APP_NAME "Stellarium v "VERSION

// Conversion in standar Julian time format
#define JD_SECOND 0.00001157407407407407407407407407407407407407407
#define JD_MINUTE 0.00069444444444444444444444444444444444444444444
#define JD_HOUR 0.04166666666666666666666666666666666666666666666
#define JD_DAY 1

#define PI 3.14159265358979323846264338327950288419716939937510
#define UA 149597870
#define myMax(a,b) (((a)>(b))?(a):(b)) 
#define RAYON 500

// The global structure
extern stellariumParams global;
extern stellariumParams globalInit;

#define code_not_reached() \
    check_assertion( 0, "supposedly unreachable code reached!" )


#endif /*_STELLARIUM_H_*/
