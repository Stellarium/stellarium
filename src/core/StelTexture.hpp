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

#ifndef _STELTEXTURE_HPP_
#define _STELTEXTURE_HPP_

#include "GLee.h"
#include "fixx11h.h"
#include "VecMath.hpp"
#include "StelTextureTypes.hpp"

#include <QObject>
#include <QImage>

class QMutex;
class QSemaphore;
class QFile;

//! @class StelTexture 
//! Base texture class. For creating an instance, use StelTextureMgr::createTexture() and StelTextureMgr::createTextureThread()
//! @sa StelTextureSP
class StelTexture : public QObject
{
	Q_OBJECT;
	
	friend class StelTextureMgr;
	friend class ImageLoadThread;

public:
	//! Destructor
	virtual ~StelTexture();
	
	//! Bind the texture so that it can be used for openGL drawing (calls glBindTexture).
	//! If the texture is lazyly loaded, this starts the loading and return false immediately.
	//! @return true if the binding successfully occured, false if the texture is not yet loaded.
	bool bind();
	
	//! Return whether the texture can be binded, i.e. it is fully loaded
	bool canBind() const {return id!=0;}
	
	//! Get the average texture luminance.
	//! @param lum 0 is black, 1 is white
	//! @return true if the returned luminance is known, false if not. In this later case, the value of num is undefined.
	bool getAverageLuminance(float& lum);

	//! Return the width and heigth of the texture in pixels
	bool getDimensions(int &width, int &height);

	//! Return the position of the 4 corners of the texture in texture coordinates
	const Vec2d* getCoordinates() const {return texCoordinates;}

	//! Get the error message which caused the texture loading to fail
	//! @return the human friendly error message or empty string if no errors occured
	const QString& getErrorMessage() const {return errorMessage;}
	
	//! Return the full path to the image file
	const QString& getFullPath() const {return fullPath;}
	
	//! Return whether the image is currently being loaded
	bool isLoading() const {return isLoadingImage && !canBind();}
	
signals:
	//! Emitted when the texture is ready to be bind(), i.e. when downloaded, imageLoading and	glLoading is over
	//! or when an error occured and the texture will never be available
	//! In case of error, you can query what the problem was by calling getErrorMessage()
	//! @param error is equal to true if an error occured while loading the texture
	void loadingProcessFinished(bool error);
	
private slots:
	//! Called when the download for the texture file terminated
	void downloadFinished();
	
	//! Called when the file loading thread has terminated
	void fileLoadFinished();
	
private:
	//! Private constructor
	StelTexture();
	
	//! Load the texture already in the RAM to the openGL memory
	//! This function uses openGL routines and must be called in the main thread
	//! @return false if an error occured
	bool glLoad();
	
	//! Loading of the image data.
	//! This method is thread safe
	//! @return false if an error occured
	bool imageLoad();
	
	//! This method should be called if the texture loading failed for any reasons
	//! @param errorMessage the human friendly error message
	void reportError(const QString& errorMessage);
	
	//! Define the range mode used to rescale the texture when loading
	StelTextureTypes::DynamicRangeMode dynamicRangeMode;
	
	//! Used to download remote files if needed
	class QNetworkReply* httpReply;
	
	//! Used to load in thread
	class ImageLoadThread* loadThread;
			
	//! Define if the texture was already downloaded if it was a remote one
	bool downloaded;
	//! Define whether the image is already loading
	bool isLoadingImage;
	
	//! The URL where to download the file
	QString fullPath;
	
	//! The data that was laoded from http
	QByteArray downloadedData;
	QImage qImage;
	
	//! Used ony when creating temporary file
	QString fileExtension;
	
	//! True when something when wrong in the loading process
	bool errorOccured;
	
	//! Human friendly error message if loading failed
	QString errorMessage;
	
	//! OpenGL id
	GLuint id;
	bool mipmapsMode;
	GLint wrapMode;
	GLint minFilter;
	GLint magFilter;
			
	///////////////////////////////////////////////////////////////////////////
	// Attributes protected by the Mutex
	//! Mutex used to protect all the attributes below 
	QMutex* mutex;

	//! Cached average luminance
	float avgLuminance;
	
	GLsizei width;	//! Texture image width
	GLsizei height;	//! Texture image height
	GLenum format;
	GLint internalFormat;
	GLubyte* texels;
	GLenum type;
	
	//! Position of the 4 corners of the texture in texture coordinates
	Vec2d texCoordinates[4];
	
	//! Fix a limit to the number of maximum simultaneous loading threads
	static QSemaphore* maxLoadThreadSemaphore;
};


#endif // _STELTEXTURE_HPP_
