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

#ifndef _STELQGL2RENDERER_HPP_
#define _STELQGL2RENDERER_HPP_


#include <QGLShader>
#include <QGLShaderProgram>
#include <QVector>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelQGLGLSLShader.hpp"
#include "StelQGLRenderer.hpp"
#include "StelProjector.hpp"
#include "StelProjectorClasses.hpp"
#include "StelQGL2InterleavedArrayVertexBufferBackend.hpp"


//! Renderer backend using OpenGL 2.1 or GLSL 2.0 with Qt.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGL2Renderer : public StelQGLRenderer
{
public:
	//! Construct a StelQGL2Renderer.
	//!
	//! @param parent       Parent widget for the renderer's GL widget.
	//! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
	//!                     This should be true for mobile platforms with PowerVR GPUs.
	StelQGL2Renderer(QGraphicsView* parent, bool pvrSupported)
		: StelQGLRenderer(parent, pvrSupported)
        , initialized(false)
		, floatTexturesDisabled(true)
		, builtinShaders()
		, customShader(NULL)
	{
	}
	
	virtual ~StelQGL2Renderer()
	{
		// Can't check the invariant here, as the renderer will be destroyed even 
		// if it's initialization has failed.

		foreach(StelQGLGLSLShader *shader, builtinShaders)
		{
			delete shader;
		}
		
		initialized = false;
	}
	
	virtual bool init()
	{
		Q_ASSERT_X(!initialized, Q_FUNC_INFO, 
		           "StelQGL2Renderer is already initialized");
		
		// Using  this instead of makeGLContextCurrent() to avoid invariant 
		// as we're not in valid state (getGLContext() isn't public - it doesn't call invariant)
		getGLContext()->makeCurrent();

		// GL2 drawing doesn't work with GL1 paint engine .
		//
		// Not checking for GL2 as there could also be e.g. X11, or GL3 in future, etc; 
		// where GL2 might work fine.
		if(qtPaintEngineType() == QPaintEngine::OpenGL)
		{
			qWarning() << "StelQGL2Renderer::init : Failed because Qt paint engine is not OpenGL2 \n"
			              "If paint engine is OpenGL3 or higher, this code needs to be updated";
			return false;
		}

		if(!QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_Version_2_1) &&
		   !QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_ES_Version_2_0))
		{
			qWarning() << "StelQGL2Renderer::init : StelQGL2Renderer needs OpenGL 2.1 or OpenGL ES 2.0";
			return false;
		}

		// Check for features required for the GL2 backend.
		if(!gl.hasOpenGLFeature(QGLFunctions::Shaders))
		{
			qWarning() << "StelQGL2Renderer::init : Required feature not supported: shaders";
			return false;
		}
		if(!gl.hasOpenGLFeature(QGLFunctions::Buffers))
		{
			qWarning() << "StelQGL2Renderer::init : Required feature not supported: VBOs/IBOs";
			return false;
		}
		// We don't want to check for multitexturing everywhere.
		// Also, GL2 requires multitexturing so this shouldn't even happen.
		// (GL1 works fine without multitexturing)
		if(!gl.hasOpenGLFeature(QGLFunctions::Multitexture))
		{
			qWarning() << "StelQGL2Renderer::init : Required feature not supported: Multitexturing";
			return false;
		}

		if(!StelQGLRenderer::init())
		{
			qWarning() << "StelQGL2Renderer::init : parent init failed";
			return false;
		}

		// Each shader here handles a specific combination of vertex attribute 
		// interpretations. E.g. vertex-color-texcoord .

		// Notes about uniform/attribute names.
		//
		// Projection matrix is always specified (and must be declared in the shader),
		// as "uniform mat4 projectionMatrix"
		// 
		// For shaders that have no vertex color attribute,
		// "uniform vec4 globalColor" must be declared.
		//
		// Furthermore, each vertex attribute interpretation has its name that 
		// must be used. These are specified in attributeGLSLName function in 
		// StelGLUtilityFunctions.cpp . These are:
		//
		// - "vertex" for vertex position
		// - "texCoord" for texture coordinate
		// - "normal" for vertex normal
		// - "color" for vertex color 
		//
		// GLSLShader can also require unprojected position attribute, 
		// "unprojectedVertex", but this is only specified if the shader requires it.
		//
		
		// All vertices have the same color.
		plainShader = loadBuiltinShader
		(
			"plainShader"
			,
			"vec4 project(in vec4 v);\n"
			"attribute mediump vec4 vertex;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * project(vertex);\n"
			"}\n"
			,
			"uniform mediump vec4 globalColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = globalColor;\n"
			"}\n"
		);
		if(NULL == plainShader)
		{
			qWarning() << "StelQGL2Renderer::init failed to load plain shader";
			return false;
		}
		builtinShaders.append(plainShader);
		
		// Vertices with per-vertex color.
		colorShader = loadBuiltinShader
		(
			"colorShader"
			,
			"vec4 project(in vec4 v);\n"
			"attribute highp vec4 vertex;\n"
			"attribute mediump vec4 color;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    outColor = color;\n"
			"    gl_Position = projectionMatrix * project(vertex);\n"
			"}\n"
			,
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = outColor;\n"
			"}\n"
		);
		if(NULL == colorShader)
		{
			qWarning() << "StelQGL2Renderer::init failed to load color shader program";
			return false;
		}
		builtinShaders.append(colorShader);
		
		// Textured mesh.
		textureShader = loadBuiltinShader
		(
			"textureShader"
			,
			"vec4 project(in vec4 v);\n"
			"attribute highp vec4 vertex;\n"
			"attribute mediump vec2 texCoord;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec2 texc;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * project(vertex);\n"
			"    texc = texCoord;\n"
			"}\n"
			,
			"varying mediump vec2 texc;\n"
			"uniform sampler2D tex;\n"
			"uniform mediump vec4 globalColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = texture2D(tex, texc) * globalColor;\n"
			"}\n"
		);
		if(NULL == textureShader)
		{
			qWarning() << "StelQGL2Renderer::init failed to load texture shader";
			return false;
		}
		builtinShaders.append(textureShader);
		
		// Textured mesh interpolated with per-vertex color.
		colorTextureShader = loadBuiltinShader
		(
			"colorTextureShader"
			,
			"vec4 project(in vec4 v);\n"
			"attribute highp vec4 vertex;\n"
			"attribute mediump vec2 texCoord;\n"
			"attribute mediump vec4 color;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec2 texc;\n"
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * project(vertex);\n"
			"    texc = texCoord;\n"
			"    outColor = color;\n"
			"}\n"
			,
			"varying mediump vec2 texc;\n"
			"varying mediump vec4 outColor;\n"
			"uniform sampler2D tex;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = texture2D(tex, texc) * outColor;\n"
			"}\n"
		);
		if(NULL == colorTextureShader)
		{
			qWarning() << "StelQGL2Renderer::init failed to load colorTexture shader";
			return false;
		}
		builtinShaders.append(colorTextureShader);
		
		// Float texture support blacklist 
		// (mainly open source drivers, which don't support them due to patents on 
		//  float textures)
		floatTexturesDisabled =
			glVendorString == "nouveau"                || // Open source NVidia drivers
			glVendorString == "DRI R300 Project"       || // Open source ATI R300 (Radeon 9000, X000 and X1000)
			glVendorString == "Intel"                  || // Intel drivers
			glVendorString == "Mesa Project"           || // Mesa software rasterizer
			glVendorString == "Microsoft Corporation"  || // Microsoft builtin GL (this doesn't even support GL2, though)
			glVendorString == "Tungsten Graphics, Inc" || // Some Gallium3D drivers
			glVendorString == "X.Org R300 Project"     || // More open source ATI R300
			glVendorString == "X.Org"                  || // Gallium3D on AMD GPUs
			glVendorString == "VMware, Inc."              // More Gallium3D, mainly llvmpipe, softpipe
			;
		
		initialized = true;
		invariant();
		return true;
	}

	virtual bool areFloatTexturesSupported() const 
	{
		return QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_Version_3_0) 
		       && !floatTexturesDisabled;
	}

	virtual bool areNonPowerOfTwoTexturesSupported() const {return true;}

	virtual StelGLSLShader* createGLSLShader()
	{
		return new StelQGLGLSLShader(this, false);
	}

	virtual bool isGLSLSupported() const {return true;}

	//! Get shader corresponding to specified vertex format.
	//!
	//! If the user binds a custom shader (StelGLSLShader, in this case StelQGLGLSLShader),
	//! this shader is returned instead. (I.e. custom shader programs override builtin ones.)
	//!
	//! @param attributes     Vertex attributes used in the vertex format.
	//! @param attributeCount Number of vertex attributes.
	//! @return Pointer to the shader.
	StelQGLGLSLShader* getShader(const StelVertexAttribute* const attributes, const int attributeCount)
	{
		invariant();

		// A custom shader (StelGLSLShader - in this case StelQGLGLSLShader)
		// overrides builtin shaders.
		if(NULL != customShader) {return customShader;}

		// Determine which vertex attributes are used.
		bool position, texCoord, normal, color;
		position = texCoord = normal = color = false;
		for(int attrib = 0; attrib < attributeCount; ++attrib)
		{
			const StelVertexAttribute& attribute(attributes[attrib]);
			switch(attribute.interpretation)
			{
				case AttributeInterpretation_Position: position = true; break;
				case AttributeInterpretation_TexCoord: texCoord = true; break;
				case AttributeInterpretation_Normal:   normal   = true; break;
				case AttributeInterpretation_Color:    color    = true; break;
				default: 
					Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex interpretation");
			}
		}

		Q_ASSERT_X(position, Q_FUNC_INFO,
		           "Vertex formats without vertex position are not supported");

		invariant();
		// There are possible combinations - 4 are implemented right now.
		if(!texCoord && !normal && !color) {return plainShader;}
		if(!texCoord && !normal && color)  {return colorShader;}
		if(texCoord  && !normal && !color) {return textureShader;}
		if(texCoord  && !normal && color)  {return colorTextureShader;}

		// If we reach here, the vertex format has no shader implemented so we 
		// least inform the user about what vertex format fails (so it can
		// be changed or a shader can be implemented for it)
		qDebug() << "position: " << position << " texCoord: " << texCoord
		         << " normal: " << normal << " color: " << color;

		Q_ASSERT_X(false, Q_FUNC_INFO,
		           "Shader for vertex format not (yet) implemented");

		invariant();
		// Prevents GCC from complaining about exiting a non-void function:
		return NULL;
	}

	//! Use a custom shader program, overriding builtin shader programs.
	//!
	//! This can only be called when no custom shader is bound already
	//! (any previously bound custom shader must be released).
	//!
	//! Used by StelQGLGLSLShader.
	//!
	//! @param shader Shader to use.
	void bindCustomShader(StelQGLGLSLShader* shader)
	{
		Q_ASSERT_X(NULL == customShader, Q_FUNC_INFO, 
		           "Trying to bind() a shader without releasing the previous shader.");

		customShader = shader;
	}

	//! Release a custom shader program, allowing use of builtin shader programs.
	//!
	//! The released shader must match the currently bound one.
	//!
	//! Used by StelQGLGLSLShader.
	//!
	//! @param shader Shader assumed to be bound, for error checking.
	void releaseCustomShader(StelQGLGLSLShader* shader)
	{
#ifdef NDEBUG
		Q_UNUSED(shader);
#endif
		Q_ASSERT_X(shader == customShader, Q_FUNC_INFO, 
		           "Trying to release() a shader when no shader or a different shader is bound.");

		customShader = NULL;
	}

