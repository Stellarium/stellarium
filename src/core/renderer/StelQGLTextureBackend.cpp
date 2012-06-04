#include <QGLContext>

#include "StelQGLTextureBackend.hpp"
#include "StelQGLRenderer.hpp"
#include "StelGLUtilityFunctions.hpp"
#include "renderer/StelTextureLoader.hpp"


//! Get QGL bind options corresponding to specified StelTextureParams.
//!
//! These are options that are specified at the moment the texture
//! is bound in QGLContext.
//!
//! @param params StelTextureParams to get bind options for.
//! @return QGL bind options.
static QGLContext::BindOptions getTextureBindOptions(const StelTextureParams& params)
{
	QGLContext::BindOptions result = QGLContext::InvertedYBindOption;
	// Set GL texture filtering mode.
	switch(params.filteringMode)
	{
		case Nearest: break;
		case Linear:  result |= QGLContext::LinearFilteringBindOption; break;
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
	(StelQGLRenderer* renderer, const QString& path, const StelTextureParams& params)
	: StelTextureBackend(path)
	, textureParams(params)
	, renderer(renderer)
	, glTextureID(0)
	, loader(NULL)
{
	invariant();
}

StelQGLTextureBackend::~StelQGLTextureBackend()
{
	invariant();
	if (getStatus() == TextureStatus_Loaded)
	{
		renderer->makeGLContextCurrent();
		if (glIsTexture(glTextureID) == GL_FALSE)
		{
			qDebug() << "WARNING: in StelQGLTextureBackend::~StelQGLTextureBackend() "
			            "tried to delete invalid texture with ID=" << glTextureID << 
			            " Current GL ERROR status is " << glErrorToString(glGetError());
		}
		else
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
	renderer->makeGLContextCurrent();
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, glTextureID);
	invariant();
}

void StelQGLTextureBackend::startLoadingInThread()
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
	 const StelTextureParams& params, QImage& image)
{
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, path, params);
	result->startedLoading();
	result->loadFromImage(image);
	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::constructFromPVR
	(StelQGLRenderer* renderer, const QString& path, const StelTextureParams& params)
{
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, path, params);
	result->startedLoading();
	result->loadFromPVR();
	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::constructAsynchronous
	(StelQGLRenderer* renderer, const QString& path, const StelTextureParams& params)
{
	StelQGLTextureBackend* result = new StelQGLTextureBackend(renderer, path, params);
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
	errorOccured(errorMessage);
}

//Image struct is copied (data is not - implicit sharing), so that
//ensureTextureSizeWithinLimits won't affect the original image.
void StelQGLTextureBackend::loadFromImage(QImage image)
{
	invariant();
	Q_ASSERT_X(!image.isNull(), Q_FUNC_INFO, "Image data is NULL");

	// Shrink if needed.
	glEnsureTextureSizeWithinLimits(image);

	QGLContext* context = prepareContextForLoading();
	glTextureID = context->bindTexture(image, GL_TEXTURE_2D, glGetPixelFormat(image),
	                                   getTextureBindOptions(textureParams));
	if(glTextureID == 0)
	{
		errorOccured("loadFromImage() failed to load an image to a GL texture");
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
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
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
