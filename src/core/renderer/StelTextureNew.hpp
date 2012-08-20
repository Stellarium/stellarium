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

#ifndef _STELTEXTURENEW_HPP_
#define _STELTEXTURENEW_HPP_

#include "StelTextureBackend.hpp"

//! Texture interface.
//!
//! StelTextureNew replaces StelTexture - it is named differently to avoid silently breaking code.
//!
//! Constructed by StelRenderer::createTexture().
//! To use the texture, it can be bound by the bind() member function.
//! The texture must be destroyed before the StelRenderer that
//! constructed it.
//!
//! A texture can be in one of 4 states, depending on how
//! it is loaded. These are Uninitialized, Loading, Loaded, Error.
//!
//! Immediately after construction, a texture is in Uninitialized state.
//! If load mode specified with <em>createTextureBackend</em> is Normal, it
//! is immediately loaded (internally, it's in Loading state),
//! and one <em>createTextureBackend</em> is done, its state changes 
//! to Loaded on success or Error if loading failed. 
//!
//! The loading stage (and no other stage) might fail, resulting in Error 
//! state. If in Error state, error message can be retrieved by getErrorMessage.
//! A texture with the Error state can still be bound, but a placeholder texture 
//! will be used internally instead.
//!
//! If load mode is Asynchronous, the texture is loaded in a background 
//! thread, and during loading its state is Loading. Again, loading might
//! fail.
//!
//! If load mode is LazyAsynchronous, loading only starts once the texture 
//! is bound (used) for the first time.
//!
//!
//! Implementation-wise, StelTextureNew is a very thin wrapper around StelTextureBackend.
//! They are kept separate so their lifetimes can be independent. Destruction of a
//! StelTextureNew doesn't necessarily destroy the underlying backend, allowing for 
//! texture caching.
//!
//! @see TextureStatus
class StelTextureNew
{
// Only StelRenderer can construct StelTextureNew
friend class StelRenderer;
public:
	//! Destroy the texture.
	~StelTextureNew();

	//! Get the current texture status.
	//!
	//! Used e.g. to determine if the texture has been loaded or if an error 
	//! has occured.
	//!
	//! @return texture status.
	TextureStatus getStatus() const
	{
		return backend->getStatus();
	}

	//! Get texture dimensions in pixels.
	//!
	//! Can only be called when the texture is in the Loaded status
	//! (this is asserted).
	//! Use getStatus() to determine whether or not this is the case.
	//!
	//! @return Texture width and height in pixels.
	QSize getDimensions() const
	{
		return backend->getDimensions();
	}

	//! Get a human-readable message describing the error that happened during loading (if any).
	//!
	//! @return Error message if status is Error, empty string otherwise.
	const QString& getErrorMessage() const
	{
		return backend->getErrorMessage();
	}

	//! Bind the texture so that it can be used for drawing.
	//!
	//! If the texture is lazily loaded and has not been loaded yet,
	//! it will start loading.
	//!
	//! If the texture is in any state other than Loaded (i.e; even in Error state),
	//! a placeholder (checkers) texture is used instead, so bind() can be called 
	//! even if the texture is still loading, if you don't mind that a placeholder 
	//! will be shown for a short moment.
	//!
	//! Use getStatus() to determine whether the texture is loaded or not.
	//!
	//! @param textureUnit Texture unit to used
	void bind(const int textureUnit = 0);

private:
	//! Construct a StelTextureNew wrapping specified backend created by specified renderer.
	StelTextureNew(class StelRenderer* renderer, StelTextureBackend* backend)
		: renderer(renderer)
		, backend(backend)
	{
	}

	//! Renderer used to bind and destroy the texture.
	class StelRenderer* renderer;

	//! Texture backend.
	//!
	//! StelTextureNew acts as a wrapper so the backend doesn't have to be destroyed with it,
	//! allowing for things like texture caching on the renderer.
	StelTextureBackend* backend;
};

#endif // _STELTEXTURENEW_HPP_
