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

#ifndef STELTEXTUREMGR_HPP
#define STELTEXTUREMGR_HPP

#include "StelTexture.hpp"
#include <QObject>
#include <QMap>
#include <QWeakPointer>
#include <QMutex>

class QNetworkReply;
class QThread;
class QThreadPool;

//! @class StelTextureMgr
//! Manage textures loading.
//! It provides method for loading images in a separate thread.
class StelTextureMgr : QObject
{
	Q_OBJECT
public:
	//! Load an image from a file and create a new texture from it
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked for in Stellarium's standard textures directories.
	//! @param params the texture creation parameters.
	//! @note If filename is invalid, this creates a fuchsia-colored (magenta) replacement texture to signify "great error".
	//!       Create an empty StelTextureSP yourself if you have no valid filename.
	//! @note If image is larger than maximum allowed texture size, it will be automatically rescaled to fit and a warning printed to the logfile.
	StelTextureSP createTexture(const QString& filename, const StelTexture::StelTextureParams& params=StelTexture::StelTextureParams());

	//! Create a texture from a QImage.
	StelTextureSP createTexture(const QImage &image, const StelTexture::StelTextureParams& params=StelTexture::StelTextureParams());

	//! Load an image from a file and create a new texture from it in a new thread.
	//! @note This method is safe to be called from threads other than the main thread.
	//! @param url the texture file name or URL, can be absolute path if starts with '/' otherwise
	//!    the file will be looked for in Stellarium's standard textures directories.
	//! @param params the texture creation parameters.
	//! @param lazyLoading define whether the texture should be actually loaded only when needed, i.e. when bind() is called the first time.
	StelTextureSP createTextureThread(const QString& url, const StelTexture::StelTextureParams& params=StelTexture::StelTextureParams(), bool lazyLoading=true);

	//! Creates or finds a StelTexture wrapper for the specified OpenGL texture object.
	//! The wrapper takes ownership of the texture and will delete it if it is destroyed.
	//! @param texID The OpenGL texture ID which should be wrapped. If this is already a StelTexture, the existing wrapper will be returned.
	//! @returns the existing or new wrapper for the texture with the given GL name. Returns a null pointer if the texture name is invalid.
	StelTextureSP wrapperForGLTexture(GLuint texId);

//	//! Returns the estimated memory usage of all textures currently loaded through StelTexture
//	int getGLMemoryUsage();

private:
	friend class StelTexture;
	friend class ImageLoader;
	friend class StelApp;

	//! Private constructor, use StelApp::getTextureManager for the correct instance
	StelTextureMgr(QObject* parent = Q_NULLPTR);

	unsigned int glMemoryUsage;

	//! We use our own thread pool to ensure only 1 texture is being loaded at a time
	QThreadPool* loaderThreadPool;

	StelTextureSP lookupCache(const QString& file);
	typedef QMap<QString,QWeakPointer<StelTexture> > TexCache;
	typedef QMap<GLuint,QWeakPointer<StelTexture> > IdMap;
	QMutex mutex;
	TexCache textureCache;
	IdMap idMap;
	GLint maxTexSize; // Useful to avoid loading too large textures
};


#endif // STELTEXTUREMGR_HPP
