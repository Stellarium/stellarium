#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


#include <QGLFunctions>
#include <QGLWidget>
#include <QGraphicsView>
#include <QThread>

#include "StelGLRenderer.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelPainter.hpp"
#include "StelTextureCache.hpp"


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
class StelQGLRenderer : public StelGLRenderer
{
public:
	//! Construct a StelQGLRenderer.
	//!
	//! @param parent       Parent widget for the renderer's GL widget.
	//! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
	StelQGLRenderer(QGraphicsView* parent, bool pvrSupported)
		: glContext(new QGLContext(QGLFormat(QGL::StencilBuffer | 
		                                     QGL::DepthBuffer   |
		                                     QGL::DoubleBuffer)))
		, glWidget(new StelQGLWidget(glContext, parent))
		, painter(NULL)
		, pvrSupported(pvrSupported)
		, textureCache()
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
		// This causes crashes for some reason 
		// (perhaps it is already destroyed by QT? - didn't find that in the docs).
		
		//delete glContext;
		
		// Hopefully this doesn't take much time.
		loaderThread->quit();
		loaderThread->wait();
		delete loaderThread;
		loaderThread = NULL;

		glContext   = NULL;
		glWidget    = NULL;
	}
	
	virtual bool init()
	{
		Q_ASSERT_X(glWidget->isValid(), Q_FUNC_INFO, 
		           "Invalid glWidget (maybe there is no OpenGL support?)");
		
		//TODO Remove after StelPainter is no longer used.
		StelPainter::initSystemGLInfo(glContext);
		
		// Prevent flickering on mac Leopard/Snow Leopard
		glWidget->setAutoFillBackground(false);
		return StelGLRenderer::init();
	}
	
	virtual QImage screenshot()
	{
		invariant();
		
		return glWidget->grabFrameBuffer();
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
	
	virtual void makeGLContextCurrent()
	{
		invariant();
		glContext->makeCurrent();
		invariant();
	}

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

	virtual void enablePainting(QPainter* painter)
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
	
	virtual void invariant()
	{
		Q_ASSERT_X(NULL != glWidget && NULL != glContext, Q_FUNC_INFO, 
		           "destroyed StelQGLRenderer");
		Q_ASSERT_X(glContext->isValid(), Q_FUNC_INFO, "Our GL context is invalid");
		StelGLRenderer::invariant();
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
protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;
};

#endif // _STELQGLRENDERER_HPP_

