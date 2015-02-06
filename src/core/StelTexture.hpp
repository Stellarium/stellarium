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

#ifndef _STELTEXTURE_HPP_
#define _STELTEXTURE_HPP_

#include "StelTextureTypes.hpp"
#include "StelOpenGL.hpp"

#include <QObject>
#include <QImage>

class QFile;
class StelTextureMgr;
class QNetworkReply;
template <class T> class QFuture;

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

//! @class StelTexture
//! Base texture class. For creating an instance, use StelTextureMgr::createTexture() and StelTextureMgr::createTextureThread()
//! @sa StelTextureSP
class StelTexture: public QObject
{
	Q_OBJECT

public:
	//! Contains the parameters defining how a texture is created.
	struct StelTextureParams
	{
		StelTextureParams(bool qgenerateMipmaps=false, GLint afiltering=GL_LINEAR, GLint awrapMode=GL_CLAMP_TO_EDGE, bool qfilterMipmaps=false) :
				generateMipmaps(qgenerateMipmaps),
				filterMipmaps(qfilterMipmaps),
				filtering(afiltering),
				wrapMode(awrapMode){;}
		//! Define if mipmaps must be created.
		bool generateMipmaps;
		//! If true, mipmapped textures are filtered with GL_LINEAR_MIPMAP_LINEAR instead of GL_LINEAR_MIPMAP_NEAREST (i.e. enabling "trilinear" filtering)
		bool filterMipmaps;
		//! Define the scaling filter to use. Must be one of GL_NEAREST or GL_LINEAR
		GLint filtering;
		//! Define the wrapping mode to use. Must be one of GL_CLAMP_TO_EDGE, or GL_REPEAT.
		GLint wrapMode;
	};

	//! Destructor
	virtual ~StelTexture();

	//! Bind the texture so that it can be used for openGL drawing (calls glBindTexture).
	//! If the texture is lazyly loaded, this starts the loading and return false immediately.
	//! @return true if the binding successfully occured, false if the texture is not yet loaded.
	
	bool bind(int slot=0);

	//! Return whether the texture can be binded, i.e. it is fully loaded
	bool canBind() const {return id!=0;}

	//! Return the width and heigth of the texture in pixels
	bool getDimensions(int &width, int &height);

	//! Returns whether the texture has an alpha channel (GL_RGBA or GL_LUMINANCE_ALPHA format)
	//! This only returns valid information after the texture is fully loaded.
	bool hasAlphaChannel() const { return alphaChannel ; }

	//! Get the error message which caused the texture loading to fail
	//! @return the human friendly error message or empty string if no errors occured
	const QString& getErrorMessage() const {return errorMessage;}

	//! Return the full path to the image file.
	//! If the texture was downloaded from a remote location, this function return the full URL.
	const QString& getFullPath() const {return fullPath;}

	//! Return whether the image is currently being loaded
	bool isLoading() const {return (loader || networkReply) && !canBind();}

signals:
	//! Emitted when the texture is ready to be bind(), i.e. when downloaded, imageLoading and	glLoading is over
	//! or when an error occured and the texture will never be available
	//! In case of error, you can query what the problem was by calling getErrorMessage()
	//! @param error is equal to true if an error occured while loading the texture
	void loadingProcessFinished(bool error);

private slots:
	void onNetworkReply();

private:
	friend class StelTextureMgr;

	//! structure returned by the loader threads, containing all the
	//! data and information to create the OpenGL texture.
	struct GLData
	{
		GLData() : data(NULL), width(0), height(0), format(0), type(0) {}
		QByteArray data;
		int width;
		int height;
		GLint format;
		GLint type;
	};
	//! Those static methods can be called by QtConcurrent::run
	static GLData imageToGLData(const QImage &image);
	static GLData loadFromPath(const QString &path);
	static GLData loadFromData(const QByteArray& data);

	//! Private constructor
	StelTexture();

	//! Convert a QImage into opengl compatible format.
	static QByteArray convertToGLFormat(const QImage& image, GLint* format, GLint* type);

	//! This method should be called if the texture loading failed for any reasons
	//! @param errorMessage the human friendly error message
	void reportError(const QString& errorMessage);

	//! Load the texture already in the RAM to the openGL memory
	//! This function uses openGL routines and must be called in the main thread
	//! @return false if an error occured
	bool glLoad(const QImage& image);
	//! Same as glLoad(QImage), but with an image already in OpenGl format
	bool glLoad(const GLData& data);

	StelTextureParams loadParams;

	//! Used to handle the connection for remote textures.
	QNetworkReply *networkReply;

	//! The loader object
	QFuture<GLData>* loader;


	//! The URL where to download the file
	QString fullPath;

	//! True when something when wrong in the loading process
	bool errorOccured;

	//! True if this texture contains an alpha channel
	bool alphaChannel;

	//! Human friendly error message if loading failed
	QString errorMessage;

	//! OpenGL id
	GLuint id;

	//! Cached average luminance
	float avgLuminance;

	GLsizei width;	//! Texture image width
	GLsizei height;	//! Texture image height
};


#endif // _STELTEXTURE_HPP_
