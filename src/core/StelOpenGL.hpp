/*
 * Stellarium
 * Copyright (C) 2014 Fabien Chereau
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

#ifndef _STELOPENGL_HPP_
#define _STELOPENGL_HPP_

#define GL_GLEXT_PROTOTYPES
#include <qopengl.h>

#include <QOpenGLFunctions>

#ifndef NDEBUG
# define GL(line) do { \
	line;\
	if (checkGLErrors(__FILE__, __LINE__))\
		exit(-1);\
	} while(0)
#else
# define GL(line) line
#endif

const char* getGLErrorText(int code);
int checkGLErrors(const char *file, int line);

#endif // _STELOPENGL_HPP_
