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
#include "glues.h"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"

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
#include <QGLWidget>
#include <QNetworkReply>

// Initialize statics
QSemaphore* StelTexture::maxLoadThreadSemaphore = new QSemaphore(5);

/*************************************************************************
  Class used to load an image and set the texture parameters in a thread
 *************************************************************************/
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

/*************************************************************************
  Constructor
 *************************************************************************/
StelTexture::StelTexture() : httpReply(NULL), loadThread(NULL), downloaded(false), isLoadingImage(false),
				   errorOccured(false), id(0), avgLuminance(-1.f), texels(NULL), type(GL_UNSIGNED_BYTE)
{
	mutex = new QMutex();
	
	texCoordinates[0].set(1., 0.);
	texCoordinates[1].set(0., 0.);
	texCoordinates[2].set(1., 1.);
	texCoordinates[3].set(0., 1.);
	
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
	
	if (texels)
		TexMalloc::free(texels);
	texels = NULL;
	if (id!=0)
	{
		if (glIsTexture(id)==GL_FALSE)
		{
			qDebug() << "WARNING: in StelTexture::~StelTexture() tried to delete invalid texture with ID=" << id << " Current GL ERROR status is " << glGetError();
		}
		else
		{
			StelApp::makeMainGLContextCurrent();
			glDeleteTextures(1, &id);
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
		StelApp::makeMainGLContextCurrent();
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
		req.setRawHeader("User-Agent", StelApp::getApplicationName().toAscii());
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
	// Call bind to activate data loading
	//bind();
}

/*************************************************************************
 Called when the file loading thread has terminated
*************************************************************************/
void StelTexture::fileLoadFinished()
{
	glLoad();
}

/*************************************************************************
 Return the average texture luminance, 0 is black, 1 is white
 *************************************************************************/
bool StelTexture::getAverageLuminance(float& lum)
{
	if (id==0)
		return false;
	
	QMutexLocker lock(mutex);
	StelApp::makeMainGLContextCurrent();
	if (avgLuminance<0)
	{
		int size = width*height;
		glBindTexture(GL_TEXTURE_2D, id);
		GLfloat* p = (GLfloat*)calloc(size, sizeof(GLfloat));
		Q_ASSERT(p);

		glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_FLOAT, p);
		float sum = 0.f;
		for (int i=0;i<size;++i)
		{
			sum += p[i];
		}
		free(p);

		avgLuminance = sum/size;
	}
	lum = avgLuminance;
	return true;
}


/*************************************************************************
 Return the width and heigth of the texture in pixels
*************************************************************************/
bool StelTexture::getDimensions(int &awidth, int &aheight)
{
	QMutexLocker lock(mutex);
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

/*************************************************************************
 Load the image data
 *************************************************************************/
bool StelTexture::imageLoad()
{
	bool res=true;
	if (downloadedData.isEmpty())
	{
		// Load the data from the file
		QMutexLocker lock(mutex);
		res = StelApp::getInstance().getTextureManager().loadImage(this);
	}
	else
	{
		// Load the image from the buffer, not from a file
		if (fullPath.endsWith(".jpg", Qt::CaseInsensitive) || fullPath.endsWith(".jpeg", Qt::CaseInsensitive))
		{
			// Special case optimized for loading jpeg
			// Could be even more optimized by re-using the texels buffers instead of allocating one for each textures
			ImageLoader::TexInfo texInfo;
			res = JpgLoader::loadFromMemory(downloadedData, texInfo);
			if (!res)
				return false;

			{
				QMutexLocker lock(mutex);
				format = texInfo.format;
				width = texInfo.width;
				height = texInfo.height;
				type = GL_UNSIGNED_BYTE;
				internalFormat = texInfo.internalFormat;
				texels = texInfo.texels;
			}
		}
		else
		{
			// Use Qt QImage which is slower but works for many formats
			// This is quite slow because Qt allocates twice the memory and needs to swap from ARGB to RGBA
			qImage = QGLWidget::convertToGLFormat(QImage::fromData(downloadedData));
			
			// Update texture parameters from loaded image
			{
				QMutexLocker lock(mutex);
				format = GL_RGBA;
				width = qImage.width();
				height = qImage.height();
				type = GL_UNSIGNED_BYTE;
				internalFormat = 4;
			}
		}
		// Release the memory
		downloadedData = QByteArray();
	}
	return res;
}

/*************************************************************************
 Actually load the texture already in the RAM to openGL memory
*************************************************************************/
bool StelTexture::glLoad()
{
	if (qImage.isNull() && !texels)
	{
		errorOccured = true;
		reportError("Unknown error");
		return false;
	}

	StelApp::makeMainGLContextCurrent();
	
	// generate texture
	glGenTextures (1, &id);
	glBindTexture (GL_TEXTURE_2D, id);

	// setup some parameters for texture filters and mipmapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
	
	if (!qImage.isNull())
	{
		// Load from qImage
		if (mipmapsMode==true)
			gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat, width, height, format, type, qImage.bits());
		else
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, qImage.bits());
		
		// Release shared memory
		qImage = QImage();
	}
	else
	{
		// Fixed the bug which caused shifted loading for LUMINANCE images with non multiple of 4 widths!
		glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		// Load from texels buffer
		if (mipmapsMode==true)
			gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat, width, height, format, type, texels);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, texels);
		
		glPopClientAttrib();
				
		// OpenGL has its own copy of texture data
		TexMalloc::free (texels);
		texels = NULL;
	}
	
	// Report success of texture loading
	emit(loadingProcessFinished(false));
	
	return true;
}
