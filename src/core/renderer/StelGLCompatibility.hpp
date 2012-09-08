/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELGLCOMPATIBILITY_HPP_
#define _STELGLCOMPATIBILITY_HPP_


// Some glext definitions not available on every platforms.
// We do this to avoid needing to bundle our own glext.h 
// (which would result in preprocessor redefinitions).
//
// If Qt provides these in the future, we can remove this 
// (try doing that after we move to Qt 5.x!)

#ifndef GL_INVALID_FRAMEBUFFER_OPERATION
	#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#endif

#ifndef GL_RGBA32F
	#ifdef GL_RGBA32F_ARB 
		#define GL_RGBA32F GL_RGBA32F_ARB
	#else
		#define GL_RGBA32F 0x8814
	#endif
#endif

#ifndef GL_MULTISAMPLE
	#ifdef GL_MULTISAMPLE_ARB
		#define GL_MULTISAMPLE GL_MULTISAMPLE_ARB
	#else
		#define GL_MULTISAMPLE 0x809D
	#endif
#endif

#ifndef GL_MAX_TEXTURE_UNITS
	#ifdef GL_MAX_TEXTURE_UNITS_ARB 
		#define GL_MAX_TEXTURE_UNITS GL_MAX_TEXTURE_UNITS_ARB
	#else
		#define GL_MAX_TEXTURE_UNITS 0x84E2
	#endif
#endif

#ifndef GL_CLAMP_TO_EDGE
	#define GL_CLAMP_TO_EDGE 0x812F
#endif

#endif // _STELGLCOMPATIBILITY_HPP_
