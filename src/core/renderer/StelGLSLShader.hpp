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
//! Note that adding a shader or building can fail. After a failure, the 
//! shader is invalid and should not be used (should be destroyed).
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
	virtual QString log() const = 0;

	//! Bind the shader, using it for following draw calls.
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

protected:

	//! "Default" overload of setUniformValue() implementation.
	//!
	//! This is called for unsupported uniform types, and results in an error.
	template<class T>
	void setUniformValue_(const char* const name, T value)
	{
		//TODO C++11: use static assert here (for a compile-time error)
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unsupported uniform data type");
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
