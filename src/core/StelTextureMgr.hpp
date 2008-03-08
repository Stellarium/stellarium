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

#ifndef STELTEXTUREMGR_H_
#define STELTEXTUREMGR_H_

#include <config.h>
#include "GLee.h"
#include "fixx11h.h"

#include <QObject>
#include <QMap>

#include "STexture.hpp"

//! @class ImageLoader 
//! Abstract class for any Image loaders.
class ImageLoader
{
public:
	struct TexInfo
	{
		GLsizei width;	//! Texture image width
		GLsizei height;	//! Texture image height
		GLenum format;
		GLint internalFormat;
		GLubyte* texels;
		GLenum type;
		QString fullPath;
	};
	
	virtual ~ImageLoader() {;}
	//! Load the data from the image and store it into tex.texels
	//! The caller is responsible for freeing the memory allocated in tex.texels
	//! This method must be thread compliant
	virtual bool loadImage(const QString& filename, TexInfo& texInfo) = 0;
};

//! @class PngLoader Define a PNG loader. This implementation supports LUMINANCE, LUMINANCE+ALPHA, RGB, RGBA.
class PngLoader : public ImageLoader
{
	virtual bool loadImage(const QString& filename, TexInfo& texinfo);
};

//! @class JpgLoader Define a JPG loader. This implementation supports LUMINANCE or RGB.
class JpgLoader : public ImageLoader
{
	virtual bool loadImage(const QString& filename, TexInfo& texinfo);
};

//! Manage textures loading and manipulation.
//! The texture loader has a current state defining the way the textures will be loaded in memory,
//! that is, whether mimap should be generated, the wrap mode or mag and min filters.
//! The state should be reinitialized by calling the StelTextureMgr::setDefaultParams method before any texture loading.
//! It provides function for loading images in a separate threads.
class StelTextureMgr : QObject
{
public:
	StelTextureMgr();
	virtual ~StelTextureMgr();
	
	//! Initialize some variable from the openGL contex.
	//! Must be called after the creation of the GLContext.
	void init();
	
	//! Load an image from a file and create a new texture from it
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	STextureSP createTexture(const QString& filename);
	
	//! Load an image from a file and create a new texture from it in a new thread.
	//! @param url the texture file name or URL, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	//! @param fileExtension the file extension to assume. If not set the extension is determined from url
	STextureSP createTextureThread(const QString& url, const QString& fileExtension="", bool lazyLoading=true);
	
	//! Define if mipmaps must be created while creating textures
	void setMipmapsMode(bool b = false) {mipmapsMode = b;}
	
	//! Define the texture wrapping mode to use while creating textures
	//! @param m can be either GL_CLAMP, GL_CLAMP_TO_EDGE, or GL_REPEAT.
	//! See doc for glTexParameter for more info.
	void setWrapMode(GLint m = GL_CLAMP) {wrapMode = m;}
	
	//! Define the texture min filter to use while creating textures
	//! @param m can be either GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
	//! GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
	//! See doc for glTexParameter for more info.
	void setMinFilter(GLint m = GL_NEAREST) {minFilter = m;}
	
	//! Define the texture magnification filter to use while creating textures
	//! @param m can be either GL_NEAREST, GL_LINEAR
	//! See doc for glTexParameter for more info.
	void setMagFilter(GLint m = GL_LINEAR) {magFilter = m;}
	
	//! Set default parameters for Mipmap mode, wrap mode, min and mag filters
	void setDefaultParams();
	
	//! Define how the dynamic range of the image will be adapted to fit on 8 bits
	//! Note that using linear mode on 8 bits images does nothing
	void setDynamicRangeMode(STextureTypes::DynamicRangeMode dMode = STextureTypes::LINEAR) {dynamicRangeMode = dMode;}
	
	//! Register a new image loader for a given image file extension
	void registerImageLoader(const QString& fileExtension, ImageLoader* loader)
	{
		imageLoaders[fileExtension] = loader;
	}
	//! Get the downloader shared for downloading images
	class QHttp* getDownloader() {return downloader;}
	
private:
	friend class STexture;

	//! Internal
	STextureSP initTex();

	//! Load the image memory
	bool loadImage(STexture* tex);
	
	//! Adapt the scaling for the texture. Return true if there was no errors
	bool reScale(STexture* tex);

	//! List of image loaders providing image loading for the given files extensions
	QMap<QString, ImageLoader*> imageLoaders;
	
	bool mipmapsMode;
	GLint wrapMode;
	GLint minFilter;
	GLint magFilter;
	STextureTypes::DynamicRangeMode dynamicRangeMode;
	
	//! The maximum texture size supported by the video card
	GLint maxTextureSize;
	
	//! Whether ARB_texture_non_power_of_two is supported on this card
	bool isNoPowerOfTwoAllowed;

	//! Used to correct a bug on some nvidia cards
	bool isNoPowerOfTwoLUMINANCEAllowed;
	
	class QHttp* downloader;
};

#endif /*STELTEXTUREMGR_H_*/
