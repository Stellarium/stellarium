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

// Class to manage s_fonts

#ifndef _S_FONT_H
#define _S_FONT_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "SDL_opengl.h"

#include "stel_utility.h"
#include "s_texture.h"
#include "typeface.h"

class s_font
{
public:
    s_font(float size_i, const string& ttfFileName) : typeFace(ttfFileName, (size_t)(size_i), 72) {;}
    virtual ~s_font() {;}
    
    void print(float x, float y, const string& s, int upsidedown = 1) {typeFace.render(StelUtility::stringToWstring(s), Vec2f(x, y), upsidedown==1);}

	void print_char(const wchar_t c) {
		wchar_t wc[] = L"xx";
		wc[0] = c;
		wc[1] = 0;  // terminate string
		typeFace.renderGlyphs((wstring(wc)));
	}

	void print_char_outlined(const wchar_t c) {

		// This is not the most elegant way to do this,
		// but avoids needing two fonts loaded

		wchar_t wc[] = L"xx";
		wc[0] = c;
		wc[1] = 0;  // terminate string

		GLfloat current_color[4];
		glGetFloatv(GL_CURRENT_COLOR, current_color);	 
 	 	 

		glColor3f(0,0,0);
		//		glColor3f(0.2,0.2,0.2);

		glPushMatrix();
		glTranslatef(1,1,0);		
		typeFace.renderGlyphs((wstring(wc)));
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-1,-1,0);		
		typeFace.renderGlyphs((wstring(wc)));
		glPopMatrix();

		glPushMatrix();
		glTranslatef(1,-1,0);		
		typeFace.renderGlyphs((wstring(wc)));
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-1,1,0);		
		typeFace.renderGlyphs((wstring(wc)));
		glPopMatrix();

		glColor4fv(current_color);

		typeFace.renderGlyphs((wstring(wc)));
	}
	
	void print_char(const unsigned char c) {
		wchar_t wc[] = L"xx";
		wc[0] = c;
		wc[1] = 0;  // terminate string
		typeFace.renderGlyphs((wstring(wc)));
	}

	void print_char_outlined(const unsigned char c) {

		print_char_outlined(wchar_t(c));
	}

    float getStrLen(const wstring& s) {return typeFace.width(s);}
    float getStrLen(const string& s) {return typeFace.width(StelUtility::stringToWstring(s));}
    float getLineHeight(void) {return typeFace.lineHeight();}
    float getAscent(void) {return typeFace.ascent();}
    float getDescent(void) {return typeFace.descent();}
protected:
	
	TypeFace typeFace;
	
};


#endif  //_S_FONT_H
