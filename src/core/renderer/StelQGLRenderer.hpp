#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


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

//TEMP
#include "StelCore.hpp"


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
		, gl(glContext)
	{
		loaderThread = new QThread();
		loaderThread->start(QThread::LowestPriority);
	}
	
	virtual ~StelQGLRenderer()
	{
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
		return viewport.screenshot();
	}
	
	virtual void enablePainting()
	{
		viewport.enablePainting();
	}
	
	virtual void disablePainting()
	{
		viewport.disablePainting();
	}

	virtual void renderFrame(StelRenderClient& renderClient)
	{
		if(previousFrameEndTime < 0.0)
		{
			previousFrameEndTime = StelApp::getTotalRunTime();
		}

		viewport.setDefaultPainter(renderClient.getPainter());

		makeGLContextCurrent();
		viewport.startFrame();

		// When using the GUI, try to have the best reactivity, 
		// even if we need to lower the FPS.
		const int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;

		while (true)
		{
			const bool keepDrawing = renderClient.drawPartial();
			if(!keepDrawing) 
			{
				viewport.finishFrame();
				break;
			}

			const double spentTime = StelApp::getTotalRunTime() - previousFrameEndTime;

			// We need FBOs to do partial drawing.
			if (viewport.useFBO() && 1. / spentTime <= minFps)
			{
				// We stop the painting operation for now
				viewport.suspendFrame();
				break;
			}
		}
		
		drawWindow(renderClient.getViewportEffect());
		viewport.setDefaultPainter(NULL);
		
		previousFrameEndTime = StelApp::getTotalRunTime();
	}

	virtual void viewportHasBeenResized(const QSize size)
	{
		invariant();
		viewport.viewportHasBeenResized(size);
		invariant();
	}

	virtual QSize getViewportSize() const {return viewport.getViewportSize();}
	
	virtual void bindTexture(StelTextureBackend* const textureBackend, const int textureUnit)
	{
		StelQGLTextureBackend* qglTextureBackend =
			dynamic_cast<StelQGLTextureBackend*>(textureBackend);
		Q_ASSERT_X(qglTextureBackend != NULL, Q_FUNC_INFO,
		           "Trying to bind a texture created by a different renderer backend");

		const TextureStatus status = qglTextureBackend->getStatus();
		if(status == TextureStatus_Loaded)
		{
			// Ignore the texture if we don't have enough texture units
			// or if texture unit is nonzero and we don't support multitexturing.
			if(textureUnit >= getTextureUnitCount())
			{
				return;
			}
			qglTextureBackend->bind(textureUnit);
		}
		if(status == TextureStatus_Uninitialized)
		{
			qglTextureBackend->startAsynchronousLoading();
		}
	}

	virtual void destroyTextureBackend(StelTextureBackend* const textureBackend)
	{
		StelQGLTextureBackend* qglTextureBackend =
			dynamic_cast<StelQGLTextureBackend*>(textureBackend);
		Q_ASSERT_X(qglTextureBackend != NULL, Q_FUNC_INFO,
		           "Trying to destroy a texture created by a different renderer backend");

		if(textureBackend->getName().isEmpty())
		{
			delete textureBackend;
			return;
		}
		textureCache.remove(qglTextureBackend);
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

protected:
	virtual StelTextureBackend* createTextureBackend_
		(const QString& filename, const StelTextureParams& params, const TextureLoadingMode loadingMode);

	virtual StelTextureBackend* getViewportTextureBackend()
	{
		return viewport.getViewportTextureBackend(this);
	}
	
	//! Return the number of texture units (this is 1 if multitexturing is not supported).
	virtual int getTextureUnitCount() const = 0;

	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelGLRenderer::invariant().
	virtual void invariant()
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

	//! Draw the result of drawing commands to the window, applying given effect if possible.
	void drawWindow(StelViewportEffect* const effect)
	{
		// At this point, FBOs have been released (if using FBOs), so we're drawing 
		// directly to the screen. The StelViewportEffect::drawToViewport call 
		// actually draws puts the rendered result onto the viewport.
		invariant();

		//Warn about any GL errors.
		checkGLErrors();
		
		//Effects are ignored when FBO is not supported.
		//That might be changed for some GPUs, but it might not be worth the effort.
		
		viewport.prepareToDrawViewport();
		viewport.enablePainting();

		if(NULL == effect)
		{
			// If using FBO, we still need to put it on the screen.
			if(viewport.useFBO())
			{
				StelTexture* screenTexture = getViewportTexture();
				int texWidth, texHeight;
				screenTexture->getDimensions(texWidth, texHeight);

				StelPainter sPainter(StelApp::getInstance().getCore()->getProjection2d());
				sPainter.setColor(1,1,1);

				sPainter.enableTexture2d(true);

				screenTexture->bind();

				sPainter.drawRect2d(0, 0, texWidth, texHeight);
			}
			// Not using FBO, the result is already drawn to the screen.
		}
		else
		{
			effect->drawToViewport(this);
		}

		disablePainting();
		invariant();
	}
	
	// Must be down due to initializer list order.
protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;
};

#endif // _STELQGLRENDERER_HPP_
