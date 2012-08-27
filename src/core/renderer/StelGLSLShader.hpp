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
//! Shader programs are created by StelRenderer. After creation, sources of used 
//! vertex and fragment shaders have to be added, and then the shader must be built 
//! using the build() member function. After that, the shader can be bound to be used 
//! for drawing. It can also be modified by calling the unlock() member function and
//! adding more shaders, and even by disabling or reenabling them. Disabling/reenabling
//! is only supported for vertex shaders, but fragment shader functions can be 
//! added when needed.
//!
//! There is currently no geometry shader support, but it can be added when needed.
//!
//! Uniform variables are set through the setUniformValue() member function.
//! Only most commonly used data types are supported, but it is easy to add 
//! support for more types if needed. 
//! Some uniform variables are specified internally and need to be declared 
//! in the shader:
//!
//! @li StelProjector's projection matrix: <em>mat4f projectionMatrix</em>
//! @li Global color (only specified if vertex has no color attribute): <em>vec4 globalColor</em>
//!
//!
//! Vertex attributes can't be set through the API - they are set internally 
//! from vertex buffer during drawing.
//!
//! Any shader that uses StelProjector projection (that is, any shader except 
//! of those working with 2D data) must declare and call the <em>project()</em> function
//! to project the vertex position. If vertex projection is done on the CPU, this is 
//! a dummy function that simply returns its argument. If it's done on the GPU, it's
//! done in this function.
//!
//! The <em>project()</em> function must be declared (but not defined), as follows:
//! @code
//! vec4 project(in vec4 v);
//! @endcode
//!
//! Usage of the <em>project()</em> function:
//! @code
//! gl_Position = projectionMatrix * project(vertex);
//! @endcode
//!
//! Attribute variable names match vertex attribute interpretations as follows:
//!
//! @li Position: <em>vertex</em>
//! @li TexCoord: <em>texCoord</em>
//! @li Normal:   <em>normal</em>
//! @li Color:    <em>color</em> 
//!
//! If useUnprojectedPositionAttribute() is called, another attribute is added:
//! 
//! @li Position before StelProjector projection: <em>unprojectedVertex</em>
//!
//! Shader program building can fail. After a failure, the 
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
//! 	   "vec4 project(in vec4 v);\n"
//! 	   "attribute mediump vec4 vertex;\n"
//! 	   "uniform mediump mat4 projectionMatrix;\n"
//! 	   "void main(void)\n"
//! 	   "{\n"
//! 	   "    gl_Position = projectionMatrix * project(vertex);\n"
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
//! @see StelRenderer
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
	//! The shader must be unlocked when this is called.
	//!
	//! @param source Source code of the shader.
	//! @return true if succesfully added and compiled, false otherwise.
	virtual bool addVertexShader(const QString& source) = 0;
	
	//! Add a fragment shader from source, compiling it in the process.
	//!
	//! This operation can fail. In case of failure, use log() to find out the cause.
	//!
	//! The shader must be unlocked when this is called.
	//!
	//! @param source Source code of the shader.
	//! @return true if succesfully added and compiled, false otherwise.
	virtual bool addFragmentShader(const QString& source) = 0;

	//! Add a named (optional) vertex shader.
	//!
	//! Named vertex shaders can be disabled and reenabled.
	//! (Unnamed shaders are always enabled.)
	//! This allows to dynamically exchange implementations of functions, 
	//! making shaders modular. GLSL projection is an example of this.
	//!
	//! The shader must be unlocked when this is called.
	//!
	//! @param name Name of the shader.
	//! @param source Source code of the shader.
	virtual bool addVertexShader(const QString& name, const QString& source) = 0;

	//! Has a named vertex shader with specified name been added?
	virtual bool hasVertexShader(const QString& name) const = 0;

	//! Enable previously added named vertex shader.
	//!
	//! The shader must be unlocked when this is called.
	//! 
	//! @param name Name of the shader to enable.
	virtual void enableVertexShader(const QString& name) = 0;

	//! Disable previously added named vertex shader.
	//!
	//! The shader must be unlocked when this is called.
	//! 
	//! @param name Name of the shader to disable.
	virtual void disableVertexShader(const QString& name) = 0;

	//! Build the shader program.
	//!
	//! This must be called before the shader can be bound.
	//!
	//! This operation can fail. In case of failure, use log() to find out the cause.
	//! If build() fails, the shader should be assumed to no longer be usable 
	//! and be destroyed.
	//!
	//! @return true if succesfully built, false otherwise.
	virtual bool build() = 0;

	//! Unlock the shader program for modifications.
	//!
	//! Allows to, add, enable, disable vertex/fragment shaders.
	//!
	//! This can be called even if the shader is bound (which is used 
	//! for last-moment modifications, like GLSL projection in renderer backend),
	//! but it must be rebuilt before drawing and releasing.
	virtual void unlock() = 0;

	//! Return a string containing the error log of the shader.
	//!
	//! If the shader is succesfully built, it will contain a success message.
	//! It might also contain GLSL code warnings.
	virtual QString log() const = 0;

	//! Bind the shader, using it for following draw calls.
	//!
	//! This must be called before setting uniforms.
	//! The shader must be built when this is called.
	//!
	//! Note that the shader must be released after any draw calls using the shader
	//! to allow default shaders to return to use.
	virtual void bind() = 0;

	//! Release a bound shader after use.
	//!
	//! This must be called after using the shader 
	//! so that the StelRenderer backend can go back to using default shaders.
	//!
	//! It also must be called before bind()-ing another StelGLSLShader.
	virtual void release() = 0;

	//! Set value of a uniform shader variable.
	//!
	//! A uniform variable has the same value for each drawn vertex/pixel.
	//! This can be used for things such as a transformation matrix,
	//! global vertex color and so on.
	//!
	//! setUniformValue() supports these data types (GLSL type in parentheses):
	//!
	//! bool(bool), int(int), float(float), Vec2f(vec2), Vec3f(vec3), Vec4f(vec4), Mat4f(mat4)
	//!
	//! The shader must be bound when this is called.
	//!
	//! @note Due to dynamic shader re-linking needed to support modular shaders,
	//! uniforms are cached internally and only uploaded at the draw call when 
	//! the shader is used. Fixed amount of storage is allocated (currently 512 
	//! bytes, which is enough for 8 4x4 matrices). Also only a fixed number of 
	//! uniforms is supported (currently 32). Both of these limits can be increased 
	//! if needed (see StelQGLGLSLShader). 
	//! This storage is written to with each setUniformValue() call, and freed on a
	//! call to release().
	//!
	//! @param name  Name of the uniform variable. Must match variable name in at least one 
	//!              of the used vertex/fragment shaders.
	//!              For efficiency reasons, the name is not guaranteed to be copied 
	//!              within the shader backend. Therefore, the name string must exist until a 
	//!              call to release().
	//!              (This is easiest to achieve by simply using string literals.)
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
	//! <em>unprojectedVertex</em>, for vertex position before StelProjector projection.
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
		           "Supported types: bool, int, float, Vec2f, Vec3f, Vec4f, Mat4f");
	}

	//! setUniformValue() implementation for type bool.
	virtual void setUniformValue_(const char* const name, const bool value) = 0;

	//! setUniformValue() implementation for type int.
	virtual void setUniformValue_(const char* const name, const int value) = 0;

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
