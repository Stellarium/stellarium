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
#else
// Assume UNIX compatible by default 
#   define COMPILER_IS_UNIX_COMPATIBLE 1
#   define HAVE_STRCASECMP 
#endif

#if defined( WIN32 ) || defined( __CYGWIN__ ) || \
    defined ( NATIVE_WIN32_COMPILER )
#  ifndef WIN32
#     define WIN32
#  endif
#  include <windows.h>
#endif 

#if defined( NATIVE_WIN32_COMPILER )
#   define HAVE_STRICMP
#endif


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h> 

#if defined( MACOSX )
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


#include "stella_types.h"
#include "s_texture.h"

#define APP_NAME "Stellarium v 0.4.7"
#define DATA_DIR "data"
#define TEXTURE_DIR "textures"
// Conversion in standar Julian time format
#define SECONDE 0.00001157407407407407407407407407407407407407407
#define MINUTE 0.00069444444444444444444444444444444444444444444
#define HEURE 0.04166666666666666666666666666666666666666666666
#define PI 3.14159265358979323846264338327950288419716939937510
#define UA 149597870
#define myMax(a,b) (((a)>(b))?(a):(b)) 
#define RAYON 500

extern stellariumParams global;


#if 0

#ifdef STELLARIUM_NO_ASSERT
#define check_assertion( condition, desc )  /* noop */
#else
#define check_assertion( condition, desc ) \
    if ( condition ) {} else { \
        fprintf( stderr, "!!! " PROG_NAME " unexpected error [%s:%d]: %s\n", \
         __FILE__, __LINE__, desc ); \
        abort(); \
    }
#endif

#define code_not_reached() \
    check_assertion( 0, "supposedly unreachable code reached!" )

#endif

#endif /*_STELLARIUM_H_*/
