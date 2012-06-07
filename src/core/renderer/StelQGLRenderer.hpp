#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


#include <QGLFramebufferObject>
#include <QGLFunctions>
#include <QGLWidget>
#include <QGraphicsView>
#include <QImage>
#include <QPainter>
#include <QThread>

#include "StelRenderer.hpp"
#include "StelPainter.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelTextureCache.hpp"
#include "StelVertexBuffer.hpp"
#include "StelUtils.hpp"

#include "StelApp.hpp"
#include "StelGuiBase.hpp"

//TEMP
#include "StelCore.hpp"

//! GLWidget specialized for Stellarium, mostly to provide better debugging information.
class StelQGLWidget : public QGLWidget
{
public:
	StelQGLWidget(QGLContext* ctx, QWidget* parent) : QGLWidget(ctx, parent)
	{
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_OpaquePaintEvent);
		//setAutoFillBackground(false);
		setBackgroundRole(QPalette::Window);
	}

protected:
	virtual void initializeGL()
	{
		qDebug() << "OpenGL supported version: " << QString((char*)glGetString(GL_VERSION));

		QGLWidget::initializeGL();

		if (!format().stencil())
			qWarning("Could not get stencil buffer; results will be suboptimal");
		if (!format().depth())
			qWarning("Could not get depth buffer; results will be suboptimal");
		if (!format().doubleBuffer())
			qWarning("Could not get double buffer; results will be suboptimal");

		QString paintEngineStr;
		switch (paintEngine()->type())
		{
			case QPaintEngine::OpenGL:  paintEngineStr = "OpenGL"; break;
			case QPaintEngine::OpenGL2: paintEngineStr = "OpenGL2"; break;
			default:                    paintEngineStr = "Other";
		}
		qDebug() << "Qt GL paint engine is: " << paintEngineStr;
	}
};

//! Manages OpenGL viewport.
//!
//! This class handles things like framebuffers and Qt-style painting to the viewport.
class StelQGLViewport
{
	//TODO FBOs
	//TODO viewport extents
	//TODO drawing bool 
public:
	//! Initialize the viewport.
	void init()
	{
		invariant();
		// Prevent flickering on mac Leopard/Snow Leopard
		glWidget->setAutoFillBackground(false);
		invariant();
	}

	//! Grab a screenshot.
	QImage screenshot() const 
	{
		invariant();
		return glWidget->grabFrameBuffer();
	}

	//! Set the default painter to use when not drawing to FBO.
	void setDefaultPainter(QPainter* painter)
	{
		defaultPainter = painter;
	}

	//! Disable Qt-style painting.
	void disablePainting()
	{
		invariant();
		Q_ASSERT_X(NULL != painter, Q_FUNC_INFO, "Painting is already disabled");
		
		StelPainter::setQPainter(NULL);
		if(usingGLWidgetPainter)
		{
			delete(painter);
			usingGLWidgetPainter = false;
		}
		painter = NULL;
		invariant();
	}

	//! Enable Qt-style painting (with the current default painter, or constructing a fallback if no default).
	void enablePainting()
	{
		enablePainting(defaultPainter);
	}

	//! Enable Qt-style painting with specified painter (or construct a fallback painter if NULL).
	void enablePainting(QPainter* painter)
	{
		invariant();
		Q_ASSERT_X(NULL == this->painter, Q_FUNC_INFO, "Painting is already enabled");
		
		// If no painter specified, create a default one painting to the glWidget.
		if(painter == NULL)
		{
			this->painter = new QPainter(glWidget);
			usingGLWidgetPainter = true;
			StelPainter::setQPainter(this->painter);
			return;
		}
		this->painter = painter;
		StelPainter::setQPainter(this->painter);
		invariant();
	}
	
	//! Asserts that we're in a valid state.
	void invariant() const
	{
		Q_ASSERT_X(NULL != glWidget, Q_FUNC_INFO, "Destroyed StelQGLViewport");
		Q_ASSERT_X(glWidget->isValid(), Q_FUNC_INFO, 
		           "Invalid glWidget (maybe there is no OpenGL support?)");
	}
	
private:
	//! Only StelQGLRenderer can construct a viewport.
	friend class StelQGLRenderer;
	//! Widget we're drawing to with OpenGL.
	StelQGLWidget* glWidget;
	//! Painter we're currently using to paint. NULL if painting is disabled.
	QPainter* painter;
	//! Painter we're using when not drawing to an FBO. 
	QPainter* defaultPainter;
	//! Are we using the fallback painter (directly to glWidget) ?
	bool usingGLWidgetPainter;

