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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelOpenGL.hpp"
#include "ShaderManager.hpp"
#include "StelFileMgr.hpp"

#include <QDir>
#include <QOpenGLShaderProgram>
#include <QCryptographicHash>
#include <QDebug>

Q_LOGGING_CATEGORY(shaderMgr, "stel.plugin.scenery3d.shadermgr")

ShaderMgr::t_UniformStrings ShaderMgr::uniformStrings;
ShaderMgr::t_FeatureFlagStrings ShaderMgr::featureFlagsStrings;

ShaderMgr::ShaderMgr()
{
	if(uniformStrings.size()==0)
	{
		//initialize the strings
		uniformStrings["u_mModelView"] = UNIFORM_MAT_MODELVIEW;
		uniformStrings["u_mProjection"] = UNIFORM_MAT_PROJECTION;
		uniformStrings["u_mMVP"] = UNIFORM_MAT_MVP;
		uniformStrings["u_mNormal"] = UNIFORM_MAT_NORMAL;
		uniformStrings["u_mShadow0"] = UNIFORM_MAT_SHADOW0;
		uniformStrings["u_mShadow1"] = UNIFORM_MAT_SHADOW1;
		uniformStrings["u_mShadow2"] = UNIFORM_MAT_SHADOW2;
		uniformStrings["u_mShadow3"] = UNIFORM_MAT_SHADOW3;
		uniformStrings["u_mCubeMVP"] = UNIFORM_MAT_CUBEMVP;
		uniformStrings["u_mCubeMVP[]"] = UNIFORM_MAT_CUBEMVP;
		uniformStrings["u_mCubeMVP[0]"] = UNIFORM_MAT_CUBEMVP;

		//textures
		uniformStrings["u_texDiffuse"] = UNIFORM_TEX_DIFFUSE;
		uniformStrings["u_texEmissive"] = UNIFORM_TEX_EMISSIVE;
		uniformStrings["u_texBump"] = UNIFORM_TEX_BUMP;
		uniformStrings["u_texHeight"] = UNIFORM_TEX_HEIGHT;
		uniformStrings["u_texShadow0"] = UNIFORM_TEX_SHADOW0;
		uniformStrings["u_texShadow1"] = UNIFORM_TEX_SHADOW1;
		uniformStrings["u_texShadow2"] = UNIFORM_TEX_SHADOW2;
		uniformStrings["u_texShadow3"] = UNIFORM_TEX_SHADOW3;

		//materials
		uniformStrings["u_vMatShininess"] = UNIFORM_MTL_SHININESS;
		uniformStrings["u_vMatAlpha"] = UNIFORM_MTL_ALPHA;

		//pre-modulated lighting (light * material)
		uniformStrings["u_vMixAmbient"] = UNIFORM_MIX_AMBIENT;
		uniformStrings["u_vMixDiffuse"] = UNIFORM_MIX_DIFFUSE;
		uniformStrings["u_vMixSpecular"] = UNIFORM_MIX_SPECULAR;
		uniformStrings["u_vMixTorchDiffuse"] = UNIFORM_MIX_TORCHDIFFUSE;
		uniformStrings["u_vMixEmissive"] = UNIFORM_MIX_EMISSIVE;

		//light
		uniformStrings["u_vLightDirectionView"] = UNIFORM_LIGHT_DIRECTION_VIEW;
		uniformStrings["u_fTorchAttenuation"] = UNIFORM_TORCH_ATTENUATION;

		//others
		uniformStrings["u_vColor"] = UNIFORM_VEC_COLOR;
		uniformStrings["u_vSplits"] = UNIFORM_VEC_SPLITDATA;
		uniformStrings["u_vLightOrthoScale"] = UNIFORM_VEC_LIGHTORTHOSCALE;
		uniformStrings["u_vLightOrthoScale[]"] = UNIFORM_VEC_LIGHTORTHOSCALE;
		uniformStrings["u_vLightOrthoScale[0]"] = UNIFORM_VEC_LIGHTORTHOSCALE;
		uniformStrings["u_fAlphaThresh"] = UNIFORM_FLOAT_ALPHA_THRESH;
	}
	if(featureFlagsStrings.size()==0)
	{
		featureFlagsStrings["TRANSFORM"] = TRANSFORM;
		featureFlagsStrings["SHADING"] = SHADING;
		featureFlagsStrings["PIXEL_LIGHTING"] = PIXEL_LIGHTING;
		featureFlagsStrings["SHADOWS"] = SHADOWS;
		featureFlagsStrings["BUMP"] = BUMP;
		featureFlagsStrings["HEIGHT"] = HEIGHT;
		featureFlagsStrings["ALPHATEST"] = ALPHATEST;
		featureFlagsStrings["SHADOW_FILTER"] = SHADOW_FILTER;
		featureFlagsStrings["SHADOW_FILTER_HQ"] = SHADOW_FILTER_HQ;
		featureFlagsStrings["MAT_SPECULAR"] = MAT_SPECULAR;
		featureFlagsStrings["MAT_DIFFUSETEX"] = MAT_DIFFUSETEX;
		featureFlagsStrings["MAT_EMISSIVETEX"] = MAT_EMISSIVETEX;
		featureFlagsStrings["GEOMETRY_SHADER"] = GEOMETRY_SHADER;
		featureFlagsStrings["CUBEMAP"] = CUBEMAP;
		featureFlagsStrings["BLENDING"] = BLENDING;
		featureFlagsStrings["TORCH"] = TORCH;
		featureFlagsStrings["DEBUG"] = DEBUG;
		featureFlagsStrings["PCSS"] = PCSS;
		featureFlagsStrings["SINGLE_SHADOW_FRUSTUM"] = SINGLE_SHADOW_FRUSTUM;
		featureFlagsStrings["OGL_ES2"] = OGL_ES2;
		featureFlagsStrings["HW_SHADOW_SAMPLERS"] = HW_SHADOW_SAMPLERS;
	}
}

