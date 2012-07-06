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
#include "renderer/StelTextureMgr.hpp"
#include "StelUtils.hpp"

#include <QHttp>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QNetworkRequest>
#include <QThread>
#include <QSettings>
#include <QGLFormat>
#include <cstdlib>

#include "StelRenderer.hpp"
#include "StelTextureBackend.hpp"
#include "StelGLUtilityFunctions.hpp"

StelTextureMgr::StelTextureMgr(StelRenderer* renderer)
	: renderer(renderer)
{
	// This thread is doing nothing but will contains all the loader objects.
	loaderThread = new QThread(this);
	loaderThread->start(QThread::LowestPriority);
}

StelTextureMgr::~StelTextureMgr()
{
	// Hopefully this doesn't take much time.
	loaderThread->quit();
	loaderThread->wait();
}

void StelTextureMgr::init()
{
};

#include "renderer/StelQGLRenderer.hpp"

StelTextureSP StelTextureMgr::createTexture(const QString& afilename, const StelTextureParams& params)
{
	if (afilename.isEmpty()) {return StelTextureSP();}
	
	// StelTexture is just a thin wrapper around StelTextureBackend now.
	StelTextureBackend* backend = 
		renderer->createTextureBackend(afilename, params, TextureLoadingMode_Normal);
	if(NULL == backend) {return StelTextureSP();}

	return StelTextureSP(new StelTexture(backend, renderer));
}

StelTextureSP StelTextureMgr::createTextureThread(const QString& url, const StelTextureParams& params, bool lazyLoading)
{
	if (url.isEmpty()) {return StelTextureSP();}

	const TextureLoadingMode mode = lazyLoading ? TextureLoadingMode_Asynchronous 
		                                         : TextureLoadingMode_LazyAsynchronous;
	// StelTexture is just a thin wrapper around StelTextureBackend now.
	StelTextureBackend* backend = renderer->createTextureBackend(url, params, mode);
	if(NULL == backend) {return StelTextureSP();}

	return StelTextureSP(new StelTexture(backend, renderer));
}
