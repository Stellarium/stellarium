/*
 * Copyright (C) 2014 Alexander Wolf
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

#include "StelOpenGL.hpp"
#include <QDebug>

QOpenGLContext* StelOpenGL::mainContext = Q_NULLPTR;

const char* StelOpenGL::getGLErrorText(GLenum code) {
	switch (code) {
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		default:
			return "unknown GL error type";
	}
}

int StelOpenGL::checkGLErrors(const char *file, int line)
{
	int errors = 0;
	for(GLenum x; (x=mainContext->functions()->glGetError())!=GL_NO_ERROR; )
	{
		++errors;
		qCritical("%s:%d: OpenGL error: %d (%s)\n",
			  file, line, x, getGLErrorText(x));
	}
	return errors;
}

void StelOpenGL::clearGLErrors()
{
	while(mainContext->functions()->glGetError()!=GL_NO_ERROR)
	{ }
}
