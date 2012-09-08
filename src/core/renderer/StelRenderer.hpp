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

#ifndef _STELRENDERER_HPP_
#define _STELRENDERER_HPP_

#include <QImage>
#include <QPainter>
#include <QSize>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGLSLShader.hpp"
#include "StelIndexBuffer.hpp"
#include "StelRendererStatistics.hpp"
#include "StelVertexAttribute.hpp"
#include "StelVertexBuffer.hpp"
#include "StelViewportEffect.hpp"
#include "StelTextureNew.hpp"
#include "StelTextureParams.hpp"

//! Pixel blending modes.
//!
//! Used for transparency, mixing colors in background 
//! with colors drawn in front, etc.
enum BlendMode
{
	//! No blending, new color overrides previous color.
	BlendMode_None,
	//! Colors of each channel are added up, clamping at maximum value 
	//! (255 for 8bit channels, 1.0 for floating-point colors)
	BlendMode_Add,
	//! Use alpha value of the front color for blending (0 is fully transparent, 255 or 1.0 fully opague).
	BlendMode_Alpha,
	//! Multiply the color channels of the back and front colors.
	BlendMode_Multiply
};

//! Represents faces to be culled (not drawn).
//!
//! Used with StelRenderer::setCulledFaces()
enum CullFace 
{
	//! Draw both back and front faces.
	CullFace_None,
	//! Don't draw front faces.
	CullFace_Front,
	//! Don't draw back faces.
	CullFace_Back
};

//! Possible depth test modes.
enum DepthTest
{
	//! No depth test. Earlier draws are overdrawn by the later draws.
	DepthTest_Disabled,
	//! Do not write to the depth buffer - draws are affected by the depth buffer without modifying it.
	DepthTest_ReadOnly,
	//! Read and write to the depth buffer - draws are affected by the depth buffer and change its values.
	DepthTest_ReadWrite
};

//! Possible stencil test modes.
enum StencilTest
{
	//! Stencil test is disabled and does not affect drawing.
	StencilTest_Disabled,
	//! Drawing works normally, but writes value 1 to the stencil buffer for every pixel drawn.
	StencilTest_Write_1,
	//! Only pixels where the stencil buffer has a value of 1 are drawn to.
	//!
	//! Stencil buffer values themselves are not changed.
	StencilTest_DrawIf_1
};

//! Describes possible formats of texture data used with the raw data overload of
//! StelRenderer::createTexture().
//!
//! Currently, this is only used for float textures, but 
//! it can be expanded when needed.
enum TextureDataFormat
{
	//! Every pixel is an RGBA color where each channel is a 32-bit float.
	TextureDataFormat_RGBA_F32
};

//! Provides access to scene rendering so StelRenderer can control it.
//!
//! Renderer implementations might decide to only draw parts of the scene 
//! each frame to increase FPS. This allows them to do so.
class StelRenderClient
{
public:
	//! Partially draw the scene.
	//!
	//! @return false if the last part of the scene was drawn 
	//!         (i.e. we're done drawing), true otherwise.
	virtual bool drawPartial() = 0;

	//! Get QPainter used for Qt painting to the viewport.
	//!
	//! (This painter might not necessarily end up being used - 
	//! e.g. if a GL backend uses FBOs, it creates a painter to draw to the
	//! FBOs.)
	virtual QPainter* getPainter() = 0;

	//! Get viewport effect to apply when drawing the viewport
	//!
	//! If this returns NULL, no viewport effect is used.
	virtual StelViewportEffect* getViewportEffect() = 0;
};

