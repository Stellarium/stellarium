/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelMainView.hpp"
#include "StelOpenGL.hpp"


#include <QImageReader>
#include <QSize>
#include <QDebug>
#include <QUrl>
#include <QImage>
#include <QNetworkReply>
#include <QtEndian>
#include <QFuture>
#include <QtConcurrent>

#ifndef GL_TEXTURE_MAX_ANISOTROPY
# define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif

Q_LOGGING_CATEGORY(Tex,"stel.Texture", QtInfoMsg)

// Let's try to keep 120 FPS even if textures are loaded every frame
// (60 FPS if all the other computations take the same time per frame).
constexpr double MAX_LOAD_NANOSEC_PER_FRAME = 1e9 / 120;

QPointer<StelTextureMgr> StelTexture::textureMgr;

StelTexture::StelTexture()
{
	// Note: we don't set gl to OpenGL functions here, because this
	// constructor may be called from another thread, which doesn't
	// have a GL context associated with it.
}

StelTexture::~StelTexture()
{
	// Don't try to access texture manager or OpenGL context if texture
	// manager has been deleted, because at this point we are shutting
	// down, and the OpenGL context may have also been deleted.
	if (!textureMgr) return;

	if (id != 0)
	{
		/// FS: make sure the correct GL context is bound!
		/// Causes #595 flicker in Night Mode with DSS/HiPS. Tentatively remove this.
		//StelApp::getInstance().ensureGLContextCurrent();

		if (gl->glIsTexture(id)==GL_FALSE)
		{
			GLenum err = gl->glGetError();
			qCWarning(Tex) << "StelTexture::~StelTexture() tried to delete invalid texture with ID="
				       << id << "Current GL ERROR status is" << err << "(" << StelOpenGL::getGLErrorText(err) << ")";
		}
		else
		{
			gl->glDeleteTextures(1, &id);
			textureMgr->glMemoryUsage -= glSize;
			textureMgr->idMap.remove(id);
			glSize = 0;
		}
#ifndef NDEBUG
		qCDebug(Tex) << "Deleted StelTexture" << id << ", total memory usage "
			     << textureMgr->glMemoryUsage / (1024.0 * 1024.0)<<"MB";
#endif
		id = 0;
	}
	else if (id)
	{
		qCWarning(Tex)<<"Cannot delete texture"<<id<<", no GL context";
	}
	if (networkReply)
	{
		networkReply->abort();
		//networkReply->deleteLater();
		delete networkReply;
		networkReply = Q_NULLPTR;
	}
	if (loader != Q_NULLPTR) {
		delete loader;
		loader = Q_NULLPTR;
	}
}

void StelTexture::wrapGLTexture(GLuint texId)
{
	gl = QOpenGLContext::currentContext()->functions();
	bool valid = gl->glIsTexture(texId);
	if(valid)
	{
		id = texId;
		//Note: there is no way to retrieve texture width/height on OpenGL ES
		//so the members will be wrong
		//also we can't estimate memory usage because of this
	}
	else
	{
		errorMessage="No valid OpenGL texture name";
		errorOccured=true;
	}
}

/*************************************************************************
 This method should be called if the texture loading failed for any reasons
 *************************************************************************/
void StelTexture::reportError(const QString& aerrorMessage)
{
	errorOccured = true;
	errorMessage = aerrorMessage;
	// Report failure of texture loading
	emit loadingProcessFinished(true);
}

StelTexture::GLData StelTexture::imageToGLData(const QImage &image, const int decimateBy)
{
	GLData ret = GLData();
	if (image.isNull())
		return ret;
	ret.data = convertToGLFormat(image, ret.format, ret.type, decimateBy, ret.width, ret.height);
	return ret;
}

/*************************************************************************
 Defined to be passed to QtConcurrent::run
 *************************************************************************/
