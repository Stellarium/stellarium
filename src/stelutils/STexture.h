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

#ifndef _STEXTURE_H_
#define _STEXTURE_H_

#include <string>
#include "vecmath.h"
#include "SDL_opengl.h"

/**
 * Base texture class.
 * @author Fabien Chereau <stellarium@free.fr>
 */
class STexture
{
public:
	STexture();
	virtual ~STexture();
	//! Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
	virtual void bind() const {glBindTexture(GL_TEXTURE_2D, id);}
	//! Return the width and heigth of the texture in pixels
	void getDimensions(int &width, int &height) const;
	//! Return the average texture luminance.
	//! @return 0 is black, 1 is white
    virtual float getAverageLuminance(void);
    
    GLsizei width;
	GLsizei height;
	
	GLenum format;
	GLint internalFormat;
	GLubyte *texels;
	
	// Position of the 4 corners of the texture in texture coordinates
	Vec2d texCoordinates[4];
protected:
	GLuint id;
	
	std::string fullPath;
	bool mipmapsMode;
	GLint wrapMode;
	GLint minFilter;
	GLint magFilter;
};

#endif // _STEXTURE_H_
