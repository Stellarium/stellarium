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
		, glWidget(new StelQGLWidget(glContext, parent))
		, painter(NULL)
		, pvrSupported(pvrSupported)
		, textureCache()
		, backBufferPainter(NULL)
		, defaultPainter(NULL)
		, frontBuffer(NULL)
		, backBuffer(NULL)
		, fboSupported(false)
		, fboDisabled(false)
		, viewportSize(QSize())
		, drawing(false)
		, gl(glContext)
	{
		loaderThread = new QThread();
		loaderThread->start(QThread::LowestPriority);
		glWidget->updateGL();
		parent->setViewport(glWidget);
	}
	
	virtual ~StelQGLRenderer()
	{
		Q_ASSERT_X(NULL == this->painter, Q_FUNC_INFO, 
		           "Painting is not disabled at destruction");

		loaderThread->quit();

		destroyFBOs();

		// This causes crashes for some reason 
		// (perhaps it is already destroyed by QT? - didn't find that in the docs).
		// delete glContext;
		
		glContext   = NULL;
		glWidget    = NULL;

		loaderThread->wait();
		delete loaderThread;
		loaderThread = NULL;
	}
	
	virtual bool init()
	{
		Q_ASSERT_X(glWidget->isValid(), Q_FUNC_INFO, 
		           "Invalid glWidget (maybe there is no OpenGL support?)");
		
		//TODO Remove after StelPainter is no longer used.
		StelPainter::initSystemGLInfo(glContext);
		
		// Prevent flickering on mac Leopard/Snow Leopard
		glWidget->setAutoFillBackground(false);

		fboSupported = QGLFramebufferObject::hasOpenGLFramebufferObjects();

		return true;
	}
	
	virtual QImage screenshot()
	{
		invariant();
		
		return glWidget->grabFrameBuffer();
	}
	
	virtual void enablePainting()
	{
		enablePainting(defaultPainter);
	}

	virtual void setDefaultPainter(QPainter* painter)
	{
		defaultPainter = painter;
	}
	
	virtual void disablePainting()
	{
		invariant();
		Q_ASSERT_X(NULL != this->painter, Q_FUNC_INFO, "Painting is already disabled");
		
		StelPainter::setQPainter(NULL);
		if(usingDefaultPainter)
		{
			delete(painter);
			usingDefaultPainter = false;
		}
		this->painter = NULL;
		invariant();
	}

	virtual void startDrawing()
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
			enablePainting(backBufferPainter);
		}
		else
		{
			enablePainting(defaultPainter);
		}
		invariant();
	}
	
	virtual void suspendDrawing()
	{
		invariant();
		disablePainting();
		
		if (useFBO())
		{
			//Release the backbuffer but don't swap it yet - we'll continue the drawing later.
			delete backBufferPainter;
			backBufferPainter = NULL;
			
			backBuffer->release();
		}
		drawing = false;
		invariant();
	}
	
	virtual void finishDrawing()
	{
		invariant();
		disablePainting();
		
		if (useFBO())
		{
			//Release the backbuffer and swap it to front.
			delete backBufferPainter;
			backBufferPainter = NULL;
			
			backBuffer->release();
			swapBuffersFBO();
		}
		drawing = false;
		invariant();
	}
	
	virtual void drawWindow(StelViewportEffect* effect)
	{
		invariant();

		//Warn about any GL errors.
		checkGLErrors();
		
		//Effects are ignored when FBO is not supported.
		//That might be changed for some GPUs, but it might not be worth the effort.
		
		//Put the result of drawing to the FBO on the screen, applying an effect.
		if (useFBO())
		{
			Q_ASSERT_X(!backBuffer->isBound() && !frontBuffer->isBound(), Q_FUNC_INFO, 
			           "Framebuffer objects loadweren't released before drawing the result");
		}
		enablePainting(defaultPainter);
		effect->drawToViewport(this);
		disablePainting();
		invariant();
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
		Q_ASSERT_X(NULL != glWidget && NULL != glContext, Q_FUNC_INFO, 
		           "destroyed StelQGLRenderer");
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
	//! Widget we're drawing to with OpenGL.
	StelQGLWidget* glWidget;
	
	//! Painter we're using when painting is enabled. NULL otherwise.
	QPainter* painter;
	//! Are we using default-constructed painter?
	bool usingDefaultPainter;

	//! Are .pvr compressed textures supported on the platform we're running on?
	bool pvrSupported;

	//! Thread used for loading graphics data (e.g. textures).
	QThread* loaderThread;

	//! Caches textures to prevent duplicate loading.
	StelTextureCache<StelQGLTextureBackend> textureCache;

	//! Painter to the FBO we're drawing to, when using FBOs.
	QPainter* backBufferPainter;
	
	//! Painter we're using when not drawing to an FBO. 
	QPainter* defaultPainter;
	
	//! Frontbuffer (i.e. displayed at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* frontBuffer;
	
	//! Backbuffer (i.e. drawn to at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* backBuffer;

	//! Are frame buffer objects supported on this system?
	bool fboSupported;
	
	//! Disable frame buffer objects even if supported?
	bool fboDisabled;
	
	//! Graphics scene size.
	QSize viewportSize;
	
	//! Are we in the middle of drawing?
	bool drawing;

	//! Enable painting using specified painter (or a construct a fallback painter if NULL).
	void enablePainting(QPainter* painter)
	{
		invariant();
		Q_ASSERT_X(NULL == this->painter, Q_FUNC_INFO, "Painting is already enabled");
		
		// If no painter specified, create a default one painting to the glWidget.
		if(painter == NULL)
		{
			this->painter = new QPainter(glWidget);
			usingDefaultPainter = true;
			StelPainter::setQPainter(this->painter);
			return;
		}
		this->painter = painter;
		StelPainter::setQPainter(this->painter);
		invariant();
	}
	
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

	// Must be down due to initializer list order.
protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;
};

#endif // _STELQGLRENDERER_HPP_
