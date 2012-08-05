#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


#include <QCache>
#include <QColor>
#include <QGLFunctions>
#include <QGraphicsView>
#include <QThread>

#include "StelApp.hpp"
#include "StelGuiBase.hpp"
#include "StelPainter.hpp"
#include "StelRenderer.hpp"
#include "StelQGLIndexBuffer.hpp"
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
		// Maximum bytes of text textures to store in the cache.
		// Increased for the Pulsars plugin.
		, textTextureCache(16777216)
		, textBuffer(NULL)
		, previousFrameEndTime(-1.0)
		, globalColor(Qt::white)
		, gl(glContext)
		, depthTest(DepthTest_Disabled)
		, stencilTest(StencilTest_Disabled)
	{
		loaderThread = new QThread();
		loaderThread->start(QThread::LowestPriority);
	}
	
	virtual ~StelQGLRenderer()
	{
		invariant();
		loaderThread->quit();

		textTextureCache.clear();
		if(NULL != textBuffer) {delete textBuffer;}

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
		setBlendMode(BlendMode_None);
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

	StelIndexBuffer* createIndexBuffer(const IndexType type)
	{
		return new StelQGLIndexBuffer(type);
	}

	virtual void drawText(const TextParams& params);
	
	virtual void setFont(const QFont& font)
	{
		viewport.setFont(font);
	}

	virtual void bindTexture(StelTextureBackend* const textureBackend, const int textureUnit);

	virtual void destroyTextureBackend(StelTextureBackend* const textureBackend);

	virtual void setGlobalColor(const Vec4f& color)
	{
		globalColor = color;
	}

	virtual void setBlendMode(const BlendMode blendMode)
	{
		switch(blendMode)
		{
			case BlendMode_None:
				glDisable(GL_BLEND);
				break;
			case BlendMode_Add:
				glBlendFunc(GL_ONE, GL_ONE);
				glEnable(GL_BLEND);
				break;
			case BlendMode_Alpha:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown blend mode");
		}
	}

	virtual void setDepthTest(const DepthTest test)
	{
		depthTest = test;
	}
	
	virtual void clearDepthBuffer()
	{
		// glDepthMask enables writing to the depth buffer so we can clear it.
		// This is a different API behavor from plain OpenGL. With OpenGL, the user 
		// must enable writing to the depth buffer to clear it.
		// StelRenderer allows it to be cleared regardless of modes set by setDepthTest
		// (which serves the same role as glDepthMask() in GL).
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
	}

	virtual void setStencilTest(const StencilTest test)
	{
		stencilTest = test;
	}

	virtual void clearStencilBuffer()
	{
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
	}

	//! Make Stellarium GL context the currently used GL context. Call this before GL calls.
	virtual void makeGLContextCurrent()
	{
		invariant();
		if(QGLContext::currentContext() != glContext)
		{
			// makeCurrent does not check if the context is already current, 
			// so it can really kill performance
			glContext->makeCurrent();
		}
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
	const Vec4f& getGlobalColor() const
	{
		return globalColor;
	}

	//! Returns true if non-power-of-two textures are supported, false otherwise.
	virtual bool areNonPowerOfTwoTexturesSupported() const = 0;

protected:
	virtual StelTextureBackend* createTextureBackend_
		(const QString& filename, const StelTextureParams& params, const TextureLoadingMode loadingMode);

	virtual StelTextureBackend* getViewportTextureBackend()
	{
		invariant();
		return viewport.getViewportTextureBackend(this);
	}
	
	//! Return the number of texture units (this is 1 if multitexturing is not supported).
	virtual int getTextureUnitCount() = 0;

	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelGLRenderer::invariant().
	virtual void invariant() const
	{
#ifndef NDEBUG
		Q_ASSERT_X(NULL != glContext, Q_FUNC_INFO,
		           "An attempt to use a destroyed StelQGLRenderer.");
		Q_ASSERT_X(glContext->isValid(), Q_FUNC_INFO, "The GL context is invalid");
#endif
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
	//!
	//! StelTextureCache is used instead of QCache to properly 
	//! support asynchronous/lazy loading.
	StelTextureCache<StelQGLTextureBackend> textureCache;

	//! Caches textures used to draw text.
	//!
	//! These are always loaded (drawn) synchronously, so QCache is enough.
	QCache<QByteArray, StelQGLTextureBackend> textTextureCache;

	//! 2D vertex with a position and texture coordinates.
	struct TexturedVertex
	{
		Vec2f position;
		Vec2f texCoord;
		TexturedVertex(const Vec2f& position, const Vec2f& texCoord)
			: position(position), texCoord(texCoord){}

		VERTEX_ATTRIBUTES(Vec2f Position, Vec2f TexCoord);
	};

	//! This buffer is reused to draw text at every drawText() call.
	StelVertexBuffer<TexturedVertex>* textBuffer;
	
	//! Time the previous frame rendered by renderFrame ended.
	//!
	//! Negative at construction to detect the first frame.
	double previousFrameEndTime;

	//! Global vertex color.
	//! This color is used when rendering vertex formats that have no vertex color attribute.
	//!
	//! Per-vertex color completely overrides this 
	//! (this is to keep behavior from before the GL refactor unchanged).
	//!
	//! Note that channel values might be outside of the 0-1 range.
	Vec4f globalColor;

	//! Draw the result of drawing commands to the window, applying given effect if possible.
	void drawWindow(StelViewportEffect* const effect);

	//! Handles gravity text logic. Called by draText().
	//!
	//! Splits the string into characters and draws each separately with its own rotation.
	//!
	//! @param params  Text drawing parameters.
	//! @param painter QPainter used for painting to the viewport.
	void drawTextGravityHelper(const TextParams& params, QPainter& painter);
	
	// Must be down due to initializer list order.
protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;

	//! Current depth test mode.
	DepthTest depthTest;

	//! Current stencil test mode.
	StencilTest stencilTest;
};

#endif // _STELQGLRENDERER_HPP_
