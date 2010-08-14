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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include <QHttp>
#include <QDebug>
#include <QUrl>
#include <QImage>
#include <QNetworkReply>
#include <QGLWidget>

// Initialize statics
QSemaphore* StelTexture::maxLoadThreadSemaphore = new QSemaphore(5);


//  Class used to load an image and set the texture parameters in a thread
class ImageLoadThread : public QThread
{
	public:
		ImageLoadThread(StelTexture* tex) : QThread((QObject*)tex), texture(tex) {;}
		virtual void run();
	private:
		StelTexture* texture;
};

void ImageLoadThread::run()
{
	StelTexture::maxLoadThreadSemaphore->acquire(1);
	texture->imageLoad();
	StelTexture::maxLoadThreadSemaphore->release(1);
}

StelTexture::StelTexture() : httpReply(NULL), loadThread(NULL), downloaded(false), isLoadingImage(false),
				   errorOccured(false), id(0), avgLuminance(-1.f)
{
	mutex = new QMutex();
	width = -1;
	height = -1;
}

StelTexture::~StelTexture()
{
	if (httpReply || (loadThread && loadThread->isRunning()))
	{
		reportError("Aborted (texture deleted)");
	}

	if (httpReply)
	{
		// HTTP is still doing something for this texture. We abort it.
		httpReply->abort();
		delete httpReply;
		httpReply = NULL;
	}

	if (loadThread && loadThread->isRunning())
	{
		// The thread is currently running, it needs to be properly stopped
		loadThread->terminate();
		loadThread->wait(500);
	}

	if (id!=0)
	{
		StelPainter::makeMainGLContextCurrent();
		if (glIsTexture(id)==GL_FALSE)
		{
			qDebug() << "WARNING: in StelTexture::~StelTexture() tried to delete invalid texture with ID=" << id << " Current GL ERROR status is " << glGetError();
		}
		else
		{
			StelPainter::glContext->deleteTexture(id);
		}
		id = 0;
	}
	delete mutex;
	mutex = NULL;
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
	if (id!=0)
	{
		// The texture is already fully loaded, just bind and return true;
#ifdef USE_OPENGL_ES2
		glActiveTexture(GL_TEXTURE0);
#endif
		glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	// The texture is not yet fully loaded
	if (downloaded==false && httpReply==NULL && fullPath.startsWith("http://"))
	{
		// We need to start download
		QNetworkRequest req = QNetworkRequest(QUrl(fullPath));
		// Define that preference should be given to cached files (no etag checks)
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		req.setRawHeader("User-Agent", StelUtils::getApplicationName().toAscii());
		httpReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(httpReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		return false;
	}

	// From this point we assume that fullPath is valid
	// Start loading the image in a thread and return imediately
	if (!isLoadingImage && downloaded==true)
	{
		isLoadingImage = true;
		loadThread = new ImageLoadThread(this);
		connect(loadThread, SIGNAL(finished()), this, SLOT(fileLoadFinished()));
		loadThread->start(QThread::LowestPriority);
	}
	return false;
}


/*************************************************************************
 Called when the download for the texture file terminated
*************************************************************************/
void StelTexture::downloadFinished()
{
	downloadedData = httpReply->readAll();
	downloaded=true;
	if (httpReply->error()!=QNetworkReply::NoError || errorOccured)
	{
		if (httpReply->error()!=QNetworkReply::OperationCanceledError)
			qWarning() << "Texture download failed for " + fullPath+ ": " + httpReply->errorString();
		errorOccured = true;
	}
	httpReply->deleteLater();
	httpReply=NULL;
}

/*************************************************************************
 Called when the file loading thread has terminated
*************************************************************************/
void StelTexture::fileLoadFinished()
{
	glLoad();
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

// Load the image data
bool StelTexture::imageLoad()
{
	if (downloadedData.isEmpty())
	{
		// Load the data from the file
		QMutexLocker lock(mutex);
		qImage = QImage(fullPath);
	}
	else
	{
		qImage = QImage::fromData(downloadedData);
		// Release the memory
		downloadedData = QByteArray();
	}
	return !qImage.isNull();
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

	QGLContext::BindOptions opt = QGLContext::InvertedYBindOption;
	if (loadParams.filtering==GL_LINEAR)
		opt |= QGLContext::LinearFilteringBindOption;

	// Mipmap seems to be pretty buggy on windows..
#ifndef Q_OS_WIN
	if (loadParams.generateMipmaps==true)
		opt |= QGLContext::MipmapBindOption;
#endif

	GLint glformat;
	if (qImage.isGrayscale())
	{
		glformat = qImage.hasAlphaChannel() ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
	}
	else if (qImage.hasAlphaChannel())
	{
		glformat = GL_RGBA;
	}
	else
		glformat = GL_RGB;

	Q_ASSERT(StelPainter::glContext==QGLContext::currentContext());
#ifdef USE_OPENGL_ES2
	glActiveTexture(GL_TEXTURE0);
#endif
	id = StelPainter::glContext->bindTexture(qImage, GL_TEXTURE_2D, glformat, opt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadParams.wrapMode);

	// Release shared memory
	qImage = QImage();

	// Report success of texture loading
	emit(loadingProcessFinished(false));
	return true;
}