	//! Construct a StelQGLViewport using specified widget.
	//!
	//! @param glWidget GL widget that contains the viewport.
	//! @param parent Parent widget of glWidget.
	StelQGLViewport(StelQGLWidget* glWidget, QGraphicsView* parent)
		: glWidget(glWidget)
		, painter(NULL)
		, defaultPainter(NULL)
		, usingGLWidgetPainter(false)
	{
		// Forces glWidget to initialize GL.
		glWidget->updateGL();
		parent->setViewport(glWidget);
		invariant();
	}

	//! Destroy the StelQGLViewport.
	~StelQGLViewport()
	{
		invariant();

		Q_ASSERT_X(NULL == this->painter, Q_FUNC_INFO, 
		           "Painting is not disabled at destruction");

		// No need to delete the GL widget, its parent widget will do that.
		glWidget = NULL;
	}
};

//TODO get rid of all StelPainter calls (gradually)

//! Base class for renderer based on OpenGL and at the same time Qt's QGL.
class StelQGLRenderer : public StelRenderer
{
public:
	//! Construct a StelQGLRenderer.
	//!
	//! @param parent       Parent widget for the renderer's GL widget.
	//! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
	StelQGLRenderer(QGraphicsView* parent, bool pvrSupported)
		: StelRenderer()
		, glContext(new QGLContext(QGLFormat(QGL::StencilBuffer | 
		                                     QGL::DepthBuffer   |
		                                     QGL::DoubleBuffer)))
		, viewport(new StelQGLWidget(glContext, parent), parent)
		, pvrSupported(pvrSupported)
		, textureCache()
		, backBufferPainter(NULL)
		, frontBuffer(NULL)
		, backBuffer(NULL)
		, fboSupported(false)
		, fboDisabled(true)
		, viewportSize(QSize())
		, drawing(false)
		, previousFrameEndTime(-1.0)
		, gl(glContext)
	{
		loaderThread = new QThread();
		loaderThread->start(QThread::LowestPriority);
	}
	
	virtual ~StelQGLRenderer()
	{
		loaderThread->quit();

		destroyFBOs();

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
		viewport.init();

		//TODO Remove after StelPainter is no longer used.
		StelPainter::initSystemGLInfo(glContext);

		fboSupported = QGLFramebufferObject::hasOpenGLFramebufferObjects();

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
		startDrawing();

		// When using the GUI, try to have the best reactivity, 
		// even if we need to lower the FPS.
		const int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;

		while (true)
		{
			const bool keepDrawing = renderClient.drawPartial();
			if(!keepDrawing) 
			{
				finishDrawing();
				break;
			}

			const double spentTime = StelApp::getTotalRunTime() - previousFrameEndTime;

			// We need FBOs to do partial drawing.
			if (useFBO() && 1. / spentTime <= minFps)
			{
				// We stop the painting operation for now
				suspendDrawing();
				break;
			}
		}
		
		drawWindow(renderClient.getViewportEffect());
		viewport.setDefaultPainter(NULL);
		
		previousFrameEndTime = StelApp::getTotalRunTime();
	}

	virtual void viewportHasBeenResized(QSize size)
	{
		invariant();
		//Can't check this in invariant because Renderer is initialized before 
		//AppGraphicsWidget sets its viewport size
		Q_ASSERT_X(size.isValid(), Q_FUNC_INFO, "Invalid scene size");
		viewportSize = size;
		//We'll need FBOs of different size so get rid of the current FBOs.
		destroyFBOs();
		invariant();
	}

	virtual QSize getViewportSize() const {return viewportSize;}
	
