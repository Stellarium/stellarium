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

#ifndef _STELQGLRENDERER_HPP_
#define _STELQGLRENDERER_HPP_


#include <QCache>
#include <QColor>
#include <QGLFunctions>
#include <QGraphicsView>
#include <QThread>

#include "GenericVertexTypes.hpp"
#include "StelApp.hpp"
#include "StelGuiBase.hpp"
#include "StelProjectorType.hpp"
#include "StelRenderer.hpp"
#include "StelQGLIndexBuffer.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelQGLViewport.hpp"
#include "StelTextureCache.hpp"
#include "StelVertexBuffer.hpp"

//! Limit on texture unit count (size of currentlyBoundTextures). Can be increased if needed.
#define STELQGLRENDERER_MAX_TEXTURE_UNITS 64

//! Base class for renderer based on OpenGL and at the same time Qt's QGL.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLRenderer : public StelRenderer
{
public:
	//! Construct a StelQGLRenderer.
	//!
	//! @param parent       Parent widget for the renderer's GL widget.
	//! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
	//!                     This should be true for mobile platforms with PowerVR GPUs.
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
		, plainRectBuffer(NULL)
		, texturedRectBuffer(NULL)
		, previousFrameEndTime(-1.0)
		, globalColor(Qt::white)
		, depthTest(DepthTest_Disabled)
		, stencilTest(StencilTest_Disabled)
		, blendMode(BlendMode_None)
		, culledFaces(CullFace_None)
		, placeholderTexture(NULL)
		, currentFontSet(false)
		, textureUnitCount(-1)
		, gl(glContext)
	{
		loaderThread = new QThread();
		loaderThread->start(QThread::LowestPriority);
		for(int t = 0; t < STELQGLRENDERER_MAX_TEXTURE_UNITS; ++t)
		{
			currentlyBoundTextures[t] = NULL;
		}
	}
	
	virtual ~StelQGLRenderer()
	{
		invariant();
		loaderThread->quit();

		if(NULL != placeholderTexture)
		{
			delete placeholderTexture;
			placeholderTexture = NULL;
		}
		textTextureCache.clear();
		if(NULL != textBuffer) 
		{
			delete textBuffer;
			textBuffer = NULL;
		}
		if(NULL != texturedRectBuffer)
		{
			delete texturedRectBuffer;
			texturedRectBuffer = NULL;
		}
		if(NULL != plainRectBuffer)
		{
			delete plainRectBuffer;
			plainRectBuffer = NULL;
		}

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
		// Can't call makeGLContextCurrent() before initialization is complete.
		glContext->makeCurrent();
		viewport.init(gl.hasOpenGLFeature(QGLFunctions::NPOTTextures));
		textureUnitCount = getTextureUnitCountBackend();
		glVendorString = QString(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		qDebug() << "GL vendor is " << glVendorString;
		return true;
	}
	
	virtual QImage screenshot()
	{
		invariant();
		return viewport.screenshot();
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
		// The font is actually set only once drawing text.
		currentFont = font;
		currentFontSet = true;
	}

	virtual void bindTextureBackend(StelTextureBackend* const textureBackend, const int textureUnit);


	//! Must be called by texture backend destructor to ensure we don't keep any pointers to it.
	//!
	//! Not called by destroyTextureBackend as texture backends are sometimes constructed and 
	//! destroyed internally (e.g. by drawText).
	void ensureTextureNotBound(StelTextureBackend* const textureBackend)
	{
		for(int t = 0; t < STELQGLRENDERER_MAX_TEXTURE_UNITS; ++t)
		{
			if(currentlyBoundTextures[t] == textureBackend)
			{
				currentlyBoundTextures[t] = NULL;
				glActiveTexture(GL_TEXTURE0 + t);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	virtual void destroyTextureBackend(StelTextureBackend* const textureBackend);

	virtual void setGlobalColor(const Vec4f& color)
	{
		globalColor = color;
	}

	virtual void setCulledFaces(const CullFace cullFace)
	{
		culledFaces = cullFace;
	}

	virtual void setBlendMode(const BlendMode mode)
	{
		blendMode = mode;
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

	virtual void swapBuffers()
	{
		invariant();
		glContext->swapBuffers();
		invariant();
	}

	virtual void drawRect(const float x, const float y, 
	                      const float width, const float height, 
	                      const float angle = 0.0f);
	
	virtual void drawTexturedRect(const float x, const float y, 
	                              const float width, const float height, 
	                              const float angle = 0.0f);

	//! Make Stellarium GL context the currently used GL context. Call this before GL calls.
	virtual void makeGLContextCurrent()
	{
		invariant();
		if(Q_UNLIKELY(QGLContext::currentContext() != glContext))
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

	//! Get a pointer to the thread used for loading graphics data (e.g. textures).
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

	//! Get the paint engine type used for Qt drawing.
	QPaintEngine::Type qtPaintEngineType() const 
	{
		return viewport.qtPaintEngineType();
	}

protected:
	//! OpenGL vendor string (used to enable/disable features based on driver).
	QString glVendorString;

	virtual StelTextureBackend* createTextureBackend
		(const QString& filename, const TextureParams& params, const TextureLoadingMode loadingMode);

	virtual StelTextureBackend* createTextureBackend
		(QImage& image, const TextureParams& params);

	virtual StelTextureBackend* createTextureBackend
		(const void* data, const QSize size, const TextureDataFormat format,
		 const TextureParams& params);

	virtual StelTextureBackend* getViewportTextureBackend()
	{
		invariant();
		return viewport.getViewportTextureBackend(this);
	}
	
	//! Return the number of texture units (implementation).
	virtual int getTextureUnitCountBackend()  = 0;

	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelQGLRenderer::invariant().
#ifndef NDEBUG
	virtual void invariant() const
	{
		Q_ASSERT_X(NULL != glContext, Q_FUNC_INFO,
		           "An attempt to use a destroyed StelQGLRenderer.");
		Q_ASSERT_X(glContext->isValid(), Q_FUNC_INFO, "The GL context is invalid");
	}
#else
	// This should guarantee this gets optimized away in release builds.
	void invariant() const{}
#endif

	//! Set up GL state common between the GL1 and GL2 backends before drawing.
	void setupGLState(StelProjector* projector)
	{
		// Instead of setting GL state when functions such as setDepthTest() or setCulledFaces()
		// are called, we only set it before drawing and reset after drawing to avoid 
		// conflicts with e.g. Qt OpenGL backend, or any other GL code that might be running.

		// GL setup before drawing.
		// Fix some problem when using Qt OpenGL2 engine
		glStencilMask(0x11111111);
		
		// Moving this to drawVertexBufferBackend causes flicker issues
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

		switch(depthTest)
		{
			case DepthTest_Disabled:
				glDisable(GL_DEPTH_TEST);
				break;
			case DepthTest_ReadOnly:
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				break;
			case DepthTest_ReadWrite:
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown depth test mode");
		}

		switch(stencilTest)
		{
			case StencilTest_Disabled:
				glDisable(GL_STENCIL_TEST);
				break;
			case StencilTest_Write_1:
				glStencilFunc(GL_ALWAYS, 0x1, 0x1);
				glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
				glEnable(GL_STENCIL_TEST);
				break;
			case StencilTest_DrawIf_1:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, 0x1, 0x1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown stencil test mode");
		}

		switch(culledFaces)
		{
			case CullFace_None:
				glDisable(GL_CULL_FACE);
				break;
			case CullFace_Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			case CullFace_Back:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown cull face type");
		}
		
		glFrontFace(projector->flipFrontBackFace() ? GL_CW : GL_CCW);
	}
	
	//! Reset GL state after drawing.
	void restoreGLState(StelProjector* projector)
	{
		// More stuff could be restored here if there are any Qt drawing problems.
		glFrontFace(projector->flipFrontBackFace() ? GL_CCW : GL_CW);
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);

		// Qt GL1 paint engine expects alpha blending to be enabled.
		//
		// If Qt fixes that, this might be removed.
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
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

	//! Vertex buffer used to draw plain, non-textured rectangles.
	StelVertexBuffer<VertexP2>* plainRectBuffer;

	//! Vertex buffer used to draw textured rectangles.
	StelVertexBuffer<TexturedVertex>* texturedRectBuffer;
	
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

	//! Current depth test mode. 
	DepthTest depthTest;

	//! Current stencil test mode.
	StencilTest stencilTest;

	//! Current blend mode.
	BlendMode blendMode;

	//! Determines whether front or back faces are culled, if any are culled at all.
	CullFace culledFaces;

	//! Texture used as a fallback when texture loading fails or a texture can't be bound.
	StelQGLTextureBackend* placeholderTexture;

	//! Currently bound texture of each texture unit (NULL if none bound).
	//!
	//! Used to reset bound texture is drawText.
	StelQGLTextureBackend* currentlyBoundTextures[STELQGLRENDERER_MAX_TEXTURE_UNITS];

	//! Font currently used for text drawing.
	QFont currentFont;

	//! Has currentFont been set since the intialization? (if not, the painter's default font will
	//! be used when drawing text)
	bool currentFontSet;

	//! Number of texture units supported (1 if multitexturing is not supproted).
	int textureUnitCount;

	//! Draw the result of drawing commands to the window, applying given effect if possible.
	void drawWindow(StelViewportEffect* const effect);

	//! Handles gravity text logic. Called by draText().
	//!
	//! Splits the string into characters and draws each separately with its own rotation.
	//!
	//! @param params  Text drawing parameters.
	//! @param painter QPainter used for painting to the viewport.
	void drawTextGravityHelper(const TextParams& params, QPainter& painter);

	//! Get the placeholder texture, lazily loading if it's not loaded yet.
	StelQGLTextureBackend* getPlaceholderTexture()
	{
		if(NULL != placeholderTexture) {return placeholderTexture;}

		const int placeholderSize = 512;
		const int cellSize = 16;
		QImage image(placeholderSize, placeholderSize, QImage::Format_RGB888);
		for(int y = 0; y < placeholderSize; ++y)
		{
			for (int x = 0; x < placeholderSize; x++) 
			{
				image.setPixel(x, y, (x / cellSize) % 2 == (y / cellSize) % 2 ? 0 : 0x00FFFF00);
			}
		}
		placeholderTexture = 
			StelQGLTextureBackend::constructFromImage(this, QString(), TextureParams(), image);

		return placeholderTexture;
	}

	//! Implements drawRect() and drawTexturedRect(). 
	//!
	//! These are pretty much the same function, but with different names to improve
	//! readability.
	//!
	//! @param renderer Renderer that's drawing the rectangle.
	//! @param textured Should the rectangle be textured?
	//! @param x        Horizontal position of the rectangle.
	//! @param y        Vertical position of the rectangle.
	//! @param width    Width of the rectangle.
	//! @param height   Height of the rectangle.
	//! @param angle    Rotation angle of the rectangle in degrees.
	//!
	//! @see drawRect, drawTexturedRect
	void drawRectInternal(const bool textured, const float x, const float y, 
	                      const float width, const float height, const float angle);

	// Must be down due to initializer list order.
protected:
	//! Wraps some GL functions for compatibility across GL and GLES.
	QGLFunctions gl;

};

#endif // _STELQGLRENDERER_HPP_
