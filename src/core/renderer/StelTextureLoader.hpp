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

#ifndef _STELTEXTURELOADER_HPP_
#define _STELTEXTURELOADER_HPP_

#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "StelApp.hpp"
#include "StelUtils.hpp"


//! Texture Loader interface - used internally by texture implementations to load image data.
//!
//! Loads image data in a separate thread.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelTextureLoader : public QObject
{
	Q_OBJECT

public:
	//! Abort texture loading (for example when destroying a texture that's still loading).
	//!
	//! Can only be called from the main thread.
	virtual void abort(){};

signals:
	//! Emitted when an error occurs during image loading, specifying error message.
	void error(const QString& errorMsg);

protected:
	//! Construct a StelTextureLoader loading in specified thread.
	StelTextureLoader(QThread* loaderThread)
		: QObject()
		, loaderThread(loaderThread)
	{
	}

	//! Move the loader to its thread (after setting up image loading).
	void moveToLoaderThread()
	{
		moveToThread(loaderThread);
	}

private:
	//! Thread in which image loading code runs.
	QThread* loaderThread;
};


//! Texture loader that loads an image from the web.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelHTTPTextureLoader : public StelTextureLoader
{
	Q_OBJECT

public:
	//! Construct a StelHTTPTextureLoader.
	//!
	//! @param url URL of the image to load the texture from.
	//! @param delay Delay when to start loading in milliseconds.
	//! @param loaderThread Thread in which the loader will run.
	StelHTTPTextureLoader(const QString& url, int delay, QThread* loaderThread)
		: StelTextureLoader(loaderThread)
		, url(url)
		, networkReply(NULL)
	{
		QTimer::singleShot(delay, this, SLOT(start()));
	}

	virtual void abort()
	{
		Q_ASSERT_X(QThread::currentThread() == QApplication::instance()->thread(),
		           Q_FUNC_INFO,
		           "StelTextureLoader::abort must be called from the main thread");
		if (networkReply != NULL) {networkReply->abort();}
	}

signals:
	//! Emitted when image loading is finished, sending loaded image.
	void finished(QImage);

private slots:
	//! Start loading the image.
	void start()
	{
		QNetworkRequest request = QNetworkRequest(QUrl(url));
		// Define that preference should be given to cached files (no etag checks)
		request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, 
		                     QNetworkRequest::PreferCache);
		request.setRawHeader("User-Agent", StelUtils::getApplicationName().toAscii());
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(request);
		connect(networkReply, SIGNAL(finished()), this, SLOT(onNetworkReply()));

		// Move this object outside of the main thread.
		moveToLoaderThread();
	}
	
	//! Called once the image data is received from network.
	void onNetworkReply()
	{
		if (networkReply->error() != QNetworkReply::NoError) 
		{
			emit error(networkReply->errorString());
		} 
		else 
		{
			QByteArray data = networkReply->readAll();
			QImage image = QImage::fromData(data);
			if (image.isNull()) 
			{
				emit error("Unable to parse image data");
			} else 
			{
				emit finished(image);
			}
		}
		networkReply->deleteLater();
		networkReply = NULL; 
	}

private:
	//! URL of the image.
	const QString url;
	//! Provides access to data received from the network.
	QNetworkReply* networkReply;
};

//! Texture loader that loads an image from the file system.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelFileTextureLoader : public StelTextureLoader
{
	Q_OBJECT

public:
	//! Construct a StelFileTextureLoader
	//!
	//! @param path Absoulte path of the image file to load from.
	//! @param delay Delay when to start loading in milliseconds.
	//! @param loaderThread Thread in which the loader will run.
	StelFileTextureLoader(const QString& path, int delay, QThread* loaderThread)
		: StelTextureLoader(loaderThread)
		, path(path)
	{
		QTimer::singleShot(delay, this, SLOT(start()));
	}

signals:
	//! Emitted when image loading is finished, sending loaded image.
	void finished(QImage);

private slots:
	//! Start loading the image.
	void start()
	{
		// At next loop iteration we start to load from the file.
		QTimer::singleShot(0, this, SLOT(load()));
		// Move this object outside of the main thread.
		moveToLoaderThread();
	}

	//! Load an image from file.
	void load()
	{
		QImage image = QImage(path);
		if(image.isNull())
		{
			emit error("Image " + path + " failed to load");
			return;
		}
		emit finished(image);
	}

private:
	//! Absolute path of the image.
	const QString path;
};
#endif // _STELTEXTURELOADER_HPP_
