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

#include "ShaderManager.hpp"
#include "StelOpenGL.hpp"
#include "StelFileMgr.hpp"

#include <QDir>
#include <QOpenGLShaderProgram>

ShaderManager::t_UniformStrings ShaderManager::uniformStrings;

ShaderManager::ShaderManager()
{
	if(uniformStrings.size()==0)
	{
		//initialize the strings
		uniformStrings["u_mModelView"] = UNIFORM_MAT_MODELVIEW;
		uniformStrings["u_mProjection"] = UNIFORM_MAT_PROJECTION;
		uniformStrings["u_mMVP"] = UNIFORM_MAT_MVP;
		uniformStrings["u_mNormal"] = UNIFORM_MAT_NORMAL;

		uniformStrings["u_texDiffuse"] = UNIFORM_TEX_DIFFUSE;

		//materials
		uniformStrings["u_vMatAmbient"] = UNIFORM_MTL_AMBIENT;
		uniformStrings["u_vMatDiffuse"] = UNIFORM_MTL_DIFFUSE;
		uniformStrings["u_vMatSpecular"] = UNIFORM_MTL_SPECULAR;
		uniformStrings["u_vMatAlpha"] = UNIFORM_MTL_ALPHA;

		//light
		uniformStrings["u_vLightDirection"] = UNIFORM_LIGHT_DIRECTION;
		uniformStrings["u_vLightAmbient"] = UNIFORM_LIGHT_AMBIENT;
		uniformStrings["u_vLightDiffuse"] = UNIFORM_LIGHT_DIFFUSE;
	}
}

ShaderManager::~ShaderManager()
{
	clearCache();
}

void ShaderManager::clearCache()
{
	for(t_ShaderCache::iterator it = m_shaderCache.begin();it!=m_shaderCache.end();++it)
	{
		if((*it))
			delete (*it);
	}

	m_shaderCache.clear();
	m_uniformCache.clear();
	qDebug()<<"[Scenery3d] Shader cache cleared";
}

QOpenGLShaderProgram* ShaderManager::getShader(bool pixelLighting, bool shadows, bool bump)
{
	//Build bitflags from bools. Some stuff requires pixelLighting to be enabled, so check it too.
	uint flags = (pixelLighting & PIXEL_LIGHTING)|
			(pixelLighting & shadows & SHADOWS)|
			(pixelLighting & bump & BUMP);

	t_ShaderCache::iterator it = m_shaderCache.find(flags);

	// This may also return NULL if the load failed.
	//We wait until user explictly forces shader reload until we try again to avoid spamming errors.
	if(it!=m_shaderCache.end())
		return *it;

	//get shader names, create program and load shaders
	QString vShader = getVShaderName(flags);
	QString gShader = getGShaderName(flags);
	QString fShader = getFShaderName(flags);

	QOpenGLShaderProgram *prog = new QOpenGLShaderProgram();
	if(!loadShader(*prog,vShader,gShader,fShader))
	{
		delete prog;
		prog = NULL;
		qCritical()<<"[Scenery3d] ERROR: Shader '"<<flags<<"' could not be created. Fix errors and reload shaders or restart program.";
	}

	//may put null in cache on fail
	m_shaderCache[flags] = prog;

	return prog;
}

QString ShaderManager::getVShaderName(uint flags)
{
	if (! (flags & PIXEL_LIGHTING ))
		return "s3d_vertexlit.vert";
	return QString();
}

QString ShaderManager::getGShaderName(uint flags)
{
	Q_UNUSED(flags);
	//currently no Gshaders used
	return QString();
}

QString ShaderManager::getFShaderName(uint flags)
{
	if (! (flags & PIXEL_LIGHTING ))
		return "s3d_vertexlit.frag";
	return QString();
}