//! Parameters specifying how to draw text.
//!
//! These are passed to StelRenderer::drawText().
//!
//! This is a builder-style struct. Parameters can be specified like this:
//!
//! @code
//! // Note that text position and string must be always specified, so they 
//! // are in the constructor.
//! // Default parameters (no rotation, no shift, no gravity, 2D projection).
//! TextParams a(16, 16, "Hello World");
//! // Rotate by 30 degrees.
//! TextParams b = TextParams(16, 16 "Hello World!").angleDegrees(30.0f);
//! // Rotate by 30 degrees and shift by (8, 4) in rotated direction.
//! TextParams c = TextParams(16, 16 "Hello World!").angleDegrees(30.0f).shift(8.0f, 4.0f);
//! @endcode
//!
struct TextParams
{
friend class StelQGLRenderer;
	//! Construct TextParams with default parameters.
	//!
	//! Text position and string are required, so they are specified here.
	//!
	//! @param x      = Horizontal position of lower left corner of the text in pixels.
	//! @param y      = Vertical position of lower left corner of the text in pixels.
	//! @param string = Text string to draw.
	//!
	//! Default values of other parameters are: rotation of 0.0 degrees,
	//! shift in rotated direction of (0.0, 0.0), don't draw with gravity,
	//! 2D (NULL) projection.
	TextParams(const float x, const float y, const QString& string)
		: position_(x, y, 0.0f)
		, string_(string)
		, angleDegrees_(0.0f)
		, xShift_(0.0f)
		, yShift_(0.0f)
		, noGravity_(true)
		, projector_(NULL)
		, doNotProject_(true)
	{}

	//! Construct TextParams to draw text at a 3D position, using specified projector.
	//!
	//! The renderer will project the position to 2D coordinates before drawing.
	//! The required string parameter is also set, as is the projector.
	//! Other parameters are at default values.
	//!
	//! @param position3D = 3D position of the text.
	//! @param projector  = Projector to project the 3D position to 2D.
	//! @param string     = Text string to draw.
	//!
	//! Default values of other parameters are: rotation of 0.0 degrees,
	//! shift in rotated direction of (0.0, 0.0), don't draw with gravity.
	template<class F>
	TextParams(Vector3<F>& position3D, StelProjectorP projector, const QString& string)
		: position_(position3D[0], position3D[1], position3D[2])
		, string_(string)
		, angleDegrees_(0.0f)
		, xShift_(0.0f)
		, yShift_(0.0f)
		, noGravity_(true)
		, projector_(projector)
		, doNotProject_(false)
	{
	}

	//! Angle of text rotation in degrees.
	TextParams& angleDegrees(const float angle)
	{
		angleDegrees_ = angle;
		return *this;
	}

	//! Shift of the text in rotated direction in pixels.
	TextParams& shift(const float x, const float y)
	{
		xShift_ = x;
		yShift_ = y;
		return *this;
	}

	//! Draw the text with gravity.
	TextParams& useGravity()
	{
		noGravity_ = false;
		return *this;
	}

	//! Projector to project coordinates of the text with.
	TextParams& projector(StelProjectorP projector)
	{
		projector_ = projector;
		return *this;
	}

private:
	//! Position of the text. If doNotProject is true, this is already projected to 2D coords.
	Vec3f position_;
	//! Text string to draw.
	QString string_;
	//! Rotation of the text in degrees.
	float angleDegrees_;
	//! X shift in rotated direction.
	float xShift_;
	//! Y shift in rotated direction.
	float yShift_;
	//! Don't draw with gravity.
	bool  noGravity_;
	//! Projector to use. If NULL, 2D projector is assumed.
	StelProjectorP projector_;
	//! Position is already in screen coordinates, so don't project again.
	bool doNotProject_;
};

//! Main class of the @ref renderer "Renderer" subsystem. Handles all drawing functionality.
//! 
//! @note This is an interface. It should have only functions, no data members,
//!       as it might be used in multiple inheritance.
class StelRenderer
{
// For destroyTextureBackend() and bindTextureBackend()
friend class StelTextureNew;
public:
	//! Destructor.
	virtual ~StelRenderer(){};

	//! Initialize the renderer. 
	//! 
	//! Must be called before any other methods.
	//!
	//! @return true on success, false if there was an error.
	virtual bool init() = 0;
	
	//! Take a screenshot and return it.
	virtual QImage screenshot() = 0;
	
