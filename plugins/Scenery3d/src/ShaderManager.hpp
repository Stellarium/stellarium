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
#include "StelOpenGL.hpp"

#include <QMap>

class QOpenGLShaderProgram;

//! A simple shader cache class that gives us the correct shader depending on desired configuration.
class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	//! Enum for OpenGL shader attribute locations
	enum ATTLOC
	{
		//! This is the OpenGL attribute location where vertex positions are mapped to
		ATTLOC_VERTEX,
		//! This is the OpenGL attribute location where vertex normals are mapped to
		ATTLOC_NORMAL,
		//! This is the OpenGL attribute location where vertex texture coordinates are mapped to
		ATTLOC_TEXTURE,
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
		UNIFORM_MAT_SHADOW4,
		UNIFORM_MAT_SHADOW5,
		UNIFORM_MAT_SHADOW6,
		UNIFORM_MAT_SHADOW7,

		//! Defines the Diffuse texture slot
		UNIFORM_TEX_DIFFUSE,
		//! First shadow map
		UNIFORM_TEX_SHADOW0,
		UNIFORM_TEX_SHADOW1,
		UNIFORM_TEX_SHADOW2,
		UNIFORM_TEX_SHADOW3,
		UNIFORM_TEX_SHADOW4,
		UNIFORM_TEX_SHADOW5,
		UNIFORM_TEX_SHADOW6,
		UNIFORM_TEX_SHADOW7,

		//! Material ambient color
		UNIFORM_MTL_AMBIENT,
		//! Material diffuse color
		UNIFORM_MTL_DIFFUSE,
		//! Material specular color
		UNIFORM_MTL_SPECULAR,
		//! Material global transparency
		UNIFORM_MTL_ALPHA,

		//! Light direction vector (view space)
		UNIFORM_LIGHT_DIRECTION,
		//! Light ambient intensity
		UNIFORM_LIGHT_AMBIENT,
		//! Light diffuse intensity
		UNIFORM_LIGHT_DIFFUSE,
	};

	//! Returns a shader that supports the specified operations. Must be called within a GL context.
	QOpenGLShaderProgram* getShader(bool pixelLighting,bool shadows,bool bump);

	//! Returns the location of this uniform for this shader, or -1 if this uniform does not exist.
	//! This is cached to elimate the overhead of the glGet calls
	inline GLint getUniformLocation(const QOpenGLShaderProgram* shad,UNIFORM uni);

	//! Clears the shaders that have been created by this manager. Must be called within a GL context.
	void clearCache();

private:
	typedef QMap<QString,UNIFORM> t_UniformStrings;
	static t_UniformStrings uniformStrings;

	enum FeatureFlags
	{
		PIXEL_LIGHTING  = 0x1u,
		SHADOWS         = 0x2u,
		BUMP            = 0x4u,
	};

	QString getVShaderName(uint flags);
	QString getGShaderName(uint flags);
	QString getFShaderName(uint flags);
	bool loadShader(QOpenGLShaderProgram& program, const QString& vShader, const QString& gShader, const QString& fShader);
	void buildUniformCache(QOpenGLShaderProgram& program);

	typedef QMap<uint,QOpenGLShaderProgram*> t_ShaderCache;
	typedef QMap<UNIFORM,GLuint> t_UniformCacheEntry;
	typedef QMap<const QOpenGLShaderProgram*, t_UniformCacheEntry> t_UniformCache;
	t_ShaderCache m_shaderCache;
	t_UniformCache m_uniformCache;
};

GLint ShaderManager::getUniformLocation(const QOpenGLShaderProgram *shad, UNIFORM uni)
{
	t_UniformCache::iterator it = m_uniformCache.find(shad);
	if(it!=m_uniformCache.end())
	{
		return it.value().value(uni,-1);
	}
	return -1;
}

#endif
