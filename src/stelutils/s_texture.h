/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _S_TEXTURE_H_
#define _S_TEXTURE_H_

#include <string>

#include "SDL_opengl.h"

using namespace std;

#define TEX_LOAD_TYPE_PNG_ALPHA 0
#define TEX_LOAD_TYPE_PNG_SOLID 1
#define TEX_LOAD_TYPE_PNG_BLEND3 2
#define TEX_LOAD_TYPE_PNG_BLEND8 5
#define TEX_LOAD_TYPE_PNG_BLEND4 6
#define TEX_LOAD_TYPE_PNG_BLEND1 7
#define TEX_LOAD_TYPE_PNG_REPEAT 3
#define TEX_LOAD_TYPE_PNG_SOLID_REPEAT 4

class STexture
{
public:
    STexture(const string& _textureName);
    STexture(bool full_path, const string& _textureName, int _loadType);
    STexture(bool full_path, const string& _textureName, int _loadType, const bool mipmap);
	STexture(const string& _textureName, int _loadType);
    STexture(const string& _textureName, int _loadType, const bool mipmap);
    virtual ~STexture();
    STexture(const STexture &t);
    const STexture &operator=(const STexture &t);
    int load(string fullName);
	int load(string fullName, bool mipmap);
    void unload();
    int reload();
    unsigned int getID(void) const {return texID;}
    // Return the average texture luminance : 0 is black, 1 is white
    float get_average_luminance(void) const;
    int getSize(void) const;
    void getDimensions(int &width, int &height) const;
    static void set_texDir(const string& _texDir) {STexture::texDir = _texDir;}

private:
    string textureName;
    GLuint texID;
    int loadType;
    int loadType2;
	static string texDir;
	bool whole_path;
};


#endif // _S_TEXTURE_H_
