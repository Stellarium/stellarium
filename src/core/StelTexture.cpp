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
#include "glues.h"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"

#include <QImageReader>
#include <QSize>
#include <QDebug>
#include <QUrl>
#include <QImage>
#include <QNetworkReply>
#include <QtEndian>
#include <QFuture>
#include <QtConcurrent>

#include <cstdlib>

StelTexture::StelTexture() : networkReply(NULL), loader(NULL), errorOccured(false), alphaChannel(false), id(0), avgLuminance(-1.f)
{
	width = -1;
	height = -1;
}

StelTexture::~StelTexture()
{
	if (id != 0)
	{
		if (glIsTexture(id)==GL_FALSE)
		{
			qDebug() << "WARNING: in StelTexture::~StelTexture() tried to delete invalid texture with ID=" << id << "Current GL ERROR status is" << glGetError() << "(" << getGLErrorText(glGetError()) << ")";
		}
		else
		{
			glDeleteTextures(1, &id);
		}
		id = 0;
	}
	if (networkReply != NULL)
	{
		networkReply->abort();
		networkReply->deleteLater();
	}
	if (loader != NULL) {
		delete loader;
		loader = NULL;
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
	emit(loadingProcessFinished(true));
}

StelTexture::GLData StelTexture::imageToGLData(const QImage &image)
{
	GLData ret = GLData();
	if (image.isNull())
		return ret;
	ret.width = image.width();
	ret.height = image.height();
	ret.data = convertToGLFormat(image, &ret.format, &ret.type);
	return ret;
}

/*************************************************************************
 Defined to be passed to QtConcurrent::run
 *************************************************************************/
StelTexture::GLData StelTexture::loadFromPath(const QString &path)
{
	return imageToGLData(QImage(path));
}

StelTexture::GLData StelTexture::loadFromData(const QByteArray& data)
{
	return imageToGLData(QImage::fromData(data));
}

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/

bool StelTexture::bind(int slot)
{
	if (id != 0)
	{
		// The texture is already fully loaded, just bind and return true;
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	// If the file is remote, start a network connection.
	if (loader == NULL && networkReply == NULL && fullPath.startsWith("http://")) {
		QNetworkRequest req = QNetworkRequest(QUrl(fullPath));
		// Define that preference should be given to cached files (no etag checks)
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(networkReply, SIGNAL(finished()), this, SLOT(onNetworkReply()));
		return false;
	}
	// The network connection is still running.
	if (networkReply != NULL)
		return false;
	// Not a remote file, start a loader from local file.
	if (loader == NULL)
	{
		loader = new QFuture<GLData>(QtConcurrent::run(loadFromPath, fullPath));
		return false;
	}
	// Wait until the loader finish.
	if (!loader->isFinished())
		return false;
	// Finally load the data in the main thread.
	glLoad(loader->result());
	delete loader;
	loader = NULL;
	return true;
}

void StelTexture::onNetworkReply()
{
	Q_ASSERT(loader == NULL);
	if (networkReply->error() != QNetworkReply::NoError)
	{
		reportError(networkReply->errorString());
	}
	else
	{
		QByteArray data = networkReply->readAll();
		loader = new QFuture<GLData>(QtConcurrent::run(loadFromData, data));
	}
	networkReply->deleteLater();
	networkReply = NULL;
}

/*************************************************************************
 Return the width and heigth of the texture in pixels
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

QByteArray StelTexture::convertToGLFormat(const QImage& image, GLint *format, GLint *type)
{
	QByteArray ret;
	const int width = image.width();
	const int height = image.height();
	if (image.isGrayscale())
	{
		*format = image.hasAlphaChannel() ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
	}
	else if (image.hasAlphaChannel())
	{
		*format = GL_RGBA;
	}
	else
		*format = GL_RGB;
	*type = GL_UNSIGNED_BYTE;
	int bpp = *format == GL_LUMINANCE_ALPHA ? 2 :
			  *format == GL_LUMINANCE ? 1 :
			  *format == GL_RGBA ? 4 :
			  3;

	ret.reserve(width * height * bpp);
	QImage tmp = image.convertToFormat(QImage::Format_ARGB32);

	// flips bits over y
	int ipl = tmp.bytesPerLine() / 4;
	for (int y = 0; y < height / 2; ++y)
	{
		int *a = (int *) tmp.scanLine(y);
		int *b = (int *) tmp.scanLine(height - y - 1);
		for (int x = 0; x < ipl; ++x)
			qSwap(a[x], b[x]);
	}

	// convert data
	for (int i = 0; i < height; ++i)
	{
		uint *p = (uint *) tmp.scanLine(i);
		for (int x = 0; x < width; ++x)
		{
			uint c = qToBigEndian(p[x]);
			const char* ptr = (const char*)&c;
			switch (*format)
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
	if (data.data.isEmpty())
	{
		reportError("Unknown error");
		return false;
	}

	width = data.width;
	height = data.height;

	//check minimum texture size
	GLint maxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxSize);
	if(maxSize < width || maxSize < height)
	{
		reportError(QString("Texture size (%1/%2) is larger than GL_MAX_TEXTURE_SIZE (%3)!").arg(width).arg(height).arg(maxSize));
		return false;
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, loadParams.filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadParams.filtering);

	//the conversion from QImage may result in tightly packed scanlines that are no longer 4-byte aligned!
	//--> we have to set the GL_UNPACK_ALIGNMENT accordingly

	//remember current alignment
	GLint oldalignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT,&oldalignment);

	switch(data.format)
	{
		case GL_RGBA:
			//RGBA pixels are always in 4 byte aligned rows
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			alphaChannel = true;
			break;
		case GL_LUMINANCE_ALPHA:
			//these ones are at least always in 2 byte aligned rows, but may also be 4 aligned
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			alphaChannel = true;
			break;
		default:
			//for the other cases, they may be on any alignment (depending on image width)
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			alphaChannel = false;
	}

	//do pixel transfer
	glTexImage2D(GL_TEXTURE_2D, 0, data.format, width, height, 0, data.format,
				 data.type, data.data.constData());

	//restore old value
	glPixelStorei(GL_UNPACK_ALIGNMENT, oldalignment);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadParams.wrapMode);
	if (loadParams.generateMipmaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadParams.filterMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	// Report success of texture loading
	emit(loadingProcessFinished(false));
	return true;
}

// Actually load the texture to openGL memory
bool StelTexture::glLoad(const QImage& image)
{
	return glLoad(imageToGLData(image));
}
