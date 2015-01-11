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

#ifndef _SHADERMANAGER_HPP_
#define _SHADERMANAGER_HPP_
#include "OBJ.hpp"
#include "StelOpenGL.hpp"

#include <QMap>

class QOpenGLShaderProgram;

//! A structure for global shader parameters
struct GlobalShaderParameters
{
	bool shadowTransform;
	bool pixelLighting;
	bool bump;
	bool shadows;
	bool shadowFilter;
	bool shadowFilterHQ;
	bool geometryShader;
};

//! A simple shader cache class that gives us the correct shader depending on desired configuration.
//! It also contains a uniform cache to avoid unnecessary glUniformLocation calls
class ShaderMgr
{
public:
	ShaderMgr();
	~ShaderMgr();

	//! Enum for OpenGL shader attribute locations
	enum ATTLOC
	{
		//! This is the OpenGL attribute location where vertex positions are mapped to
		ATTLOC_VERTEX,
		//! This is the OpenGL attribute location where vertex normals are mapped to
		ATTLOC_NORMAL,
		//! This is the OpenGL attribute location where vertex texture coordinates are mapped to
		ATTLOC_TEXCOORD,
		//! This is the OpenGL attribute location where vertex tangents are mapped to
		ATTLOC_TANGENT,
		//! This is the OpenGL attribute location where vertex bitangents are mapped to
		ATTLOC_BITANGENT
	};

	//! Enum for OpenGL shader uniform locations (faster than accessing by string each time)
	enum UNIFORM
	{
		//! Defines the ModelView matrix
		UNIFORM_MAT_MODELVIEW,
		//! Defines the Projection matrix
		UNIFORM_MAT_PROJECTION,
		//! Defines the combined ModelViewProjection matrix, used if shader needs no separation of the 2 for a bit of a speedup
		UNIFORM_MAT_MVP,
		//! Defines the 3x3 Normal matrix (transpose of the inverse MV matrix)
		UNIFORM_MAT_NORMAL,
		//! The first shadow matrix
		UNIFORM_MAT_SHADOW0,
		UNIFORM_MAT_SHADOW1,
		UNIFORM_MAT_SHADOW2,
		UNIFORM_MAT_SHADOW3,
		//! The first cube MVP (array mat4, total 6)
		UNIFORM_MAT_CUBEMVP,

		//! Defines the Diffuse texture slot
		UNIFORM_TEX_DIFFUSE,
		//! Defines the Bump texture slot
		UNIFORM_TEX_BUMP,
		//! Defines the Height texture slot
		UNIFORM_TEX_HEIGHT,
		//! First shadow map
		UNIFORM_TEX_SHADOW0,
		UNIFORM_TEX_SHADOW1,
		UNIFORM_TEX_SHADOW2,
		UNIFORM_TEX_SHADOW3,

		//! Material ambient color
		UNIFORM_MTL_AMBIENT,
		//! Material diffuse color
		UNIFORM_MTL_DIFFUSE,
		//! Material specular color
		UNIFORM_MTL_SPECULAR,
		//! Material specular shininess (exponent)
		UNIFORM_MTL_SHININESS,
		//! Material global transparency
		UNIFORM_MTL_ALPHA,

		//! Light direction vector (world space)
		UNIFORM_LIGHT_DIRECTION,
		//! Light direction vector (view space)
		UNIFORM_LIGHT_DIRECTION_VIEW,
		//! Light ambient intensity
		UNIFORM_LIGHT_AMBIENT,
		//! Light diffuse intensity
		UNIFORM_LIGHT_DIFFUSE,

		//! Squared frustum splits (vec4)
		UNIFORM_VEC_SQUAREDSPLITS,
		//! Alpha test threshold
		UNIFORM_FLOAT_ALPHA_THRESH,
	};



	//! Returns a shader that supports the specified operations. Must be called within a GL context.
	inline QOpenGLShaderProgram* getShader(const GlobalShaderParameters &globals, const OBJ::Material *mat = NULL);

	//! Returns the cubemapping shader
	inline QOpenGLShaderProgram* getCubeShader();

	//! Returns the basic texturing shader
	inline QOpenGLShaderProgram* getTextureShader();

	//! Returns the location of this uniform for this shader, or -1 if this uniform does not exist.
	//! This is cached to elimate the overhead of the glGet calls
	inline GLint uniformLocation(const QOpenGLShaderProgram* shad,UNIFORM uni);

	//! Clears the shaders that have been created by this manager. Must be called within a GL context.
	void clearCache();

private:
	typedef QMap<QString,UNIFORM> t_UniformStrings;
	static t_UniformStrings uniformStrings;

