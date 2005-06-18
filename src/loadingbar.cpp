/*
 * Stellarium
 * Copyright (C) 2005 Fabien Chéreau
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

#include "loadingbar.h"

LoadingBar::LoadingBar(Projector* _prj, string _font_filename, int _barx, int _bary) :
	prj(_prj), barx(_barx), bary(_bary)
{
	barfont = new s_font(12., "spacefont", _font_filename);
	assert(barfont);
}
	
LoadingBar::~LoadingBar()
{
	if (barfont) delete barfont;
	barfont = NULL;
}
	
void LoadingBar::Draw(float val)
{
	// percent complete bar only draws in 2d mode
	prj->set_orthographic_projection();
  
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	// black out background of text for redraws (so can keep sky unaltered)
	glColor3f(0, 0, 0);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(1, 0);		// Bottom Right
		glVertex3f(barx + 302, bary + 36, 0.0f);
		glTexCoord2i(0, 0);		// Bottom Left
		glVertex3f(barx - 2, bary + 36, 0.0f);
		glTexCoord2i(1, 1);		// Top Right
		glVertex3f(barx + 302, bary + 22, 0.0f);
		glTexCoord2i(0, 1);		// Top Left
		glVertex3f(barx - 2, bary + 22, 0.0f);
	glEnd();
	glColor3f(1, 1, 1);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(1, 0);		// Bottom Right
		glVertex3f(barx + 302, bary + 22, 0.0f);
		glTexCoord2i(0, 0);		// Bottom Left
		glVertex3f(barx - 2, bary + 22, 0.0f);
		glTexCoord2i(1, 1);		// Top Right
		glVertex3f(barx + 302, bary - 2, 0.0f);
		glTexCoord2i(0, 1);		// Top Left
		glVertex3f(barx - 2, bary - 2, 0.0f);
	glEnd();
	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(1, 0);		// Bottom Right
		glVertex3f(barx + 300 * val, bary + 20, 0.0f);
		glTexCoord2i(0, 0);		// Bottom Left
		glVertex3f(barx, bary + 20, 0.0f);
		glTexCoord2i(1, 1);		// Top Right
		glVertex3f(barx + 300 * val, bary, 0.0f);
		glTexCoord2i(0, 1);		// Top Left
		glVertex3f(barx, bary, 0.0f);
	glEnd();
	
	glColor3f(1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	barfont->print(barx - 2, bary + 35, message);
	SDL_GL_SwapBuffers();	// And swap the buffers
	
	prj->reset_perspective_projection();
}
