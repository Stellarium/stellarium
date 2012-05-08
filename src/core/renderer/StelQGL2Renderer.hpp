#ifndef _STELQGL2RENDERER_HPP_
#define _STELQGL2RENDERER_HPP_


#include "StelQGLRenderer.hpp"
#include <qt4/QtCore/qvector.h>

#include <QGLShader>
#include <QGLShaderProgram>


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
	//! Construct a StelQGL2Renderer whose GL widget will be a child of specified QGraphicsView.
	StelQGL2Renderer(QGraphicsView* parent)
		: StelQGLRenderer(parent)
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
		Q_ASSERT_X(!initialized, "StelQGL2Renderer::init()", 
		           "StelQGLR2enderer is already initialized");
		
		// Using  this instead of makeGLContextCurrent() to avoid invariant 
		// as we're not in valid state (getGLContext() isn't public - it doesn't call invariant)
		getGLContext()->makeCurrent();
		
		// All vertices have the same color.
		plainShaderProgram = loadShaderProgram
		(
			"plainShaderProgram"
			,
			"attribute mediump vec3 vertex;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix*vec4(vertex, 1.);\n"
			"}\n"
			,
			"uniform mediump vec4 color;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = color;\n"
			"}\n"
		);
		if(NULL == plainShaderProgram)
		{
			return false;
		}
		shaderPrograms.append(plainShaderProgram);
		
		// Vertices with per-vertex color.
		colorShaderProgram = loadShaderProgram
		(
			"colorShaderProgram"
			,
			"attribute highp vec3 vertex;\n"
			"attribute mediump vec4 color;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    outColor = color;\n"
			"    gl_Position = projectionMatrix*vec4(vertex, 1.);\n"
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
			return false;
		}
		shaderPrograms.append(colorShaderProgram);
		
		// Textured mesh.
		textureShaderProgram = loadShaderProgram
		(
			"textureShaderProgram"
			,
			"attribute highp vec3 vertex;\n"
			"attribute mediump vec2 texCoord;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec2 texc;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * vec4(vertex, 1.);\n"
			"    texc = texCoord;\n"
			"}\n"
			,
			"varying mediump vec2 texc;\n"
			"uniform sampler2D tex;\n"
			"uniform mediump vec4 texColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = texture2D(tex, texc)*texColor;\n"
			"}\n"
		);
		if(NULL == colorTextureShaderProgram)
		{
			return false;
		}
		shaderPrograms.append(textureShaderProgram);
		
		// Textured mesh interpolated with per-vertex color.
		colorTextureShaderProgram = loadShaderProgram
		(
			"colorTextureShaderProgram"
			,
			"attribute highp vec3 vertex;\n"
			"attribute mediump vec2 texCoord;\n"
			"attribute mediump vec4 color;\n"
			"uniform mediump mat4 projectionMatrix;\n"
			"varying mediump vec2 texc;\n"
			"varying mediump vec4 outColor;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * vec4(vertex, 1.);\n"
			"    texc = texCoord;\n"
			"    outColor = color;\n"
			"}\n"
			,
			"varying mediump vec2 texc;\n"
			"varying mediump vec4 outColor;\n"
			"uniform sampler2D tex;\n"
			"void main(void)\n"
			"{\n"
			"    gl_FragColor = texture2D(tex, texc)*outColor;\n"
			"}\n"
		);
		if(NULL == colorTextureShaderProgram)
		{
			return false;
		}
		shaderPrograms.append(colorTextureShaderProgram);
		
		if(!StelQGLRenderer::init())
		{
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
	
protected:
	virtual void invariant()
	{
		Q_ASSERT_X(initialized, "StelQGL2Renderer::invariant()", 
		           "uninitialized StelQGL2Renderer");
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