StelTexture::GLData StelTexture::loadFromPath(const QString &path, const int decimateBy)
{
	try
	{
		QImageReader imgReader(path);
		QImage img = imgReader.read();
		if (img.isNull())
		{
			QImageReader::ImageReaderError error=imgReader.error();
			qCritical() << "Error reading image file " << path << ":" << imgReader.errorString();

			if (error==QImageReader::InvalidDataError)
				qCritical() << "This may also indicate an out-of-memory error.";
		}
		return imageToGLData(img, decimateBy);
	}
	catch(std::bad_alloc& ex) //this catches out-of-memory errors from file conversion
	{
		qCritical()<<"Failed loading texture from"<<path<<"error:"<<ex.what();
		qCritical() << "Try higher value for config.ini:[astro]/nebula_texture_decimation";
		GLData ret;
		ret.loaderError = ex.what();
		return ret;
	}
	catch(std::exception& ex) //this catches other errors from file conversion
	{
		qCritical()<<"Failed loading texture from"<<path<<"error:"<<ex.what();
		GLData ret;
		ret.loaderError = ex.what();
		return ret;
	}
}

StelTexture::GLData StelTexture::loadFromData(const QByteArray& data, const int decimateBy)
{
	try
	{
		return imageToGLData(QImage::fromData(data), decimateBy);
	}
	catch(std::exception& ex)  //this catches out-of-memory errors from file conversion
	{
		qCritical()<<"Failed loading texture"<<ex.what();
		GLData ret;
		ret.loaderError = ex.what();
		return ret;
	}
}

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/

