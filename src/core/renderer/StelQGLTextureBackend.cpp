#include <QGLContext>
#include <QGLFormat>
#include <QGLFramebufferObject>
#include <QImage>

#include "StelGLUtilityFunctions.hpp"
#include "StelQGLRenderer.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelTextureLoader.hpp"
#include "StelUtils.hpp"


//! Get QGL bind options corresponding to specified TextureParams.
//!
//! These are options that are specified at the moment the texture
//! is bound in QGLContext.
//!
//! @param params TextureParams to get bind options for.
//! @return QGL bind options.
static QGLContext::BindOptions getTextureBindOptions(const TextureParams& params)
{
	QGLContext::BindOptions result = QGLContext::InvertedYBindOption;
	// Set GL texture filtering mode.
	switch(params.filteringMode)
	{
		case TextureFiltering_Nearest: break;
		case TextureFiltering_Linear:  result |= QGLContext::LinearFilteringBindOption; break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture filtering mode");
	}
	//TODO investigate (maybe we're using deprecated stuff to generate mipmaps?)
	// Mipmap seems to be pretty buggy on windows
#ifndef Q_OS_WIN
	if (params.autoGenerateMipmaps)
	{
		result |= QGLContext::MipmapBindOption;
	}
#endif

	return result;
}

StelQGLTextureBackend::StelQGLTextureBackend
	(StelQGLRenderer* renderer, const QString& path, const TextureParams& params)
	: StelTextureBackend(path)
	, textureParams(params)
	, renderer(renderer)
	, glTextureID(0)
	, ownsTexture(true)
	, loader(NULL)
{
	invariant();
}

StelQGLTextureBackend::~StelQGLTextureBackend()
{
	invariant();
	renderer->ensureTextureNotBound(this);
	if (getStatus() == TextureStatus_Loaded)
	{
		renderer->makeGLContextCurrent();
		if (glIsTexture(glTextureID) == GL_FALSE)
		{
			qDebug() << "WARNING: in StelQGLTextureBackend::~StelQGLTextureBackend() "
			            "tried to delete invalid texture with ID=" << glTextureID << 
			            " Current GL ERROR status is " << glErrorToString(glGetError());
		}
		else if(ownsTexture)
		{
			renderer->getGLContext()->deleteTexture(glTextureID);
		}
		glTextureID = 0;
	}
	if (loader != NULL) 
	{
		loader->abort();
		// Don't forget that the loader has no parent.
		loader->deleteLater();
		loader = NULL;
	}
}

void StelQGLTextureBackend::bind(const int textureUnit)
{
	invariant();
	Q_ASSERT_X(textureUnit >= 0, Q_FUNC_INFO,
	           "Trying to bind to a texture unit with negative index");

	renderer->makeGLContextCurrent();
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, glTextureID);
	invariant();
}

void StelQGLTextureBackend::startAsynchronousLoading()
{
	invariant();
	startedLoading();

	bool http = path.startsWith("http://");
	bool pvr  = path.endsWith(".pvr");

	Q_ASSERT_X(!(http && pvr), Q_FUNC_INFO,
	           "Can't load .pvr textures from network");

	if(pvr)
	{
		// We're using a single QGLContext::bindTexture() call to load .pvr,
		// and that must be done in the main (GL) thread.
		//
		// (TODO): It might be possible to load it manually through GL calls
		// and separate file loading (separate thread) from uploading to the GPU
		// (GL thread).
		qWarning() << "Trying to load a .pvr texture asynchronously."
		              "Asynchronous loading of .pvr is not implemented at the moment."
		              "Will load the texture normally (this might cause lag)";

		loadFromPVR();
		return;
	}

	if(http)
	{
		loader = new StelHTTPTextureLoader(path, 100, renderer->getLoaderThread());
		connect(loader, SIGNAL(finished(QImage)), this, SLOT(onImageLoaded(QImage)));
		connect(loader, SIGNAL(error(QString)), this, SLOT(onLoadingError(QString)));
		return;
	}

	loader = new StelFileTextureLoader(path, 100, renderer->getLoaderThread());
	connect(loader, SIGNAL(finished(QImage)), this, SLOT(onImageLoaded(QImage)));
	connect(loader, SIGNAL(error(QString)), this, SLOT(onLoadingError(QString)));
	invariant();
}