	//! Must be called once at startup and on every GL viewport resize, specifying new size.
	virtual void viewportHasBeenResized(const QSize size) = 0;

	//! Create an empty index buffer and return a pointer to it.
	virtual class StelIndexBuffer* createIndexBuffer(const IndexType type) = 0;
	
	//! Create an empty vertex buffer and return a pointer to it.
	//!
	//! The vertex buffer must be deleted by the user once it is not used
	//! (and before the Renderer is destroyed).
	//!
	//! @tparam V Vertex type. See the example in StelVertexBuffer documentation
	//!           on how to define a vertex type.
	//! @param primitiveType Graphics primitive type stored in the buffer.
	//!
	//! @return New vertex buffer storing vertices of type V.
	template<class V>
	StelVertexBuffer<V>* createVertexBuffer(const PrimitiveType primitiveType)
	{
		return new StelVertexBuffer<V>
			(createVertexBufferBackend(primitiveType, V::attributes()), primitiveType);
	}
	
	//! Draw contents of a vertex buffer.
	//!
	//! @param vertexBuffer Vertex buffer to draw.
	//! @param indexBuffer  Index buffer specifying which vertices from the buffer to draw.
	//!                     If NULL, indexing will not be used and vertices will be drawn
	//!                     directly in order they are in the buffer.
	//! @param projector    Projector to project vertices' positions before drawing.
	//!                     Also determines viewport to draw in.
	//!                     If NULL, no projection will be done and the vertices will be drawn
	//!                     directly.
	//! @param dontProject  Disable vertex position projection.
	//!                     (Projection matrix and viewport information of the 
	//!                     projector are still used)
	//!                     This is a hack to support StelSkyDrawer, which already 
	//!                     projects the star positions before drawing them.
	//!                     Avoid using this if possible. StelSkyDrawer might be 
	//!                     refactored in future to remove the need for dontProject,
	//!                     in which case it should be removed.
	//!
	//! @note When drawing with a custom StelProjector only 3D vertex positions are
	//! supported.
	template<class V>
	void drawVertexBuffer(StelVertexBuffer<V>* vertexBuffer, 
	                      class StelIndexBuffer* indexBuffer = NULL,
	                      StelProjectorP projector = StelProjectorP(NULL),
	                      bool dontProject = false)
	{
		drawVertexBufferBackend(vertexBuffer->backend, indexBuffer, &(*projector), dontProject);
	}

	//! Draw contents of a vertex buffer.
	//!
	//! This version takes a StelProjector* instead of StelProjectorP - this is not a 
	//! problem, as the renderer subsystem doesn't store or delete the projector.
	//!
	//! @param vertexBuffer Vertex buffer to draw.
	//! @param indexBuffer  Index buffer specifying which vertices from the buffer to draw.
	//!                     If NULL, indexing will not be used and vertices will be drawn
	//!                     directly in order they are in the buffer.
	//! @param projector    Projector to project vertices' positions before drawing.
	//!                     Also determines viewport to draw in.
	//!                     If NULL, no projection will be done and the vertices will be drawn
	//!                     directly.
	//! @param dontProject  Disable vertex position projection.
	//!                     (Projection matrix and viewport information of the 
	//!                     projector are still used)
	//!                     This is a hack to support StelSkyDrawer, which already 
	//!                     projects the star positions before drawing them.
	//!                     Avoid using this if possible. StelSkyDrawer might be 
	//!                     refactored in future to remove the need for dontProject,
	//!                     in which case it should be removed.
	//!
	//! @note When drawing with a custom StelProjector only 3D vertex positions are
	//! supported.
	template<class V>
	void drawVertexBuffer(StelVertexBuffer<V>* vertexBuffer, 
	                      class StelIndexBuffer* indexBuffer,
	                      StelProjector* projector,
	                      bool dontProject = false)
	{
		drawVertexBufferBackend(vertexBuffer->backend, indexBuffer, projector, dontProject);
	}

