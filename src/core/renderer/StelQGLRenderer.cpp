#include "StelQGLRenderer.hpp"

void StelQGLRenderer::bindTexture
	(StelTextureBackend* const textureBackend, const int textureUnit)
{
	invariant();
	StelQGLTextureBackend* qglTextureBackend =
		dynamic_cast<StelQGLTextureBackend*>(textureBackend);
	Q_ASSERT_X(qglTextureBackend != NULL, Q_FUNC_INFO,
				  "Trying to bind a texture created by a different renderer backend");

	const TextureStatus status = qglTextureBackend->getStatus();
	if(status == TextureStatus_Loaded)
	{
		// Ignore the texture if we don't have enough texture units
		// or if texture unit is nonzero and we don't support multitexturing.
		if(textureUnit >= getTextureUnitCount())
		{
			invariant();
			return;
		}
		qglTextureBackend->bind(textureUnit);
	}
	if(status == TextureStatus_Uninitialized)
	{
		qglTextureBackend->startAsynchronousLoading();
	}
	invariant();
}

void StelQGLRenderer::destroyTextureBackend(StelTextureBackend* const textureBackend)
{
	invariant();
	StelQGLTextureBackend* qglTextureBackend =
		dynamic_cast<StelQGLTextureBackend*>(textureBackend);
	Q_ASSERT_X(qglTextureBackend != NULL, Q_FUNC_INFO,
				  "Trying to destroy a texture created by a different renderer backend");

	if(textureBackend->getName().isEmpty())
	{
		delete textureBackend;
		invariant();
		return;
	}
	textureCache.remove(qglTextureBackend);
	invariant();
}

StelTextureBackend* StelQGLRenderer::createTextureBackend_
	(const QString& filename, const StelTextureParams& params, const TextureLoadingMode loadingMode)
{
	invariant();
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
		invariant();
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
	invariant();
	return result;
}

void StelQGLRenderer::renderFrame(StelRenderClient& renderClient)
{
	invariant();
	if(previousFrameEndTime < 0.0)
	{
		previousFrameEndTime = StelApp::getTotalRunTime();
	}

	viewport.setDefaultPainter(renderClient.getPainter());

	makeGLContextCurrent();
	viewport.startFrame();

	// When using the GUI, try to have the best reactivity, 
	// even if we need to lower the FPS.
	const int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;

	while (true)
	{
		const bool keepDrawing = renderClient.drawPartial();
		if(!keepDrawing) 
		{
			viewport.finishFrame();
			break;
		}

		const double spentTime = StelApp::getTotalRunTime() - previousFrameEndTime;

		// We need FBOs to do partial drawing.
		if (viewport.useFBO() && 1. / spentTime <= minFps)
		{
			// We stop the painting operation for now
			viewport.suspendFrame();
			break;
		}
	}
	
	drawWindow(renderClient.getViewportEffect());
	viewport.setDefaultPainter(NULL);

	// Flushing GL commands helps FPS somewhat (not sure why, though)
	glFlush();
	
	previousFrameEndTime = StelApp::getTotalRunTime();
	invariant();
}

void StelQGLRenderer::drawWindow(StelViewportEffect* const effect)
{
	// At this point, FBOs have been released (if using FBOs), so we're drawing 
	// directly to the screen. The StelViewportEffect::drawToViewport call 
	// actually draws puts the rendered result onto the viewport.
	invariant();

	//Warn about any GL errors.
	checkGLErrors();
	
	//Effects are ignored when FBO is not supported.
	//That might be changed for some GPUs, but it might not be worth the effort.
	
	viewport.prepareToDrawViewport();
	viewport.enablePainting();

	if(NULL == effect)
	{
		// If using FBO, we still need to put it on the screen.
		if(viewport.useFBO())
		{
			StelTexture* screenTexture = getViewportTexture();
			int texWidth, texHeight;
			screenTexture->getDimensions(texWidth, texHeight);

			setGlobalColor(Qt::white);
			screenTexture->bind();
			drawRect(0, 0, texWidth, texHeight);
		}
		// If not using FBO, the result is already drawn to the screen.
	}
	else
	{
		effect->drawToViewport(this);
	}

	disablePainting();
	invariant();
}
