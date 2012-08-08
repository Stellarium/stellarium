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
#include "StelTestQGL2VertexBufferBackend.hpp"

//TODO:
// GL1/GL2 ifdefs (but is this really needed? 
// Shouldn't GL2 always be available at compile time?)

// About NPOT textures:
//
// GL2 can use non-power-of-two textures.
// GL1 can not.
// However even on modern hardware, npot textures are a bad idea as they might end
// up in POT storage anyway. Consider virtualizing
// textures instead, if we have enough time.
//
// Also, note that on R300, NPOT don't work with mipmaps, and on GeForceFX, they are emulated.

//! Renderer backend using OpenGL2 with Qt.
class StelQGL2Renderer : public StelQGLRenderer
{
public:
	//! Construct a StelQGL2Renderer.
	//!
	//! @param parent       Parent widget for the renderer's GL widget.
	//! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
	StelQGL2Renderer(QGraphicsView* parent, bool pvrSupported)
		: StelQGLRenderer(parent, pvrSupported)
		, initialized(false)
		, builtinShaders()
		, customShader(NULL)
		, textureUnitCount(-1)
	{
	}
	
	virtual ~StelQGL2Renderer()
	{
		invariant();
		
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
			"attribute mediump vec4 vertex;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * vertex;\n"
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
			"attribute highp vec4 vertex;\n"
			"attribute mediump vec4 color;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    outColor = color;\n"
			"    gl_Position = projectionMatrix * vertex;\n"
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
			"attribute highp vec4 vertex;\n"
			"attribute mediump vec2 texCoord;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec2 texc;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * vertex;\n"
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
			"attribute highp vec4 vertex;\n"
			"attribute mediump vec2 texCoord;\n"
			"attribute mediump vec4 color;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec2 texc;\n"
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * vertex;\n"
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
		
		if(!StelQGLRenderer::init())
		{
			qWarning() << "StelQGL2Renderer::init : parent init failed";
			return false;
		}
		
		initialized = true;
		invariant();
		return true;
	}

	virtual bool areNonPowerOfTwoTexturesSupported() const {return true;}

	virtual StelGLSLShader* createGLSLShader()
	{
		return new StelQGLGLSLShader(this);
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
	//! No custom shader can be bound when this is called 
	//! (previous custom shader must be released).
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
		return new StelTestQGL2VertexBufferBackend(primitiveType, attributes);
	}

	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     StelIndexBuffer* indexBuffer = NULL,
	                                     StelProjectorP projector = NULL,
	                                     bool dontProject = false)
	{
		invariant();

		StelTestQGL2VertexBufferBackend* backend =
			dynamic_cast<StelTestQGL2VertexBufferBackend*>(vertexBuffer);
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
		}

		// Instead of setting GL state when functions such as setDepthTest() or setCulledFaces()
		// are called, we only set it before drawing and reset after drawing to avoid 
		// conflicts with e.g. Qt OpenGL backend, or any other GL code that might be running.

		// GL setup before drawing.
		// Fix some problem when using Qt OpenGL2 engine
		glStencilMask(0x11111111);
		
		switch(depthTest)
		{
			case DepthTest_Disabled:
				glDisable(GL_DEPTH_TEST);
				break;
			case DepthTest_ReadOnly:
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				break;
			case DepthTest_ReadWrite:
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown depth test mode");
		}

		switch(stencilTest)
		{
			case StencilTest_Disabled:
				glDisable(GL_STENCIL_TEST);
				break;
			case StencilTest_Write_1:
				glStencilFunc(GL_ALWAYS, 0x1, 0x1);
				glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
				glEnable(GL_STENCIL_TEST);
				break;
			case StencilTest_DrawIf_1:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, 0x1, 0x1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown stencil test mode");
		}

		switch(culledFaces)
		{
			case CullFace_None:
				glDisable(GL_CULL_FACE);
				break;
			case CullFace_Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			case CullFace_Back:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown cull face type");
		}

		if(NULL == projector)
		{
			projector = StelApp::getInstance().getCore()->getProjection2d();
		}
		// XXX: we should use a more generic way to test whether or not to do the projection.
		else if(!dontProject && (NULL == dynamic_cast<StelProjector2d*>(projector.data())))
		{
			backend->projectVertices(projector, glIndexBuffer);
		}

		// Need to transpose the matrix for GL.
		const Mat4f& m = projector->getProjectionMatrix();
		const QMatrix4x4 transposed(m[0], m[4], m[8], m[12],
		                            m[1], m[5], m[9], m[13], 
		                            m[2], m[6], m[10], m[14], 
		                            m[3], m[7], m[11], m[15]);


		glFrontFace(projector->flipFrontBackFace() ? GL_CW : GL_CCW);

		// Set up viewport for the projector.
		const Vec4i viewXywh = projector->getViewportXywh();
		glViewport(viewXywh[0], viewXywh[1], viewXywh[2], viewXywh[3]);
		backend->draw(*this, transposed, glIndexBuffer);

		// Restore default state to avoid interfering with Qt OpenGL drawing.
		glFrontFace(projector->flipFrontBackFace() ? GL_CCW : GL_CW);
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);

		invariant();
	}

	virtual int getTextureUnitCount() 
	{
		invariant();
		if(textureUnitCount < 0)
		{
			// GL1 version should use GL_MAX_TEXTURE_UNITS instead.
			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnitCount);
			textureUnitCount = std::max(textureUnitCount, STELQGLRENDERER_MAX_TEXTURE_UNITS);
		}
		invariant();
		return textureUnitCount;
	}

	virtual void invariant(const char* const caller = Q_FUNC_INFO) const
	{
#ifndef NDEBUG
		Q_ASSERT_X(initialized, caller, "uninitialized StelQGL2Renderer");
		StelQGLRenderer::invariant(caller);
#endif
	}
	
private:
	//! Is the renderer initialized?
	bool initialized;
	
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

	//! Number of texture units. Lazily initialized by getTextureUnitCount().
	GLint textureUnitCount;
	
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
		StelQGLGLSLShader* result = dynamic_cast<StelQGLGLSLShader*>(createGLSLShader());
		
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
 			qWarning() << "Warnings during creation of builtin shader \"" << name 
			           << "\" :" << result->log();
 		}
		
		return result;
	}
};

#endif // _STELQGL2RENDERER_HPP_
