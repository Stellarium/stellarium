#include "StelQGLRenderer.hpp"


StelTextureBackend* StelQGLRenderer::createTextureBackend_
	(const QString& filename, const StelTextureParams& params, const TextureLoadingMode loadingMode)
{
	const QString fullPath = filename.startsWith("http://") 
		? filename 
		: glFileSystemTexturePath(filename, pvrSupported);
	if(fullPath.isEmpty())
	{
		qWarning() << "createTextureBackend failed: file not found. Returning NULL.";
		return NULL;
	}

	StelQGLTextureBackend* cached = textureCache.get(fullPath, loadingMode);
	if(NULL != cached)
	{
		return cached;
	}

	StelQGLTextureBackend* result = NULL;
	//Synchronous (not in a separate thread) loading.
	if(loadingMode == TextureLoadingMode_Normal)
	{
		//Load from a normal image.
		if(!fullPath.endsWith(".pvr"))
		{
			QImage image = QImage(fullPath);
			if(image.isNull())
			{
				qWarning() << "createTextureBackend failed: found image file " << fullPath
				           << " but failed to load image data.";
				return NULL;
			}

			//Uploads to GL
			result = StelQGLTextureBackend::constructFromImage(this, fullPath, params, image);
		}
		//Load from PVR (compressed textures on PowerVR (mobile)).
		else
		{
			result = StelQGLTextureBackend::constructFromPVR(this, fullPath, params);
		}
	}
	else
	{
		Q_ASSERT_X(loadingMode == TextureLoadingMode_Asynchronous || 
		           loadingMode == TextureLoadingMode_LazyAsynchronous,
		           Q_FUNC_INFO, 
		           "Unknown texture loading mode");
		result = StelQGLTextureBackend::constructAsynchronous(this, fullPath, params);
		if(loadingMode != TextureLoadingMode_LazyAsynchronous)
		{
			//Web or file
			result->startAsynchronousLoading();
		}
	}

	Q_ASSERT_X(result != NULL, Q_FUNC_INFO, "Result texture backend was not set");

	textureCache.add(result);
	return result;
}

StelTextureBackend* StelQGLRenderer::getViewportTextureBackend()
{
	return useFBO() 
		? StelQGLTextureBackend::fromFBO(this, frontBuffer)
		: StelQGLTextureBackend::fromViewport(this, getViewportSize(), glContext->format());
}