	//! Draw a line with current global color to the screen.
	//!
	//! @param startX X position of the starting point of the line in pixels.
	//! @param startY Y position of the starting point of the line in pixels.
	//! @param endX   X position of the ending point of the line in pixels.
	//! @param endY   Y position of the ending point of the line in pixels.
	virtual void drawLine(const float startX, const float startY, 
	                      const float endX, const float endY);

	//! Draw a rectangle to the screen.
	//!
	//! The rectangle will be colored by the global color
	//! (which can be specified by setGlobalColor()).
	//!
	//! The rectangle can be rotated around its center by an angle specified 
	//! in degrees.
	//!
	//! @param x      X position of the top left corner on the screen in pixels.
	//! @param y      Y position of the top left corner on the screen in pixels. 
	//! @param width  Width in pixels.
	//! @param height Height in pixels.
	//! @param angle  Angle to rotate the rectangle around its center in degrees.
	virtual void drawRect(const float x, const float y, 
	                      const float width, const float height, 
	                      const float angle = 0.0f) = 0;
	
	//! Draw a textured rectangle to the screen.
	//!
	//! The rectangle will be textured by the currently bound texture.
	//!
	//! The rectangle can be rotated around its center by an angle specified 
	//! in degrees.
	//!
	//! @param x      X position of the top left corner on the screen in pixels.
	//! @param y      Y position of the top left corner on the screen in pixels. 
	//! @param width  Width in pixels.
	//! @param height Height in pixels.
	//! @param angle  Angle to rotate the rectangle around its center in degrees.
	virtual void drawTexturedRect(const float x, const float y, 
	                              const float width, const float height, 
	                              const float angle = 0.0f) = 0;

	//! Draw text with specified parameters.
	//!
	//! Parameters are specified by a builder-style struct.
	//!
	//! Examples:
	//!
	//! @code
	//! // Note that text position and string must be always specified, so they 
	//! // are in the constructor.
	//! // Draw with default parameters (no rotation, no shift, no gravity, 2D projection).
	//! renderer->drawText(TextParams(16, 16, "Hello World"));
	//! // Rotate by 30 degrees.
	//! renderer->drawText(TextParams(16, 16 "Hello World!").angleDegrees(30.0f));
	//! // Rotate by 30 degrees and shift by (8, 4) in rotated direction.
	//! renderer->drawText(TextParams(16, 16 "Hello World!").angleDegrees(30.0f).shift(8.0f, 4.0f));
	//! @endcode
	//!
	//! @param params Parameters of the text to draw.
	//!
	//! @see TextParams
	virtual void drawText(const TextParams& params) = 0;

	//! Set font to use for drawing text.
	virtual void setFont(const QFont& font) = 0;

	//! Render a frame.
	//!
	//! This might not render the entire frame - if rendering takes too long,
	//! the backend may (or may not) suspend the rendering to finish it next 
	//! time renderFrame is called.
	//!
	//! @param renderClient Allows the renderer to draw the scene in parts.
	//!
	//! @see StelRenderClient
	virtual void renderFrame(StelRenderClient& renderClient) = 0;

