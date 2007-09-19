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

#include <cassert>
#include "STexture.hpp"

/*************************************************************************
 Constructor for the STexture class
*************************************************************************/
STexture::STexture() : texels(NULL), type(GL_UNSIGNED_BYTE), id(0)
{
	texCoordinates[0].set(1., 0.);
	texCoordinates[1].set(0., 0.);
	texCoordinates[2].set(1., 1.);
	texCoordinates[3].set(0., 1.);
}

/*************************************************************************
 Destructor for the STexture class
*************************************************************************/
STexture::~STexture()
{
	if (texels)
		delete texels;
	texels = NULL;
	if (glIsTexture(id)==GL_FALSE)
	{
		// std::cerr << "Warning: in STexture::~STexture() tried to delete invalid texture with ID=" << id << " Current GL ERROR status is " << glGetError() << std::endl;
	}
	else
	{
		glDeleteTextures(1, &id);
		// std::cerr << "Delete texture with ID=" << id << std::endl;
	}
}


/*************************************************************************
 Return the average texture luminance : 0 is black, 1 is white
*************************************************************************/
float STexture::getAverageLuminance(void)
{
	int size = width*height;
	glBindTexture(GL_TEXTURE_2D, id);
	GLfloat* p = (GLfloat*)calloc(size, sizeof(GLfloat));
	assert(p);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_FLOAT, p);
	float sum = 0.f;
	for (int i=0;i<size;++i)
	{
		sum += p[i];
	}
	free(p);

	return sum/size;
}

/*************************************************************************
 Return the width and heigth of the texture in pixels
*************************************************************************/
void STexture::getDimensions(int &awidth, int &aheight) const
{
	awidth = width;
	aheight = height;
}