ShaderMgr::~ShaderMgr()
{
	clearCache();
}

void ShaderMgr::clearCache()
{
	qCDebug(shaderMgr)<<"Clearing"<<m_shaderContentCache.size()<<"shaders";

	//iterate over the shaderContentCache - this contains the same amount of shaders as actually exist!
	//the shaderCache could contain duplicate entries
	QHashIterator<QByteArray, QOpenGLShaderProgram*>it(m_shaderContentCache);
	while (it.hasNext())
	{
		it.next();
		if (it.value())
			delete it.value();
	}

	m_shaderCache.clear();
	m_uniformCache.clear();
	m_shaderContentCache.clear();
}

QOpenGLShaderProgram* ShaderMgr::findOrLoadShader(uint flags)
{
	auto it = m_shaderCache.find(flags);

	// This may also return nullptr if the load failed.
	//We wait until user explicitly forces shader reload until we try again to avoid spamming errors.
	if(it!=m_shaderCache.end())
		return *it;

	//get shader file names
	QString vShaderFile = getVShaderName(flags);
	QString gShaderFile = getGShaderName(flags);
	QString fShaderFile = getFShaderName(flags);
	qCDebug(shaderMgr)<<"Loading Scenery3d shader: flags:"<<flags<<", vs:"<<vShaderFile<<", gs:"<<gShaderFile<<", fs:"<<fShaderFile<<"";

	//load shader files & preprocess
	QByteArray vShader,gShader,fShader;

	QOpenGLShaderProgram *prog = nullptr;

	if(preprocessShader(vShaderFile,flags,vShader) &&
			preprocessShader(gShaderFile,flags,gShader) &&
			preprocessShader(fShaderFile,flags,fShader)
			)
	{
		//check if this content-hash was already created for optimization
		//(so that shaders with different flags, but identical implementation use the same program)
		QCryptographicHash hash(QCryptographicHash::Sha256);
		hash.addData(vShader);
		hash.addData(gShader);
		hash.addData(fShader);

		QByteArray contentHash = hash.result();
		if(m_shaderContentCache.contains(contentHash))
		{
#ifndef NDEBUG
			//qCDebug(shaderMgr)<<"Using existing shader with content-hash"<<contentHash.toHex();
#endif
			prog = m_shaderContentCache[contentHash];
		}
		else
		{
			//we have to compile the shader
			prog = new QOpenGLShaderProgram();

			if(!loadShader(*prog,vShader,gShader,fShader))
			{
				delete prog;
				prog = nullptr;
				qCCritical(shaderMgr)<<"ERROR: Shader '"<<flags<<"' could not be compiled. Fix errors and reload shaders or restart program.";
			}
#ifndef NDEBUG
			else
			{
				//qCDebug(shaderMgr)<<"Shader '"<<flags<<"' created, content-hash"<<contentHash.toHex();
			}
#endif
			m_shaderContentCache[contentHash] = prog;
		}
	}
	else
	{
		qCCritical(shaderMgr)<<"ERROR: Shader '"<<flags<<"' could not be loaded/preprocessed.";
	}


	//may put null in cache on fail!
	m_shaderCache[flags] = prog;

	return prog;
}

