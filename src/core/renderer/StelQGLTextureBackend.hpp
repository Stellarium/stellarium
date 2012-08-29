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

#ifndef _STELQGLTEXTUREBACKEND_HPP_
#define _STELQGLTEXTUREBACKEND_HPP_

#include "StelGLUtilityFunctions.hpp"
#include "StelRenderer.hpp"
#include "StelTextureBackend.hpp"


//! Texture backend based on QGL, usable with both GL1 and GL2.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLTextureBackend : public StelTextureBackend
{
Q_OBJECT
public:
	//! Destroy the StelQGLTextureBackend. Must be called before the Renderer is destroyed.
	~StelQGLTextureBackend();
	
	//! Called by QGLRenderer to bind the texture to specified texture unit.
	void bind(const int textureUnit);
	
	//! Start asynchrounously loading the texture in a separate thread.
	//!
	//! Can only be called on a StelQGLTextureBackend returned by constructAsynchronous.
	//!
	//! Changes status from Uninitialized to Loading.
	void startAsynchronousLoading();
	
	//! Construct a StelQGLTextureBackend from an image.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param path     Full path of the image file in the filesystem.
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.).
	//! @param image    Image to load from.
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* constructFromImage
		(class StelQGLRenderer* renderer, const QString& path, 
		 const TextureParams& params, QImage& image);
	
	//! Construct a StelQGLTextureBackend from a PVR (compressed texture on some mobile platforms) file.
	//!
	//! This includes loading the texture from file, which might fail if the file
	//! does not exist, creating a texture with Error status.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param path     Full path of the PVR image file in the filesystem.
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.).
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* constructFromPVR
		(class StelQGLRenderer* renderer, const QString& path, const TextureParams& params);

	//! Construct a StelQGLTextureBackend asynchronously (in a separate thread).
	//!
	//! Will return a StelQGLTextureBackend in Uninitialized state -
	//! if not lazy loading, caller must start loading themselves.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param path     Full path of image file in the file system (or a URL pointing to the image).
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.).
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* constructAsynchronous
		(class StelQGLRenderer* renderer, const QString& path, const TextureParams& params);
	
	//! Construct a StelQGLTextureBackend from a framebuffer object.
	//!
	//! This will simply wrap a texture of a framebuffer object 
	//! in a StelQGLTextureBackend and return it. The texture itself 
	//! will still be owned by the framebuffer object, and destroyed 
	//! with the framebuffer objects, not with the StelQGLTextureBackend.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param fbo      Framebuffer object to get the texture from.
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* fromFBO
		(StelQGLRenderer* renderer, class QGLFramebufferObject* fbo);

	//! Construct a StelQGLTextureBackend from the viewport.
	//!
	//! Used to get a texture of the viewport. The returned texture will be 
	//! power-of-two containing the image data in area matching viewport size.
	//!
	//! @note This only works when FBOs are not used. When using FBOs,
	//!       use fromFBO on the front buffer instead.
	//!
	//! @param renderer       Renderer this texture belongs to.
	//! @param viewportSize   Size of the viewport in pixels.
	//! @param viewportFormat Pixel format of the viewport.
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* fromViewport
		(StelQGLRenderer* renderer, const QSize viewportSize, const QGLFormat& viewportFormat);

	//! Construct a texture from raw data.
	//!
	//! Used to create textures from data Qt does not support, 
	//! e.g. floating point textures.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param data     Pointer to the texture data.
	//! @param size     Texture size in pixels.
	//! @param format   Format of texture pixels stored in data.
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.)
	static StelQGLTextureBackend* fromRawData
		(StelQGLRenderer* renderer, const void* data, const QSize size, 
		 const TextureDataFormat format, const TextureParams& params);
private slots:
	//! Called by the loader when the image data has finished loading.
	void onImageLoaded(QImage image);
	
	//! Called by the loader in case of an error.
	void onLoadingError(const QString& errorMessage) ;
	
private:
	//! Texture parameters (e.g. filtering, wrapping, etc.).
	TextureParams textureParams;

	//! Renderer that created this texture.
	class StelQGLRenderer* renderer;

	//! GL handle of the texture.
	GLuint glTextureID;

	//! Does this StelQGLTextureBackend own its texture?
	//!
	//! This is usually true, but false e.g. if the texture belongs to 
	//! an FBO. If false, destructor won't delete the texture.
	bool ownsTexture;
	
	//! If true, delete the texture directly through glDeleteTextures() instead of
	//! QGLContext::deleteTexture().
	bool deleteManually;

	//! Used when asynchronously loading the texture, otherwise NULL.
	class StelTextureLoader* loader;

	//! Estimated bytes per pixel.
	//!
	//! This is only used for statistics and is not always precise.
	//! (e.g. with compressed or FBO textures)
	float pixelBytes;

	//! Construct a StelQGLTextureBackend, default-initializing but not yet loading.
	//!
	//! The actual construction is done by static member functions like 
	//! constructFromImage, constructFromPVR, constructAsynchronous.
	//!
	//! @param  renderer Renderer that owns this texture.
	//! @param  path Full file system path or URL of the texture.
	//! @param  params Texture parameters (e.g. filtering, wrapping, etc).
	StelQGLTextureBackend(class StelQGLRenderer* renderer, const QString& path, 
	                      const TextureParams& params);

	//! Load the texture from a QImage.
	void loadFromImage(QImage image);

	//! Load the texture from a PVR (PVRTC compressed texture on some mobile platforms) file.
	void loadFromPVR();
	
	//! Prepare OpenGL context to load a texture and return it.
	QGLContext* prepareContextForLoading();
	
	//! Complete texture loading, determining texture size.
	void completeLoading();
	
	//! Set GL texture wrapping (during texture loading).
	void setTextureWrapping();
	
	//! Ensure that we're in consistent state.
	void invariant() const
	{
#ifndef NDEBUG
		const TextureStatus status = getStatus();
		if((glTextureID != 0) != (status == TextureStatus_Loaded))
		{
			qDebug() << "Handle: " << glTextureID << " Status: " << textureStatusName(status);
			Q_ASSERT_X(false, Q_FUNC_INFO, "Texture can be specified if and only if loaded.");
		}

		Q_ASSERT_X(loader == NULL || status == TextureStatus_Loading,
		           Q_FUNC_INFO, "Texture loader can only exist during loading");
#endif
	}
};
#endif // _STELQGLTEXTUREBACKEND_HPP_
