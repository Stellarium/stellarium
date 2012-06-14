#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


#include <QColor>
#include <QGLFunctions>
#include <QGraphicsView>
#include <QThread>

#include "StelApp.hpp"
#include "StelGuiBase.hpp"
#include "StelPainter.hpp"
#include "StelRenderer.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelQGLViewport.hpp"
#include "StelTextureCache.hpp"
#include "StelVertexBuffer.hpp"


//TODO get rid of all StelPainter calls (gradually)

//! Base class for renderer based on OpenGL and at the same time Qt's QGL.
class StelQGLRenderer : public StelRenderer
{
public:
	//! Construct a StelQGLRenderer.
	//!
	//! @param parent       Parent widget for the renderer's GL widget.
	//! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
	StelQGLRenderer(QGraphicsView* const parent, const bool pvrSupported)
		: StelRenderer()
		, glContext(new QGLContext(QGLFormat(QGL::StencilBuffer | 
		                                     QGL::DepthBuffer   |
		                                     QGL::DoubleBuffer)))
		, viewport(new StelQGLWidget(glContext, parent), parent)
		, pvrSupported(pvrSupported)
		, textureCache()
		, previousFrameEndTime(-1.0)
		, globalColor(Qt::white)
		, gl(glContext)
	{
		loaderThread = new QThread();
		loaderThread->start(QThread::LowestPriority);
	}
	
	virtual ~StelQGLRenderer()
	{
		invariant();
		loaderThread->quit();

		// This causes crashes for some reason 
		// (perhaps it is already destroyed by QT? - didn't find that in the docs).
		//
		// delete glContext;
		
		glContext   = NULL;

		loaderThread->wait();
		delete loaderThread;
		loaderThread = NULL;
	}
	
	virtual bool init()
	{
		//TODO Remove after StelPainter is no longer used.
		StelPainter::initSystemGLInfo(glContext);
		viewport.init(gl.hasOpenGLFeature(QGLFunctions::NPOTTextures));
		return true;
	}
	
	virtual QImage screenshot()
	{
		invariant();
		return viewport.screenshot();
	}
	
	virtual void enablePainting()
	{
		invariant();
		viewport.enablePainting();
		invariant();
	}
	
	virtual void disablePainting()
	{
		invariant();
		viewport.disablePainting();
		invariant();
	}

	virtual void renderFrame(StelRenderClient& renderClient);

	virtual void viewportHasBeenResized(const QSize size)
	{
		invariant();
		viewport.viewportHasBeenResized(size);
		invariant();
	}

	virtual QSize getViewportSize() const 
	{
		invariant();
		return viewport.getViewportSize();
	}
	
	virtual void bindTexture(StelTextureBackend* const textureBackend, const int textureUnit);

	virtual void destroyTextureBackend(StelTextureBackend* const textureBackend);

	virtual void setGlobalColor(const QColor& color)
	{
		globalColor = color;
	}
	
	//! Make Stellarium GL context the currently used GL context. Call this before GL calls.
	virtual void makeGLContextCurrent()
	{
		invariant();
		glContext->makeCurrent();
		invariant();
	}

	//! Used to access the GL context. 
	//!
	//! Safer than making glContext protected as derived classes can't overwrite it.
	QGLContext* getGLContext()
	{
		return glContext;
	}

	//! Used for GL/GLES-compatible function wrappers and to determine which GL features are available.
	QGLFunctions& getGLFunctions()
	{
		return gl;
	}

	//! Get a pointer to thread used for loading graphics data (e.g. textures).
	QThread* getLoaderThread()
	{
		return loaderThread;
	}

	//! Get global vertex color (used for drawing).
	const QColor& getGlobalColor() const
	{
		return globalColor;
	}

protected:
	virtual StelTextureBackend* createTextureBackend_
		(const QString& filename, const StelTextureParams& params, const TextureLoadingMode loadingMode);

	virtual StelTextureBackend* getViewportTextureBackend()
	{
		invariant();
		return viewport.getViewportTextureBackend(this);
	}
	
	//! Return the number of texture units (this is 1 if multitexturing is not supported).
	virtual int getTextureUnitCount() const = 0;

	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelGLRenderer::invariant().
	virtual void invariant() const
	{
		Q_ASSERT_X(NULL != glContext, Q_FUNC_INFO,
		           "An attempt to use a destroyed StelQGLRenderer.");
		Q_ASSERT_X(glContext->isValid(), Q_FUNC_INFO, "The GL context is invalid");
	}
	
private:
	//! OpenGL context.
	QGLContext* glContext;

	//! OpenGL viewport and related code (FBOs, if used, etc.).
	StelQGLViewport viewport;
	
	//! Are .pvr compressed textures supported on the platform we're running on?
	bool pvrSupported;

	//! Thread used for loading graphics data (e.g. textures).
	QThread* loaderThread;

	//! Caches textures to prevent duplicate loading.
	StelTextureCache<StelQGLTextureBackend> textureCache;
	
	//! Time the previous frame rendered by renderFrame ended.
	//!
	//! Negative at construction to detect the first frame.
	double previousFrameEndTime;

	//! Global vertex color.
	//! This color is used when rendering vertex formats that have no vertex color attribute.
	//!
	//! Per-vertex color completely overrides this 
	//! (this is to keep behavior from before the GL refactor unchanged).
	QColor globalColor;

	//! Draw the result of drawing commands to the window, applying given effect if possible.
	void drawWindow(StelViewportEffect* const effect);
	
	// Must be down due to initializer list order.
protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;
};

#endif // _STELQGLRENDERER_HPP_