StelQGLTextureBackend* StelQGLTextureBackend::constructFromImage
	(StelQGLRenderer* renderer, const QString& path, 
	 const TextureParams& params, QImage& image)
{
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, path, params);
	result->startedLoading();
	result->loadFromImage(image);
	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::constructFromPVR
	(StelQGLRenderer* renderer, const QString& path, const TextureParams& params)
{
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, path, params);
	result->startedLoading();
	result->loadFromPVR();
	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::constructAsynchronous
	(StelQGLRenderer* renderer, const QString& path, const TextureParams& params)
{
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, path, params);
	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::fromFBO
	(StelQGLRenderer* renderer, QGLFramebufferObject* fbo)
{
	renderer->makeGLContextCurrent();

	const TextureParams params = TextureParams();
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, QString(), params);

	result->startedLoading();
	result->glTextureID = fbo->texture();
	// Prevent the StelQGLTextureBackend destructor from destroying the texture.
	result->ownsTexture = false;

	result->finishedLoading(fbo->size());

	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::fromViewport
	(StelQGLRenderer* renderer, const QSize viewportSize, const QGLFormat& viewportFormat)
{
	// This function should only be used as a fallback for when FBOs aren't supported.

	// Get image and GL pixel format matching viewport format.
	int glFormat;

	const int r = viewportFormat.redBufferSize();
	const int g = viewportFormat.greenBufferSize();
	const int b = viewportFormat.blueBufferSize();
	const int a = viewportFormat.alpha() ? viewportFormat.alphaBufferSize() : 0;

	if(r == 8 && g == 8 && b == 8 && a == 8)     {glFormat = GL_RGBA8;}
	else if(r == 8 && g == 8 && b == 8 && a == 0){glFormat = GL_RGB8;}
	else if(r == 5 && g == 6 && b == 5 && a == 0){glFormat = GL_RGB5;}
	else
	{
		// This is extremely unlikely, but we can't rule it out.
		Q_ASSERT_X(false, Q_FUNC_INFO,
		           "Unknown screen vertex format when getting texture from viewport. "
		           "Switching to OpenGL2, disabling viewport effects or "
		           "chaning video mode bit depth might help");
		// Invalid value to avoid warnings.
		glFormat = -1;
	}

	// Creating a texture from a dummy image.
	QImage dummyImage(64, 64, QImage::Format_Mono);
	const TextureParams params = TextureParams();

	StelQGLTextureBackend* result = 
		new StelQGLTextureBackend(renderer, QString(), params);

	QGLContext* context = result->prepareContextForLoading();
	const GLuint glTextureID = 
	    context->bindTexture(dummyImage, GL_TEXTURE_2D, glFormat,
	                         getTextureBindOptions(params));

	// Need a power-of-two texture (as this is used mainly with GL1)
	const QSize size = 
		StelUtils::smallestPowerOfTwoSizeGreaterOrEqualTo(viewportSize);

	// Set viewport so it matches the size of the texture
	glViewport(0, 0, size.width(), size.height());
	// Copy viewport to texture (this overrides texture size - we must use POT for GL1)
	glCopyTexImage2D(GL_TEXTURE_2D, 0, glFormat, 0, 0, size.width(), size.height(), 0);
	// Restore viewport
	glViewport(0, 0, viewportSize.width(), viewportSize.height());

	result->startedLoading();
	result->glTextureID = glTextureID;
	result->finishedLoading(size);

	return result;
}

void StelQGLTextureBackend::onImageLoaded(QImage image)
{
	invariant();
	Q_ASSERT_X(!image.isNull(), Q_FUNC_INFO,
	           "onImageLoaded slot was called even though the image failed to load");

	loader->deleteLater();
	loader = NULL;
	loadFromImage(image);
	invariant();
}

void StelQGLTextureBackend::onLoadingError(const QString& errorMessage) 
{
	loader->deleteLater();
	loader = NULL;
	errorOccured(errorMessage);
}

//Image struct is copied (data is not - implicit sharing), so that
//ensureTextureSizeWithinLimits won't affect the original image.
void StelQGLTextureBackend::loadFromImage(QImage image)
{
	invariant();

	if(image.isNull())
	{
		errorOccured("loadFromImage(): Image data to load from is null. "
		             "Maybe the image failed to load from file?");
		invariant();
		return;
	}

	// Shrink if needed.
	glEnsureTextureSizeWithinLimits(image);

	QGLContext* context = prepareContextForLoading();
	glTextureID = context->bindTexture(image, GL_TEXTURE_2D, glGetPixelFormat(image),
	                                   getTextureBindOptions(textureParams));
	if(!renderer->areNonPowerOfTwoTexturesSupported() && 
		(!StelUtils::isPowerOfTwo(image.width()) ||
		 !StelUtils::isPowerOfTwo(image.height())))
	{
		errorOccured("loadFromImage(): Failed to load because the image has "
		             "non-power-of-two width and/or height while the renderer "
		             "backend only supports power-of-two dimensions");
		invariant();
		return;
	}
	if(glTextureID == 0)
	{
		errorOccured("loadFromImage(): Failed to load an image to a GL texture");
		invariant();
		return;
	}
	setTextureWrapping();

	completeLoading();                       
	invariant();
}

void StelQGLTextureBackend::loadFromPVR()
{
	invariant();
	//NOTE: We ignore bind options (mipmapping and filtering type) from params
	//      at the moment.
	//
	//      We also ignore maximum texture size (QGLContext::bindTexture should 
	//      fail if the texture is too large.
	//
	//      QGLContext::bindTexture handles gl texture format for us.
	QGLContext* context = prepareContextForLoading();
	glTextureID = context->bindTexture(path);
	if(glTextureID == 0)
	{
		errorOccured("loadFromPVR() failed to load a PVR file to a GL texture - "
		             "Maybe the file \"" + path + "\" is missing?");
		invariant();
		return;
	}
	// For some reason only LINEAR seems to work.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	setTextureWrapping();

	completeLoading();
	invariant();
}

QGLContext* StelQGLTextureBackend::prepareContextForLoading()
{
	renderer->makeGLContextCurrent();
	// Apparently needed for GLES2 (test?)
	glActiveTexture(GL_TEXTURE0);
	return renderer->getGLContext();
}

void StelQGLTextureBackend::completeLoading()
{
	// Determine texture size.
	GLint width, height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	if(width == 0 || height == 0)
	{
		qWarning() << "Zero-area texture: " << width << "x" << height;
	}
	finishedLoading(QSize(width, height));
}

void StelQGLTextureBackend::setTextureWrapping()
{
	const GLint wrap = glTextureWrap(textureParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}