	//! Create a texture from specified file or URL.
	//!
	//! The texture must be deleted before the StelRenderer is destroyed.
	//!
	//! This method never fails, but the texture returned might have the Error status.
	//! Even in that case, the texture can be bound, but a placeholder 
	//! texture will be used internally instead.
	//!
	//! @param filename    File name or URL of the image to load the texture from.
	//!                    If it's a file and it's not found, it's searched for in
	//!                    the <em>textures/</em> directory. Some renderer backends might also 
	//!                    support compressed textures with custom file formats.
	//!                    These backend-specific files should
	//!                    not be specified by filename - instead, if a compressed
	//!                    texture with the same file name but different extension
	//!                    exists, it will be used.
	//!                    E.g. the GLES backend prefers a .pvr version of a texture
	//!                    if PVR is supported and it exists.
	//! @param params      Texture parameters, such as filtering, wrapping, etc.
	//! @param loadingMode Texture loading mode to use. Normal immediately loads
	//!                    the texture, Asynchronous starts loading it in a background
	//!                    thread and LazyAsynchronous starts loading it when it's
	//!                    first needed.
	//!
	//! @return New texture.
	//!
	//! @note Some renderer backends only support textures with power of two 
	//!       dimensions (e.g. 512x512 or 2048x256). On these backends, loading 
	//!       a texture with non-power-of-two dimensions will fail and result 
	//!       in a StelTextureNew with status of TextureStatus_Error.
	StelTextureNew* createTexture
		(const QString& filename, const TextureParams& params = TextureParams(), 
		 const TextureLoadingMode loadingMode = TextureLoadingMode_Normal)
	{
		//This function tests preconditions and calls implementation.
		Q_ASSERT_X(!filename.endsWith(".pvr"), Q_FUNC_INFO,
		           "createTexture() can't load a PVR texture directly, as PVR "
		           "support may not be implemented by all Renderer backends. Request "
		           "a non-PVR texture, and if a PVR version exists and the backend "
		           "supports it, it will be loaded.");
		Q_ASSERT_X(!filename.isEmpty(), Q_FUNC_INFO,
		           "Trying to load a texture with an empty filename or URL");
		Q_ASSERT_X(!(filename.startsWith("http://") && loadingMode == TextureLoadingMode_Normal),
		           Q_FUNC_INFO,
		           "When loading a texture from network, texture loading mode must be "
		           "Asynchronous or LazyAsynchronous");

		return new StelTextureNew(this, createTextureBackend(filename, params, loadingMode));
	}

	//! Create a texture from an image.
	//!
	//! The texture must be deleted before the StelRenderer is destroyed.
	//!
	//! This method never fails, but the texture returned might have the Error status.
	//! Even in that case, the texture can be bound, but a placeholder 
	//! texture will be used internally instead.
	//!
	//! The texture is created immediately, as with TextureLoadingMode_Normal.
	//!
	//! @param image  Image to load the texture from.
	//! @param params Texture parameters, such as filtering, wrapping, etc.
	//!
	//! @return New texture.
	//!
	//! @note Some renderer backends only support textures with power of two 
	//!       dimensions (e.g. 512x512 or 2048x256). On these backends, loading 
	//!       a texture with non-power-of-two dimensions will fail and result 
	//!       in a StelTextureNew with status of TextureStatus_Error.
	StelTextureNew* createTexture
		(QImage& image, const TextureParams& params = TextureParams())
	{
		Q_ASSERT_X(!image.isNull(), Q_FUNC_INFO, "Trying to create a texture from a null image");
		return new StelTextureNew(this, createTextureBackend(image, params));
	}

	//! Create a texture from raw data.
	//!
	//! Used to load textures from image data unsupported by QImage,
	//! e.g. floating point textures.
	//!
	//! @note If creating a floating point texture, use 
	//! areFloatTexturesSupported() to determine if if floating 
	//! point textures are supported.
	//!
	//! The texture must be deleted before the StelRenderer is destroyed.
	//!
	//! This method never fails, but the texture returned might have the Error status.
	//! Even in that case, the texture can be bound, but a placeholder 
	//! texture will be used internally instead.
	//!
	//! The texture is created immediately, as with TextureLoadingMode_Normal.
	//!
	//! @param data   Pointer to raw image data. Size of the data must be
	//!               size.width() * size.height() * <em>pixelSize</em>, where 
	//!               <em>pixelSize</em> is size of a pixel of specified format.
	//! @param size   Size of the texture in pixels.
	//! @param format Format of pixels in data.
	//! @param params Texture parameters, such as filtering, wrapping, etc.
	//!
	//! @return New texture.
	//!
	//! @note Some renderer backends only support textures with power of two 
	//!       dimensions (e.g. 512x512 or 2048x256). On these backends, loading 
	//!       a texture with non-power-of-two dimensions will fail and result 
	//!       in a StelTextureNew with status of TextureStatus_Error.
	StelTextureNew* createTexture
		(const void* const data, const QSize size, const TextureDataFormat format, 
		 const TextureParams& params = TextureParams())
	{
		Q_ASSERT_X(NULL != data, Q_FUNC_INFO, "Trying to load a texture from a NULL pointer to data");
		if(!areFloatTexturesSupported())
		{
			Q_ASSERT_X(format != TextureDataFormat_RGBA_F32, Q_FUNC_INFO,
			           "Trying to load a floating-point texture even though "
			           "float textures are not supported");
		}
		return new StelTextureNew(this, createTextureBackend(data, size, format, params));
	}

