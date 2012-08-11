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

#ifndef _STELGLSLSHADER_HPP_
#define _STELGLSLSHADER_HPP_

#include "VecMath.hpp"

//! GLSL shader program. 
//!
//! Can be used with Renderer backends that support GLSL, such 
//! as the StelQGL2Renderer.
//!
//! A shader program can be created by adding a vertex and fragment shader 
//! from source and then calling the build() member function.
//!
//! There is currently no geometry shader support, but it can be added when needed.
//!
//! Uniform variables are set through the setUniformValue() member functions.
//! Only most commonly used data types are supported, but it is easy to add 
//! support for more types if needed. 
//! Some uniform variables are specified internally and need to be declared 
//! in the shader:
//!
//! @li StelProjector's projection matrix: mat4f projectionMatrix
//! @li Global color (only specified if vertex has no color attribute): vec4 globalColor
//!
//!
//! Attribute variables can't be set through the API - they are set internally 
//! from vertex buffer during drawing.
//!
//! Attribute variable names match vertex attribute interpretations as follows:
//!
//! @li Position: vertex
//! @li TexCoord: texCoord
//! @li Normal:   normal
//! @li Color:    color 
//!
//! Furthermore, is useUnprojectedPositionAttribute() is called, another attribute is added:
//! 
//! @li Position before StelProjector projection: unprojectedVertex
//!
//! Note that adding a shader or building can fail. After a failure, the 
//! shader is invalid and should not be used (should be destroyed).
//!
//!
//! Shader creation example:
//!
//! @code 
//! // renderer is a StelRenderer*
//! if(renderer->isGLSLSupported())
//! {
//! 	StelGLSLShader* shader = renderer->createGLSLShader();
//! 	if(!shader->addVertexShader(
//! 	   "attribute mediump vec4 vertex;\n"
//! 	   "uniform mediump mat4 projectionMatrix;\n"
//! 	   "void main(void)\n"
//! 	   "{\n"
//! 	   "    gl_Position = projectionMatrix * vertex;\n"
//! 	   "}\n"))
//! 	{
//! 		qWarning() << "Error adding vertex shader: " << shader->log();
//! 		delete shader;
//! 		return false; // Failed creating shader
//! 	}
//! 	if(!shader->addFragmentShader(
//! 	   "uniform mediump vec4 globalColor;\n"
//! 	   "void main(void)\n"
//! 	   "{\n"
//! 	   "    gl_FragColor = globalColor;\n"
//! 	   "}\n"))
//! 	{
//! 		qWarning() << "Error adding fragment shader: " << shader->log();
//! 		delete shader;
//! 		return false; // Failed creating shader
//! 	}
//! 	if(!shader->build())
//! 	{
//! 		qWarning() << "Error building shader: " << shader->log();
//! 		delete shader;
//! 		return false; // failed creating shader
//! 	}
//! 	// Shader log might contain warnings.
//! 	if(!shader->log().isEmpty())
//! 	{
//! 		qWarning() << "Shader creation log: " << shader->log();
//! 	}
//! }
//! @endcode
//!
//! Shader usage example:
//!
//! @code
//! // shader is a StelGLSLShader*
//! // renderer is a StelRenderer*
//! // vertices is a StelVertexBuffer<SomeVertexType>*
//! // indices is a StelIndexBuffer*
//! // projector is a StelProjectorP
//!
//! shader->bind();
//! shader->setUniformValue("ambientLight", Vec4f(0.1f, 0.1f, 0.1f,0.1f));
//! renderer->drawVertexBuffer(vertices, indices, projector);
//! shader->release();
//! @endcode
//!
class StelGLSLShader
{
public:
	//! Destroy the shader program.
	//!
	//! StelGLSLShader must be destroyed by the user before its Renderer is destroyed.
	virtual ~StelGLSLShader(){};

