/*
 * Stellarium
 * Copyright (C) 2005 Fabien Chereau
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

#include "Projector.hpp"
#include "LoadingBar.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"

LoadingBar::LoadingBar(Projector* _prj, float font_size, const string&  splash_tex, 
	int screenw, int screenh, const wstring& extraTextString, float extraTextSize, 
	float extraTextPosx, float extraTextPosy) :
	prj(_prj), width(512), height(512), barwidth(400), barheight(10),
barfont(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), font_size)),
extraTextFont(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), extraTextSize)),
			  extraText(extraTextString), clearBuffer(false)
{
	splashx = prj->getViewportPosX() + (screenw - width)/2;
	splashy = prj->getViewportPosY() + (screenh - height)/2;
	barx = prj->getViewportPosX() + (screenw - barwidth)/2;
	bary = splashy + 34;
	StelApp::getInstance().getTextureManager().setDefaultParams();
	if (!splash_tex.empty()) splash = StelApp::getInstance().getTextureManager().createTexture(splash_tex);
	extraTextPos.set(extraTextPosx, extraTextPosy);
	timeCounter = StelApp::getInstance().getTotalRunTime();
}

LoadingBar::~LoadingBar()
{
}

void LoadingBar::Draw(float val)
{
	// Ensure that the refresh frequency is not too high 
	// (display may be very slow when no 3D acceleration works)
	if (StelApp::getInstance().getTotalRunTime()-timeCounter<0.050)
		return;
	timeCounter = StelApp::getInstance().getTotalRunTime();
	if (val>1.)
		val = 1.;
  
	// Draw the splash screen if available
	if (splash)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);
		glDisable(GL_CULL_FACE);
		splash->bind();

		glBegin(GL_QUADS);
			glTexCoord2i(0, 0);		// Bottom Left
			glVertex3f(splashx, splashy, 0.0f);
			glTexCoord2i(1, 0);		// Bottom Right
			glVertex3f(splashx + width, splashy, 0.0f);
			glTexCoord2i(1, 1);		// Top Right
			glVertex3f(splashx + width, splashy + height, 0.0f);
			glTexCoord2i(0, 1);		// Top Left
			glVertex3f(splashx, splashy + height, 0.0f);
		glEnd();
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	// black out background of text for redraws (so can keep sky unaltered)
	glColor3f(0, 0, 0);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(barx + barwidth, bary-17, 0.0f);
		glVertex3f(barx, bary-17, 0.0f);
		glVertex3f(barx + barwidth, bary-5, 0.0f);
		glVertex3f(barx, bary-5, 0.0f);
	glEnd();
	glColor3f(0.8, 0.8, 1);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(barx + barwidth, bary + barheight, 0.0f);
		glVertex3f(barx, bary + barheight, 0.0f);
		glVertex3f(barx + barwidth, bary, 0.0f);
		glVertex3f(barx, bary, 0.0f);
	glEnd();
	glColor3f(0.4f, 0.4f, 0.6f);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-1 + barx + barwidth * val, bary + barheight - 1, 0.0f);
		glVertex3f(1 + barx, bary + barheight - 1, 0.0f);
		glVertex3f(-1 + barx + barwidth * val, bary + 1, 0.0f);
		glVertex3f(1 + barx, bary + 1, 0.0f);
	glEnd();
	
	glColor3f(1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	prj->drawText(&barfont, barx, bary-barfont.getLineHeight()-1, message);
	prj->drawText(&extraTextFont, splashx + extraTextPos[0], splashy + extraTextPos[1]-extraTextFont.getLineHeight()-1, extraText);
	
	StelApp::getInstance().swapGLBuffers();	// And swap the buffers
	if (clearBuffer)
		glClear(GL_COLOR_BUFFER_BIT);
}
