#ifndef _STELQGLPROVIDER_HPP_
#define _STELQGLPROVIDER_HPP_

#include <QGLContext>
#include <QGLWidget>
#include <QImage>
#include <QPaintEngine>
#include <QPainter>
#include <QString>
#include <QWidget>

#include "StelGLProvider.hpp"
#include "StelPainter.hpp"



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



//! Qt-based StelGLProvider. Handles all needed QGL functionality.
class StelQGLProvider : public QObject, public StelGLProvider
{
public:
	explicit StelQGLProvider(QGraphicsView* parent) 
	    : QObject(parent)
	    , glContext(new QGLContext(QGLFormat(QGL::StencilBuffer | 
	                                         QGL::DepthBuffer   |
	                                         QGL::DoubleBuffer)))
	    , glWidget(new StelQGLWidget(glContext, parent))
		, painter(NULL)
	    , usingDefaultPainter(false)
		, initialized(false)
	{
		glWidget->updateGL();
		parent->setViewport(glWidget);
	}
	
	virtual void init()
	{
		Q_ASSERT_X(!initialized, "StelQGLProvider::init()", 
		           "StelQGLProvider is already initialized");
		initialized = true;
		invariant();
		Q_ASSERT_X(glWidget->isValid(), "StelQGLProvider::init()", 
		           "Invalid glWidget (maybe there is no OpenGL support?)");
		
		glWidget->makeCurrent();
		StelPainter::initSystemGLInfo(glContext);
		// Prevent flickering on mac Leopard/Snow Leopard
		glWidget->setAutoFillBackground (false);
	}
	
	virtual QImage grabFrameBuffer()
	{
		Q_ASSERT(glWidget != NULL && glContext != NULL);
		invariant();
		
		return glWidget->grabFrameBuffer();
	}
	
	virtual void enablePainting(QPainter* painter)
	{
		invariant();
		Q_ASSERT_X(NULL == this->painter, "StelQGLProvider::enablePainting()", 
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
	}
	
	virtual void disablePainting()
	{
		invariant();
		Q_ASSERT_X(NULL != this->painter, "StelQGLProvider::disablePainting()", 
		           "Painting is already disabled");
		
		StelPainter::setQPainter(NULL);
		if(usingDefaultPainter)
		{
			delete(painter);
			usingDefaultPainter = false;
		}
		this->painter = NULL;
	}
	
	virtual ~StelQGLProvider()
	{
		Q_ASSERT_X(NULL == this->painter, "StelQGLProvider::~StelQGLProvider()", 
		           "Painting is not disabled at destruction");
		invariant();
		
		
		// This causes crashes for some reason 
		// (perhaps it is already destroyed by QT? - didn't find that in the docs).
		
		//delete glContext;
		
		glContext   = NULL;
		glWidget    = NULL;
		initialized = false;
	}
	
private:
	//! Asserts that we're in a valid state.
	void invariant() const
	{
		Q_ASSERT_X(NULL != glWidget && NULL != glContext, "StelQGLProvider::invariant()", 
		           "destroyed StelQGLProvider");
		Q_ASSERT_X(initialized, "StelQGLProvider::invariant()", 
		           "uninitialized StelQGLProvider");
	}
	
	//! OpenGL context.
	QGLContext* glContext;
	//! Widget we're drawing with OpenGL to.
	StelQGLWidget* glWidget;
	//! Painter when paiting is enabled. NULL otherwise.
	QPainter* painter;
	//! Are we using default-constructed painter?
	bool usingDefaultPainter;
	//! Are we initialized (i.e. was init() called)?
	bool initialized;
};

#endif // _STELQGLPROVIDERHPP_