protected:
	virtual StelVertexBufferBackend* createVertexBufferBackend
		(const PrimitiveType primitiveType, const QVector<StelVertexAttribute>& attributes)
	{
		invariant();
		statistics[VERTEX_BUFFERS_CREATED] += 1.0;
		return new StelQGL2InterleavedArrayVertexBufferBackend(primitiveType, attributes);
	}

	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     StelIndexBuffer* indexBuffer = NULL,
	                                     StelProjector* projector = NULL,
	                                     bool dontProject = false)
	{
		invariant();

		StelQGL2InterleavedArrayVertexBufferBackend* backend =
			dynamic_cast<StelQGL2InterleavedArrayVertexBufferBackend*>(vertexBuffer);
		Q_ASSERT_X(backend != NULL, Q_FUNC_INFO,
		           "StelQGL2Renderer: Vertex buffer created by different renderer backend "
		           "or uninitialized");
		
		StelQGLIndexBuffer* glIndexBuffer = NULL;
		if(indexBuffer != NULL)
		{
			glIndexBuffer = dynamic_cast<StelQGLIndexBuffer*>(indexBuffer);
			Q_ASSERT_X(glIndexBuffer != NULL, Q_FUNC_INFO,
			           "StelQGL2Renderer: Index buffer created by different renderer " 
			           "backend or uninitialized");
			if(indexBuffer->length() == 0)
			{
				statistics[EMPTY_BATCHES] += 1.0;
				return;
			}
		}
		else if(backend->length() == 0)
		{
			statistics[EMPTY_BATCHES] += 1.0;
			return;
		}

		// We don't own this - we just have a pointer to it.
		StelProjector::GLSLProjector* glslProjector = NULL;
		StelQGLGLSLShader* shader = 
			getShader(backend->attributes.attributes, backend->attributes.count);

		// Internal shader can't be bound by the user so we bind/release it here.
		if(NULL == customShader) {shader->bind();}

		// We need a shared pointer when we're getting the projector ourselves (the 2D case), 
		// to prevent destructor of returned shared pointer from destroying it 
		// before we can use it.
		StelProjectorP projector2DDummy = 
			(NULL != projector ? StelProjectorP(NULL)
			                   : StelApp::getInstance().getCore()->getProjection2d());
		
		projector = (NULL != projector ? projector : &(*(projector2DDummy)));
		// XXX: we should use a more generic way to test whether or not to do the projection.
		if(!dontProject && (NULL == dynamic_cast<StelProjector2d*>(projector)))
		{
			// Try to do projection in GLSL, use projectVertices() as fallback.
			glslProjector = projector->getGLSLProjector();
			if(NULL == glslProjector || !glslProjector->init(shader))
			{
				backend->projectVertices(projector, glIndexBuffer);
				glslProjector = NULL;
				statistics[BATCH_PROJECTIONS_CPU_TOTAL] += 1.0;
				statistics[BATCH_PROJECTIONS_CPU]       += 1.0;
			}
			else
			{
				statistics[BATCH_PROJECTIONS_GPU_TOTAL] += 1.0;
				statistics[BATCH_PROJECTIONS_GPU]       += 1.0;
			}
		}
		else
		{
			statistics[BATCH_PROJECTIONS_NONE_TOTAL] += 1.0;
			statistics[BATCH_PROJECTIONS_NONE]       += 1.0;
		}
		
		if(!shader->getProgram().bind())
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "Failed to bind shader program");
		}
		if(NULL != glslProjector)
		{
			shader->pushUniformStorage();
			glslProjector->preDraw(shader);
		}

		// Instead of setting GL state when functions such as setDepthTest() or setCulledFaces()
		// are called, we only set it before drawing and reset after drawing to avoid 
		setupGLState(projector);

		// Need to transpose the matrix for QGL.
		const Mat4f& projectionMatrix = projector->getProjectionMatrix();

		// We might relink the shaders here, so we can only upload uniforms after the relinking.
		shader->uploadUniforms();

		// Set up viewport for the projector.
		const Vec4i viewXywh = projector->getViewportXywh();
		glViewport(viewXywh[0], viewXywh[1], viewXywh[2], viewXywh[3]);

		updateDrawStatistics(backend, glIndexBuffer);
		backend->draw(*this, projectionMatrix, glIndexBuffer, shader);

		// Restore default state to avoid interfering with Qt OpenGL drawing.
		restoreGLState(projector);

		shader->getProgram().release();

		if(NULL != glslProjector)
		{
			shader->popUniformStorage();
			glslProjector->postDraw(shader);
		}

		if(NULL == customShader) {shader->release();}

		invariant();
	}

	virtual int getTextureUnitCountBackend() 
	{
		// Called at initialization, so can't call invariant
		int textureUnitCount;
		// GL1 version should use GL_MAX_TEXTURE_UNITS instead.
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnitCount);
		return std::max(textureUnitCount, STELQGLRENDERER_MAX_TEXTURE_UNITS);
	}

