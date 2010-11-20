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

QSet<TextureLoader*> test;

TextureLoader::TextureLoader(StelTextureSP texture, StelTextureMgr *mgr)
    : QObject(), texturePtr(texture), textureMgr(mgr), networkReply(NULL)
{
}

TextureLoader::~TextureLoader() {
    test.remove(this);
    qDebug() << "nb active loader" << test.size();
    foreach(TextureLoader* loader, test) {
        if (loader->texturePtr.isNull())
            continue;
        qDebug() << loader->texturePtr.data()->fullPath;
        if (loader->networkReply)
            qDebug() << loader->networkReply->isFinished() << loader->networkReply->isRunning();
    }
}

void TextureLoader::start(int timer) {
    QTimer::singleShot(timer, this, SLOT(start()));
}

void TextureLoader::start()
{
    // If we already have 4 file downloading, we wait one second before trying again.
    // Just for testing cause it seems like when we download to many files we get stuck.
    if (test.size() >= 4) {
        QTimer::singleShot(1000, this, SLOT(start()));
        return;
    }

    test.insert(this);
    StelTextureSP texture = texturePtr.toStrongRef();
    if (texture.isNull()) {
        // The texture has already been deleted.
        deleteLater();
        return;
    }

    connect(this, SIGNAL(dataLoaded(QImage)), texture.data(), SLOT(onDataLoaded(QImage)));
    if (texture->fullPath.startsWith("http://")) {
        QNetworkRequest req = QNetworkRequest(QUrl(texture->fullPath));
        // Define that preference should be given to cached files (no etag checks)
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        req.setRawHeader("User-Agent", StelUtils::getApplicationName().toAscii());
        networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
        connect(networkReply, SIGNAL(finished()), this, SLOT(onNetworkReply()));
        connect(networkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
    } else {
        // At next loop iteration we start to load from the file.
        QTimer::singleShot(0, this, SLOT(directLoad()));
    }
    connect(texture.data(), SIGNAL(destroyed()), this, SLOT(onTextureDestroyed()));

    // Move this object outside of the main thread.
    moveToThread(textureMgr->loaderThread);
}


void TextureLoader::onNetworkReply()
{
    qDebug() << "TEST networkReply";

    StelTextureSP texture = texturePtr.toStrongRef();
    if (!texture.isNull()) {
        qDebug() << "TEST downloaded " << texture->fullPath;
        if (networkReply->error() != QNetworkReply::NoError) {
            qDebug() << "ERROR" << networkReply->errorString();
        } else {
            QByteArray data = networkReply->readAll();
            QImage image = QImage::fromData(data);
            if (image.isNull()) {
                qDebug() << "ERROR parsing image failed " << texture->fullPath;
            } else {
                emit dataLoaded(image);
            }
        }
    }
    networkReply->deleteLater();
    networkReply = NULL;
    deleteLater();
}

void TextureLoader::onNetworkError(QNetworkReply::NetworkError code)
{
    qDebug() << "ERROR" << code << networkReply->errorString() << networkReply->url();
    deleteLater();
}

void TextureLoader::directLoad() {
    StelTextureSP texture = texturePtr.toStrongRef();
    if (!texture.isNull()) {
        QImage image = QImage(texture->fullPath);
        emit dataLoaded(image);
        qDebug() << "delete loader :" << texture->fullPath;
    }
    deleteLater();
}

void TextureLoader::onTextureDestroyed()
{
    if (networkReply != NULL) {
        qDebug() << "TEST abort download";
        networkReply->abort();
    }
}


StelTexture::StelTexture() : loader(NULL), downloaded(false), isLoadingImage(false),
				   errorOccured(false), id(0), avgLuminance(-1.f)
{
	width = -1;
	height = -1;
}

StelTexture::~StelTexture()
{
	if (id != 0)
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
	// qDebug() << "TEST bind" << fullPath;
	if (id != 0)
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

	if (!isLoadingImage && loader != NULL) {
		isLoadingImage = true;
		loader->start(20);
	}

	return false;
}

void StelTexture::onDataLoaded(QImage image)
{
	qImage = image;
	Q_ASSERT(!qImage.isNull());
	glLoad();
	isLoadingImage = false;
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
