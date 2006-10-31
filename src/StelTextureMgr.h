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
#include "SDL_opengl.h"

class STexture;

/**
 * Class used to manage textures loading and manipulation.
 * @author Fabien Chereau <stellarium@free.fr>
 */
class StelTextureMgr
{
public:
	StelTextureMgr(const std::string& textureDir);
	virtual ~StelTextureMgr();
	
	//! Load an image from a file and create a new texture from it
	//! @param the texture file name, can be absolute path if starts with '/' otherwise
	//! the file will be looked in stellarium standard textures directories.
	STexture& createTexture(const std::string& filename);
	
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
private:
	//! Load a PNG image from a file.
	bool readPNGFromFile(const std::string& filename, class ManagedSTexture& texinfo);

	std::string textureDir;
	bool mipmapsMode;
	GLint wrapMode;
	GLint minFilter;
	GLint magFilter;
	
	static STexture NULL_STEXTURE;
};

#endif /*STELTEXTUREMGR_H_*/