QString ShaderMgr::getVShaderName(uint flags)
{
	if(flags & SHADING)
	{
		if (! (flags & PIXEL_LIGHTING ))
			return "s3d_vertexlit.vert";
		else
			return "s3d_pixellit.vert";
	}
	else if (flags & CUBEMAP)
	{
		return "s3d_cube.vert";
	}
	else if (flags & TRANSFORM)
	{
		return "s3d_transform.vert";
	}
	else if (flags & MAT_DIFFUSETEX)
	{
		return "s3d_texture.vert";
	}
	else if (flags & DEBUG)
	{
		return "s3d_debug.vert";
	}
	return QString();
}

QString ShaderMgr::getGShaderName(uint flags)
{
	if(flags & GEOMETRY_SHADER)
	{
		if(flags & PIXEL_LIGHTING)
			return "s3d_pixellit.geom";
		else
			return "s3d_vertexlit.geom";
	}
	return QString();
}

QString ShaderMgr::getFShaderName(uint flags)
{
	if(flags & SHADING)
	{
		if (! (flags & PIXEL_LIGHTING ))
			return "s3d_vertexlit.frag";
		else
		{
			if(flags & OGL_ES2)
				return "s3d_pixellit_es.frag"; //for various reasons, ES version is separate
			else
				return "s3d_pixellit.frag";
		}
	}
	else if (flags & CUBEMAP)
	{
		return "s3d_cube.frag";
	}
	else if (flags & TRANSFORM)
	{
		//OGL ES2 always wants a fragment shader (at least on ANGLE)
		if((flags & ALPHATEST) || (flags & OGL_ES2))
			return "s3d_transform.frag";
	}
	else if (flags == MAT_DIFFUSETEX)
	{
		return "s3d_texture.frag";
	}
	else if (flags & DEBUG)
	{
		return "s3d_debug.frag";
	}
	return QString();
}

