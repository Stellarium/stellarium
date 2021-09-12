/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef GLFUNCS_HPP
#define GLFUNCS_HPP

#include <QOpenGLContext>
#include <QOpenGLFunctions_1_0>

#ifndef QT_OPENGL_ES_2
//! Defines some OpenGL functions not resolved through StelOpenGL (which only contains base OpenGL ES2 functions)
//! Using the QOpenGLFunctions_*_* directly would solve this better, but it conflicts with the
//! current StelOpenGL header dramatically.
class GLExtFuncs : public QOpenGLFunctions_1_0
{
public:
	//! Since 3.2
	PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;

	void init(QOpenGLContext* ctx)
	{
		glFramebufferTexture = reinterpret_cast<PFNGLFRAMEBUFFERTEXTUREPROC>(ctx->getProcAddress("glFramebufferTexture"));

		if(!ctx->isOpenGLES())
			initializeOpenGLFunctions();
	}
};

extern GLExtFuncs* glExtFuncs;
#endif

#endif
