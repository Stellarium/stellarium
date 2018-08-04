/*
 * Stellarium
 * Copyright (C) 2014 Fabien Chereau
 * Copyright (C) 2016 Florian Schaukowitsch
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

#ifndef STELOPENGL_HPP
#define STELOPENGL_HPP

#include <QOpenGLFunctions>

#ifndef QT_NO_DEBUG
# define GL(line) do { \
	/*for debugging, we make sure we use our main context*/\
	Q_ASSERT(StelOpenGL::mainContext == QOpenGLContext::currentContext());\
	line;\
	Q_ASSERT_X(!StelOpenGL::checkGLErrors(__FILE__, __LINE__), "GL macro", "OpenGL errors encountered");\
	} while(0)
#else
# define GL(line) line
#endif

//! Namespace containing some OpenGL helpers
namespace StelOpenGL
{
	//! The main context as created by the StelMainView (only used for debugging)
	extern QOpenGLContext* mainContext;

	//! Converts a GLenum from glGetError to a string
	const char* getGLErrorText(GLenum code);
	//! Retrieves and prints all current OpenGL errors to console (glGetError in a loop). Returns number of errors.
	int checkGLErrors(const char *file, int line);
	//! Clears all queued-up OpenGL errors without handling them
	void clearGLErrors();
}

// This is still needed for the ARM platform (armhf)

#if defined(QT_OPENGL_ES_2)
#ifndef GL_DOUBLE
#define GL_DOUBLE GL_FLOAT
#endif
#endif

#endif // STELOPENGL_HPP
