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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelProjector.hpp"
#include "StelLoadingBar.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"

#include <QDebug>
#include <QtOpenGL>

StelLoadingBar::StelLoadingBar(const QString& splashTex,
	const QString& extraTextString, float extraTextSize,
	float extraTextPosx, float extraTextPosy, int aWidth, int aHeight) :
	width(aWidth), height(aHeight), extraText(extraTextString), sPainter(NULL)
{
	extraTextFont.setPixelSize(extraTextSize);
	sPainter = new StelPainter(StelApp::getInstance().getCore()->getProjection2d());
	int screenw = sPainter->getProjector()->getViewportWidth();
	int screenh = sPainter->getProjector()->getViewportHeight();
	splashx = sPainter->getProjector()->getViewportPosX() + (screenw - width)/2;
	splashy = sPainter->getProjector()->getViewportPosY() + (screenh - height)/2;
	if (!splashTex.isEmpty())
		splash = StelApp::getInstance().getTextureManager().createTexture(splashTex);
	extraTextPos.set(extraTextPosx, extraTextPosy);
}

StelLoadingBar::~StelLoadingBar()
{
	delete sPainter;
}

void StelLoadingBar::draw()
{
	sPainter->setColor(1.f, 1.f, 1.f);
	sPainter->enableTexture2d(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw the splash screen if available
	if (splash)
	{
		splash->bind();
		sPainter->drawRect2d(splashx, splashy, width, height);
	}

	sPainter->setFont(extraTextFont);
	sPainter->drawText(splashx + extraTextPos[0], splashy + extraTextPos[1]-sPainter->getFontMetrics().height()-1, extraText);
	StelPainter::swapBuffer();	// And swap the buffers
}
