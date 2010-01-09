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

#include "StelProjector.hpp"
#include "StelLoadingBar.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"

#include <QDebug>
#include <QtOpenGL>

StelLoadingBar::StelLoadingBar(float fontSize, const QString&  splashTex,
	const QString& extraTextString, float extraTextSize, 
	float extraTextPosx, float extraTextPosy) :
	width(512), height(512), barwidth(400), barheight(10), extraText(extraTextString), sPainter(NULL)
{
	barfont.setPixelSize(fontSize);
	extraTextFont.setPixelSize(extraTextSize);
	sPainter = new StelPainter(StelApp::getInstance().getCore()->getProjection2d());
	int screenw = sPainter->getProjector()->getViewportWidth();
	int screenh = sPainter->getProjector()->getViewportHeight();
	splashx = sPainter->getProjector()->getViewportPosX() + (screenw - width)/2;
	splashy = sPainter->getProjector()->getViewportPosY() + (screenh - height)/2;
	barx = sPainter->getProjector()->getViewportPosX() + (screenw - barwidth)/2;
	bary = splashy + 34;
	if (!splashTex.isEmpty())
		splash = StelApp::getInstance().getTextureManager().createTexture(splashTex);
	extraTextPos.set(extraTextPosx, extraTextPosy);
	timeCounter = StelApp::getInstance().getTotalRunTime();
}

StelLoadingBar::~StelLoadingBar()
{
	delete sPainter;
}

void StelLoadingBar::Draw(float val)
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
		sPainter->enableTexture2d(true);
		sPainter->setColor(1., 1., 1.);
		splash->bind();

		sPainter->drawRect2d(splashx, splashy, width, height);
	}
	sPainter->enableTexture2d(false);

	// black out background of text for redraws (so can keep sky unaltered)
	sPainter->setColor(0, 0, 0, 1);
	sPainter->drawRect2d(barx, bary-5, barwidth, 12., false);
	sPainter->setColor(0.8, 0.8, 1, 1);
	sPainter->drawRect2d(barx, bary, barwidth, barheight, false);
	sPainter->setColor(0.4f, 0.4f, 0.6f, 1.f);
	sPainter->drawRect2d(barx+1, bary+1, barwidth * val-2, barheight-2, false);
	
	sPainter->setColor(1, 1, 1, 1);
	sPainter->enableTexture2d(true);
	glEnable(GL_BLEND);
	sPainter->setFont(barfont);
	sPainter->drawText(barx, bary-sPainter->getFontMetrics().height()-1, message);
	sPainter->setFont(extraTextFont);
	sPainter->drawText(splashx + extraTextPos[0], splashy + extraTextPos[1]-sPainter->getFontMetrics().height()-1, extraText);
	
	StelMainGraphicsView::getInstance().swapBuffer();	// And swap the buffers
	glClear(GL_COLOR_BUFFER_BIT);
}
