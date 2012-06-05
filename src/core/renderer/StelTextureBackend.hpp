#ifndef _STELTEXTUREBACKEND_HPP_
#define _STELTEXTUREBACKEND_HPP_

#include <QDebug>
#include <QObject>
#include <QSize>
#include <QString>


//! Enumerates possible states of a texture.
enum TextureStatus
{
	//! Texture is not loaded yet. 
	//!
	//! At this point, the texture can't be bound (if it loads lazily,
	//! a bind call will start loading without actually binding the texture).
	//!
	//! This state can only change into Loading.
	TextureStatus_Uninitialized,
	//! Texture is currently being loaded.
	//!
	//! Depending on whether loading succeeds or fails, this will change to
	//! Loading or Error.
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

//! Enumerates ways to load a texture.
enum TextureLoadingMode
{
	//! Texture is loaded as it is created during the createTextureBackend call.
	//!
	//! The returned texture is in Loaded or Error status.
	TextureLoadingMode_Normal,
	//! Texture is loaded asynchronously (e.g. in a separate thread).
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

//! Texture interface.
//!
//! Constructed by StelRenderer::createTextureBackend().
//! To use the texture, it can be bound by StelRenderer::bindTexture().
//! The texture should be destroyed before the StelRenderer that
//! constructed it.
//!
//! A texture can be in one of 4 states, depending on how
//! it is loaded. These are Uninitialized, Loading, Loaded, Error.
//!
//! Immediately after construction, a texture is in Uninitialized state.
//! If load mode specified with createTextureBackend is Normal, it
//! is immediately loaded (internally, it's in Loading state),
//! and its state changes to Loaded on success or Error if loading failed. 
//!
//! The loading stage (and no other stage) might fail, resulting in Error 
//! state. If in Error state, error message can be retrieved by getErrorMessage.
//!
//! If load mode is Asynchronous, the texture is loaded in background 
//! thread, and during loading its state is Loading. Again, loading might
//! fail.
//!
//! If load mode is LazyAsynchronous, loading only starts once the texture 
//! is bound (used) for the first time.
class StelTextureBackend : public QObject
{
	Q_OBJECT

public:
	//! Destroy the texture. 
	//!
	//! It's user's, nor Renderer's, responsibility do destroy the texture.
	//! Note that the texture must be destroyed before Renderer is destroyed.
	virtual ~StelTextureBackend(){};

	//! Get the current texture status.
	//!
	//! Used e.g. to determine if the texture hsa been loaded or if an error 
	//! has occured.
	//!
	//! @return texture status.
	TextureStatus getStatus() const
	{
		invariant();
		return status;
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

	//! Get the "name" of this texture.
	//!
	//! The name might be the full path if loaded from file, 
	//! URL if loaded from network, or nothing at all when generated.
	const QString& getName() const
	{
		return path;
	}

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

	//! Must be called after loading an image (whether normally or asynchronously).
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
		Q_ASSERT_X(status == TextureStatus_Loading,
		           Q_FUNC_INFO,
		           "The only time an error can occur with a texture is during loading");
		qWarning() << "Error occured during loading of texture " << path << 
		              ": " << error;
		errorMessage = error;
		status = TextureStatus_Error;
		invariant();
	}
	
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
