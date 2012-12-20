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

#include <QGLShader>
#include <QGLShaderProgram>
#include <QMap>
#include <QtOpenGL>

#include "StelGLSLShader.hpp"
#include "StelGLUtilityFunctions.hpp"


//! Used to allow pointer casts to/from 2D float array for matrix uniform storage/upload.
typedef float RAW_GL_MATRIX [4][4];

//! QGL based StelGLSLShader implementation, used by the QGL2 renderer backend.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLGLSLShader : public StelGLSLShader
{
private:
	//! Simple struct that adds an "enabled" flag to a shader.
	struct OptionalShader
	{
		//! Is the shader enabled?
		bool enabled;
		//! Optional shader.
		QGLShader* shader;
	};

	//! Enumerates supported uniform data types.
	enum UniformType
	{
		//! Never used.
		//!
		//! Avoids bugs that might be caused by interpreting zeroed data
		//! valid values, detecting the error.
		UniformType_none  = 0,
		//! 32 bit float. float in both C++ and GLSL.
		UniformType_float = 1,
		//! 2D float vector. Vec2f in Stellarium, vec2 in GLSL.
		UniformType_vec2  = 2,
		//! 3D float vector. Vec3f in Stellarium, vec3 in GLSL.
		UniformType_vec3  = 3,
		//! 4D float vector. Vec4f in Stellarium, vec4 in GLSL.
		UniformType_vec4  = 4,
		//! 4x4 float matrix. Mat4f in Stellarium, mat4 in GLSL.
		UniformType_mat4  = 5,
		//! Boolean. bool in both C++ and GLSL.
		UniformType_bool  = 6,
		//! 32 bit integer. int in both C++ and GLSL.
		UniformType_int   = 7,
		//! Higher than all other values. Used for UNIFORM_SIZES array size.
		UniformType_max   = 8
	};

	//! Storage allocated for uniforms in bytes. Increase this if needed.
	//!
	//! This is enough for 8 Mat4f, or 32 Vec4f.
	static const unsigned int UNIFORM_STORAGE = 512;

	//! Maximum uniform count. Increase this if needed.
	static const int MAX_UNIFORMS = 32;

	//! Sizes of uniform data types, indexed by UniformType.
	//!
	//! Used for iteration of uniforms stored in uniformStorage.
	static int UNIFORM_SIZES[UniformType_max];

public:
	//! Construct a StelQGLGLSLShader owned by specified renderer.
	//!
	//! Used by QGL2 renderer backend.
	//!
	//! @param renderer Renderer that created this shader.
	//! @param internal Is this shader internal to the renderer backend?
	//!                 (Not directly used by the user)
    StelQGLGLSLShader(class StelQGL2Renderer* renderer, bool internal);

	virtual ~StelQGLGLSLShader()
	{
		Q_ASSERT_X(!bound, Q_FUNC_INFO,
		           "Forgot to release() a bound shader before destroying");
		foreach(QGLShader* shader, defaultVertexShaders)  {delete shader;}
		foreach(OptionalShader shader, namedVertexShaders){delete shader.shader;}
		foreach(QGLShader* shader, defaultFragmentShaders){delete shader;}
		foreach(QGLShaderProgram* program, programCache)  {delete program;}
	}

	virtual bool addVertexShader(const QString& source);
	
	virtual bool addFragmentShader(const QString& source);

	virtual bool build();

	virtual QString log() const
	{
		return aggregatedLog + ((NULL == program) ? QString() : program->log());
	}

	virtual void unlock()
	{
		switch(state)
		{
			case State_Unlocked:
			case State_Modified:
				return;
			case State_Built:
				state = State_Unlocked;
				return;
		}
	}

	virtual bool addVertexShader(const QString& name, const QString& source);

	virtual bool hasVertexShader(const QString& name) const
	{
		return namedVertexShaders.contains(name);
	}

	virtual void enableVertexShader(const QString& name)
	{
		Q_ASSERT_X(namedVertexShaders.contains(name), Q_FUNC_INFO, 
		           "Trying to enable a vertex shader with an unknown name");
		Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO, 
		           "Can't enable a vertex shader after the shader has been built");

		OptionalShader& shader(namedVertexShaders[name]);
		if(shader.enabled) {return;}
		shader.enabled = true;
		state = State_Modified;
	}

	virtual void disableVertexShader(const QString& name)
	{
		Q_ASSERT_X(namedVertexShaders.contains(name), Q_FUNC_INFO, 
		           "Trying to disable a vertex shader with an unknown name");
		Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO, 
		           "Can't disable a vertex shader after the shader has been built");

		OptionalShader& shader(namedVertexShaders[name]);
		if(!shader.enabled) {return;}
		shader.enabled = false;
		state = State_Modified;
	}

	virtual void bind();

	virtual void release();

	virtual void useUnprojectedPositionAttribute()
	{
		useUnprojectedPosition_ = true;
	}

	//! Get a reference to the current underlying shader program.
	//!
	//! Used by QGL2 renderer backend.
	QGLShaderProgram& getProgram()
	{
		Q_ASSERT_X(bound && state == State_Built,
		           Q_FUNC_INFO, "Trying to use an unbound shader for drawing");
		return *program;
	}

	//! Does this shader need the unprojected vertex position attribute?
	//!
	//! Used by vertex buffer backend to determine if this attribute should be provided.
	bool useUnprojectedPosition() const
	{
		return useUnprojectedPosition_;
	}

	//! Upload the stored uniform variables.
	//!
	//! Called when the internally used shader program has been bound, before drawing,
	//! by the drawVertexBufferBackend() member function of the renderer backend.
	void uploadUniforms()
	{
		Q_ASSERT_X(bound && state == State_Built, Q_FUNC_INFO, 
		           "uploadUniforms called on a shader that is not bound");
        const unsigned char* data = uniformStorage;
		for (int u = 0; u < uniformCount; u++) 
		{
            const UniformType type = static_cast<UniformType>(uniformTypes[u]);
			switch(type)
			{
				case UniformType_float:
                {
					const float& value(*reinterpret_cast<const float* const>(data));
					program->setUniformValue(uniformNames[u], value);
					break;
				}
				case UniformType_vec2:
                {
					const Vec2f& v(*reinterpret_cast<const Vec2f* const>(data));
					program->setUniformValue(uniformNames[u], v[0], v[1]);
					break;
				}
				case UniformType_vec3:
                {
#ifndef ANDROID
                    const Vec3f& v(*reinterpret_cast<const Vec3f* const>(data));
#else
                    //Why do I need to do this? :|
                    //TODO: look at the assembly and find out why assignments are failing
                    Vec3f v;
                    memcpy(&v,reinterpret_cast<const Vec3f* const>(data), sizeof(Vec3f));
#endif
                    program->setUniformValue(uniformNames[u], v[0], v[1], v[2]);
					break;
				}
				case UniformType_vec4:
                {
					const Vec4f& v(*reinterpret_cast<const Vec4f* const>(data));
					program->setUniformValue(uniformNames[u], v[0], v[1], v[2], v[3]);
					break;
				}
				case UniformType_mat4:
                {
					const RAW_GL_MATRIX& m(*reinterpret_cast<const RAW_GL_MATRIX* const>(data));
					program->setUniformValue(uniformNames[u], m);
					break;
				}
				case UniformType_bool:
                {
					const bool& value(*reinterpret_cast<const bool* const>(data));
					program->setUniformValue(uniformNames[u], value);
                    break;
				}
				case UniformType_int:
                {
					const int& value(*reinterpret_cast<const int* const>(data));
					program->setUniformValue(uniformNames[u], value);
					break;
				}
				default:
					Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown or invalid uniform type");
			}
			data += UNIFORM_SIZES[type];
		}
	}

	//! Push uniform storage state.
	//!
	//! This is an optimization used internally by the renderer backend.
	//! This "saves" the state of uniform storage, allowing to "restore" it 
	//! by calling popUniformStorage(). This prevents the internal uniform-setting 
	//! in repeated draw calls from filling the entire uniform storage.
	//!
	//! How this works:
	//!
	//! Setting uniforms just appends new data to uniform storage; setting the 
	//! same uniform multiple times just uses more data (determining 
	//! if a uniform with this name was set already would be expensive).
	//!
	//! pushUniformStorage() pushes the number of uniforms and used storage to 
	//! an internal stack.
	//! popUniformStorage() restores this state, throwing away any uniforms set since 
	//! the push.
	void pushUniformStorage()
	{
		Q_ASSERT_X(bound && state == State_Built, Q_FUNC_INFO,
		           "pushUniformStorage() called when the shader is not bound");
		Q_ASSERT_X(uniformStorageStackSize < MAX_UNIFORMS - 1, Q_FUNC_INFO,
		           "Too many uniform storage stack pushes");
		uniformStorageUsedStack[uniformStorageStackSize] = uniformStorageUsed;
		uniformCountStack[uniformStorageStackSize]       = uniformCount;
		++uniformStorageStackSize;
	}

	//! Restores pushed uniform storage stage.
	//!
	//! @see pushUniformStorage
	void popUniformStorage()
	{
		Q_ASSERT_X(bound && state == State_Built, Q_FUNC_INFO,
		           "popUniformStorage() called when the shader is not bound");
		Q_ASSERT_X(uniformStorageStackSize >= 1, Q_FUNC_INFO,
		           "Too many uniform storage stack pops (nothing left to pop)");
		--uniformStorageStackSize;
		uniformStorageUsed    = uniformStorageUsedStack[uniformStorageStackSize];
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
		uniformCount       = uniformCountStack[uniformStorageStackSize];
	}

	//! Clear all stored uniforms, freeing uniform storage.
	//!
	//! Called by release().
	void clearUniforms()
	{
		Q_ASSERT_X(bound && state == State_Built, Q_FUNC_INFO,
		           "clearUniforms() called when the shader is not bound");
		// We don't zero out the storage - we'll overwrite it as we add new uniforms.
		uniformStorageUsed = uniformCount = uniformStorageStackSize = 0;
		uniformStoragePointer = uniformStorage;
	}

