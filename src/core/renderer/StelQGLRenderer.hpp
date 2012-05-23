#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


#include "StelGLRenderer.hpp"
#include "StelPainter.hpp"

#include <QGLWidget>
#include <QGLFunctions>
#include <QGraphicsView>


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
		case QPaintEngine::OpenGL:
			paintEngineStr = "OpenGL";
			break;
		case QPaintEngine::OpenGL2:
			paintEngineStr = "OpenGL2";
			break;
		default:
			paintEngineStr = "Other";
		}
		qDebug() << "Qt GL paint engine is: " << paintEngineStr;
	}
};

//TODO get rid of all StelPainter calls (gradually)

//! Base class for renderer based on OpenGL and at the same time Qt's QGL.
class StelQGLRenderer : public StelGLRenderer
{
public:
	//! Construct a StelQGLRenderer whose GL widget will be a child of specified QGraphicsView.
	StelQGLRenderer(QGraphicsView* parent)
		: glContext(new QGLContext(QGLFormat(QGL::StencilBuffer | 
		                                     QGL::DepthBuffer   |
		                                     QGL::DoubleBuffer)))
		, glWidget(new StelQGLWidget(glContext, parent))
		, painter(NULL)
		, gl(glContext)
	{
		glWidget->updateGL();
		parent->setViewport(glWidget);
	}
	
	virtual ~StelQGLRenderer()
	{
		Q_ASSERT_X(NULL == this->painter, "StelQGLRenderer::~StelQGLRenderer()", 
		           "Painting is not disabled at destruction");
		// This causes crashes for some reason 
		// (perhaps it is already destroyed by QT? - didn't find that in the docs).
		
		//delete glContext;
		
		glContext   = NULL;
		glWidget    = NULL;
	}
	
	virtual bool init()
	{
		Q_ASSERT_X(glWidget->isValid(), "StelQGLRenderer::init()", 
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
		Q_ASSERT_X(NULL != this->painter, "StelQGLRenderer::disablePainting()", 
		           "Painting is already disabled");
		
		StelPainter::setQPainter(NULL);
		if(usingDefaultPainter)
		{
			delete(painter);
			usingDefaultPainter = false;
		}
		this->painter = NULL;
		invariant();
	}

protected:
	virtual void enablePainting(QPainter* painter)
	{
		invariant();
		Q_ASSERT_X(NULL == this->painter, "StelQGLRenderer::enablePainting()", 
		           "Painting is already enabled");
		
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
	
	virtual void makeGLContextCurrent()
	{
		invariant();
		glContext->makeCurrent();
		invariant();
	}
	
protected:
	//! Used to access the GL context. 
	//!
	//! Safer than making glContext protected as derived classes can't overwrite it.
	QGLContext* getGLContext()
	{
		return glContext;
	}
	
	virtual void invariant()
	{
		Q_ASSERT_X(NULL != glWidget && NULL != glContext, "StelQGLRenderer::invariant()", 
		           "destroyed StelQGLRenderer");
		Q_ASSERT_X(glContext->isValid(), "StelGLRenderer::invariant", 
		           "Our GL context is invalid");
		StelGLRenderer::invariant();
	}
	
private:
	//! OpenGL context..
	QGLContext* glContext;
	//! Widget we're drawing to with OpenGL.
	StelQGLWidget* glWidget;
	
	//! Painter we're using when painting is enabled. NULL otherwise.
	QPainter* painter;
	//! Are we using default-constructed painter?
	bool usingDefaultPainter;

protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;
};

#endif // _STELQGLRENDERER_HPP_

