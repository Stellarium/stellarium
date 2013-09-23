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


StelTextureMgr::StelTextureMgr()
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
}


StelTextureSP StelTextureMgr::createTexture(const QString& afilename, const StelTexture::StelTextureParams& params)
{
	if (afilename.isEmpty())
		return StelTextureSP();

	StelTextureSP tex = StelTextureSP(new StelTexture());
	tex->fullPath = afilename;

	tex->qImage = QImage(tex->fullPath);
	if (tex->qImage.isNull())
		return StelTextureSP();

	tex->loadParams = params;
	tex->downloaded = true;

	if (tex->glLoad())
		return tex;
	else
		return StelTextureSP();
}


StelTextureSP StelTextureMgr::createTextureThread(const QString& url, const StelTexture::StelTextureParams& params, const QString& fileExtension, bool lazyLoading)
{
	if (url.isEmpty())
		return StelTextureSP();

	StelTextureSP tex = StelTextureSP(new StelTexture());
	tex->loadParams = params;
	if (!url.startsWith("http://"))
	{
		// Assume a local file
		tex->fullPath = url;
		tex->downloaded = true;
	}
	else
	{
		tex->fullPath = url;
		if (fileExtension.isEmpty())
		{
			const int idx = url.lastIndexOf('.');
			if (idx!=-1)
				tex->fileExtension = url.right(url.size()-idx-1);
		}
	}
	if (!fileExtension.isEmpty())
		tex->fileExtension = fileExtension;

	if (!lazyLoading)
	{
		tex->bind();
	}
	return tex;
}
