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

#include "stellarium.h"
#include "s_texture.h"
#include "glpng.h"

s_texture::s_texture(char * _textureName) : loadType(PNG_BLEND3)
{   
    texID=0;
    textureName=strdup(_textureName);
    if (!textureName) exit(1);
    load();
}

s_texture::s_texture(char * _textureName, int _loadType)
{   
    switch (_loadType)
    {
        case TEX_LOAD_TYPE_PNG_ALPHA : loadType=PNG_ALPHA; break;
        case TEX_LOAD_TYPE_PNG_SOLID : loadType=PNG_SOLID; break;
        case TEX_LOAD_TYPE_PNG_BLEND3 : loadType=PNG_BLEND3; break;
        default : loadType=PNG_BLEND3;
    }
    texID=0;
    textureName=strdup(_textureName);
    if (!textureName) exit(1);
    load();
}

s_texture::~s_texture()
{   
    unload();
    if (textureName) free(textureName);
}

int s_texture::load()
{
    char * fullName = (char*)malloc(sizeof(char)*strlen(TEXTURE_DIR)+sizeof(char)*strlen(textureName)+6);
    sprintf(fullName,"%s/%s.png",TEXTURE_DIR,textureName);
    //printf("Loading %s\n",fullName);
    pngInfo info;
    pngSetStandardOrientation(1);
    texID = pngBind(fullName, PNG_NOMIPMAP, loadType, &info, GL_CLAMP, GL_LINEAR, GL_LINEAR);
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
