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

#if defined( MACOSX )
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
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
    s_font(float size_i, char * textureName, char * dataFileName);
    virtual ~s_font();
    void print(float x, float y, char * str);
    float getStrLen(const char * str);
    float getAverageCharLen(void) {return averageCharLen*ratio;}
    float getLineHeight(void) {return lineHeight*ratio;}
    float getStrHeight(const char * str);
    s_texture * s_fontTexture;
private:
    int buildDisplayLists(char * dataFileName, char * textureName);
    GLuint g_base;
    s_fontGlyphInfo theSize[256];
    float size;
    int lineHeight;
    float averageCharLen;
    float ratio;
    char name[20];
};

#endif  //_S_FONT_H
