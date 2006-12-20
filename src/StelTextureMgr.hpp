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

#include <string>
#include <map>
#include <vector>
#include "SDL_opengl.h"
#include "STexture.h"

class ManagedSTexture : public STexture
{
	friend class StelTextureMgr;
	friend class ImageLoader;
public:
	//! Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
	//! If the texture was lazy loaded, load it now.
	virtual void lazyBind();
	//! Return the average texture luminance.
	//! @return 0 is black, 1 is white
    virtual float getAverageLuminance(void);
    
    virtual ~ManagedSTexture()
    {
    	if (threadedLoading && loadState==ManagedSTexture::LOADING_IMAGE)
    	{
    		// TODO should search in the loading queue the matching thread and kill it 
    		assert(0);
    	}
    }
private:
	friend int loadTextureThread(void* tparam);
	
	enum LoadState
	{
		UNLOADED=0,
		LOADED=1,
		ERROR,
		LOADING_IMAGE
	};

	ManagedSTexture() : loadState(UNLOADED), avgLuminance(-1.f), threadedLoading(false) {;}
	LoadState loadState;
	void load(void);
	
	// Cached average luminance
	float avgLuminance;
	
	// Define whether the loading must be done in a thread
	bool threadedLoading;
};

class ImageLoader
{
public:
	virtual ~ImageLoader() {;}
	virtual bool loadImage(const std::string& filename, ManagedSTexture& texinfo) = 0;
private:
	void setWH(ManagedSTexture& texinfo, GLsizei w, GLsizei h) {texinfo.width = w; texinfo.height = h;}
};

/**
 * Class used to manage textures loading and manipulation.
 * @author Fabien Chereau <stellarium@free.fr>
 */
class StelTextureMgr
{
public:
	StelTextureMgr(const std::string& textureDir);
	virtual ~StelTextureMgr();
	
	//! Initialize some variable from the openGL contex.
	//! Must be called after the creation of the GLContext.
	void init();
	
	//! Update loading of textures in threads
	void update();
	
	//! Load an image from a file and create a new texture from it
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	ManagedSTexture& createTexture(const std::string& afilename, bool lazyLoading=false);
	
	//! Load an image from a file and create a new texture from it in a new thread, the value of the return pointer
	//!    and of the status boolean are only set in the update() method and therefore don't need to be protected
	//!    by a Mutex.
	//! @param tex the pointer where the loaded texture will be returned. In case of threaded loading, this pointer will
	//!    remain to NULL until the texture is actually loaded.
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	//! @param status is set to false if the creation of the texture failed
	bool createTextureThread(STexture** tex, const std::string& filename, bool* status);
	
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
	
	//! Register a new image loader for a given image file extension
	void registerImageLoader(const std::string& fileExtension, ImageLoader* loader)
	{
		imageLoaders[fileExtension] = loader;
	}
private:
	friend class ManagedSTexture;

	//! Internal
	ManagedSTexture* initTex(const string& afilename);

	//! Load the image memory. If we use threaded loading, the texture will
	//! be uploaded to openGL memory at the next update() call.
	bool loadImage(ManagedSTexture* tex);
	
	//! Load the texture already in the RAM to openGL memory
	bool glLoadTexture(ManagedSTexture* tex);
	
	// List of image loaders providing image loading for the given files extensions
	std::map<std::string, ImageLoader*> imageLoaders;
	
	std::string textureDir;
	bool mipmapsMode;
	GLint wrapMode;
	GLint minFilter;
	GLint magFilter;
	
	// The maximum texture size supported by the video card
	GLint maxTextureSize;
	
	// Whether GL_ARB_texture_float is supported on this card
	bool isFloatingPointTexAllowed;
	
	// Whether ARB_texture_non_power_of_two is supported on this card
	bool isNoPowerOfTwoAllowed;
	bool isNoPowerOfTwoLUMINANCEAllowed;
	
	// The null texture to return in case of problems
	static ManagedSTexture NULL_STEXTURE;
	
	// Everything used for the threaded loading
	friend struct LoadQueueParam;
	friend int loadTextureThread(void* tparam);
	struct SDL_mutex* loadQueueMutex;
	std::vector<class LoadQueueParam*> loadQueue;
	
	// Define a PNG loader. This implementation supports LUMINANCE, LUMINANCE+ALPHA, RGB, RGBA. 
	class PngLoader : public ImageLoader
	{
		virtual bool loadImage(const std::string& filename, ManagedSTexture& texinfo);
	};
	static PngLoader pngLoader;
	
	// Define a JPG loader. This implementation supports LUMINANCE or RGB.
	class JpgLoader : public ImageLoader
	{
		virtual bool loadImage(const std::string& filename, ManagedSTexture& texinfo);
	};
	static JpgLoader jpgLoader;
};

#endif /*STELTEXTUREMGR_H_*/