bool ShaderManager::loadShader(QOpenGLShaderProgram& program, const QString& vShader, const QString& gShader, const QString& fShader)
{
	qDebug()<<"Loading Scenery3d shader: vs:"<<vShader<<", gs:"<<gShader<<", fs:"<<fShader<<"";

	//clear old shader data, if exists
	program.removeAllShaders();

	QDir dir("data/shaders/");

	if(!vShader.isEmpty())
	{
		QString vs = StelFileMgr::findFile(dir.filePath(vShader),StelFileMgr::File);

		if(!program.addShaderFromSourceFile(QOpenGLShader::Vertex,vs))
		{
			qCritical() << "Scenery3d: unable to compile " << vs << " vertex shader file";
			qCritical() << program.log();
			return false;
		}
		else
		{
			QString log = program.log().trimmed();
			if(!log.isEmpty())
			{
				qWarning()<<vShader<<" warnings:";
				qWarning()<<log;
			}
		}
	}

	if(!gShader.isEmpty())
	{
		QString gs = StelFileMgr::findFile(dir.filePath(gShader),StelFileMgr::File);
		if(!program.addShaderFromSourceFile(QOpenGLShader::Geometry,gs))
		{
			qCritical() << "Scenery3d: unable to compile " << gs << " geometry shader file";
			qCritical() << program.log();
			return false;
		}
		else
		{
			QString log = program.log().trimmed();
			if(!log.isEmpty())
			{
				qWarning()<<fShader<<" warnings:";
				qWarning()<<log;
			}
		}
	}

	if(!fShader.isEmpty())
	{
		QString fs = StelFileMgr::findFile(dir.filePath(fShader),StelFileMgr::File);
		if(!program.addShaderFromSourceFile(QOpenGLShader::Fragment,fs))
		{
			qCritical() << "Scenery3d: unable to compile " << fs << " fragment shader file";
			qCritical() << program.log();
			return false;
		}
		else
		{
			QString log = program.log().trimmed();
			if(!log.isEmpty())
			{
				qWarning()<<fShader<<" warnings:";
				qWarning()<<log;
			}
		}
	}

	//Set attribute locations to hardcoded locations.
	//This enables us to use a single VAO configuration for all shaders!
	program.bindAttributeLocation("a_vertex",ATTLOC_VERTEX);
	program.bindAttributeLocation("a_normal", ATTLOC_NORMAL);
	program.bindAttributeLocation("a_texcoord",ATTLOC_TEXTURE);
	program.bindAttributeLocation("a_tangent",ATTLOC_TANGENT);
	program.bindAttributeLocation("a_bitangent",ATTLOC_BITANGENT);


	//link program
	if(!program.link())
	{
		qCritical()<<"Scenery3d: unable to link shader files "<<vShader<<", "<<gShader<<", "<<fShader;
		qCritical()<<program.log();
		return false;
	}

	buildUniformCache(program);
	return true;
}

void ShaderManager::buildUniformCache(QOpenGLShaderProgram &program)
{
	//this enumerates all available uniforms of this shader, and stores their locations in a map
	GLuint prog = program.programId();
	GLint numUniforms=0,bufSize;
	glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &numUniforms);
	glGetProgramiv(prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSize);

	QByteArray buf(bufSize,'\0');
	GLsizei length;
	GLint size;
	GLenum type;

	qDebug()<<"Shader has "<<numUniforms<<" uniforms";
	for(int i =0;i<numUniforms;++i)
	{
		glGetActiveUniform(prog,i,bufSize,&length,&size,&type,buf.data());
		QString str(buf);
		str = str.trimmed(); // no idea if this is required
		qDebug()<<i<<str<<size<<type;

		t_UniformStrings::iterator it = uniformStrings.find(str);

		// This may also return NULL if the load failed.
		//We wait until user explictly forces shader reload until we try again to avoid spamming errors.
		if(it!=uniformStrings.end())
		{
			//this is uniform we recognize
			//need to get the uniforms location (!= index)
			m_uniformCache[&program][*it] = program.uniformLocation(str);
		}
	}
}
