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

#ifndef _STELTEXTUREBACKEND_HPP_
#define _STELTEXTUREBACKEND_HPP_

#include <QDebug>
#include <QObject>
#include <QSize>
#include <QString>


//! Possible states of a texture.
enum TextureStatus
{
	//! Texture is not loaded yet. 
	//!
	//! Only lazily loaded textures are returned with this state. If bound, 
	//! the texture starts loading (meanwhile, a placeholder texture will be used)
	//!
	//! This state can only change into Loading.
	TextureStatus_Uninitialized,
	//! Texture is currently being loaded.
	//!
	//! Depending on whether loading succeeds or fails, this will change to
	//! Loaded or Error.
	TextureStatus_Loading,
	//! The texture was succesfully loaded and can be used.
	//!
	//! This state won't change until the texture is destroyed.
	TextureStatus_Loaded,
	//! The texture failed to load and can not be used (attempts to bind it will be ignored).
	//!
	//! This state won't change until the texture is destroyed.
	TextureStatus_Error
};

//! Convert a texture status to string (used for debugging).
inline QString textureStatusName(const TextureStatus status)
{
	switch(status)
	{
		case TextureStatus_Uninitialized: return "TextureStatus_Uninitialized"; break;
		case TextureStatus_Loaded:        return "TextureStatus_Loaded"; break;
		case TextureStatus_Loading:       return "TextureStatus_Loading"; break;
		case TextureStatus_Error:         return "TextureStatus_Error"; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture status");
	}
	// Avoid compiler warnings.
	return QString();
}

//! Ways to load a texture.
enum TextureLoadingMode
{
	//! Texture is loaded as it is created during the StelRenderer::createTextureBackend() call.
	//!
	//! The returned texture is in Loaded or Error status.
	TextureLoadingMode_Normal,
	//! Texture is loaded asynchronously (in a separate thread).
	//!
	//! The returned texture is in Loading state,
	//! which changes to Loaded or Error after loading is finished.
	TextureLoadingMode_Asynchronous,
	//! Texture is loaded asynchronously, but only when it's first used.
	//!
	//! The returned texture is in Uninitialized state. It starts to load at
	//! the first attempt to use (bind) it, changing into Loading, and the into 
	//! Loaded or Error after loading finishes.
	TextureLoadingMode_LazyAsynchronous
};

//! Base class for texture implementations.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
//!
//! @see StelTextureNew
class StelTextureBackend : public QObject
{
	Q_OBJECT

public:
	//! Destroy the texture. 
	virtual ~StelTextureBackend(){};

	//! Get the current texture status.
	//!
	//! Used e.g. to determine if the texture has been loaded or if an error 
	//! has occured.
	//!
	//! @return texture status.
	TextureStatus getStatus() const
	{
		return status;
	}

	//! Get the "name" of this texture.
	//!
	//! The name might be the full path if loaded from file, 
	//! URL if loaded from network, or nothing at all when generated.
	const QString& getName() const
	{
		return path;
	}

	//! Get texture dimensions in pixels.
	//!
	//! Can only be called when the texture has been successfully loaded 
	//! (this is asserted).
	//! Use getStatus() to determine whether or not this is the case.
	//!
	//! @return Texture width and height in pixels.
	QSize getDimensions() const
	{
        invariant();
		Q_ASSERT_X(status == TextureStatus_Loaded, Q_FUNC_INFO,
		           "Trying to get dimensions of a texture that is not loaded. "
		           "Use StelTextureBackend::getStatus to determine if the texture "
                   "is loaded or not.");
		return size;
	}

	//! Get a human-readable message describing the error that happened during loading (if any).
	//!
	//! @return Error message if status is TextureStatus_Error, empty string otherwise.
	const QString& getErrorMessage() const
	{
		invariant();
		return errorMessage;
    }

#ifdef USE_OPENGL_ES2
    //! Save image dimensions. OpenGL ES 2 only.
    //!
    //! @param dimensions Size of texture in pixels
    void setImageSize(const QSize dimensions)
    {
        invariant();
        Q_ASSERT_X(status == TextureStatus_Loading, Q_FUNC_INFO,
                   "Only force set image dimensions on unloaded images");
        size = dimensions;
        invariant();
    }
#endif

protected:
	//! Full file system path or URL of the texture file.
	const QString path;

	//! Construct a StelTextureBackend with specified texture path/url.
	//!
	//! Note that it makes no sense to instantiate StelTextureBackend itself -
	//! it needs to be derived by a backend.
	StelTextureBackend(const QString& path)
		: QObject()
		, path(path)
		, errorMessage(QString())
		, size(QSize(0, 0))
		, status(TextureStatus_Uninitialized)
	{
		invariant();
	}

	//! Must be called before loading an image (whether normally or asynchronously).
	//!
	//! Asserts that status is uninitialized and changes it to loading.
	void startedLoading()
	{
		invariant();
		Q_ASSERT_X(status == TextureStatus_Uninitialized, Q_FUNC_INFO,
		           "Only a texture that has not yet been initialized can start loading");
		status = TextureStatus_Loading;
		invariant();
	}

	//! Must be called after succesfully loading an image (whether normally or asynchronously).
	//!
	//! At this point, texture size is initialized and becomes valid.
	//! Asserts that status is loading and changes it to loaded.
	//!
	//! @param dimensions Size of loaded texture in pixels.
	void finishedLoading(const QSize dimensions)
	{
		invariant();
		Q_ASSERT_X(status == TextureStatus_Loading, Q_FUNC_INFO,
		           "Only a texture that has started loading can finish loading");
		size = dimensions;
        status = TextureStatus_Loaded;
		invariant();
	}

	//! Must be called when an error occurs during loading.
	//!
	//! Texture loading is the only stage when an error may occur 
	//! (i.e. the texture can't just become invalid at any time).
	//! Asserts that status is loading, changes it to error, and specifies error message.
	void errorOccured(const QString& error)
	{
		invariant();
		if(status != TextureStatus_Loading)
		{
			qWarning() << "Unexpected error - texture " << path;
			qWarning() << "Texture status: " << textureStatusName(status);
			Q_ASSERT_X(false, Q_FUNC_INFO,
			           "The only time an error can occur with a texture is during loading");
		}
		qWarning() << "Error occured during loading of texture " << path << 
		              ": " << error;
		errorMessage = error;
		status = TextureStatus_Error;
		invariant();
	}

#ifdef USE_OPENGL_ES2
    //! Allows us to retrieve size before the texture is built. This is not guaranteed to be representative of the final size; if it's 0, size hasn't been set up yet.
    QSize getDimensionsEarly() const
    {
        return size;
    }
#endif

private:
	//! Stores the error message if status is TextureStatus_Error.
	QString errorMessage;

    //! Size of the texture in pixels.
    //!
    //! Only valid once the texture is loaded.
    QSize size;

	//! Current texture status.
	//!
	//! Specifies whether the texture is loaded and ready to draw or still loading and so on.
	TextureStatus status;

	//! Ensure that we're in consistent state.
	void invariant() const
	{
		Q_ASSERT_X(errorMessage.isEmpty() == (status != TextureStatus_Error),
		           Q_FUNC_INFO,
		           "Error message must be empty when status is not Error and non-empty otherwise");
	}
};

#endif // _STELTEXTUREBACKEND_HPP_