bool ShaderMgr::preprocessShader(const QString &fileName, const uint flags, QByteArray &processedSource)
{
	if(fileName.isEmpty())
	{
		//no shader of this type
		return true;
	}

	QDir dir("data/shaders/");
	QString filePath = StelFileMgr::findFile(dir.filePath(fileName),StelFileMgr::File);

	//open and load file
	QFile file(filePath);
#ifndef NDEBUG
	//qCDebug(shaderMgr)<<"File path:"<<filePath;
#endif
	if(!file.open(QFile::ReadOnly))
	{
		qCCritical(shaderMgr)<<"Could not open file"<<filePath;
		return false;
	}

	processedSource.clear();
	processedSource.reserve(static_cast<int>(file.size()));

	//use a text stream for "parsing"
	QTextStream in(&file),lineStream;

	QString line,word;
	while(!in.atEnd())
	{
		line = in.readLine();
		lineStream.setString(&line,QIODevice::ReadOnly);

		QString write = line;

		//read first word
		lineStream>>word;
		if(word == "#define")
		{
			//read second word
			lineStream>>word;

			//try to find it in our flags list
			auto it = featureFlagsStrings.find(word);
			if(it!=featureFlagsStrings.end())
			{
				bool val = it.value() & flags;
				write = "#define " + word + (val?" 1":" 0");
#ifdef NDEBUG
			}
#else
				//output matches for debugging
				//qCDebug(shaderMgr)<<"preprocess match: "<<line <<" --> "<<write;
			}
			else
			{
				//qCDebug(shaderMgr)<<"unknown define, ignoring: "<<line;
			}
#endif
		}

		//write output
		processedSource.append(write.toUtf8());
		processedSource.append('\n');
	}
	return true;
}

bool ShaderMgr::loadShader(QOpenGLShaderProgram& program, const QByteArray& vShader, const QByteArray& gShader, const QByteArray& fShader)
{
	//clear old shader data, if exists
	program.removeAllShaders();

	if(!vShader.isEmpty())
	{
		const auto prefix = StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER);
		if(!program.addShaderFromSourceCode(QOpenGLShader::Vertex, prefix + vShader))
		{
			qCCritical(shaderMgr) << "Unable to compile vertex shader";
			qCCritical(shaderMgr) << program.log();
			return false;
		}
		else
		{
			//TODO Qt wrapper does not seem to provide warnings (regardless of what its doc says)!
			//Raise a bug with them or handle shader loading ourselves?
			QString log = program.log().trimmed();
			if(!log.isEmpty())
			{
				qCWarning(shaderMgr)<<"Vertex shader warnings:";
				qCWarning(shaderMgr)<<log;
			}
		}
	}

	if(!gShader.isEmpty())
	{
		if(!program.addShaderFromSourceCode(QOpenGLShader::Geometry,gShader))
		{
			qCCritical(shaderMgr) << "Unable to compile geometry shader";
			qCCritical(shaderMgr) << program.log();
			return false;
		}
		else
		{
			//TODO Qt wrapper does not seem to provide warnings (regardless of what its doc says)!
			//Raise a bug with them or handle shader loading ourselves?
			QString log = program.log().trimmed();
			if(!log.isEmpty())
			{
				qCWarning(shaderMgr)<<"Geometry shader warnings:";
				qCWarning(shaderMgr)<<log;
			}
		}
	}

	if(!fShader.isEmpty())
	{
		const auto globalPrefix = StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER);
		const bool prefixHasVersion = globalPrefix.contains("#version");
		const bool shaderHasVersion = fShader.contains("#version");
		QByteArray prefix;
		QByteArray finalShader = fShader;
		if(shaderHasVersion)
		{
			static const QRegularExpression versionRE("^#version ([0-9]+)\\b.*");
			static const QRegularExpression versionRE2("(^|\n)(#version[ \t]+)([0-9]+)\\b");
			const QString shaderVersionString = QString(fShader.simplified()).replace(versionRE, "\\1");
			bool svOK = false;
			const int shaderVersion = shaderVersionString.toInt(&svOK);

			const QString prefixVersionString = QString(globalPrefix.simplified()).replace(versionRE, "\\1");
			bool pvOK = false;
			const int prefixVersion = prefixVersionString.toInt(&pvOK);

			bool failed = false;
			if(!svOK)
			{
				qCCritical(shaderMgr) << "Failed to get shader version, string:" << shaderVersionString;
				failed = true;
			}

			if(prefixHasVersion && !pvOK)
			{
				qCCritical(shaderMgr) << "Failed to get prefix version, string:" << prefixVersionString;
				failed = true;
			}

			if(failed) return false;

			if(!prefixHasVersion)
			{
				prefix = QString("#version %1\n").arg(shaderVersion).toUtf8() + globalPrefix;
			}
			else if(shaderVersion > prefixVersion)
			{
				prefix = QString(globalPrefix).replace(versionRE2, QString("\\1\\2%1").arg(shaderVersion)).toUtf8();
			}
			else
			{
				prefix = globalPrefix;
			}
			finalShader = QString(fShader).replace(versionRE2, "//\\1\\2\\3").toUtf8();
		}
		else
		{
			prefix = globalPrefix;
		}
		if(!program.addShaderFromSourceCode(QOpenGLShader::Fragment, prefix + finalShader))
		{
			qCCritical(shaderMgr) << "Unable to compile fragment shader";
			qCCritical(shaderMgr) << program.log();
			return false;
		}
		else
		{
			//TODO Qt wrapper does not seem to provide warnings (regardless of what its doc says)!
			//Raise a bug with them or handle shader loading ourselves?
			QString log = program.log().trimmed();
			if(!log.isEmpty())
			{
				qCWarning(shaderMgr)<<"Fragment shader warnings:";
				qCWarning(shaderMgr)<<log;
			}
		}
	}

	//Set attribute locations to hardcoded locations.
	//This enables us to use a single VAO configuration for all shaders!
	program.bindAttributeLocation("a_vertex",StelOpenGLArray::ATTLOC_VERTEX);
	program.bindAttributeLocation("a_normal", StelOpenGLArray::ATTLOC_NORMAL);
	program.bindAttributeLocation("a_texcoord",StelOpenGLArray::ATTLOC_TEXCOORD);
	program.bindAttributeLocation("a_tangent",StelOpenGLArray::ATTLOC_TANGENT);
	program.bindAttributeLocation("a_bitangent",StelOpenGLArray::ATTLOC_BITANGENT);


	//link program
	if(!program.link())
	{
		qCCritical(shaderMgr)<<"[ShaderMgr] unable to link shader";
		qCCritical(shaderMgr)<<program.log();
		return false;
	}

	buildUniformCache(program);
	return true;
}

