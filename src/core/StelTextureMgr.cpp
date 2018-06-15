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

#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"

#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QNetworkRequest>
#include <QThread>
#include <QSettings>
#include <cstdlib>
#include <QOpenGLContext>
#include <QThreadPool>

StelTextureMgr::StelTextureMgr(QObject *parent)
	: QObject(parent), glMemoryUsage(0), loaderThreadPool(new QThreadPool(this))
{
#ifdef Q_PROCESSOR_X86_64
	//allow up to 4 textures to be loaded in parallel on 64 bit
	loaderThreadPool->setMaxThreadCount(std::min(4,QThread::idealThreadCount()));
#else
	//on other archs, for now ensure that just 1 texture is at once in background
	//otherwise, for large textures loaded in parallel (some scenery3d scenes), the risk of an out-of-memory error is greater on 32bit systems
	loaderThreadPool->setMaxThreadCount(1);
#endif
}

StelTextureSP StelTextureMgr::createTexture(const QString& afilename, const StelTexture::StelTextureParams& params)
{
	if (afilename.isEmpty())
		return StelTextureSP();

	QFileInfo info(afilename);
	QString canPath = info.canonicalFilePath();

	if(canPath.isEmpty()) //file does not exist
	{
		qWarning()<<"Texture"<<afilename<<"does not exist";
		return StelTextureSP();
	}

	//lock it for thread safety
	QMutexLocker locker(&mutex);

	//try to find out if the tex is already loaded
	StelTextureSP cache = lookupCache(canPath);
	if(!cache.isNull()) return cache;

	StelTextureSP tex = StelTextureSP(new StelTexture(this));
	tex->fullPath = canPath;

	QImage image(tex->fullPath);
	if (image.isNull())
		return StelTextureSP();

	tex->loadParams = params;
	if (tex->glLoad(image))
	{
		textureCache.insert(canPath,tex);
		return tex;
	}
	else
	{
		qWarning()<<tex->getErrorMessage();
		return StelTextureSP();
	}
}


StelTextureSP StelTextureMgr::createTextureThread(const QString& url, const StelTexture::StelTextureParams& params, bool lazyLoading)
{
	if (url.isEmpty())
		return StelTextureSP();

	QString canPath = url;
	if(!url.startsWith("http") && !url.startsWith("file://"))
	{
		QFileInfo info(url);
		canPath = info.canonicalFilePath();
	}

	if(canPath.isEmpty()) //file does not exist
	{
		qWarning()<<"Texture"<<url<<"does not exist";
		return StelTextureSP();
	}

	//lock it for thread safety
	QMutexLocker locker(&mutex);

	//try to find out if the tex is already loaded
	StelTextureSP cache = lookupCache(canPath);
	if(!cache.isNull()) return cache;

	StelTextureSP tex = StelTextureSP(new StelTexture(this));
	tex->loadParams = params;
	tex->fullPath = canPath;
	if (!lazyLoading)
	{
		//use load() instead of bind() to prevent potential - if very unlikey - OpenGL errors
		//because GL must be called in the main thread
		tex->load();
	}
	textureCache.insert(canPath,tex);
	return tex;
}

//! Create a texture from a QImage.
StelTextureSP StelTextureMgr::createTexture(const QImage &image, const StelTexture::StelTextureParams& params)
{
	bool r;
	StelTextureSP tex = StelTextureSP(new StelTexture(this));
	tex->loadParams = params;
	r = tex->glLoad(image);
	Q_ASSERT(r);
	return tex;
}

StelTextureSP StelTextureMgr::wrapperForGLTexture(GLuint texId)
{
	auto it = idMap.find(texId);
	if(it!=idMap.end())
	{
		//find out if it is valid
		StelTextureSP ref = it->toStrongRef();
		if(ref)
		{
			return ref; //valid texture!
		}
		else
		{
			//remove the cache entry
			it=idMap.erase(it);
		}
	}


	//no existing tex with this ID found, create a new wrapper
	StelTextureSP newTex(new StelTexture(this));
	newTex->wrapGLTexture(texId);
	if(!newTex->errorOccured)
	{
		idMap.insert(texId, newTex);
		return newTex;
	}
	else
	{
		//error while wrapping
		qWarning()<<newTex->getErrorMessage();
		return StelTextureSP();
	}
}

StelTextureSP StelTextureMgr::lookupCache(const QString &file)
{
	auto it = textureCache.find(file);
	if(it!=textureCache.end())
	{
		//find out if it is valid
		StelTextureSP ref = it->toStrongRef();
		if(ref)
		{
			return ref; //valid texture!
		}
		else
		{
			//remove the cache entry
			it=textureCache.erase(it);
		}
	}
	return StelTextureSP();
}
