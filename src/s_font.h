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

#ifdef MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

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
    s_font(float size_i, const char * textureName, const char * dataFileName);
    virtual ~s_font();
    void print(float x, float y, const char * str, int upsidedown = 1) const;
    float getStrLen(const char * str) const;
    float getAverageCharLen(void) const {return averageCharLen*ratio;}
    float getLineHeight(void) const {return lineHeight*ratio;}
    float getStrHeight(const char * str) const;
    s_texture * s_fontTexture;
private:
    int buildDisplayLists(const char * dataFileName, const char * textureName);
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