void ShaderMgr::buildUniformCache(QOpenGLShaderProgram &program)
{
	//this enumerates all available uniforms of this shader, and stores their locations in a map
	GLuint prog = program.programId();
	GLint numUniforms=0,bufSize;

	QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
	GL(gl->glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &numUniforms));
	GL(gl->glGetProgramiv(prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSize));

	QByteArray buf(bufSize,'\0');
	GLsizei length;
	GLint size;
	GLenum type;

#ifndef NDEBUG
	//qCDebug(shaderMgr)<<"Shader has"<<numUniforms<<"uniforms";
#endif
	for(uint i =0;i<static_cast<GLuint>(numUniforms);++i)
	{
		GL(gl->glGetActiveUniform(prog,i,bufSize,&length,&size,&type,buf.data()));
		QString str(buf);
		str = str.left(str.indexOf(QChar(0x00)));

		GLint loc = program.uniformLocation(str);

		auto it = uniformStrings.find(str);

		// This may also return nullptr if the load failed.
		//We wait until user explicitly forces shader reload until we try again to avoid spamming errors.
		if(it!=uniformStrings.end())
		{
			//this is uniform we recognize
			//need to get the uniforms location (!= index)
			m_uniformCache[&program][*it] = static_cast<GLuint>(loc);
			//output mapping for debugging
			//qCDebug(shaderMgr)<<i<<loc<<str<<size<<type<<" mapped to "<<*it;
		}
		else
		{
			qCWarning(shaderMgr)<<i<<loc<<str<<size<<type<<" --- unknown uniform! ---";
		}
	}
}
