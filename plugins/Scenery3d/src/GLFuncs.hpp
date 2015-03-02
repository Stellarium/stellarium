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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _GLFUNCS_HPP_
#define _GLFUNCS_HPP_

#include <QOpenGLContext>
#include <QOpenGLFunctions_1_0>

//! Defines some OpenGL functions not resolved through StelOpenGL (which only contains base OpenGL ES2 functions)
//! Using the QOpenGLFunctions_*_* would solve this better, but it conflicts with the
//! current StelOpenGL header dramatically.
struct GLExtFuncs
{
#ifndef QT_OPENGL_ES_2
	//! Since 3.2
	PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;

	//! Old fixed-function functionality that is not in ES2, but some debugging stuff needs it
	QOpenGLFunctions_1_0 glBase;
#endif

	void init(QOpenGLContext* ctx)
	{
		#ifndef QT_OPENGL_ES_2
		glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)ctx->getProcAddress("glFramebufferTexture");

		if(!ctx->isOpenGLES())
			glBase.initializeOpenGLFunctions();
		#endif
	}

};

extern GLExtFuncs glExtFuncs;

#endif