	virtual void bindTexture(StelTextureBackend* textureBackend, const int textureUnit)
	{
		StelQGLTextureBackend* qglTextureBackend =
			dynamic_cast<StelQGLTextureBackend*>(textureBackend);
		Q_ASSERT_X(qglTextureBackend != NULL, Q_FUNC_INFO,
		           "Trying to bind a texture created by a different renderer backend");

		const TextureStatus status = qglTextureBackend->getStatus();
		if(status == TextureStatus_Loaded)
		{
			if(gl.hasOpenGLFeature(QGLFunctions::Multitexture) && textureUnit !=0)
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

	virtual void destroyTextureBackend(StelTextureBackend* textureBackend)
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

	virtual StelTextureBackend* getViewportTextureBackend();
	
	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelGLRenderer::invariant().
	virtual void invariant()
	{
		viewport.invariant();
		Q_ASSERT_X(NULL != glContext, Q_FUNC_INFO, "destroyed StelQGLRenderer");
		Q_ASSERT_X(glContext->isValid(), Q_FUNC_INFO, "Our GL context is invalid");

		const bool fbo = useFBO();
		Q_ASSERT_X(NULL == backBufferPainter || fbo, Q_FUNC_INFO,
		           "We have a backbuffer painter even though we're not using FBO");
		Q_ASSERT_X(drawing && fbo ? backBufferPainter != NULL : true, Q_FUNC_INFO,
		           "We're drawing and using FBOs, but the backBufferPainter is NULL");
		Q_ASSERT_X(NULL == backBuffer || fbo, Q_FUNC_INFO,
		           "We have a backbuffer even though we're not using FBO");
		Q_ASSERT_X(NULL == frontBuffer || fbo, Q_FUNC_INFO,
		           "We have a frontbuffer even though we're not using FBO");
		Q_ASSERT_X(drawing && fbo ? backBuffer != NULL : true, Q_FUNC_INFO,
		           "We're drawing and using FBOs, but the backBuffer is NULL");
		Q_ASSERT_X(drawing && fbo ? frontBuffer != NULL : true, Q_FUNC_INFO,
		           "We're drawing and using FBOs, but the frontBuffer is NULL");
	}
	
private:
	//! OpenGL context.
	QGLContext* glContext;

	StelQGLViewport viewport;
	
	//! Are .pvr compressed textures supported on the platform we're running on?
	bool pvrSupported;

	//! Thread used for loading graphics data (e.g. textures).
	QThread* loaderThread;

	//! Caches textures to prevent duplicate loading.
	StelTextureCache<StelQGLTextureBackend> textureCache;

	//! Painter to the FBO we're drawing to, when using FBOs.
	QPainter* backBufferPainter;

	//! Frontbuffer (i.e. displayed at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* frontBuffer;
	
	//! Backbuffer (i.e. drawn to at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* backBuffer;

	//! Are frame buffer objects supported on this system?
	bool fboSupported;
	
	//! Disable frame buffer objects even if supported?
	//!
	//! Currently, this is only used for debugging. 
	//! It might be loaded from a config file later.
	bool fboDisabled;
	
	//! Graphics scene size.
	QSize viewportSize;
	
	//! Are we in the middle of drawing?
	bool drawing;

	//! Time the previous frame rendered by renderFrame ended.
	//!
	//! Negative at construction to detect the first frame.
	double previousFrameEndTime;

	//! Are we using framebuffer objects?
	bool useFBO() const
	{
		return fboSupported && !fboDisabled;
	}
	
	//! Initialize the frame buffer objects.
	void initFBO()
	{
		Q_ASSERT_X(useFBO(), Q_FUNC_INFO, "We're not using FBO");
		if (NULL == backBuffer)
		{
			Q_ASSERT_X(NULL == frontBuffer, Q_FUNC_INFO, 
			           "frontBuffer is not null even though backBuffer is");

			const bool npot = gl.hasOpenGLFeature(QGLFunctions::NPOTTextures);

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

	//! Destroy FBOs, if used.
	void destroyFBOs()
	{
		// Destroy framebuffers
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

	//! Start using drawing calls.
	void startDrawing()
	{
		invariant();
		makeGLContextCurrent();
		
		drawing = true;
		if (useFBO())
		{
			//Draw to backBuffer.
			initFBO();
			backBuffer->bind();
			backBufferPainter = new QPainter(backBuffer);
			viewport.enablePainting(backBufferPainter);
		}
		else
		{
			viewport.enablePainting();
		}
		invariant();
	}
	
	// Separate from finishDrawing only for readability
	//! Suspend drawing, not showing the result on the screen.
	//!
	//! Finishes using draw calls for this frame. 
	//! Drawing can continue later. Only usable with FBOs.
	void suspendDrawing() {finishDrawing(true);}
	
	//! Finish using draw calls.
	void finishDrawing(bool swapBuffers = true)
	{
		invariant();
		disablePainting();
		
		if (useFBO())
		{
			//Release the backbuffer.
			delete backBufferPainter;
			backBufferPainter = NULL;
			
			backBuffer->release();
			//Swap buffers if finishing, don't swap yet if suspending.
			if(swapBuffers){swapBuffersFBO();}
		}
		drawing = false;
		invariant();
	}
	
	//! Draw the result of drawing commands to the window, applying given effect if possible.
	void drawWindow(StelViewportEffect* effect)
	{
		// At this point, FBOs are released (if using FBOs), so we're drawing 
		// directly to the screen. The StelViewportEffect::drawToViewport call 
		// actually draws puts the rendered result onto the viewport.
		invariant();

		//Warn about any GL errors.
		checkGLErrors();
		
		//Effects are ignored when FBO is not supported.
		//That might be changed for some GPUs, but it might not be worth the effort.
		
		//Put the result of drawing to the FBO on the screen, applying an effect.
		if (useFBO())
		{
			Q_ASSERT_X(!backBuffer->isBound() && !frontBuffer->isBound(), Q_FUNC_INFO, 
			           "Framebuffer objects weren't released before drawing the result");
		}
		viewport.enablePainting();

		if(NULL == effect)
		{
			// If using FBO, we still need to put it on the screen.
			if(useFBO())
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
