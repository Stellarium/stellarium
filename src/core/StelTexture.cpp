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
#include <cstdlib>
#include "StelTextureMgr.hpp"
#include "StelTexture.hpp"
#include "glues.h"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"

#include <QThread>
#include <QMutexLocker>
#include <QSemaphore>
#include <QImageReader>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QSize>
#include <QDebug>
#include <QUrl>
#include <QImage>
#include <QNetworkReply>
#include <QTimer>
#include <QtEndian>
#include <QFuture>
#include <QtConcurrent>

StelTexture::StelTexture() : loader(NULL), errorOccured(false), id(0), avgLuminance(-1.f)
{
	width = -1;
	height = -1;
	initializeOpenGLFunctions();
}

StelTexture::~StelTexture()
{
	if (id != 0)
	{
		if (glIsTexture(id)==GL_FALSE)
		{
			qDebug() << "WARNING: in StelTexture::~StelTexture() tried to delete invalid texture with ID=" << id << " Current GL ERROR status is " << glGetError();
		}
		else
		{
			glDeleteTextures(1, &id);
		}
		id = 0;
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

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/

bool StelTexture::bind()
{
	if (id != 0)
	{
		// The texture is already fully loaded, just bind and return true;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	if (loader == NULL)
	{
		loader = new QFuture<QImage>(QtConcurrent::run(StelTexture::load, fullPath));
		return false;
	}
	if (!loader->isFinished())
		return false;
	glLoad(loader->result());
	delete loader;
	loader = NULL;
	return true;
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

QImage StelTexture::load(const QString &path)
{
	if (path.startsWith("http://")) {
		QNetworkRequest req = QNetworkRequest(QUrl(path));
		// Define that preference should be given to cached files (no etag checks)
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
		QNetworkReply *reply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		// We block until we get a reply.
		QEventLoop loop;
		connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		loop.exec();
		if (reply->error() != QNetworkReply::NoError)
		{
			qWarning() << reply->errorString();
			return QImage();
		}
		return QImage::fromData(reply->readAll());
	}
	else // Directly load from a file.
	{
		return QImage(path);
	}
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


// Actually load the texture to openGL memory
bool StelTexture::glLoad(const QImage& image)
{
	if (image.isNull())
	{
		reportError("Unknown error");
		return false;
	}
	width = image.width();
	height = image.height();
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, loadParams.filtering);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadParams.filtering);

	GLint format, type;
	QByteArray data = convertToGLFormat(image, &format, &type);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
				 type, data.constData());
	bool genMipmap = false;
	if (genMipmap)
		glGenerateMipmap(GL_TEXTURE_2D);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadParams.wrapMode);

	// Report success of texture loading
	emit(loadingProcessFinished(false));
	return true;
}
