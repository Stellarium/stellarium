/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELQGLVIEWPORT_HPP_
#define _STELQGLVIEWPORT_HPP_

#include <QGLFramebufferObject>
#include <QGLWidget>
#include <QGraphicsView>
#include <QImage>
#include <QPainter>

#include "StelUtils.hpp"

//! GLWidget specialized for Stellarium, mainly to provide better debugging information.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
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

	//! Get the Qt paint engine type used.
	QPaintEngine::Type getPaintEngineType() const
	{
		return paintEngineType;
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
		paintEngineType = paintEngine()->type();
		switch (paintEngineType)
		{
			case QPaintEngine::OpenGL:  paintEngineStr = "OpenGL"; break;
			case QPaintEngine::OpenGL2: paintEngineStr = "OpenGL2"; break;
			default:                    paintEngineStr = "Other";
		}
		qDebug() << "Qt GL paint engine is: " << paintEngineStr;
	}

	//! Qt paint engine type used.
	QPaintEngine::Type paintEngineType;
};

//! Manages OpenGL viewport.
//!
//! This class handles things like framebuffers and Qt painting to the viewport.
class StelQGLViewport
{
public:
	//! Construct a StelQGLViewport using specified widget.
	//!
	//! @param glWidget GL widget that contains the viewport.
	//! @param parent Parent widget of glWidget.
	StelQGLViewport(StelQGLWidget* const glWidget, QGraphicsView* const parent)
		: viewportSize(QSize())
		, glWidget(glWidget)
		, painter(NULL)
		, defaultPainter(NULL)
		, backBufferPainter(NULL)
		, frontBuffer(NULL)
		, backBuffer(NULL)
		, drawing(false)
		, usingGLWidgetPainter(false)
		, fboSupported(false)
		, fboDisabled(false)
		, nonPowerOfTwoTexturesSupported(false)
	{
		// Forces glWidget to initialize GL.
		glWidget->updateGL();

		// Qt GL2 paint engine does some FBO magic depending 
		// on glBlendFunc having a particular value. I wasn't able 
		// to figure out what value it was, so another workaround 
		// was to not use FBOs ourselves.
		// Not sure _why exactly_ this helps though.
		//
		// TODO
		// If Qt is newer than 4.0, try removing this if - 
		// FBOs are an important feature so we want to use it 
		// if the Qt people cleaned up their mess.
		if(qtPaintEngineType() == QPaintEngine::OpenGL2)
		{
			fboDisabled = true;
		}
		parent->setViewport(glWidget);
		invariant();
	}

	//! Destroy the StelQGLViewport.
	~StelQGLViewport()
	{
		invariant();
		Q_ASSERT_X(NULL == this->painter, Q_FUNC_INFO, 
		           "Painting is not disabled at destruction");

		destroyFBOs();
		// No need to delete the GL widget, its parent widget will do that.
		glWidget = NULL;
	}

	//! Initialize the viewport.
	//!
	//! @param npot       Are non-power-of-two textures supported?
	//! @param glVendor   The GL_VENDOR string (usually specifies the GPU driver).
	//! @param glRenderer The GL_RENDERER string (usually specifies the GPU).
	void init(const bool npot, const QString& glVendor, const QString& glRenderer)
	{
		invariant();
		this->nonPowerOfTwoTexturesSupported = npot;
		// Prevent flickering on mac Leopard/Snow Leopard
		glWidget->setAutoFillBackground(false);

		// In some virtual machines.
		// Not all needed FBO functionality needed is supported.
		if(glRenderer == "Chromium")
		{
			fboDisabled = true;
		}
		if(glVendor == "Tungsten Graphics, Inc")
		{
			if(glRenderer.contains("945") || 
			   glRenderer.contains("810") || 
			   glRenderer.contains("845") || 
			   glRenderer.contains("855") || 
			   glRenderer.contains("865") || 
			   glRenderer.contains("915") || 
			   glRenderer.contains("946") || 
			   glRenderer.contains("500") || 
			   glRenderer.contains("965") || 
			   glRenderer.contains("950") || 
			   glRenderer.contains("X3100") || 
			   glRenderer.contains("GM45") || 
			   glRenderer.contains("Ironlake") || 
			   glRenderer.contains("G33") || 
			   glRenderer.contains("G41") || 
			   glRenderer.contains("IGD"))
			{
				fboDisabled = true;
			}
		}

		fboSupported = QGLFramebufferObject::hasOpenGLFramebufferObjects();
		invariant();
	}

