/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#include <iostream>

#include "glpng.h"
#include "s_texture.h"
#include "stellarium.h"

string s_texture::texDir = "./";
string s_texture::suffix = "";

s_texture::s_texture(const string& _textureName) : textureName(_textureName), texID(0),
	loadType(PNG_BLEND1), loadType2(GL_CLAMP)
{
    load();
}

s_texture::s_texture(const string& _textureName, int _loadType) : textureName(_textureName),
	texID(0), loadType(PNG_BLEND1), loadType2(GL_CLAMP_TO_EDGE)
{
    switch (_loadType)
    {
        case TEX_LOAD_TYPE_PNG_ALPHA : loadType=PNG_ALPHA;  break;
        case TEX_LOAD_TYPE_PNG_SOLID : loadType=PNG_SOLID; break;
        case TEX_LOAD_TYPE_PNG_BLEND3: loadType=PNG_BLEND3; break;
        case TEX_LOAD_TYPE_PNG_BLEND4: loadType=PNG_BLEND4; break;
        case TEX_LOAD_TYPE_PNG_BLEND1: loadType=PNG_BLEND1; break;
		case TEX_LOAD_TYPE_PNG_BLEND8: loadType=PNG_BLEND8; break;
        case TEX_LOAD_TYPE_PNG_REPEAT: loadType=PNG_BLEND1; loadType2=GL_REPEAT; break;
        case TEX_LOAD_TYPE_PNG_SOLID_REPEAT: loadType=PNG_SOLID; loadType2=GL_REPEAT; break;
        default : loadType=PNG_BLEND3;
    }
    texID=0;
    load();
}

s_texture::~s_texture()
{
    unload();
}

int s_texture::load()
{
	// Create the full texture name
    string fullName = texDir + textureName + suffix;

    FILE * tempFile = fopen(fullName.c_str(),"r");
    if (!tempFile)
	{
		cout << "WARNING : Can't find texture file " << fullName << "!" << endl;
		return 0;
	}
	fclose(tempFile);

    pngInfo info;
    pngSetStandardOrientation(1);
    texID = pngBind(fullName.c_str(), PNG_BUILDMIPMAPS, loadType, &info, loadType2, GL_LINEAR, GL_LINEAR);

	return (texID!=0);
}

void s_texture::unload()
{   
    glDeleteTextures(1, &texID);	// Delete The Texture
}

int s_texture::reload()
{
    unload();
    return load();
}

// Return the texture size in pixels
int s_texture::getSize(void) const
{
	glBindTexture(GL_TEXTURE_2D, texID);
	GLint w;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	return w;
}

// Return the average texture luminance : 0 is black, 1 is white
float s_texture::get_average_luminance(void) const
{
	glBindTexture(GL_TEXTURE_2D, texID);
	GLint w, h;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	GLfloat* p = (GLfloat*)calloc(w*h, sizeof(GLfloat));

	glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_FLOAT, p);
	float sum = 0.f;
	for (int i=0;i<w*h;++i)
	{
		sum += p[i];
	}
	free(p);


	/*
	// This provides more correct result on some video cards (matrox)
	// TODO test more before switching

	GLubyte* pix = (GLubyte*)calloc(w*h*3, sizeof(GLubyte));

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pix);

	float lum = 0.f;
	for (int i=0;i<w*h*3;i+=3)
	{
	  double r = pix[i]/255.;
	  double g = pix[i+1]/255.;
	  double b = pix[i+2]/255.;
	  lum += r*.299 + g*.587 + b*.114;
	}
	free(pix);

	printf("Luminance calc 2: Sum %f\tw %d h %d\tlum %f\n", lum, w, h, lum/(w*h));
	*/

	return sum/(w*h);

}


void s_texture::getDimensions(int &width, int &height) const
{
  glBindTexture(GL_TEXTURE_2D, texID);

  GLint w, h;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

  width = w;
  height = h;

}
