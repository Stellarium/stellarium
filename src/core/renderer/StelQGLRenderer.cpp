#include <cmath>

#include "StelQGLRenderer.hpp"
#include "StelLocaleMgr.hpp"

void StelQGLRenderer::bindTextureBackend
	(StelTextureBackend* const textureBackend, const int textureUnit)
{
	invariant();
	StelQGLTextureBackend* qglTextureBackend =
		dynamic_cast<StelQGLTextureBackend*>(textureBackend);
	Q_ASSERT_X(qglTextureBackend != NULL, Q_FUNC_INFO,
				  "Trying to bind a texture created by a different renderer backend");

	const TextureStatus status = qglTextureBackend->getStatus();
	if(status == TextureStatus_Uninitialized)
	{
		qglTextureBackend->startAsynchronousLoading();
	}
	// Ignore the texture if we don't have enough texture units
	// or if texture unit is nonzero and we don't support multitexturing.
	if(textureUnit >= getTextureUnitCount())
	{
		invariant();
		return;
	}
	if(status == TextureStatus_Loaded)
	{
		qglTextureBackend->bind(textureUnit);
		currentlyBoundTextures[textureUnit] = qglTextureBackend;
	}
	else
	{
		getPlaceholderTexture()->bind(textureUnit);
		currentlyBoundTextures[textureUnit] = getPlaceholderTexture();
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

StelTextureBackend* StelQGLRenderer::createTextureBackend
	(const QString& filename, const TextureParams& params, const TextureLoadingMode loadingMode)
{
	invariant();
	const QString fullPath = filename.startsWith("http://") 
		? filename 
		: glFileSystemTexturePath(filename, pvrSupported);

	if(fullPath.isEmpty())
	{
		QImage image;
		// Texture in error state will be returned (loaded from NULL image), and
		// when bound, the placeholder texture will be used.
		qWarning() << "createTextureBackend failed: file \"" << filename << "\" not found.";
		return StelQGLTextureBackend::constructFromImage(this, QString(), params, image);
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
				// Texture in error state will be returned (loaded from NULL image), and
				// when bound, the placeholder texture will be used.
				qWarning() << "createTextureBackend failed: found image file \"" << fullPath
				           << "\" but failed to load image data. ";
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

StelTextureBackend* StelQGLRenderer::createTextureBackend
	(QImage& image, const TextureParams& params)
{
	invariant();
	return StelQGLTextureBackend::constructFromImage(this, QString(), params, image);
}

void StelQGLRenderer::renderFrame(StelRenderClient& renderClient)
{
	invariant();
	if(previousFrameEndTime < 0.0)
	{
		previousFrameEndTime = StelApp::getTotalRunTime();
	}

	viewport.setDefaultPainter(renderClient.getPainter(), glContext);

	makeGLContextCurrent();
	viewport.startFrame();

	// When using the GUI, try to have the best reactivity, 
	// even if we need to lower the FPS.
	const int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
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
	viewport.setDefaultPainter(NULL, glContext);

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

	if(NULL == effect)
	{
		// If using FBO, we still need to put it on the screen.
		if(viewport.useFBO())
		{
			StelTextureNew* screenTexture = getViewportTexture();
			const QSize size = screenTexture->getDimensions();

			glDisable(GL_BLEND);
			setGlobalColor(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
			screenTexture->bind();
			drawTexturedRect(0, 0, size.width(), size.height());
			delete screenTexture;
		}
		// If not using FBO, the result is already drawn to the screen.
	}
	else
	{
		effect->drawToViewport(this);
	}

	viewport.disablePainting();
	invariant();
}

void StelQGLRenderer::drawTextGravityHelper(const TextParams& params, QPainter& painter)
{
	StelProjectorP projector   = params.projector_;
	const Vec2f viewportCenter = projector->getViewportCenterAbsolute();

	const float dx = params.x_ - viewportCenter[0];
	const float dy = params.y_ - viewportCenter[1];
	const float d  = std::sqrt(dx * dx + dy * dy);
	
	// Cull the text if it's too far away to be visible in the screen.
	if (d > qMax(projector->getViewportXywh()[3], projector->getViewportXywh()[2]) * 2)
	{
		return;
	}

	const QString string  = params.string_;
	const int charCount   = string.length();
	const float charWidth = painter.fontMetrics().width(string) / charCount;
	float theta           = std::atan2(dy - 1, dx);
	const float psi       = qMin(5.0, std::atan2(charWidth, d + 1) * 180.0f / M_PI);
	const float xVc       = viewportCenter[0] + params.xShift_;
	const float yVc       = viewportCenter[1] + params.yShift_;
	const QString lang    = StelApp::getInstance().getLocaleMgr().getAppLanguage();

	const bool leftToRight = !QString("ar fa ur he yi").contains(lang);
	// Draw each character separately
	for (int i = 0; i < charCount; ++i)
	{
		const int charIndex = leftToRight ? i : charCount - 1 - i;
		const float x       = d * std::cos(theta) + xVc;
		const float y       = d * std::sin(theta) + yVc;
		const float angle   = 90.0f + theta * 180.0f / M_PI;

		drawText(TextParams(x, y, string[charIndex]).angleDegrees(angle));
	
		// Compute how much the character contributes to the angle 
		const float width = painter.fontMetrics().width(string[charIndex]);
		theta += psi * M_PI / 180.0 * (1 + (width - charWidth) / charWidth);
	}
}

void StelQGLRenderer::drawText(const TextParams& params)
{
	StelQGLTextureBackend* currentTexture = currentlyBoundTextures[0];

	viewport.enablePainting();
	if(currentFontSet)
	{
		viewport.setFont(currentFont);
	}
	QPainter* painter = viewport.getPainter();
	Q_ASSERT_X(NULL != painter, Q_FUNC_INFO, 
	           "Trying to draw text but painting is disabled");

	QFontMetrics fontMetrics = painter->fontMetrics();

	const int x = params.x_;
	const int y = params.y_;

	// Avoid drawing if outside viewport.
	// We do a worst-case approximation as getting exact text dimensions is expensive.
	// We also account for rotation by assuming the worst case in bot X and Y 
	// (culling with a rotating rectangle would be expensive)
	const int cullDistance = 
		std::max(fontMetrics.height(), params.string_.size() * fontMetrics.maxWidth());
	const Vec4i viewXywh = params.projector_->getViewportXywh();
	const int viewMinX = viewXywh[0];
	const int viewMinY = viewXywh[1];
	const int viewMaxX = viewMinX + viewXywh[2];
	const int viewMaxY = viewMinY + viewXywh[3];

	if(y + cullDistance < viewMinY || y - cullDistance > viewMaxY ||
	   x + cullDistance < viewMinX || x - cullDistance > viewMaxX)
	{
		viewport.disablePainting();
		return;
	}

	if(params.projector_->useGravityLabels() && !params.noGravity_)
	{
		drawTextGravityHelper(params, *painter);
		return;
	}
	
	const int pixelSize   = painter->font().pixelSize();
	// Strings drawn by drawText() can differ by text, font size, or the font itself.
	const QByteArray hash = params.string_.toUtf8() + QByteArray::number(pixelSize) + 
	                        painter->font().family().toUtf8();
	StelQGLTextureBackend* textTexture = textTextureCache.object(hash);

	// No texture in cache for this string, need to draw it.
	if (NULL == textTexture) 
	{
		const QRect extents = fontMetrics.boundingRect(params.string_);

		// Width and height of the text. 
		// Texture width/height is required to be at least equal to this.
		//
		// Both X and Y need to be at least 1 so we don't create an empty image 
		// (doesn't work with textures)
		const int requiredWidth  = std::max(1, extents.width() + 1 + static_cast<int>(0.02f * extents.width()));
		const int requiredHeight = std::max(1, extents.height());
		// Create temporary image and render text into it

		// QImage is used solely to reuse existing QGLTextureBackend constructor 
		// function. QPixmap could be used as well (not sure which is faster, 
		// needs profiling)
		QImage image = areNonPowerOfTwoTexturesSupported() 
		             ? QImage(requiredWidth, requiredHeight, QImage::Format_ARGB32) 
		             : QImage(StelUtils::smallestPowerOfTwoGreaterOrEqualTo(requiredWidth), 
		                      StelUtils::smallestPowerOfTwoGreaterOrEqualTo(requiredHeight),
		                      QImage::Format_ARGB32);
		image.fill(Qt::transparent);

		QPainter fontPainter(&image);
		fontPainter.setFont(painter->font());
		fontPainter.setRenderHints(QPainter::TextAntialiasing, true);
		fontPainter.setPen(Qt::white);

		// The second argument ensures the text is positioned correctly even if 
		// the image is enlarged to power-of-two.
		fontPainter.drawText(-extents.x(), 
		                     image.height() - requiredHeight - extents.y(), 
		                     params.string_);

		textTexture = StelQGLTextureBackend::constructFromImage
			(this, QString(), TextureParams().filtering(TextureFiltering_Linear), image);
		const QSize size = textTexture->getDimensions();
		if(!textTexture->getStatus() == TextureStatus_Loaded)
		{
			qWarning() << "Texture error: " << textTexture->getErrorMessage();
			Q_ASSERT_X(false, Q_FUNC_INFO, "Failed to construct a text texture");
		}
		textTextureCache.insert(hash, textTexture, 4 * size.width() * size.height());
	}

	// Even if NPOT textures are not supported, we always draw the full rectangle 
	// of the texture. The extra space is fully transparent, so it's not an issue.

	// Shortcut variables to calculate the rectangle.
	const QSize size   = textTexture->getDimensions();
	const float w      = size.width();
	const float h      = size.height();
	const float xShift = params.xShift_;
	const float yShift = params.yShift_;

	const float angleDegrees = 
		params.angleDegrees_ + 
		(params.noGravity_ ? 0.0f : params.projector_->getDefaultAngleForGravityText());
	// Zero out very small angles.
	// 
	// (this could also be used to optimize the case with zero angled
	//  to avoid sin/cos if needed)
	const bool  angled = std::fabs(angleDegrees) >= 1.0f * M_PI / 180.f;
	const float cosr   = angled  ? std::cos(angleDegrees * M_PI / 180.0) : 1.0f;
	const float sinr   = angled  ? std::sin(angleDegrees * M_PI / 180.0) : 0.0f;

	// Corners of the (possibly rotated) texture rectangle.
	const Vec2f ne(round(x + cosr * xShift       - sinr * yShift),
	               round(y + sinr * xShift       + cosr * yShift));
	const Vec2f nw(round(x + cosr * (w + xShift) - sinr * yShift),
	               round(y + sinr * (w + xShift) + cosr * yShift));
	const Vec2f se(round(x + cosr * xShift       - sinr * (h + yShift)),
	               round(y + sinr * xShift       + cosr * (h + yShift)));
	const Vec2f sw(round(x + cosr * (w + xShift) - sinr * (h + yShift)),
	               round(y + sinr * (w + xShift) + cosr * (h + yShift)));

	// Construct the text vertex buffer if it doesn't exist yet, otherwise clear it.
	if(NULL == textBuffer)
	{
		textBuffer = createVertexBuffer<TexturedVertex>(PrimitiveType_TriangleStrip);
	}
	else
	{
		textBuffer->unlock();
		textBuffer->clear();
	}

	textBuffer->addVertex(TexturedVertex(ne, Vec2f(0.0f, 0.0f)));
	textBuffer->addVertex(TexturedVertex(nw, Vec2f(1.0f, 0.0f)));
	textBuffer->addVertex(TexturedVertex(se, Vec2f(0.0f, 1.0f)));
	textBuffer->addVertex(TexturedVertex(sw, Vec2f(1.0f, 1.0f)));
	textBuffer->lock();

	// Draw.
	const BlendMode oldBlendMode = blendMode;
	setBlendMode(BlendMode_Alpha);
	textTexture->bind(0);
	drawVertexBuffer(textBuffer);
	setBlendMode(oldBlendMode);

	// Reset user-bound texture.
	if(NULL != currentTexture)
	{
		currentTexture->bind(0);
	}
	viewport.disablePainting();
}