	//! Get paint engine type used for Qt drawing.
	QPaintEngine::Type qtPaintEngineType() const
	{
		return glWidget->getPaintEngineType();
	}

	//! Called when viewport size changes so we can replace the FBOs.
	void viewportHasBeenResized(const QSize newSize)
	{
		invariant();
		//Can't check this in invariant because Renderer is initialized before 
		//AppGraphicsWidget sets its viewport size
		Q_ASSERT_X(newSize.isValid(), Q_FUNC_INFO, "Invalid scene size");
		viewportSize = newSize;
		//We'll need FBOs of different size so get rid of the current FBOs.
		destroyFBOs();
		invariant();
	}
	
	//! Set the default painter to use when not drawing to FBO.
	//!
	//! This is the only place where we might be getting a painter from outside world,
	//! so we require that it uses a GL or GL2 paint engine.
	//!
	//! @param painter Painter to set.
	//! @param context GL context used by the renderer, for error checking.
	void setDefaultPainter(QPainter* const painter, const QGLContext* const context)
	{
		invariant();

#ifndef NDEBUG
		if(NULL != painter)
		{
			Q_ASSERT_X(painter->paintEngine()->type() == QPaintEngine::OpenGL ||
			           painter->paintEngine()->type() == QPaintEngine::OpenGL2,
			           Q_FUNC_INFO, 
			           "QGL StelRenderer backends need a QGLWidget to be set as the "
			           "viewport on the graphics view");
			QGLWidget* widget = dynamic_cast<QGLWidget*>(painter->device());
			Q_ASSERT_X(NULL != widget && widget->context() == context, Q_FUNC_INFO,
			           "Painter used with QGL StelRenderer backends needs to paint on a QGLWidget "
			           "with the same GL context as the one used by the renderer");
		}
#else
		Q_UNUSED(context);
#endif

		defaultPainter = painter;
		invariant();
	}

	//! Grab a screenshot.
	QImage screenshot() const 
	{
		invariant();
		return glWidget->grabFrameBuffer();
	}

	//! Return current viewport size in pixels.
	QSize getViewportSize() const 
	{
		invariant();
		return viewportSize;
	}

	//! Get a texture of the viewport.
	StelTextureBackend* getViewportTextureBackend(class StelQGLRenderer* renderer);

	//! Are we using framebuffer objects?
	bool useFBO() const
	{
		// Can't call invariant here as invariant calls useFBO
		return fboSupported && !fboDisabled;
	}

	//! Set font to use for drawing text.
	//!
	//! Can be only called when Qt painting is enabled.
	void setFont(const QFont& font)
	{
		Q_ASSERT_X(NULL != painter, Q_FUNC_INFO,
		           "Trying to set text (painting) font but painting is disabled");
		painter->setFont(font);
	}

	//! Start using drawing calls.
	void startFrame()
	{
		invariant();
		if (useFBO())
		{
			//Draw to backBuffer.
			initFBOs();
			backBuffer->bind();
			backBufferPainter = new QPainter(backBuffer);
			enablePainting(backBufferPainter);
		}
		else
		{
			enablePainting();
		}
		drawing = true;
		invariant();
	}

	//! Suspend drawing, not showing the result on the screen.
	//!
	//! Finishes using draw calls for this frame. 
	//! Drawing can continue later. Only usable with FBOs.
	void suspendFrame() 
	{
		Q_ASSERT_X(useFBO(), Q_FUNC_INFO, "Partial rendering only works with FBOs");
		finishFrame(false);
	}
	
	//! Finish using draw calls.
	void finishFrame(const bool swapBuffers = true)
	{
		invariant();
		drawing = false;

		disablePainting();
		
		if (useFBO())
		{
			//Release the backbuffer.
			delete backBufferPainter;
			backBufferPainter = NULL;

			backBuffer->release();
			//Swap buffers if finishing, don't swap yet if suspending.
			if(swapBuffers){swapFBOs();}
		}
		invariant();
	}

	//! Code that must run before drawing the final result of the rendering to the viewport.
	void prepareToDrawViewport()
	{
		invariant();
		//Put the result of drawing to the FBO on the screen, applying an effect.
		if (useFBO())
		{
			Q_ASSERT_X(!backBuffer->isBound() && !frontBuffer->isBound(), Q_FUNC_INFO, 
			           "Framebuffer objects weren't released before drawing the result");
		}
	}

