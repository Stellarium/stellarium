/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include "SFont.hpp"
#include "GLee.h"
#include "fixx11h.h"
#include <QFile>

SFont::SFont(float size_i, const QString& ttfFileName) : typeFace(QFile::encodeName(ttfFileName).constData(), (size_t)(size_i), 72) 
{;}

void SFont::print_char_outlined(const wchar_t c) const
{
	// This is not the most elegant way to do this,
	// but avoids needing two fonts loaded
	wchar_t wc[] = L"xx";
	wc[0] = c;
	wc[1] = 0;  // terminate string
	
	GLfloat current_color[4];
	glGetFloatv(GL_CURRENT_COLOR, current_color);	 
	 
	glColor3f(0,0,0);
	
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
