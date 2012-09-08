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

#ifndef _STELTEXTURECACHE_HPP_
#define _STELTEXTURECACHE_HPP_

#include <QMap>
#include <QString>

//! Caches textures, ensuring no texture is loaded twice.
//! 
//! StelTextureCache is templated by the textured backend used.
//! This way e.g. StelQGLRenderer can have a texture cache storing
//! StelQGLTextureBackend. The
//! template argument class must have a member function called 
//! <em>startAsynchronousLoading()</em> which is called to start loading when 
//! a texture created with lazy loading mode is requested again with 
//! asynchronous or normal loading mode.
//!
//! Textures are identified by their <em>name</em>, that is full filesystem path or URL.
//! Generated textures without a name can't be cached.
//! The cache keeps pointers to all textures with reference counts; 
//! when a texture with the same name is requested more than once,
//! pointer to the previously created texture is returned and the reference
//! count is incremented.
//!
//! If a texture was created already with asynchronous/lazy loading mode
//! is requested with normal loading mode, and isn't fully loaded yet,
//! the cache waits until the texture loads and then returns it.
//!
//! Similarly, if the texture was created with lazy loading mode, is 
//! requested with asynchronous mode and didn't start loading yet,
//! loading is started and then the texture is returned.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
template<class TextureBackend>
class StelTextureCache
{
private:
	//! Pointer to a texture backend with a reference count.
	struct TextureBackendRefCounted
	{
		//! Reference count.
		int refCount;
		//! Texture backend pointer.
		TextureBackend* backend;

		//! Default constructor for QMap.
		TextureBackendRefCounted(){}
		//! Construct with reference count of 1 pointing to specified texture backend.
		TextureBackendRefCounted(TextureBackend* backend):refCount(1),backend(backend){}
	};

public:
	//! Get a pointer to a cached texture or NULL if not yet in cache.
	//!
	//! If the texture is already in cache, its reference count is incremented
	//! when returing the pointer.
	//!
	//! @param name        Name of the texture to get.
	//! @param loadingMode Loading mode used when creating the texture.
	//!                    If the texture is cached but loaded asynchronously,
	//!                    it might not be fully loaded - so if loadingMode is Normal,
	//!                    we wait until it finishes loading.
	//!                    Similarly with lazy/normal or lazy/asynchronous.
	//! @return Pointer to the texture if cached, NULL otherwise.
	virtual TextureBackend* get(const QString& name, const TextureLoadingMode loadingMode)
	{
		Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Can't cache textures without a name");

		if(!cache.contains(name)) {return NULL;}

		TextureBackend* result = cache[name].backend;

		switch(loadingMode)
		{
			case TextureLoadingMode_Normal:
				//If cached texture is loading (asynchronous), wait,
				//if uninitialized(lazy), start loading and wait
				if(result->getStatus() == TextureStatus_Uninitialized)
				{
					result->startAsynchronousLoading();
				}
				//Waiting in a hot loop, this could be done better
				while(result->getStatus() == TextureStatus_Loading)
				{
					continue;
				}
			case TextureLoadingMode_Asynchronous:
				//If cached texture is uninitialized (lazy), start loading and wait.
				if(result->getStatus() == TextureStatus_Uninitialized)
				{
					result->startAsynchronousLoading();
				}
				break;
			case TextureLoadingMode_LazyAsynchronous: 
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture loading mode");
		}

		(cache[name].refCount)++;

		return result;
	}

	//! Add a texture to the cache (setting its reference count to 1).
	//!
	//! Only a texture that is not yet in cache can be added,
	//! use get() to get a texture and increase its reference count.
	//!
	//! @param backend Texture backend to add.
	virtual void add(TextureBackend* backend)
	{
		const QString& name = backend->getName();
		Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Can't cache textures without a name");
		Q_ASSERT_X(!cache.contains(name), Q_FUNC_INFO, 
		           "Adding the same texture to the cache more than once");

		cache.insert(name, TextureBackendRefCounted(backend));
	}

	//! Remove a texture from the cache.
	//!
	//! Decrements the reference count. The texture is only actually removed (destroyed)
	//! when its reference count hits zero.
	//!
	//! @param backend Texture backend to remove.
	virtual void remove(TextureBackend* backend)
	{
		const QString& name = backend->getName();
		Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Can't cache textures without a name");
		Q_ASSERT_X(cache.contains(name), Q_FUNC_INFO,
		           "Trying to remove unknown texture from cache");

		TextureBackendRefCounted& cached = cache[name];
		--(cached.refCount);
		Q_ASSERT_X(cached.refCount >= 0, Q_FUNC_INFO,
		           "Negative reference count of a texture in texture cache");

		// If reference count hits 0, delete the texture.
		if(cached.refCount == 0)
		{
			// Assert that we're really deleting what we thing we're deleting
			// instead of having two textures with the same name.
			Q_ASSERT_X(backend == cached.backend, Q_FUNC_INFO,
			           "Texture in cache has the same name as but is not specified texture");
			cache.remove(name);
			delete backend;
		}
	}

private:
	//! Texture cache data storage - texture-refcount pairs indexed by strings.
	QMap<QString, TextureBackendRefCounted> cache;
};

#endif // _STELTEXTURECACHE_HPP_
