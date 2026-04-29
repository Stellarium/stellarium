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
#include "StelMainView.hpp"
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
# include <QOpenGLVersionFunctionsFactory>
#endif

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



QByteArray StelOpenGL::globalShaderPrefix(const ShaderType type)
{
	const auto& glInfo = StelMainView::getInstance().getGLInformation();

	QByteArray prefix;

	if(glInfo.isGLES)
		prefix = 
			"#version 300 es\n"
			"precision mediump float;\n";
	else
		prefix =
			"#version 330\n";

	if(glInfo.isHighGraphicsMode)
		prefix +=
			"#define texture2D(a,b) texture(a,b)\n"
			"#define texture2D_3(a,b,c) texture(a,b,c)\n"
			"#define texture2DProj(a,b) textureProj(a,b)\n"
			"#define texture2DProj_3(a,b,c) textureProj(a,b,c)\n"
			"#define shadow2D_x(a,b) texture(a,b)\n"
			"#define shadow2DProj_x(a,b) textureProj(a,b)\n"
			"#define textureCube(a,b) texture(a,b)\n"
			"#define textureCube_3(a,b,c) texture(a,b,c)\n";
	else
		prefix +=
			"#define texture2D_3(a,b,c) texture2D(a,b,c)\n"
			"#define texture2DProj_3(a,b,c) texture2DProj(a,b,c)\n"
			"#define shadow2D_x(a,b) shadow2D(a,b).x\n"
			"#define shadow2DProj_x(a,b) shadow2DProj(a,b).x\n"
			"#define textureCube_3(a,b,c) textureCube(a,b,c)\n";

	switch(type){
	case VERTEX_SHADER:
		if(glInfo.isHighGraphicsMode)
			prefix +=
				"#define ATTRIBUTE in\n"
				"#define VARYING out\n";
		else
			prefix +=
				"#define ATTRIBUTE attribute\n"
				"#define VARYING varying\n";
		break;
	
	case FRAGMENT_SHADER:
		if(glInfo.isHighGraphicsMode)
			prefix +=
				"#define VARYING in\n"
				"#define textureGrad_SUPPORTED\n"
				"out highp vec4 FRAG_COLOR;\n";
		else
			prefix +=
				"#define VARYING varying\n"
				"#define FRAG_COLOR gl_FragColor\n";
		break;
		
	case GEOMETRY_SHADER:
		break;
	}
	
	prefix += "#line 1\n";

	return prefix;
}


StelOpenGL::Functions* StelOpenGL::highGraphicsFunctions()
{
#if !QT_CONFIG(opengles2)
# if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	return QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(QOpenGLContext::currentContext());
# else
	return QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
# endif
#else
	return QOpenGLContext::currentContext()->extraFunctions();
#endif
}
