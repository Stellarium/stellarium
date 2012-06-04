#ifndef _STELQGL2RENDERER_HPP_
#define _STELQGL2RENDERER_HPP_


#include <QGLShader>
#include <QGLShaderProgram>
#include <QVector>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelQGLRenderer.hpp"
#include "StelProjector.hpp"

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
		, shaderPrograms()
	{
	}
	
	virtual ~StelQGL2Renderer()
	{
		invariant();
		
		foreach(QGLShaderProgram *program, shaderPrograms)
		{
			delete program;
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
		if(!gl.hasOpenGLFeature(QGLFunctions::Framebuffers))
		{
			qWarning() << "StelQGL2Renderer::init : Required feature not supported: framebuffers";
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
		
		// All vertices have the same color.
		plainShaderProgram = loadShaderProgram
		(
			"plainShaderProgram"
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
		if(NULL == plainShaderProgram)
		{
			qWarning() << "StelQGL2Renderer::init failed to load plain shader program";
			return false;
		}
		shaderPrograms.append(plainShaderProgram);
		
		// Vertices with per-vertex color.
		colorShaderProgram = loadShaderProgram
		(
			"colorShaderProgram"
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
		if(NULL == colorShaderProgram)
		{
			qWarning() << "StelQGL2Renderer::init failed to load color shader program";
			return false;
		}
		shaderPrograms.append(colorShaderProgram);
		
		// Textured mesh.
		textureShaderProgram = loadShaderProgram
		(
			"textureShaderProgram"
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
		if(NULL == textureShaderProgram)
		{
			qWarning() << "StelQGL2Renderer::init failed to load texture shader program";
			return false;
		}
		shaderPrograms.append(textureShaderProgram);
		
		// Textured mesh interpolated with per-vertex color.
		colorTextureShaderProgram = loadShaderProgram
		(
			"colorTextureShaderProgram"
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
		if(NULL == colorTextureShaderProgram)
		{
			qWarning() << "StelQGL2Renderer::init failed to load colorTexture shader program";
			return false;
		}
		shaderPrograms.append(colorTextureShaderProgram);
		
		if(!StelQGLRenderer::init())
		{
			qWarning() << "StelQGL2Renderer::init : parent init failed";
			return false;
		}
		
		// TODO remove this as we refactor StelPainter.
		StelPainter::TEMPSpecifyShaders(plainShaderProgram,
		                                colorShaderProgram,
		                                textureShaderProgram,
		                                colorTextureShaderProgram);
		initialized = true;
		invariant();
		return true;
	}

	//! Get shader program corresponding to specified vertex format.
	//!
	//! @param attributes Vertex attributes used in the vertex format.
	//! @return Pointer to the shader program.
	QGLShaderProgram* getShaderProgram(const QVector<StelVertexAttribute>& attributes)
	{
		// Determine which vertex attributes are used.
		bool position, texCoord, normal, color;
		position = texCoord = normal = color = false;
		foreach(const StelVertexAttribute& attribute, attributes)
		{
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

		// There are possible combinations - 4 are implemented right now.
		if(!texCoord && !normal && !color) {return plainShaderProgram;}
		if(!texCoord && !normal && color)  {return colorShaderProgram;}
		if(texCoord  && !normal && !color) {return textureShaderProgram;}
		if(texCoord  && !normal && color)  {return colorTextureShaderProgram;}

		// If we reach here, the vertex format has no shader implemented so we 
		// least inform the user about what vertex format fails (so it can
		// be changed or a shader can be implemented for it)
		qDebug() << "position: " << position << " texCoord: " << texCoord
		         << " normal: " << normal << " color: " << color;

		Q_ASSERT_X(false, Q_FUNC_INFO,
		           "Shader for vertex format not (yet) implemented");

		// Prevents GCC from complaining about exiting a non-void function:
		return NULL;
	}

protected:
	virtual StelVertexBufferBackend* createVertexBufferBackend
		(const PrimitiveType primitiveType, const QVector<StelVertexAttribute>& attributes)
	{
		return new StelTestQGL2VertexBufferBackend(primitiveType, attributes);
	}
	
	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     class StelIndexBuffer* indexBuffer = NULL,
	                                     StelProjectorP projector = NULL)
	{
		Q_ASSERT_X(indexBuffer == NULL, Q_FUNC_INFO,
		           "TODO: Using index buffer when drawing not yet implemented");

		//TODO Projection using StelProjector 
		//TODO IndexBuffer 
		
		StelTestQGL2VertexBufferBackend* backend =
			dynamic_cast<StelTestQGL2VertexBufferBackend*>(vertexBuffer);
		Q_ASSERT_X(backend != NULL, Q_FUNC_INFO,
		           "StelQGL2Renderer: Trying to draw a vertex buffer created by a "
		           "different renderer backend");

		// GL setup before drawing.

		glDisable(GL_DEPTH_TEST);
		// Not sure why this is used - experiment with removing once more code uses Renderer
		glDisable(GL_CULL_FACE);
		// Fix some problem when using Qt OpenGL2 engine
		glStencilMask(0x11111111);
		// Deactivate drawing in depth buffer by default
		glDepthMask(GL_FALSE);

		if(NULL == projector)
		{
			projector = StelApp::getInstance().getCore()->getProjection2d();
		}
		else
		{
			Q_ASSERT_X(projector == NULL, Q_FUNC_INFO,
			           "TODO: Projection when drawing not yet implemented");
		}

		// Need to transpose the matrix for GL.
		const Mat4f& m = projector->getProjectionMatrix();
		const QMatrix4x4 transposed(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);

		glFrontFace(projector->needGlFrontFaceCW() ? GL_CW : GL_CCW);

		// Set up viewport for the projector.
		const Vec4i viewXywh = projector->getViewportXywh();
		glViewport(viewXywh[0], viewXywh[1], viewXywh[2], viewXywh[3]);
		backend->draw(*this, transposed);
	}

	virtual void invariant()
	{
		Q_ASSERT_X(initialized, Q_FUNC_INFO, "uninitialized StelQGL2Renderer");
		StelQGLRenderer::invariant();
	}
	
private:
	//! Is the renderer initialized?
	bool initialized;
	
	//! Contains all loaded shaders.
	//!
	//! Currently, these are duplicates of plainShaderProgram, etc. and only simplify cleanup.
	QVector<QGLShaderProgram *> shaderPrograms;
	
	//! Shader that sets all vertices to the same color.
	QGLShaderProgram* plainShaderProgram;
	//! Shader used for per-vertex color.
	QGLShaderProgram* colorShaderProgram;
	//! Shader used for texturing.
	QGLShaderProgram* textureShaderProgram;
	//! Shader used for texturing interpolated with a per-vertex color.
	QGLShaderProgram* colorTextureShaderProgram;
	
	//Note:
	//We don't keep handles to shader variable locations.
	//Instead, we access them when used through QGLShaderProgram.
	//That should only change if profiling shows that it wastes
	//too much time (which is unlikely)
	
	
	//! Load a shader program from specified source.
	//!
	//! This will load a shader program composed of one vertex shader and one fragment shader.
	//!
	//! @param name Name used for debugging.
	//! @param vSrc Vertex shader source.
	//! @param fSrc Fragment shader source.
	//!
	//! @return Complete shader program at success, NULL on compiling or linking error.
	QGLShaderProgram *loadShaderProgram(const char* const name,
	                                    const char* const vSrc, const char* const fSrc)
	{
		QGLShaderProgram* result = new QGLShaderProgram(getGLContext());
		
		// We add shaders directly from source instead of working with
		// QGLShader, as in that case we would also need to take care of QGLShader destruction.
		
		// Compile and add vertex shader.
		if(!result->addShaderFromSourceCode(QGLShader::Vertex, vSrc))
		{
			qWarning() << "Failed to compile vertex shader of program \"" 
			           << name << "\" : " << result->log();
			delete result;
			return NULL;
		}
		// Compile and add fragment shader.
		if(!result->addShaderFromSourceCode(QGLShader::Fragment, fSrc))
		{
			qWarning() << "Failed to compile fragment shader of program \"" 
			           << name << "\" : " << result->log();
			delete result;
			return NULL;
		}
		// Link the shader program.
		if(!result->link())
		{
			qWarning() << "Failed to link shader program \"" 
			           << name << "\" : " << result->log();
			delete result;
			return NULL;
		}
		
		return result;
	}
};

#endif // _STELQGL2RENDERER_HPP_
