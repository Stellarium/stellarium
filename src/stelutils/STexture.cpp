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

#include "STexture.hpp"
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

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif

// Initialize statics
QSemaphore* STexture::maxLoadThreadSemaphore = new QSemaphore(1);

STexture::STexture() : downloaded(false), downloadId(0), isLoadingImage(false), imageFile(NULL), 
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

STexture::~STexture()
{
	if (texels)
		delete texels;
	texels = NULL;
	if (glIsTexture(id)==GL_FALSE)
	{
		// std::cerr << "Warning: in STexture::~STexture() tried to delete invalid texture with ID=" << id << " Current GL ERROR status is " << glGetError() << std::endl;
	}
	else
	{
		glDeleteTextures(1, &id);
		// std::cerr << "Delete texture with ID=" << id << std::endl;
	}
	id = 0;
	delete mutex;
	mutex = NULL;
}

/*************************************************************************
 This method should be called if the texture loading failed for any reasons
 *************************************************************************/
void STexture::reportError(const QString& aerrorMessage)
{
	errorOccured = true;
	errorMessage = aerrorMessage;
	// Report failure of texture loading
	emit(loadingProcessFinished(this, true));
}
	
//! Load an image and set the texture parameters in a thread
class ImageLoadThread : public QThread
{
	public:
		ImageLoadThread(STexture* tex) : QThread((QObject*)tex), texture(tex) {;}
		virtual void run();
	private:
		STexture* texture;
};

void ImageLoadThread::run()
{
	STexture::maxLoadThreadSemaphore->acquire(1);
	texture->imageLoad();
	STexture::maxLoadThreadSemaphore->release(1);
}

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/
bool STexture::bind()
{
	if (id!=0)
	{
		// The texture is already fully loaded, just bind and return true;
		glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	// The texture is not yet fully loaded
	if (downloaded==false && downloadId!=0 && fullPath.startsWith("http://"))
	{
		// We need to start download
		imageFile = new QTemporaryFile(QDir::tempPath() + "XXXXXX" + fileExtension, this);
		if (!imageFile->open(QIODevice::ReadWrite))
		{
			qWarning() << "STexture::bind(): can't create temporary file " << QDir::tempPath() + "XXXXXX" + fileExtension;
			errorOccured = true;
			return false;
		}
		QHttp* http = StelApp::getInstance().getTextureManager().getDownloader();
		connect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadFinished(int, bool)));
		downloadId = http->get(fullPath, imageFile);
		fullPath = imageFile->fileName();
		return false;
	}
	
	// From this point we assume that fullPath is valid
	// Start loading the image in a thread and return imediately
	if (!isLoadingImage)
	{
		isLoadingImage = true;
		ImageLoadThread* thread = new ImageLoadThread(this);
		connect(thread, SIGNAL(finished()), this, SLOT(fileLoadFinished()));
		thread->start();
	}
	return false;
}


/*************************************************************************
 Called when the download for the texture file terminated
*************************************************************************/
void STexture::downloadFinished(int did, bool error)
{
	if (did!=downloadId)
		return;
	imageFile->close();
	downloaded=true;
	downloadId=0;
	if (error)
	{
		qWarning() << "Texture download failed for " + imageFile->fileName()+ ": " + StelApp::getInstance().getTextureManager().getDownloader()->errorString();
		errorOccured = true;
	}
	QHttp* http = StelApp::getInstance().getTextureManager().getDownloader();
	disconnect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadFinished(int, bool)));
}

/*************************************************************************
 Called when the file loading thread has terminated
*************************************************************************/
void STexture::fileLoadFinished()
{
	glLoad();
}

/*************************************************************************
 Actually load the texture already in the RAM to openGL memory
*************************************************************************/
bool STexture::glLoad()
{
	if (!texels)
	{
		errorOccured = true;
		return false;
	}
	
	// generate texture
	glGenTextures (1, &id);
	glBindTexture (GL_TEXTURE_2D, id);

	// setup some parameters for texture filters and mipmapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

	glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, texels);
	if (mipmapsMode==true)
		gluBuild2DMipmaps (GL_TEXTURE_2D, internalFormat, width, height, format, type, texels);
	
	// OpenGL has its own copy of texture data
	free (texels);
	texels = NULL;
	
	// Report success of texture loading
	emit(loadingProcessFinished(this, false));
	
	return true;
}

/*************************************************************************
 Return the average texture luminance, 0 is black, 1 is white
 *************************************************************************/
bool STexture::getAverageLuminance(float& lum)
{
	if (id==0)
		return false;
	
	QMutexLocker lock(mutex);
	if (avgLuminance<0)
	{
		int size = width*height;
		glBindTexture(GL_TEXTURE_2D, id);
		GLfloat* p = (GLfloat*)calloc(size, sizeof(GLfloat));
		assert(p);

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
bool STexture::getDimensions(int &awidth, int &aheight)
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
bool STexture::imageLoad()
{
	QMutexLocker lock(mutex);
	return StelApp::getInstance().getTextureManager().loadImage(this);
}

