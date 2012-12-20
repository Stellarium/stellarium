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
	// TODO investigate (maybe we're using deprecated stuff to generate mipmaps?)
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
	, deleteManually(false)
	, loader(NULL)
	, pixelBytes(0.0f)
{
	renderer->getStatistics()[TEXTURES_CREATED] += 1.0;
	invariant();
}

StelQGLTextureBackend::~StelQGLTextureBackend()
{
	invariant();
	renderer->ensureTextureNotBound(this);
	
	if (getStatus() == TextureStatus_Loaded)
	{
		const QSize size = getDimensions();
		renderer->getStatistics()[ESTIMATED_TEXTURE_MEMORY] -= 
			size.width() * size.height() * pixelBytes;
		renderer->makeGLContextCurrent();
		if (glIsTexture(glTextureID) == GL_FALSE)
		{
			qDebug() << "WARNING: in StelQGLTextureBackend::~StelQGLTextureBackend() "
			            "tried to delete invalid texture with ID=" << glTextureID << 
			            " Current GL ERROR status is " << glErrorToString(glGetError());
		}
		else if(ownsTexture)
		{
			// Texture was uploaded manually, not through the QGL context object.
			if(deleteManually)
			{
				glDeleteTextures(1, &glTextureID);
			}
			else
			{
				renderer->getGLContext()->deleteTexture(glTextureID);
			}
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
	QGLFunctions& gl = renderer->getGLFunctions();
	gl.glActiveTexture(GL_TEXTURE0 + textureUnit);
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

	// To simplify code, we assume 4 bytes per fixel (32bit RGBA) - this 
	// is most common, but might not always be the case.
	result->pixelBytes = 4.0f;
	renderer->getStatistics()[ESTIMATED_TEXTURE_MEMORY]
		+= fbo->size().width() * fbo->size().height() * result->pixelBytes;

	result->finishedLoading(fbo->size());

	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::fromViewport
	(StelQGLRenderer* renderer, const QSize viewportSize, const QGLFormat& viewportFormat)
{
	// This function should only be used as a fallback for when FBOs aren't supported.


	const int r = viewportFormat.redBufferSize();
	const int g = viewportFormat.greenBufferSize();
	const int b = viewportFormat.blueBufferSize();
	const int a = viewportFormat.alpha() ? viewportFormat.alphaBufferSize() : 0;

	// Creating a texture from a dummy image.
	QImage dummyImage(64, 64, QImage::Format_Mono);
	const TextureParams params = TextureParams();

	StelQGLTextureBackend* result = 
		new StelQGLTextureBackend(renderer, QString(), params);

	// Get image and GL pixel format matching viewport format.
	int glFormat;

	if(r == 8 && g == 8 && b == 8 && a == 8)     
	{
#ifdef USE_OPENGL_ES2
        glFormat           = GL_RGBA;
#else
		glFormat           = GL_RGBA8;
#endif
		result->pixelBytes = 4.0f;
	}
	else if(r == 8 && g == 8 && b == 8 && a == 0)
	{
#ifdef USE_OPENGL_ES2
        glFormat           = GL_RGB;
#else
		glFormat           = GL_RGB8;
#endif
		result->pixelBytes = 3.0f;
	}
	else if(r == 5 && g == 6 && b == 5 && a == 0)
	{
#ifdef USE_OPENGL_ES2
        glFormat           = GL_RGB565;
#else
        glFormat           = GL_RGB5;
#endif
		result->pixelBytes = 2.0f;
	}
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

	// Will need change if different screen bit depths are ever supported
	renderer->getStatistics()[ESTIMATED_TEXTURE_MEMORY]
		+= size.width() * size.height() * result->pixelBytes;

	result->startedLoading();
	result->glTextureID = glTextureID;
	result->finishedLoading(size);

	return result;
}

StelQGLTextureBackend* StelQGLTextureBackend::fromRawData
	(StelQGLRenderer* renderer, const void* data, const QSize size, 
	 const TextureDataFormat format, const TextureParams& params)
{
	StelQGLTextureBackend* result = 
		new StelQGLTextureBackend(renderer, QString(), params);
	result->startedLoading();

	// Check if texture dimensions are acceptable for GL
	if(!glTextureSizeWithinLimits(size, format))
	{
		result->errorOccured("fromRawData(): Texture size too large");
		return result;
	}

#ifdef USE_OPENGL_ES2
    //Cannot grab texture width/height in OpenGL ES 2, so force it and hope the texture loaded in
    result->setImageSize(size);
#endif

	if(!renderer->areNonPowerOfTwoTexturesSupported() && 
		(!StelUtils::isPowerOfTwo(size.width()) ||
		 !StelUtils::isPowerOfTwo(size.height())))
	{
		result->errorOccured("fromRawData(): Failed to load because the image has "
		                    "non-power-of-two width and/or height while the renderer "
		                    "backend only supports power-of-two dimensions");
		return result;
	}

	result->prepareContextForLoading();

	// Create the GL texture object.
	GLuint glTextureID;
	// According to GL docs, this always succeeds.
	glGenTextures(1, &glTextureID);
	glBindTexture(GL_TEXTURE_2D, glTextureID);

	// Set texture parameters.
	switch(params.filteringMode)
	{
		case TextureFiltering_Nearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		  	break;
		case TextureFiltering_Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture filtering mode");
	}
	// TODO investigate if this is still buggy
	// Mipmap seems to be pretty buggy on windows
#if !defined(Q_OS_WIN) && !defined(USE_OPENGL_ES2)
	if (params.autoGenerateMipmaps)
	{
		if(QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_Version_1_4))
		{
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		}
		else
		{
			qDebug() << "Ignoring automatic mipmap generation, requires OpenGL 1.4";
		}
	}
#endif

	// Determine GL texture format.
	switch(format)
	{
		case TextureDataFormat_RGBA_F32: result->pixelBytes = 16.0f; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture data format"); break;
	}
	const GLint internalFormat = glGetTextureInternalFormat(format);
	const GLenum loadFormat    = glGetTextureLoadFormat(format);
	const GLenum type          = glGetTextureType(format);

	// Flushes any previous errors.
	checkGLErrors("fromRawData() before texture data upload");
	// Upload the texture
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.width(), size.height(), 0,
	             loadFormat, type, data);

#ifdef USE_OPENGL_ES2
    glGenerateMipmap(GL_TEXTURE_2D);
#endif

	// Check for errors during upload.
	const GLenum glError = glGetError();
	if(glError == GL_INVALID_VALUE)
	{
		glDeleteTextures(1, &glTextureID);
		result->errorOccured("fromRawData(): Failed with GL_INVALID_VALUE, "
		                     "maybe the texture is too big?");
		return result;
	}
	if(glError != GL_NO_ERROR)
	{
		glDeleteTextures(1, &glTextureID);
		result->errorOccured("fromRawData(): Failed with an unknown error. "
		                     "NOTE: THIS IS EITHER A BUG IN CODE (FORMAT MISMATCH?) "
		                     "THAT NEEDS TO BE FIXED OR A NEW ERROR CONDITION THAT "
		                     "NEEDS AN APPROPRIATE ERROR MESSAGE");
		return result;
	}

	renderer->getStatistics()[ESTIMATED_TEXTURE_MEMORY]
		+= size.width() * size.height() * result->pixelBytes;
	// Finish constructing the texture.
	result->setTextureWrapping();
	result->glTextureID = glTextureID;
	result->deleteManually = true;
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

#ifdef USE_OPENGL_ES2
    //Cannot grab texture width/height in OpenGL ES 2, so force it and hope the texture loaded in
    setImageSize(image.size());
#endif

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

	GLint internalFormat = glGetTextureInternalFormat(image);
	switch(internalFormat)
	{
#ifndef USE_OPENGL_ES2
		case GL_RGBA8:             pixelBytes = 4.0f; break;
		case GL_RGB8:              pixelBytes = 3.0f; break;
		case GL_LUMINANCE8_ALPHA8: pixelBytes = 2.0f; break;
		case GL_LUMINANCE8:        pixelBytes = 1.0f; break;
#else
        case GL_RGBA:               pixelBytes = 4.0f; break;
        case GL_RGB:                pixelBytes = 3.0f; break;
        case GL_LUMINANCE_ALPHA:    pixelBytes = 2.0f; break;
        case GL_LUMINANCE:          pixelBytes = 1.0f; break;
#endif
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown GL internal format for QImage");
	}

	QGLContext* context = prepareContextForLoading();
	glTextureID = context->bindTexture(image, GL_TEXTURE_2D, internalFormat,
	                                   getTextureBindOptions(textureParams));
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

	// This is an assumption; PVRTC can be either 4 or 2 bits per pixel - we assume the worse case.
	pixelBytes = 0.5f;

	completeLoading();
	invariant();
}

QGLContext* StelQGLTextureBackend::prepareContextForLoading()
{
	renderer->makeGLContextCurrent();
	// Apparently needed for GLES2 (test?)
	QGLFunctions& gl = renderer->getGLFunctions();
	gl.glActiveTexture(GL_TEXTURE0);
	return renderer->getGLContext();
}

void StelQGLTextureBackend::completeLoading()
{
	// Determine texture size.
#ifndef USE_OPENGL_ES2
	GLint width, height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	if(width == 0 || height == 0)
	{
		qWarning() << "Zero-area texture: " << width << "x" << height;
	}
	renderer->getStatistics()[ESTIMATED_TEXTURE_MEMORY]
		+= width * height * pixelBytes;
	finishedLoading(QSize(width, height));
#else
    finishedLoading(getDimensionsEarly());
#endif
}

void StelQGLTextureBackend::setTextureWrapping()
{
	const GLint wrap = glTextureWrap(textureParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}