protected:
    //! Possible states the shader can be found in.
    enum State
    {
        //! The shader program can be modified by adding/enabling/disabling vertex/fragment shaders.
        //!
        //! Adding/enabling/disabling a shader will result in the Modified state.
        //! A successful call to build() will result in the Built state.
        State_Unlocked,
        //! The shader has been modified since the last call to build(),
        //! which means the next build() call might need to re-link or use a previously
        //! linked shader program from cache.
        //! A successful call to build() will result in the Built state.
        State_Modified,
        //! The shader has been succesfully built and can't be modified unless unlock() is
        //! called, but can be bound. A call to unlock() results in Unlocked state.
        State_Built,
    };

	//! Renderer that created this shader.
	class StelQGL2Renderer* renderer;

	//! Vertex shaders that are always linked in (added by the nameless addVertexShader() overload).
	QVector<QGLShader*> defaultVertexShaders;

	//! Optional vertex shaders that may be enabled or disabled.
	QMap<QString, OptionalShader> namedVertexShaders;

	// There are no namedFragmentShaders, but they can be added if/when needed.
	//! Fragment shaders that are always linked in (added by the nameless addFragmentShader() overload).
	QVector<QGLShader*> defaultFragmentShaders;

	//! All shader programs linked so far.
	//!
	//! When build() is called, the default and currently enabled optional shaders 
	//! are linked to a shader program. As re-linking shaders on each frame would 
	//! be too expensive, any shader combination is only linked once and then retrieved 
	//! from this cache.
	//!
	//! build() is called for each draw call (due to modular shaders being used for vertex 
	//! projection), so the lookup must be very fast.
	//! The ID used for lookup - a 64 bit unsigned integer, is a sum of
	//! pointers of all shaders linked in the program.
	//!
	//! It is not impossible for false positives to happen, but it's very unlikely. 
	//! 64bit is not going to overflow any soon (you need a 16EiB address space for that),
	//! and as shaders are never deleted or moved in memory, two shaders can never have
	//! identical pointers. Then only risk is that two sets of pointers will accidentally
	//! add up to the same number, but this is very unlikely as well.
	//!
	//! In case this happens, some very simple hash algorithm (still on pointers) might 
	//! be used instead.
	QMap<uintptr_t, QGLShaderProgram*> programCache;

	//! Currently used shader program (linked from default and currently enabled shaders).
	QGLShaderProgram* program;

	//! Log aggregated during all addXXXShader() and build() calls.
	//!
	//! May be aggregated from multiple vertex programs if built multiple times with 
	//! different shaders enabled.
	QString aggregatedLog;

	//! Current state of the shader.
	State state;

	//! Is the shader bound for drawing?
	bool bound;

	//! Does this shader need the <em>unprojectedVertex</em> attribute (position before StelProjector
	//! projection) ?
	bool useUnprojectedPosition_;

	//! Is this an internal shader used by the renderer backend?
	//!
	//! Needed to avoing bind/release from calling renderer backend custom shader bind/release.
	const bool internal;

	//! Storage used for uniform data.
	//!
	//! As we're linking shaders dynamically and the final shader program is only
	//! bound directly before drawing, we need to delay uniform upload until that 
	//! point. Therefore, setUniformValue_ functions just add data to this storage.
	//! The data is then uploaded by uploadUniforms().
	//!
	//! @see uploadUniforms, clearUniforms, pushUniformStorage
	unsigned char uniformStorage [UNIFORM_STORAGE];

	//! Pointer to uniformStorage[uniformStorageUsed]. Avoids breaking strict aliasing.
	void* uniformStoragePointer;

	//! Types of consecutive uniforms in uniformStorage.
	//!
	//! Needed to interpret uniformStorage data when uploading.
	unsigned char uniformTypes [MAX_UNIFORMS];   

	//! Names of consecutive uniforms in uniformStorage.
	//!
	//! We don't own these strings, we just have pointers to them.
	//! This means the user code needs to preserve the string until release() is called.
	const char* uniformNames [MAX_UNIFORMS];

	//! Bytes in uniformStorage used up at this moment.
	int uniformStorageUsed;

	//! Number of uniforms in uniformStorage at this moment.
	int uniformCount;

	//! Stack of uniformStorageUsed values used with pushUniformStorage/popUniformStorage.
	//!
	//! @see pushUniformStorage, popUniformStorage
	unsigned int uniformStorageUsedStack[MAX_UNIFORMS];

	//! Stack of uniformCount values used with pushUniformStorage/popUniformStorage.
	//!
	//! @see pushUniformStorage, popUniformStorage
	unsigned char uniformCountStack[MAX_UNIFORMS];

	//! Number of items in uniformStorageUsedStack and uniformCountStack.
	int uniformStorageStackSize;

	virtual void setUniformValue_(const char* const name, const float value)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
		Q_ASSERT_X((uniformStorageUsed + sizeof(float)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
        memcpy(uniformStoragePointer, &value, sizeof(float));
		uniformNames[uniformCount] = name;
		uniformTypes[uniformCount++] = UniformType_float;
		uniformStorageUsed += sizeof(float);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

	virtual void setUniformValue_(const char* const name, const Vec2f value)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
		Q_ASSERT_X((uniformStorageUsed + sizeof(Vec2f)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
        memcpy(uniformStoragePointer, &value, sizeof(Vec2f));
		uniformNames[uniformCount] = name;
		uniformTypes[uniformCount] = UniformType_vec2;
		++uniformCount;
		uniformStorageUsed += sizeof(Vec2f);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

	virtual void setUniformValue_(const char* const name, const Vec3f& value)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
		Q_ASSERT_X((uniformStorageUsed + sizeof(Vec3f)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
        memcpy(uniformStoragePointer, &value, sizeof(Vec3f));
		uniformNames[uniformCount] = name;
		uniformTypes[uniformCount] = UniformType_vec3;
		++uniformCount;
		uniformStorageUsed += sizeof(Vec3f);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

	virtual void setUniformValue_(const char* const name, const Vec4f& value)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
        memcpy(uniformStoragePointer, &value, sizeof(Vec4f));
		Q_ASSERT_X((uniformStorageUsed + sizeof(Vec4f)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
		uniformNames[uniformCount] = name;
		uniformTypes[uniformCount] = UniformType_vec4;
		++uniformCount;
		uniformStorageUsed += sizeof(Vec4f);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

	virtual void setUniformValue_(const char* const name, const Mat4f& m)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
		Q_ASSERT_X((uniformStorageUsed + sizeof(Mat4f)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
        memcpy(uniformStoragePointer, &m, sizeof(Mat4f));
        uniformNames[uniformCount] = name;
        uniformTypes[uniformCount] = UniformType_mat4;
        ++uniformCount;
        uniformStorageUsed += sizeof(Mat4f);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

	virtual void setUniformValue_(const char* const name, const bool value)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
		Q_ASSERT_X((uniformStorageUsed + sizeof(bool)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
		*static_cast<bool*>(uniformStoragePointer) = value;
		uniformNames[uniformCount] = name;
		uniformTypes[uniformCount] = UniformType_bool;
		++uniformCount;
		uniformStorageUsed += sizeof(bool);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

	virtual void setUniformValue_(const char* const name, const int value)
	{
		Q_ASSERT_X(bound, Q_FUNC_INFO,
		           "Trying to set a uniform value with an unbound shader");
		Q_ASSERT_X(uniformCount < MAX_UNIFORMS, Q_FUNC_INFO, "Too many uniforms");
		Q_ASSERT_X((uniformStorageUsed + sizeof(int)) < UNIFORM_STORAGE, Q_FUNC_INFO,
		           "Uniform storage exceeded");
		*static_cast<int*>(uniformStoragePointer) = value;
		uniformNames[uniformCount] = name;
		uniformTypes[uniformCount] = UniformType_int;
		++uniformCount;
		uniformStorageUsed += sizeof(int);
		uniformStoragePointer = &(uniformStorage[uniformStorageUsed]);
	}

private:
	//! Get the vertex program matching currently enabled shaders from cache.
	//!
	//! If there is no matching program, return NULL.
	//!
	//! @see programCache
	QGLShaderProgram* getProgramFromCache();

	//! Code shared between addVertexShader overloads.
	//!
	//! Tries to create and compile a vertex shader from source.
	//!
	//! @return compiled shader on success, NULL on failure.
	QGLShader* createVertexShader(const QString& source);
};

#endif // _STELQGLGLSLSHADER_HPP_
