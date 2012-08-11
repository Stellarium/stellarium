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

#ifndef _STELQGLGLSLSHADER_HPP_
#define _STELQGLGLSLSHADER_HPP_

#include <QGLShaderProgram>
#include <QtOpenGL>

#include "StelGLSLShader.hpp"


//! QGL based StelGLSLShader implementation, used by the QGL2 renderer backend.
class StelQGLGLSLShader : public StelGLSLShader
{
public:
	//! Construct a StelQGLGLSLShader owned by specified renderer.
	//!
	//! Used by QGL2 renderer backend.
	StelQGLGLSLShader(class StelQGL2Renderer* renderer);

	virtual ~StelQGLGLSLShader()
	{
		Q_ASSERT_X(!bound, Q_FUNC_INFO, "Forgot to release() a bound shader before destroying");
	}

	virtual bool addVertexShader(const QString& source)
	{
		Q_ASSERT_X(!built, Q_FUNC_INFO, 
		           "Can't add a vertex shader after the shader has been built");
		return program.addShaderFromSourceCode(QGLShader::Vertex, source);
	}
	
	virtual bool addFragmentShader(const QString& source)
	{
		Q_ASSERT_X(!built, Q_FUNC_INFO, 
		           "Can't add a fragment shader after the shader has been built");
		return program.addShaderFromSourceCode(QGLShader::Fragment, source);
	}

	virtual bool build()
	{
		built = true;
		return program.link();
	}

	virtual QString log() const
	{
		return program.log();
	}

	virtual void bind();

	virtual void release();

	virtual void useUnprojectedPositionAttribute()
	{
		useUnprojectedPosition_ = true;
	}

	//! Get a reference to underlying shader program.
	//!
	//! Used by QGL2 renderer backend.
	QGLShaderProgram& getProgram()
	{
		Q_ASSERT_X(built, Q_FUNC_INFO, "Trying to use a non-built shader for drawing");
		return program;
	}

	//! Does this vertex need the unprojected vertex position attribute?
	//!
	//! Used by vertex buffer backend to determine if this attribute should be provided.
	bool useUnprojectedPosition() const
	{
		return useUnprojectedPosition_;
	}

protected:
	//! Renderer owning this shader.
	class StelQGL2Renderer* renderer;

	//! Shader program.
	QGLShaderProgram program;

	//! Is the shader currently bound?
	//!
	//! true between a bind() and release() call. 
	bool bound;

	//! Has the shader been built?
	bool built;

	//! Does this shader need the "unprojectedVertex" attribute (position before StelProjector
	//! projection) ?
	bool useUnprojectedPosition_;

	virtual void setUniformValue_(const char* const name, const float value)
	{
		Q_ASSERT_X(built, Q_FUNC_INFO, 
		           "Trying to set a uniform value with a non-built shader");
		program.setUniformValue(name, value);
	}

	virtual void setUniformValue_(const char* const name, const Vec2f value)
	{
		Q_ASSERT_X(built, Q_FUNC_INFO, 
		           "Trying to set a uniform value with a non-built shader");
		program.setUniformValue(name, value[0], value[1]);
	}

	virtual void setUniformValue_(const char* const name, const Vec3f& value)
	{
		Q_ASSERT_X(built, Q_FUNC_INFO, 
		           "Trying to set a uniform value with a non-built shader");
		program.setUniformValue(name, value[0], value[1], value[2]);
	}

	virtual void setUniformValue_(const char* const name, const Vec4f& value)
	{
		Q_ASSERT_X(built, Q_FUNC_INFO, 
		           "Trying to set a uniform value with a non-built shader");
		program.setUniformValue(name, value[0], value[1], value[2], value[3]);
	}

	virtual void setUniformValue_(const char* const name, const Mat4f& m)
	{
		Q_ASSERT_X(built, Q_FUNC_INFO, 
		           "Trying to set a uniform value with a non-built shader");
		const QMatrix4x4 transposed
			(m[0], m[4], m[8],  m[12],
			 m[1], m[5], m[9],  m[13],
			 m[2], m[6], m[10], m[14],
			 m[3], m[7], m[11], m[15]);
		program.setUniformValue(name, transposed);
	}
};



#endif // _STELQGLGLSLSHADER_HPP_