#ifndef NDEBUG
	virtual void invariant() const
	{
		Q_ASSERT_X(initialized, Q_FUNC_INFO, "uninitialized StelQGL2Renderer");
		StelQGLRenderer::invariant();
	}
#endif
	
#ifndef USE_OPENGL_ES2
//uh, I shouldn't do this. OTOH, I don't really want to duplicate the whole renderer for ES2
private:
#endif
	//! Is the renderer initialized?
	bool initialized;

	//! Are floating point textures disabled? (E.g. disabled for open source drivers).
	bool floatTexturesDisabled;
	
	//! Contains all loaded shaders.
	//!
	//! Currently, these are duplicates of plainShader, etc. and only simplify cleanup.
	QVector<StelQGLGLSLShader *> builtinShaders;
	
	//! Shader that sets all vertices to the same color.
	StelQGLGLSLShader* plainShader;
	//! Shader used for per-vertex color.
	StelQGLGLSLShader* colorShader;
	//! Shader used for texturing.
	StelQGLGLSLShader* textureShader;
	//! Shader used for texturing interpolated with a per-vertex color.
	StelQGLGLSLShader* colorTextureShader;

	//! Custom shader program specified by the user.
	//!
	//! If not NULL, overrides builtin vertex format specific shader programs.
	StelQGLGLSLShader* customShader;
	
	//Note:
	//We don't keep handles to shader variable locations.
	//Instead, we access them when used through QGLShaderProgram.
	//That should only change if profiling shows that it wastes
	//too much time (which is unlikely)
	
	
	//! Load a builtin shader from specified source.
	//!
	//! @param name Name used for debugging.
	//! @param vSrc Vertex shader source.
	//! @param fSrc Fragment shader source.
	//!
	//! @return Complete shader program at success, NULL on compiling or linking error.
    StelQGLGLSLShader *loadBuiltinShader(const char* const name,
	                                     const char* const vSrc, const char* const fSrc)
	{
		// No invariants, as this is called from init - before the Renderer is
		// in fully valid state.
		StelQGLGLSLShader* result = 
			dynamic_cast<StelQGLGLSLShader*>(new StelQGLGLSLShader(this, true));
		
		// Compile and add vertex shader.
		if(!result->addVertexShader(QString(vSrc)))
		{
			qWarning() << "Failed to compile vertex shader of builtin shader \"" 
                       << name << "\" : " << result->log();
			delete result;
			return NULL;
		}
		// Compile and add fragment shader.
		if(!result->addFragmentShader(QString(fSrc)))
		{
			qWarning() << "Failed to compile fragment shader of builtin shader \"" 
			           << name << "\" : " << result->log();
			delete result;
			return NULL;
		}
		// Link the shader program.
		if(!result->build())
		{
			qWarning() << "Failed to link builtin shader \"" 
			           << name << "\" : " << result->log();
			delete result;
			return NULL;
		}
		if(!result->log().isEmpty())
 		{
 			qWarning() << "Log of the compilation of builtin shader \"" << name 
			           << "\" :" << result->log();
 		}
		
		return result;
	}
};

#endif // _STELQGL2RENDERER_HPP_
