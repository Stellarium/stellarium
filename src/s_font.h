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

// Class to manage s_fonts

#ifndef _S_FONT_H
#define _S_FONT_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "SDL_opengl.h"

#include "s_texture.h"

typedef struct
{   int sizeX;
    int sizeY;
    int leftSpacing;
    int rightSpacing;
} s_fontGlyphInfo;


class s_font
{
public:
    s_font(float size_i, const string& textureName, const string& dataFileName);
    virtual ~s_font();
    void print(float x, float y, const string& str, int upsidedown = 1) const;
    void print_char(const unsigned char c) const;
    void print_char_outlined(const unsigned char c) const;
    float getStrLen(const string&) const;
    float getAverageCharLen(void) const {return averageCharLen*ratio;}
    float getLineHeight(void) const {return lineHeight*ratio;}
    float getStrHeight(const string&) const;
    s_texture * s_fontTexture;
protected:
    int buildDisplayLists(const string& dataFileName, const string& textureName);
    GLuint g_base;
    s_fontGlyphInfo theSize[256];
    float size;
    int lineHeight;
    float averageCharLen;
    float ratio;
    char name[20];
    const static int SPACING = 1;
};


#endif  //_S_FONT_H