	//! Returns true if floating point textures are supported, false otherwise.
	virtual bool areFloatTexturesSupported() const = 0;

	//! Get a texture of the viewport, with everything drawn to the viewport so far.
	//!
	//! @note Since some backends only support textures with power of two 
	//! dimensions, the returned texture might be larger than the viewport
	//! in which case only the part of the texture matching viewport size 
	//! (returned by getViewportSize) will be taken up by the viewport.
	//!
	//! @return Viewport texture.
	StelTextureNew* getViewportTexture()
	{
		return new StelTextureNew(this, getViewportTextureBackend());
	}

	//! Create a GLSL shader.
	//!
	//! This can only be called if isGLSLSupported() is true.
	//!
	//! The constructed shader must be deleted before the StelRenderer is destroyed.
	virtual StelGLSLShader* createGLSLShader()
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, 
		           "Trying to create a GLSL shader with a renderer backend that doesn't "
		           "support GLSL");
		// Avoids compiler warnings.
		return NULL;
	}

	//! Are GLSL shaders supported?
	virtual bool isGLSLSupported() const = 0;

	//! Get size of the viewport in pixels.
	virtual QSize getViewportSize() const = 0;

	//! Set the global vertex color.
	//!
	//! Default color is white.
	//!
	//! This color is used when rendering vertex formats that have no vertex color attribute,
	//! lines, non-textured rectangles, etc. .
	//!
	//! Per-vertex color completely overrides this 
	//! (this is to keep behavior from before the GL refactor unchanged).
	//!
	//! @note Color channel values can be outside of the 0-1 range.
	virtual void setGlobalColor(const Vec4f& color) = 0;

	//! setGlobalColor overload specifying color channels directly instead of through Vec4f.
	//! 
	//! @see setGlobalColor
	void setGlobalColor(const float r, const float g, const float b, const float a = 1.0f)
	{
		setGlobalColor(Vec4f(r, g, b, a));
	}

	//! Set blend mode.
	//!
	//! Used to enable/disable transparency and similar effects.
	//!
	//! On startup, the blend mode is BlendMode_None.
	virtual void setBlendMode(const BlendMode blendMode) = 0;

	//! Set which faces (triangles) should be culled.
	//!
	//! Front faces are usually those whose vertices are in counter-clockwise order,
	//! but a StelProjector might flip this order after projection. If such a
	//! StelProjector is used with StelRenderer::drawVertexBuffer(), front 
	//! faces will be clock wise. This doesn't affect the user in any way as
	//! the projection is done inside renderer.
	//!
	//! However, if doing manual projection and sending the already projected vertices 
	//! for drawing, this order will be flipped, so previously counter-clockwise(front) 
	//! faces will be clock-wise (back) faces, so if culling is used, opposite face 
	//! should be used with setCulledFaces().
	//!
	//! Whether a StelProjector changes clockwise-counterclockwise winding can be
	//! determined by StelProjector::flipFrontBackFace().
	virtual void setCulledFaces(const CullFace cullFace) = 0;

	//! Clear the depth buffer to zeroes, removing any depth information.
	virtual void clearDepthBuffer() = 0;

	//! Set depth test mode.
	//!
	//! Depth test is implemented to be as specific as possible, 
	//! only supporting what Stellarium needs. This might be counterintuitive, 
	//! as much of the power of OpenGL is removed. However, 
	//! this makes it easier to implement different Renderer backends that 
	//! might not necessarily be based on OpenGL.
	virtual void setDepthTest(const DepthTest test) = 0;

	//! Clear the stencil buffer to zeroes, removing any stencil information.
	virtual void clearStencilBuffer() = 0;

	//! Set stencil test mode.
	//!
	//! Stencil test is implemented to be as specific as possible, 
	//! only supporting what Stellarium needs. This might be counterintuitive, 
	//! as much of the power of OpenGL is removed. However, 
	//! this makes it easier to implement different Renderer backends that 
	//! might not necessarily be based on OpenGL.
	virtual void setStencilTest(const StencilTest test) = 0;

	//! Swap front and back buffers.
	//!
	//! This is only needed before the Renderer takes control of frame start/end,
	//! e.g. during loading at startup.
	virtual void swapBuffers() = 0;

	//! Access statistics data.
	//!
	//! Contents are backend specific - 
	//! might include thing like vertices per frame, estimated texture memory, etc. .
	//!
	//! User code might add its own statistics as well (e.g. StelSkyDrawer).
	virtual StelRendererStatistics& getStatistics() = 0;