	//! Disable Qt painting.
	void disablePainting()
	{
		invariant();
		
		if(usingGLWidgetPainter)
		{
			delete painter;
			usingGLWidgetPainter = false;
		}
		painter = NULL;
		invariant();
	}

	//! Enable Qt painting (with the current default painter, or constructing a fallback if no default).
	void enablePainting()
	{
		invariant();
		enablePainting(defaultPainter);
		invariant();
	}

	//! Return the painter currently used to paint, if any.
	QPainter* getPainter()
	{
		return painter;
	}

private:
	//! Size of the viewport (i.e. graphics resolution).
	QSize viewportSize;
	//! Widget we're drawing to with OpenGL.
	StelQGLWidget* glWidget;

	//! Painter we're currently using to paint. NULL if painting is disabled.
	QPainter* painter;
	//! Painter we're using if not using FBOs or when not drawing to an FBO.
	QPainter* defaultPainter;
	//! Painter to the FBO we're drawing to, when using FBOs.
	QPainter* backBufferPainter;

	//! Frontbuffer (i.e. displayed at the moment) frame buffer object, when using FBOs.
	QGLFramebufferObject* frontBuffer;
	//! Backbuffer (i.e. drawn to at the moment) frame buffer object, when using FBOs.
	QGLFramebufferObject* backBuffer;

	//! Are we in the middle of drawing a frame?
	bool drawing;

	//! Are we using the fallback painter (directly to glWidget) ?
	bool usingGLWidgetPainter;

	//! Are frame buffer objects supported on this system?
	bool fboSupported;
	//! Disable frame buffer objects even if supported?
	//!
	//! Currently, this is only used for debugging. 
	//! It might be loaded from a config file later.
	bool fboDisabled;

	//! Are non power of two textures supported?
	bool nonPowerOfTwoTexturesSupported;

	//! Asserts that we're in a valid state.
	void invariant() const
	{
#ifndef NDEBUG
		Q_ASSERT_X(NULL != glWidget, Q_FUNC_INFO, "Destroyed StelQGLViewport");
		Q_ASSERT_X(glWidget->isValid(), Q_FUNC_INFO, 
		           "Invalid glWidget (maybe there is no OpenGL support?)");

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
#endif
	}

	//! Enable Qt painting with specified painter (or construct a fallback painter if NULL).
	void enablePainting(QPainter* painter)
	{
		invariant();
		
		// If no painter specified, create a default one painting to the glWidget.
		if(painter == NULL)
		{
			this->painter = new QPainter(glWidget);
			usingGLWidgetPainter = true;
			return;
		}
		this->painter = painter;
		invariant();
	}

	//! Initialize the frame buffer objects.
	void initFBOs()
	{
		invariant();
		Q_ASSERT_X(useFBO(), Q_FUNC_INFO, "We're not using FBO");
		if (NULL == backBuffer)
		{
			Q_ASSERT_X(NULL == frontBuffer, Q_FUNC_INFO, 
			           "frontBuffer is not null even though backBuffer is");

			// If non-power-of-two textures are supported,
			// FBOs must have power of two size large enough to fit the viewport.
			const QSize bufferSize = nonPowerOfTwoTexturesSupported 
			    ? StelUtils::smallestPowerOfTwoSizeGreaterOrEqualTo(viewportSize) 
			    : viewportSize;

			// We want a depth and stencil buffer attached to our FBO if possible.
			const QGLFramebufferObject::Attachment attachment =
				QGLFramebufferObject::CombinedDepthStencil;
			backBuffer  = new QGLFramebufferObject(bufferSize, attachment);
			frontBuffer = new QGLFramebufferObject(bufferSize, attachment);

			Q_ASSERT_X(backBuffer->isValid() && frontBuffer->isValid(),
			           Q_FUNC_INFO, "Framebuffer objects failed to initialize");
		}
		invariant();
	}
	
	//! Swap front and back buffers, when using FBO.
	void swapFBOs()
	{
		invariant();
		Q_ASSERT_X(useFBO(), Q_FUNC_INFO, "We're not using FBO");
		QGLFramebufferObject* tmp = backBuffer;
		backBuffer = frontBuffer;
		frontBuffer = tmp;
		invariant();
	}

	//! Destroy FBOs, if used.
	void destroyFBOs()
	{
		invariant();
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
		invariant();
	}
};

#endif // _STELQGLVIEWPORT_HPP_
