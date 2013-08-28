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
#include <QGLContext>

ImageLoader::ImageLoader(const QString& path, int delay)
	: QObject(), path(path), networkReply(NULL)
{
	QTimer::singleShot(delay, this, SLOT(start()));
}

void ImageLoader::abort()
{
	// XXX: Assert that we are in the main thread.
	if (networkReply != NULL)
	{
		networkReply->abort();
	}
}

void ImageLoader::start()
{
	if (path.startsWith("http://")) {
		QNetworkRequest req = QNetworkRequest(QUrl(path));
		// Define that preference should be given to cached files (no etag checks)
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(networkReply, SIGNAL(finished()), this, SLOT(onNetworkReply()));
	} else {
		// At next loop iteration we start to load from the file.
		QTimer::singleShot(0, this, SLOT(directLoad()));
	}

	// Move this object outside of the main thread.
	StelTextureMgr* textureMgr = &StelApp::getInstance().getTextureManager();
	moveToThread(textureMgr->loaderThread);
}


void ImageLoader::onNetworkReply()
{
	if (networkReply->error() != QNetworkReply::NoError) {
		emit error(networkReply->errorString());
	} else {
		QByteArray data = networkReply->readAll();
		QImage image = QImage::fromData(data);
		if (image.isNull()) {
			emit error("Unable to parse image data");
		} else {
			emit finished(image);
		}
	}
	networkReply->deleteLater();
	networkReply = NULL;
}

void ImageLoader::directLoad() {
	QImage image = QImage(path);
	emit finished(image);
}

StelTexture::StelTexture() : loader(NULL), downloaded(false), isLoadingImage(false),
				   errorOccured(false), id(0), avgLuminance(-1.f)
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
		loader->abort();
		// Don't forget that the loader has no parent.
		loader->deleteLater();
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
	isLoadingImage = false;
	// Report failure of texture loading
	emit(loadingProcessFinished(true));
}

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/

bool StelTexture::bind()
{
	// qDebug() << "TEST bind" << fullPath;
	if (id != 0)
	{
		// The texture is already fully loaded, just bind and return true;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	if (!isLoadingImage && loader == NULL) {
		isLoadingImage = true;
		loader = new ImageLoader(fullPath, 100);
		connect(loader, SIGNAL(finished(QImage)), this, SLOT(onImageLoaded(QImage)));
		connect(loader, SIGNAL(error(QString)), this, SLOT(onLoadingError(QString)));
	}

	return false;
}

void StelTexture::onImageLoaded(QImage image)
{
	qImage = image;
	Q_ASSERT(!qImage.isNull());
	glLoad();
	isLoadingImage = false;
	loader->deleteLater();
	loader = NULL;
}

/*************************************************************************
 Return the width and heigth of the texture in pixels
*************************************************************************/
bool StelTexture::getDimensions(int &awidth, int &aheight)
{
	if (width<0 || height<0)
	{

		if (!qImage.isNull())
		{
			width = qImage.width();
			height = qImage.height();
		}
		else
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

	}
	awidth = width;
	aheight = height;
	return true;
}


// Actually load the texture to openGL memory
bool StelTexture::glLoad()
{
	if (qImage.isNull())
	{
		errorOccured = true;
		reportError("Unknown error");
		return false;
	}

	GLint internalFormat;
	if (qImage.isGrayscale())
	{
		internalFormat = qImage.hasAlphaChannel() ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
	}
	else if (qImage.hasAlphaChannel())
	{
		internalFormat = GL_RGBA;
	}
	else
		internalFormat = GL_RGB;

	// Q_ASSERT(StelPainter::glContext==QGLContext::currentContext());
	glActiveTexture(GL_TEXTURE0);
	
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	
	bool genMipmap = false;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, loadParams.filtering);	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadParams.filtering);
	
	QImage::Format target_format = qImage.format();
	GLenum externalFormat;
	GLuint pixel_type;
	
	externalFormat = GL_RGBA;
	pixel_type = GL_UNSIGNED_BYTE;
	
	switch (target_format) {
	case QImage::Format_ARGB32:
		break;
	case QImage::Format_ARGB32_Premultiplied:
			qImage = qImage.convertToFormat(target_format = QImage::Format_ARGB32);
		break;
	case QImage::Format_RGB16:
		pixel_type = GL_UNSIGNED_SHORT_5_6_5;
		externalFormat = GL_RGB;
		internalFormat = GL_RGB;
		break;
	case QImage::Format_RGB32:
		break;
	default:
		if (qImage.hasAlphaChannel()) {
			qImage = qImage.convertToFormat(QImage::Format_ARGB32);
		} else {
			qImage = qImage.convertToFormat(QImage::Format_RGB32);
		}
	}
	
	// flips bits over y
	int ipl = qImage.bytesPerLine() / 4;
	int h = qImage.height();
	for (int y=0; y<h/2; ++y) {
		int *a = (int *) qImage.scanLine(y);
		int *b = (int *) qImage.scanLine(h - y - 1);
		for (int x=0; x<ipl; ++x)
			qSwap(a[x], b[x]);
	}
	
	if (externalFormat == GL_RGBA) {
		// The only case where we end up with a depth different from
		// 32 in the switch above is for the RGB16 case, where we set
		// the format to GL_RGB
		Q_ASSERT(qImage.depth() == 32);
		const int width = qImage.width();
		const int height = qImage.height();
	
		if (pixel_type == GL_UNSIGNED_INT_8_8_8_8_REV
			|| (pixel_type == GL_UNSIGNED_BYTE && QSysInfo::ByteOrder == QSysInfo::LittleEndian)) {
			for (int i=0; i < height; ++i) {
				uint *p = (uint *) qImage.scanLine(i);
				for (int x=0; x<width; ++x)
					p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
			}
		} else {
			for (int i=0; i < height; ++i) {
				uint *p = (uint *) qImage.scanLine(i);
				for (int x=0; x<width; ++x)
					p[x] = (p[x] << 8) | ((p[x] >> 24) & 0xff);
			}
		}
	}
	
	const QImage &constRef = qImage; // to avoid detach in bits()...
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, qImage.width(), qImage.height(), 0, externalFormat,
				 pixel_type, constRef.bits());
	if (genMipmap)
		glGenerateMipmap(GL_TEXTURE_2D);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadParams.wrapMode);

	// Release shared memory
	qImage = QImage();

	// Report success of texture loading
	emit(loadingProcessFinished(false));
	return true;
}