	//A Bitflag enum which contains features that shaders can implement, and which this manager can dynamically select
	enum FeatureFlags
	{
		INVALID		= 0,
		//Transform-only shader (all flags off) (use for depth-only render), should be mutually exclusive with SHADING
		TRANSFORM	= (1<<0),
		//The shader has some sort of light-dependant color output,  should be mutually exclusive with TRANSFORM
		SHADING		= (1<<1),
		//Per-pixel lighting
		PIXEL_LIGHTING  = (1<<2),
		//Shader applies shadows from shadow maps
		SHADOWS         = (1<<3),
		//Shader applies bump/normal maps
		BUMP            = (1<<4),
		//Shader applies height maps (in addition to bump map)
		HEIGHT          = (1<<5),
		//Shader applies alpha testing (w. fragment discard)
		ALPHATEST	= (1<<6),
		//Shader filters shadows
		SHADOW_FILTER	= (1<<7),
		//shader filters shadows (higher quality)
		SHADOW_FILTER_HQ = (1<<8),
		//uses ambient material
		MAT_AMBIENT	= (1<<9),
		//uses specular material
		MAT_SPECULAR	= (1<<10),
		//has diffuse texture. On its own (i.e. esp. without SHADING) it defines the basic texture shader.
		MAT_DIFFUSETEX	= (1<<11),
		//needs geometry shader cubemapping
		GEOMETRY_SHADER = (1<<12),
		//shader performs cubemap lookup
		CUBEMAP		= (1<<13),
		//shader performs blending, otherwise it is expected to output alpha 1.0
		//it is required for correct blending for our cubemapping
		BLENDING	= (1<<14),
	};

	typedef QMap<QString,FeatureFlags> t_FeatureFlagStrings;
	static t_FeatureFlagStrings featureFlagsStrings;

	QString getVShaderName(uint flags);
	QString getGShaderName(uint flags);
	QString getFShaderName(uint flags);
	QOpenGLShaderProgram* findOrLoadShader(uint flags);
	//! A simple shader preprocessor that can replace #defines
	bool preprocessShader(const QString& fileName, const uint flags, QByteArray& processedSource);
	//compiles and links the shader with this specified source code
	bool loadShader(QOpenGLShaderProgram& program, const QByteArray& vShader, const QByteArray& gShader, const QByteArray& fShader);
	void buildUniformCache(QOpenGLShaderProgram& program);

	typedef QHash<uint,QOpenGLShaderProgram*> t_ShaderCache;
	t_ShaderCache m_shaderCache;

	typedef QHash<QByteArray,QOpenGLShaderProgram*> t_ShaderContentCache;
	t_ShaderContentCache m_shaderContentCache;

	typedef QHash<UNIFORM,GLuint> t_UniformCacheEntry;
	typedef QHash<const QOpenGLShaderProgram*, t_UniformCacheEntry> t_UniformCache;
	t_UniformCache m_uniformCache;
};

QOpenGLShaderProgram* ShaderMgr::getShader(const GlobalShaderParameters& globals,const OBJ::Material* mat)
{
	//Build bitflags from bools. Some stuff requires pixelLighting to be enabled, so check it too.

	uint flags = INVALID;
	if(!globals.shadowTransform)
	{
		flags = SHADING;
		if(globals.pixelLighting)            flags|= PIXEL_LIGHTING;
		if(globals.pixelLighting && globals.shadows) flags|= SHADOWS;
		if(globals.pixelLighting && globals.shadows && globals.shadowFilter) flags|= SHADOW_FILTER;
		if(globals.pixelLighting && globals.shadows && globals.shadowFilter && globals.shadowFilterHQ) flags|= SHADOW_FILTER_HQ;
		if(globals.geometryShader) flags|= GEOMETRY_SHADER;
	}
	else
	{
		flags = TRANSFORM;
	}

	if(mat)
	{
		if(mat->illum>0)
			flags|= MAT_AMBIENT;
		if(mat->alphatest && mat->texture) //alpha test needs diffuse texture, otherwise it would not make sense
			flags|= ALPHATEST;
		if(mat->illum == OBJ::SPECULAR)
			flags|= MAT_SPECULAR;
		if(mat->illum == OBJ::TRANSLUCENT)
			flags|= BLENDING;
		if(mat->texture)
			flags|= MAT_DIFFUSETEX;
		if(mat->bump_texture && globals.bump && globals.pixelLighting)
			flags|= BUMP;
		if(mat->height_texture && globals.bump && globals.pixelLighting)
			flags|= HEIGHT;
	}

	return findOrLoadShader(flags);
}

QOpenGLShaderProgram* ShaderMgr::getCubeShader()
{
	return findOrLoadShader(CUBEMAP);
}

QOpenGLShaderProgram* ShaderMgr::getTextureShader()
{
	return findOrLoadShader(MAT_DIFFUSETEX);
}

GLint ShaderMgr::uniformLocation(const QOpenGLShaderProgram *shad, UNIFORM uni)
{
	t_UniformCache::iterator it = m_uniformCache.find(shad);
	if(it!=m_uniformCache.end())
	{
		return it.value().value(uni,-1);
	}
	return -1;
}

#endif
