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
#include <QMap>
#include <QThread>

#include "GenericVertexTypes.hpp"
#include "StelApp.hpp"
#include "StelGuiBase.hpp"
#include "StelUtils.hpp"
#include "StelProjectorType.hpp"
#include "StelRenderer.hpp"
#include "StelQGLArrayVertexBufferBackend.hpp"
#include "StelQGLIndexBuffer.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelQGLViewport.hpp"
#include "StelTextureCache.hpp"
#include "StelVertexBuffer.hpp"

//! Limit on texture unit count (size of currentlyBoundTextures). Can be increased if needed.
#define STELQGLRENDERER_MAX_TEXTURE_UNITS 64

//! Enumerates statistics that can be collected about a StelQGLRenderer.
//!
//! Used to access individual statistics.
//!
//! @note This is an internal enum of the Renderer subsystem and should not be used elsewhere.
enum QGLRendererStatistics
{
	ESTIMATED_TEXTURE_MEMORY     = 0,
	BATCHES                      = 1,
	EMPTY_BATCHES                = 2,
	BATCHES_TOTAL                = 3,
	VERTICES                     = 4,
	VERTICES_TOTAL               = 5,
	TRIANGLES                    = 6,
	TRIANGLES_TOTAL              = 7,
	LINES                        = 8,
	LINES_TOTAL                  = 9,
	FRAMES                       = 10,
	TEXT_DRAWS                   = 11,
	RECT_DRAWS                   = 12,
	GL_FPS                       = 13,
	BATCH_PROJECTIONS_NONE_TOTAL = 14,
	BATCH_PROJECTIONS_NONE       = 15,
	BATCH_PROJECTIONS_CPU_TOTAL  = 16,
	BATCH_PROJECTIONS_CPU        = 17,
	BATCH_PROJECTIONS_GPU_TOTAL  = 18,
	BATCH_PROJECTIONS_GPU        = 19,
	VERTEX_BUFFERS_CREATED       = 20,
	INDEX_BUFFERS_CREATED        = 21,
	TEXTURES_CREATED             = 22,
	SHADERS_CREATED              = 23
};

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
		textureUnitCount = getTextureUnitCountBackend();
		glVendorString = QString(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		qDebug() << "GL vendor is " << glVendorString;
		glRendererString = QString(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		qDebug() << "GL renderer is " << glRendererString;
		viewport.init(gl.hasOpenGLFeature(QGLFunctions::NPOTTextures), 
		              glVendorString, glRendererString);
		initStatistics();
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
		statistics[INDEX_BUFFERS_CREATED] += 1.0;
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
				gl.glActiveTexture(GL_TEXTURE0 + t);
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

	virtual StelRendererStatistics& getStatistics() 
	{
		return statistics;
	}

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

	//! OpenGL renderer (usually GPU) string (can be used to enable/disable features based on GPU).
	QString glRendererString;

	//! Statistics collected during program run (such as estimated texture memory usage, etc.).
	StelRendererStatistics statistics;

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
		//
		// Optimization note: 
		// Disabling GL state setup and reset improves performance by 3-8% 
		// (open source AMD driver), so that is the maximum speedup achievable 
		// by moving state setup into functions such as setBlendMode, setDepthTest, etc.

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
			case BlendMode_Multiply:
				glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
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

	//! Update statistics in a drawVertexBufferBackend() call.
	//!
	//! @param vertexBuffer Vertex buffer drawn.
	//! @param indexBuffer  Index buffer (if any), specifying which vertices to draw.
	template<class VBufferBackend>
	void updateDrawStatistics(VBufferBackend* vertexBuffer,
	                          StelQGLIndexBuffer* indexBuffer)
	{
		statistics[BATCHES]           += 1.0;
		statistics[BATCHES_TOTAL]     += 1.0;

		const int vertices  = 
			(NULL == indexBuffer ? vertexBuffer->length() : indexBuffer->length());

		int triangles = 0;
		int lines     = 0;
		switch(vertexBuffer->getPrimitiveType())
		{
			case PrimitiveType_Points: 
				break;
			case PrimitiveType_Triangles:
				triangles = vertices / 3;
				break;
			case PrimitiveType_TriangleStrip:
			case PrimitiveType_TriangleFan:
				triangles = vertices >= 3 ? vertices - 2 : 0;
				break;
			case PrimitiveType_Lines: 
				lines = vertices / 2;
				break;
			case PrimitiveType_LineStrip:
				lines = vertices >= 2 ? vertices - 1 : 0;
				break;
			case PrimitiveType_LineLoop:
				lines = vertices;
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown graphics primitive type");
		}

		statistics[VERTICES]        += vertices;
		statistics[VERTICES_TOTAL]  += vertices;
		statistics[TRIANGLES]       += triangles;
		statistics[TRIANGLES_TOTAL] += triangles;
		statistics[LINES]           += lines;
		statistics[LINES_TOTAL]     += lines;
	}

private:
	//! Stores times (relative to start) of most recent frames in a queue (using a circular array).
	//!
	//! @note This is an internal struct of the Renderer subsystem and should not be used elsewhere.
	struct FrameTimeQueue
	{
	public:
		//! Construct a frame queue with all frames set to zero.
		FrameTimeQueue()
			: nextFrame(0)
		{
			memset(frames, '\0', FRAME_CAPACITY * sizeof(long double));
		}

		//! Update the frame queue (record new frame).
		//!
		//! Called at the start of each frame.
		void update()
		{
			frames[nextFrame++] = StelUtils::secondsSinceStart();
			nextFrame = nextFrame % FRAME_CAPACITY;
		}

		//! Get current FPS.
		double getFPS()
		{
			const long double newest = frames[(nextFrame - 1 + 256) % FRAME_CAPACITY];
			long double oldest = frames[nextFrame];
			Q_ASSERT_X(newest >= oldest, Q_FUNC_INFO, 
						  "Oldest frame time is more recent than newest");
			int tooOldCount = 0;
			// Don't compute FPS from more than one second of frames.
			while(newest - oldest > 1.0 && tooOldCount < FRAME_CAPACITY - 1)
			{
				++tooOldCount;
				oldest = frames[(nextFrame + tooOldCount) % FRAME_CAPACITY];
			}
			return (FRAME_CAPACITY - tooOldCount) / (newest - oldest);
		}

	private:
		//! Index in the frames array where the next frame will be written.
		int nextFrame;

		//! Number of frames stored in the queue.
		static const int FRAME_CAPACITY = 128;

		//! Stores frame start times.
		long double frames[FRAME_CAPACITY];
	};

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

	//! Stores start times of most recent frames (see FrameTimeQueue).
	FrameTimeQueue recentFrames;

	//! Draw the result of drawing commands to the window, applying given effect if possible.
	void drawWindow(StelViewportEffect* const effect);
	
	//! Initialize default values for statistics.
	void initStatistics()
	{
		// NOTE: Order of statistics added here _must_ match the order 
		// of values of the QGLRendererStatistics enum.
		statistics.addStatistic("estimated_texture_memory");

		// Draw statistics
		statistics.addStatistic("batches", StatisticSwapMode_SetToZero);
		statistics.addStatistic("empty_batches", StatisticSwapMode_SetToZero);
		statistics.addStatistic("batches_total");
		statistics.addStatistic("vertices", StatisticSwapMode_SetToZero);
		statistics.addStatistic("vertices_total");
		statistics.addStatistic("triangles", StatisticSwapMode_SetToZero);
		statistics.addStatistic("triangles_total");
		statistics.addStatistic("lines", StatisticSwapMode_SetToZero);
		statistics.addStatistic("lines_total");
		statistics.addStatistic("frames");
		statistics.addStatistic("text_draws", StatisticSwapMode_SetToZero);
		statistics.addStatistic("rect_draws", StatisticSwapMode_SetToZero);
		statistics.addStatistic("gl_fps");

		// Projection statistics
		statistics.addStatistic("batch_projections_none_total");
		statistics.addStatistic("batch_projections_none", StatisticSwapMode_SetToZero);
		statistics.addStatistic("batch_projections_cpu_total");
		statistics.addStatistic("batch_projections_cpu", StatisticSwapMode_SetToZero);
		statistics.addStatistic("batch_projections_gpu_total");
		statistics.addStatistic("batch_projections_gpu", StatisticSwapMode_SetToZero);

		// Creation of Renderer objects
		statistics.addStatistic("vertex_buffers_created");
		statistics.addStatistic("index_buffers_created");
		statistics.addStatistic("textures_created");
		statistics.addStatistic("shaders_created");
	}

	//! Clear per-frame statistics (called when a frame starts).
	void clearFrameStatistics()
	{
		statistics.swap();
		statistics[FRAMES] += 1.0;
		statistics[GL_FPS] = recentFrames.getFPS();
	}

	//! Handles gravity text logic. Called by draText().
	//!
	//! Splits the string into characters and draws each separately with its own rotation.
	//!
	//! @param params    Text drawing parameters.
	//! @param painter   QPainter used for painting to the viewport.
	//! @param baseX     Base X coordinate of the text on the screen (before shifting, etc).
	//! @param baseY     Base Y coordinate of the text on the screen (before shifting, etc).
	//! @param projector Projector used to project the text to the screen.
	void drawTextGravityHelper
		(const TextParams& params, QPainter& painter, const int baseX, const int baseY, StelProjectorP projector);

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
