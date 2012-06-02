#ifndef _STELQGLTEXTUREBACKEND_HPP_
#define _STELQGLTEXTUREBACKEND_HPP_

#include "renderer/StelGLUtilityFunctions.hpp"
#include "renderer/StelTextureBackend.hpp"


//! Texture backend based on QGL, usable with both GL1 and GL2.
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
	void startLoadingInThread();
	
	//! Construct a StelQGLTextureBackend from an image.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param path     Full path of the image file in the filesystem.
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.).
	//! @param image    Image to load from.
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* constructFromImage
		(class StelQGLRenderer* renderer, const QString& path, 
		 const StelTextureParams& params, QImage& image);
	
	//! Construct a StelQGLTextureBackend from a PVR (compressed texture on mobile) file.
	//!
	//! This includes loading the texture from file, which might fail if the file
	//! does not exist, creating a texture with Error status.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param path     Full path of the PVR image file in the filesystem.
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.).
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* constructFromPVR
		(class StelQGLRenderer* renderer, const QString& path, const StelTextureParams& params);
	

	//! Construct a StelQGLTextureBackend asynchronously (in a separate thread).
	//!
	//! Will return a StelQGLTextureBackend in Uninitialized state -
	//! if not lazy loading, caller must start loading themselves.
	//!
	//! @param renderer Renderer this texture belongs to.
	//! @param path     Full path of the PVR image file in the filesystem.
	//! @param params   Texture parameters (e.g. filtering, wrapping, etc.).
	//! @return Pointer to the new StelQGLTextureBackend.
	static StelQGLTextureBackend* constructAsynchronous
		(class StelQGLRenderer* renderer, const QString& path, const StelTextureParams& params);
	
private slots:
	//! Called by the loader when the image data has finished loading.
	void onImageLoaded(QImage image);
	
	//! Called by the loader in case of an error.
	void onLoadingError(const QString& errorMessage) ;
	
private:
	//! Texture parameters (e.g. filtering, wrapping, etc.).
	StelTextureParams textureParams;

	//! Renderer that created this texture.
	class StelQGLRenderer* renderer;

	//! GL handle of the texture.
	GLuint glTextureID;

	//! Used when asynchronously loading the texture, otherwise NULL.
	class StelTextureLoader* loader;

	//! Construct a StelQGLTextureBackend, default-initializing but not yet loading.
	//!
	//! The actual construction is done by static member functions like 
	//! constructFromImage, constructFromPVR, constructAsynchronous.
	//!
	//! @param  renderer Renderer that owns this texture.
	//! @param  path Full file system path or URL of the texture.
	//! @param  params Texture parameters (e.g. filtering, wrapping, etc).
	StelQGLTextureBackend(class StelQGLRenderer* renderer, const QString& path, 
	                      const StelTextureParams& params);

	//! Load the texture from a QImage.
	void loadFromImage(QImage image);

	//! Load the texture from a PVR (PVRTC compressed texture on mobile) file.
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
		const TextureStatus status = getStatus();
		Q_ASSERT_X((glTextureID != 0) == (status == TextureStatus_Loaded),
		           "StelQGLTextureBackend::invariant",
		           "Texture can only be specified once loaded.");
		Q_ASSERT_X(loader == NULL || status == TextureStatus_Loading,
		           "StelQGLTextureBackend::invariant",
		           "Texture loader can only exist during loading");
	}
};
#endif // _STELQGLTEXTUREBACKEND_HPP_
