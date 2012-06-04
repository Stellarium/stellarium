#include "StelQGLRenderer.hpp"


StelTextureBackend* StelQGLRenderer::createTextureBackend_
	(const QString& filename, const StelTextureParams& params, TextureLoadingMode loadingMode)
{
	Q_ASSERT_X(!filename.isEmpty(), Q_FUNC_INFO,
	           "Trying to load a texture with an empty filename or URL");
	Q_ASSERT_X(!(filename.startsWith("http://") && loadingMode == TextureLoadingMode_Normal),
	           Q_FUNC_INFO,
	           "When loading a texture from network, texture loading mode must be "
	           "Asynchronous or LazyAsynchronous");

	const QString fullPath = filename.startsWith("http://") 
		? filename 
		: glFileSystemTexturePath(filename, pvrSupported);
	if(fullPath.isEmpty())
	{
		qWarning() << "createTextureBackend failed: file not found. Returning NULL.";
		return NULL;
	}

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
			return StelQGLTextureBackend::constructFromImage(this, fullPath, params, image);
		}
		//Load from PVR (compressed textures on PowerVR (mobile)).
		else
		{
			return StelQGLTextureBackend::constructFromPVR(this, fullPath, params);
		}
	}
	else
	{
		Q_ASSERT_X(loadingMode == TextureLoadingMode_Asynchronous || 
		           loadingMode == TextureLoadingMode_LazyAsynchronous,
		           Q_FUNC_INFO, 
		           "Unknown texture loading mode");
		StelQGLTextureBackend* result = 
			StelQGLTextureBackend::constructAsynchronous(this, fullPath, params);
		if(loadingMode != TextureLoadingMode_LazyAsynchronous)
		{
			//Web or file
			result->startLoadingInThread();
		}
		return result;
	}

	Q_ASSERT_X(false, Q_FUNC_INFO, 
	           "This code should never be reached");
	return NULL;
}
