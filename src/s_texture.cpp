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

#include "s_texture.h"
#include "stdlib.h"
#include "glpng.h"

char s_texture::texDir[255] = "./";
char s_texture::suffix[10] = "";

s_texture::s_texture(const char * _textureName) : texID(0), loadType(PNG_BLEND3), loadType2(GL_CLAMP)
{
    if (!_textureName) exit(-1);
    textureName=strdup(_textureName);
    load();
}

s_texture::s_texture(const char * _textureName, int _loadType)
{
    if (!_textureName) exit(-1);
    switch (_loadType)
    {
        case TEX_LOAD_TYPE_PNG_ALPHA : loadType=PNG_ALPHA; break;
        case TEX_LOAD_TYPE_PNG_SOLID : loadType=PNG_SOLID; break;
        case TEX_LOAD_TYPE_PNG_BLEND3: loadType=PNG_BLEND1; break;
        case TEX_LOAD_TYPE_PNG_REPEAT: loadType=PNG_BLEND3; loadType2=GL_REPEAT; break;
        default : loadType=PNG_BLEND3;
    }
    texID=0;
    textureName=strdup(_textureName);
    load();
}

s_texture::~s_texture()
{
//printf("Unloading texture ID=%u %s\n",texID, textureName);
    unload();
    if (textureName) free(textureName);
}

int s_texture::load()
{
	// Create the full texture name
    char * fullName = (char*)malloc( sizeof(char) * ( strlen(texDir) + strlen(textureName) + strlen(suffix) + 1) );
    sprintf(fullName,"%s%s%s",texDir,textureName,suffix);

    FILE * tempFile = fopen(fullName,"r");
    if (!tempFile) printf("WARNING : Can't find texture file %s!\n",fullName);
	fclose(tempFile);

    pngInfo info;
    pngSetStandardOrientation(1);
    texID = pngBind(fullName, PNG_BUILDMIPMAPS, loadType, &info, loadType2, GL_LINEAR, GL_LINEAR);

//printf("loaded texture ID=%u %s\n",texID, fullName);

	if (fullName) free(fullName);
	fullName=NULL;

	return (texID!=0);
}

void s_texture::unload()
{   
    glDeleteTextures(1, &texID);						// Delete The Texture
}

int s_texture::reload()
{	
    unload();
    return load();
}
