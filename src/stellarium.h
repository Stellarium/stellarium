/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

// mac seems to use KMOD_META instead of KMOD_CTRL
#ifdef MACOSX
#define COMPATIBLE_KMOD_CTRL KMOD_META
#else
#define COMPATIBLE_KMOD_CTRL KMOD_CTRL
#endif

#define APP_NAME "Stellarium "PACKAGE_VERSION

#endif /*_STELLARIUM_H_*/