	//! Add a vertex shader from source, compiling it in the process.
	//!
	//! This operation can fail. In case of failure, use log() to find out the cause.
	//!
	//! @param source GLSL source code of the fragment shader.
	//!
	//! @return true if succesfully added and compiled, false otherwise.
	virtual bool addVertexShader(const QString& source) = 0;
	
	//! Add a fragment shader from source, compiling it in the process.
	//!
	//! This operation can fail. In case of failure, use log() to find out the cause.
	//!
	//! @param source GLSL source code of the fragment shader.
	//!
	//! @return true if succesfully added and compiled, false otherwise.
	virtual bool addFragmentShader(const QString& source) = 0;

	//! Build the shader program.
	//!
	//! This operation can fail. In case of failure, use log() to find out the cause.
	//!
	//! @return true if succesfully built, false otherwise.
	virtual bool build() = 0;

	//! Return a string containing the error log of the shader.
	//!
	//! If the shader is succesfully built, it will contain a success message.
	//! It might also contain GLSL code warnings.
	virtual QString log() const = 0;

	//! Bind the shader, using it for following draw calls.
	//!
	//! This must be called before setting any uniforms.
	//!
	//! Note that the shader must be released after any draw calls using the shader
	//! to allow any builtin default shaders to return to use.
	virtual void bind() = 0;

	//! Release a bound shader to stop using it.
	//!
	//! This must be called when done using the shader 
	//! so that a shader based Renderer backend can return to using default shaders.
	//!
	//! It also must be called before bind()-ing another StelGLSLShader.
	//! (This is forced so that code binding a StelGLSLShader will continue work 
	//! correctly - not overriding default shaders - even after any following
	//! StelGLSLShader bind is removed.)
	virtual void release() = 0;

	//! Set value of a uniform shader variable.
	//!
	//! A uniform variable has the same value for each drawn vertex/pixel.
	//! This can be used for things such as a transformation matrix,
	//! global vertex color and so on.
	//!
	//! setUniformValue() supports these data types (GLSL type in parentheses):
	//!
	//! float(float), Vec2f(vec2), Vec3f(vec3), Vec4f(vec4), Mat4f(mat4)
	//!
	//! @param name  Name of the uniform variable. Must match variable name in a shader source.
	//! @param value Value to set the variable to.
	template<class T>
	void setUniformValue(const char* const name, T value)
	{
		// Calls a virtual function for every type (since we can't have virtual templates).
		// Unsupported types are asserted out at runtime 
		// (with C++11, static assert should be used instead)
		setUniformValue_(name, value);
	}

	//! Does this shader need the unprojected position attribute?
	//!
	//! If called, the shader will have to declare another vertex attribute, 
	//! unprojectedVertex, for vertex position before StelProjector projection.
	//! Useful e.g. for lighting.
	virtual void useUnprojectedPositionAttribute() = 0;

protected:

	//! "Default" overload of setUniformValue() implementation.
	//!
	//! This is called for unsupported uniform types, and results in an error.
	template<class T>
	void setUniformValue_(const char* const name, T value)
	{
		Q_UNUSED(name);
		Q_UNUSED(value);
		//TODO C++11: use static assert here (for a compile-time error)
		Q_ASSERT_X(false, Q_FUNC_INFO,
		           "Unsupported uniform data type. "
		           "Supported types: float, Vec2f, Vec3f, Vec4f, Mat4f");
	}

	//! setUniformValue() implementation for type float.
	virtual void setUniformValue_(const char* const name, const float value) = 0;

	//! setUniformValue() implementation for type Vec2f.
	virtual void setUniformValue_(const char* const name, const Vec2f value) = 0;

	//! setUniformValue() implementation for type Vec3f.
	virtual void setUniformValue_(const char* const name, const Vec3f& value) = 0;

	//! setUniformValue() implementation for type Vec4f.
	virtual void setUniformValue_(const char* const name, const Vec4f& value) = 0;

	//! setUniformValue() implementation for type Mat4f.
	virtual void setUniformValue_(const char* const name, const Mat4f& value) = 0;
};

#endif // _STELGLSLSHADER_HPP_