protected:
	//! Create a vertex buffer backend. Used by createVertexBuffer.
	//!
	//! This allows each Renderer backend to create its own vertex buffer backend.
	//!
	//! @param primitiveType Graphics primitive type stored in the buffer.
	//! @param attributes    Descriptions of all attributes of the vertex type stored in the buffer.
	//!
	//! @return Pointer to a vertex buffer backend specific to the Renderer backend.
	virtual StelVertexBufferBackend* createVertexBufferBackend
		(const PrimitiveType primitiveType, const QVector<StelVertexAttribute>& attributes) = 0;

	//! Draw contents of a vertex buffer (backend). Used by drawVertexBuffer.
	//!
	//! @see drawVertexBuffer
	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     class StelIndexBuffer* indexBuffer,
	                                     StelProjector* projector,
	                                     const bool dontProject) = 0;

	//! Implementation of createTexture.
	//!
	//! Returns texture backend to be wrapped in a StelTextureNew.
	//! Must not fail, but can return a backend in error state.
	//!
	//! @see createTexture
	virtual class StelTextureBackend* createTextureBackend
		(const QString& filename, const TextureParams& params, 
		 const TextureLoadingMode loadingMode) = 0;


	//! Implementation of createTexture loading from image.
	//!
	//! Returns texture backend to be wrapped in a StelTextureNew.
	//! Must not fail, but can return a backend in error state.
	//!
	//! @see createTexture
	virtual class StelTextureBackend* createTextureBackend
		(QImage& image, const TextureParams& params) = 0;

	//! Implementation of createTexture loading from raw data.
	//!
	//! Returns texture backend to be wrapped in a StelTextureNew.
	//! Must not fail, but can return a backend in error state.
	//!
	//! @see createTexture
	virtual class StelTextureBackend* createTextureBackend
		(const void* data, const QSize size, const TextureDataFormat format, 
		 const TextureParams& params) = 0;

	//! Implementation of getViewportTexture.
	//!
	//! @see getViewportTexture.
	virtual class StelTextureBackend* getViewportTextureBackend() = 0;

	//! Destroy a StelTextureBackend.
	//!
	//! The backend might be destroyed, but the implementation might 
	//! also cache the texture, not destroying it if it has muliple users.
	virtual void destroyTextureBackend(class StelTextureBackend* backend) = 0;

	//! Bind a texture (following draw calls will use this texture on specified texture unit).
	//!
	//! @param  textureBackend Texture to bind.
	//! @param  textureUnit Texture unit to use. 
	//!                     If multitexturing is not supported, 
	//!                     binds to texture units other than 0 are ignored.
	virtual void bindTextureBackend(class StelTextureBackend* textureBackend, const int textureUnit) = 0;
};

#endif // _STELRENDERER_HPP_
