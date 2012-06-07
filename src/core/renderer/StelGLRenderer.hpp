#ifndef _STELGLRENDERER_HPP_
#define _STELGLRENDERER_HPP_

#include <QImage>
#include <QPainter>
#include <QSize>

#include "StelGLUtilityFunctions.hpp"
#include "StelRenderer.hpp"
#include "StelVertexBuffer.hpp"

// TODO remove as FBOs are moved to QGLRenderer.
#include <QGLFramebufferObject>
#include <QGLFunctions>
#include "StelUtils.hpp"

//TODO At the moment, we're always using FBOs when supported.
//     We need a config file for Renderer implementation to disable it.

//TODO move QGL FBO (and maybe QPainter?) code to QGLRenderer

//! Base class for OpenGL based renderers.
//!
class StelGLRenderer : public StelRenderer
{
public:
	//! Construct a StelGLRenderer.
	StelGLRenderer()
		: frontBuffer(NULL)
		, backBuffer(NULL)
		, fboSupported(false)
		, fboDisabled(false)
		, viewportSize(QSize())
		, drawing(false)
	{
	}
	
	virtual ~StelGLRenderer()
	{
		if(NULL != frontBuffer)
		{
			delete frontBuffer;
			frontBuffer = NULL;
		}
		if(NULL != backBuffer)
		{
			delete backBuffer;
			backBuffer = NULL;
		}
	}
	
	virtual bool init()
	{
		fboSupported = QGLFramebufferObject::hasOpenGLFramebufferObjects();
		if (!useFBO())
		{
			qWarning() << "OpenGL framebuffer objects are disabled or not supported. "
			              "Can't use Viewport effects.";
		}
		return true;
	}
	
	virtual void viewportHasBeenResized(QSize size)
	{
		invariant();
		//Can't check this in invariant because Renderer is initialized before 
		//AppGraphicsWidget sets its viewport size
		Q_ASSERT_X(size.isValid(), Q_FUNC_INFO, "Invalid scene size");
		viewportSize = size;
		//We'll need FBOs of different size so get rid of the current FBOs.
		if (NULL != backBuffer)
		{
			delete backBuffer;
			backBuffer = NULL;
		}
		if (NULL != frontBuffer)
		{
			delete frontBuffer;
			frontBuffer = NULL;
		}
		invariant();
	}
	
	virtual QSize getViewportSize() const {return viewportSize;}
	
protected:
	//! Make Stellarium GL context the currently used GL context. Call this before GL calls.
	virtual void makeGLContextCurrent() = 0;

protected:
	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelGLRenderer::invariant().
	virtual void invariant() const
	{
		const bool fbo = useFBO();
		Q_ASSERT_X(NULL == backBuffer || fbo, Q_FUNC_INFO,
		           "We have a backbuffer even though we're not using FBO");
		Q_ASSERT_X(NULL == frontBuffer || fbo, Q_FUNC_INFO,
		           "We have a frontbuffer even though we're not using FBO");
		Q_ASSERT_X(drawing && fbo ? backBuffer != NULL : true, Q_FUNC_INFO,
		           "We're drawing and using FBOs, but the backBuffer is NULL");
		Q_ASSERT_X(drawing && fbo ? frontBuffer != NULL : true, Q_FUNC_INFO,
		           "We're drawing and using FBOs, but the frontBuffer is NULL");
	}

	//! Check for any OpenGL errors. Useful for detecting incorrect GL code.
	void checkGLErrors() const
	{
		const GLenum glError = glGetError();
		if(glError == GL_NO_ERROR) {return;}

		qWarning() << "OpenGL error detected: " << glErrorToString(glError);
	}

	//These will be private once moved to StelQGLRenderer
	
	//! Are we using framebuffer objects?
	bool useFBO() const
	{
		return fboSupported && !fboDisabled;
	}
	
	//! Frontbuffer (i.e. displayed at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* frontBuffer;
	
	//! Backbuffer (i.e. drawn to at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* backBuffer;

private:
	//! Are frame buffer objects supported on this system?
	bool fboSupported;
	
	//! Disable frame buffer objects even if supported?
	bool fboDisabled;
	
	//! Graphics scene size.
	QSize viewportSize;
	
//TEMP, this will be moved to QGLRenderer
protected:
	//! Are we in the middle of drawing?
	bool drawing;
	
	//! Initialize the frame buffer objects.
	void initFBO()
	{
		Q_ASSERT_X(useFBO(), Q_FUNC_INFO, "We're not using FBO");
		if (NULL == backBuffer)
		{
			Q_ASSERT_X(NULL == frontBuffer, Q_FUNC_INFO, 
			           "frontBuffer is not null even though backBuffer is");

			// TODO Use default QGLFunctions instead of constructing new one
			// after moving this to QGLRenderer.
			const bool npot = QGLFunctions().hasOpenGLFeature(QGLFunctions::NPOTTextures);

			// If non-power-of-two textures are supported,
			// FBOs must have power of two size large enough to fit the viewport.
			const QSize bufferSize = npot 
				? StelUtils::smallestPowerOfTwoSizeGreaterOrEqualTo(viewportSize) 
				: viewportSize;

			backBuffer = new QGLFramebufferObject(bufferSize,
			                                      QGLFramebufferObject::CombinedDepthStencil);
			frontBuffer = new QGLFramebufferObject(bufferSize,
			                                       QGLFramebufferObject::CombinedDepthStencil);
			Q_ASSERT_X(backBuffer->isValid() && frontBuffer->isValid(),
			           Q_FUNC_INFO, "Framebuffer objects failed to initialize");
		}
	}
	
	//! Swap front and back buffers, when using FBO.
	void swapBuffersFBO()
	{
		Q_ASSERT_X(useFBO(), Q_FUNC_INFO, "We're not using FBO");
		QGLFramebufferObject* tmp = backBuffer;
		backBuffer = frontBuffer;
		frontBuffer = tmp;
	}
};

#endif // _STELGLRENDERER_HPP_