bool StelTexture::bind(uint slot)
{
	if (id != 0)
	{
		// The texture is already fully loaded, just bind and return true;
		gl->glActiveTexture(GL_TEXTURE0 + slot);
		gl->glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	if(load())
	{
		// Take advantage of time given by the desired frame rate (which may be lower than maximum),
		// but don't give up too much if it's very high.
		const double maxTimeNS = std::max(1e9 / (2 * StelMainView::getInstance().getDesiredFps()),
		                                  MAX_LOAD_NANOSEC_PER_FRAME);
		if(textureMgr->getTotalLoadTimeTaken() > maxTimeNS)
			return false;
		gl = QOpenGLContext::currentContext()->functions();
		// Finally load the data in the main thread.
		gl->glActiveTexture(GL_TEXTURE0 + slot);
		textureMgr->reportTextureLoadStart();
		glLoad(loader->result());
		delete loader;
		loader = Q_NULLPTR;
		textureMgr->reportTextureLoadEnd();
		if (id != 0)
		{
			// The texture is already fully loaded, just bind and return true;
			gl->glBindTexture(GL_TEXTURE_2D, id);
			return true;
		}
		if (errorOccured)
			return false;
	}
	return false;
}

void StelTexture::waitForLoaded()
{
	if(networkReply)
	{
		qCWarning(Tex) << "StelTexture::waitForLoaded called for a network-loaded texture"<<fullPath;
		Q_ASSERT(0);
	}
	if(loader)
	{
		loader->waitForFinished();

		glLoad(loader->result());
		delete loader;
		loader = nullptr;
	}
}

template <typename T, typename...Params, typename...Args>
void StelTexture::startAsyncLoader(T (*functionPointer)(Params...), Args&&...args)
{
	Q_ASSERT(loader==Q_NULLPTR);
	if (!textureMgr) return;
	//own thread pool supported with Qt 5.4+
	loader = new QFuture<GLData>(QtConcurrent::run(textureMgr->loaderThreadPool,
	                                               functionPointer, std::forward<Args>(args)...));
}

bool StelTexture::load()
{
	// If the file is remote, start a network connection.
	if (loader == Q_NULLPTR && networkReply == Q_NULLPTR &&
			(fullPath.startsWith("http", Qt::CaseInsensitive) ||
			 fullPath.startsWith("file://", Qt::CaseInsensitive)))
	{
		QNetworkRequest req = QNetworkRequest(QUrl(fullPath));
		// Define that preference should be given to cached files (no etag checks)
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(networkReply, SIGNAL(finished()), this, SLOT(onNetworkReply()));
		return false;
	}
	// The network connection is still running.
	if (networkReply != Q_NULLPTR)
		return false;
	// Not a remote file, start a loader from local file.
	if (loader == Q_NULLPTR)
	{
		startAsyncLoader(static_cast<GLData(*)(const QString&, const int)>(loadFromPath),
		                 fullPath, loadParams.decimation);
		return false;
	}
	// Wait until the loader finish.
	return loader->isFinished();
}

void StelTexture::onNetworkReply()
{
	Q_ASSERT(loader == Q_NULLPTR);
	if (networkReply->error() == QNetworkReply::NoError && networkReply->bytesAvailable()>0)
	{
		QByteArray data = networkReply->readAll();
		if(data.isEmpty()) //prevent starting the loader when there is nothing to load
			reportError(QString("Empty result received for URL: %1").arg(networkReply->url().toString()));
		else
			startAsyncLoader(static_cast<GLData(*)(const QByteArray&, const int)>(loadFromData),
			                 data, loadParams.decimation);
	}
	else
		reportError(networkReply->errorString());

	networkReply->deleteLater();
	networkReply = Q_NULLPTR;
}

/*************************************************************************
 Return the width and height of the texture in pixels
*************************************************************************/
bool StelTexture::getDimensions(int &awidth, int &aheight)
{
	if (width<0 || height<0)
	{
		// Try to get the size from the file without loading data
		QImageReader im(fullPath);
		if (!im.canRead())
		{
			return false;
		}
		QSize size = im.size();
		width = size.width();
		height = size.height();
	}
	awidth = width;
	aheight = height;
	return true;
}

QByteArray StelTexture::convertToGLFormat(QImage image, GLint& format, GLint& type, int decimate, int& width, int& height)
{
	QByteArray ret;
	const auto glInfo = StelMainView::getInstance().getGLInformation();
	width = std::min(image.width()/decimate, glInfo.maxTextureSize);
	height = std::min(image.height()/decimate, glInfo.maxTextureSize);

#ifndef NDEBUG
	if (decimate>1)
		qCDebug(Tex) << "decimated texture width: " << image.width() << "/" << decimate << "->" << width;
#endif
	if(width != image.width() || height != image.height())
	{
		if (decimate == 1 || width > glInfo.maxTextureSize || height > glInfo.maxTextureSize)
			qCWarning(Tex).nospace() << "Got a texture with too large dimensions: "
						 << image.width() << "x" << image.height()
						 << ", while maximum size is " << glInfo.maxTextureSize
						 << ". Shrinking to fit in the limit.";
		image = image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}

	if (glInfo.supportsLuminanceTextures && image.isGrayscale())
	{
		format = image.hasAlphaChannel() ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
	}
	else if (image.hasAlphaChannel())
	{
		format = GL_RGBA;
	}
	else
		format = GL_RGB;
	type = GL_UNSIGNED_BYTE;
	int bpp = format == GL_LUMINANCE_ALPHA ? 2 :
	          format == GL_LUMINANCE ? 1 :
	          format == GL_RGBA ? 4 :
	                          3;

	ret.reserve(static_cast<long>(width) * height * bpp);
	QImage tmp = image.convertToFormat(QImage::Format_ARGB32);

	// convert data
	// we always use a tightly packed format, with 1-4 bpp
	// the image should be flipped over y, so read it backwards from the end
	for (int i = height - 1; i >= 0; --i)
	{
		uint *p = reinterpret_cast<uint *>( tmp.scanLine(i));
		for (int x = 0; x < width; ++x)
		{
			uint c = qToBigEndian(p[x]);
			const char* ptr = reinterpret_cast<const char*>(&c);
			switch (format)
			{
				case GL_RGBA:
					ret.append(ptr + 1, 3);
					ret.append(ptr, 1);
					break;
				case GL_RGB:
					ret.append(ptr + 1, 3);
					break;
				case GL_LUMINANCE:
					ret.append(ptr + 1, 1);
					break;
				case GL_LUMINANCE_ALPHA:
					ret.append(ptr + 1, 1);
					ret.append(ptr, 1);
					break;
				default:
					Q_ASSERT(false);
			}
		}
	}
	return ret;
}

bool StelTexture::glLoad(const GLData& data)
{
	// Don't try to access texture manager or OpenGL context if texture
	// manager has been deleted, because at this point we are shutting
	// down, and the OpenGL context may have also been deleted.
	if (!textureMgr) return false;

	if (data.data.isEmpty())
	{
		reportError(data.loaderError.isEmpty() ? "Unknown error" : data.loaderError);
		return false;
	}

	width = data.width;
	height = data.height;

	/// FS: make sure the correct GL context is bound!
	/// Causes #595 flicker in Night Mode with DSS/HiPS. Tentatively remove this.
	//StelApp::getInstance().ensureGLContextCurrent();
	gl = QOpenGLContext::currentContext()->functions();

	//check minimum texture size
	GLint maxSize;
	gl->glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxSize);
	if(maxSize < width || maxSize < height)
	{
		reportError(QString("Texture size (%1/%2) is larger than GL_MAX_TEXTURE_SIZE (%3)!")
		                .arg(width).arg(height).arg(maxSize));
		return false;
	}

	gl->glGenTextures(1, &id);
	gl->glBindTexture(GL_TEXTURE_2D, id);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, loadParams.filtering);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadParams.filtering);

	//the conversion from QImage may result in tightly packed scanlines that are no longer 4-byte aligned!
	//--> we have to set the GL_UNPACK_ALIGNMENT accordingly

	//remember current alignment
	GLint oldalignment;
	gl->glGetIntegerv(GL_UNPACK_ALIGNMENT,&oldalignment);

	switch(data.format)
	{
		case GL_RGBA:
			//RGBA pixels are always in 4 byte aligned rows
			gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			alphaChannel = true;
			break;
		case GL_LUMINANCE_ALPHA:
			//these ones are at least always in 2 byte aligned rows, but may also be 4 aligned
			gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			alphaChannel = true;
			break;
		default:
			//for the other cases, they may be on any alignment (depending on image width)
			gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			alphaChannel = false;
	}

	//do pixel transfer
	gl->glTexImage2D(GL_TEXTURE_2D, 0, data.format, width, height, 0, static_cast<GLenum>(data.format),
	                 static_cast<GLenum>(data.type), data.data.constData());

	//for now, assume full sized 8 bit GL formats used internally
	glSize = static_cast<uint>(data.data.size());

	//restore old value
	gl->glPixelStorei(GL_UNPACK_ALIGNMENT, oldalignment);

	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadParams.wrapMode);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadParams.wrapMode);
	if (loadParams.generateMipmaps)
	{
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		                    loadParams.filterMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);

		const auto conf = StelApp::getInstance().getSettings();
		const GLint desiredAnisotropyLevel = conf->value("video/anisotropic_filtering", 16).toUInt();
		const auto maxAnisotropy = StelMainView::getInstance().getGLInformation().maxAnisotropy;
		if(maxAnisotropy > 0 && desiredAnisotropyLevel > 0)
		{
			const int anisotropy = std::min(maxAnisotropy, desiredAnisotropyLevel);
			gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);

			// Assuming maximum possible anisotropy, all mipmaps will require
			// 4× the size of base mip level. Generally anisotropy may be
			// lower, but we prefer to overestimate VRAM consumption than to
			// underestimate it.
			glSize *= 4;
		}
		else
		{
			glSize = glSize + glSize/3; //mipmaps require 1/3 more mem
		}
		gl->glGenerateMipmap(GL_TEXTURE_2D);
	}

	//register ID with textureMgr and increment size
	textureMgr->glMemoryUsage += glSize;
	textureMgr->idMap.insert(id,sharedFromThis());

#ifndef NDEBUG
	qCDebug(Tex) << "StelTexture" << id << "of size" << width << u8"×" << height
		     << "uploaded, total memory usage "
		     << textureMgr->glMemoryUsage / (1024.0 * 1024.0) << "MB";
#endif

	// Report success of texture loading
	emit loadingProcessFinished(false);
	return true;
}

// Actually load the texture to openGL memory
bool StelTexture::glLoad(const QImage& image)
{
	return glLoad(imageToGLData(image, loadParams.decimation));
}
