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
#include "StelLocaleMgr.hpp"
#include "StelCore.hpp"
#include "renderer/StelRenderer.hpp"

#include <QDebug>

StelLoadingBar::StelLoadingBar(const QString& splashTex,
	const QString& extraTextString, float extraTextSize,
	float extraTextPosx, float extraTextPosy, int aWidth, int aHeight) 
	: width(aWidth)
	, height(aHeight)
	, splash(NULL)
	, extraText(extraTextString)
	, splashName(splashTex)
	, viewportXywh(StelApp::getInstance().getCore()->getProjection2d()->getViewportXywh())
{
	extraTextFont.setPixelSize(extraTextSize);
	splashx = viewportXywh[0] + (viewportXywh[2] - width)/2;
	splashy = viewportXywh[1] + (viewportXywh[3] - height)/2;
	extraTextPos.set(extraTextPosx, extraTextPosy);
}

StelLoadingBar::~StelLoadingBar()
{
	if(NULL != splash) {delete splash;}
}

void StelLoadingBar::draw(StelRenderer* renderer)
{
	if(NULL == splash)
	{
		splash = renderer->createTexture(splashName);
	}

	// Background.
	renderer->setGlobalColor(0.0f, 0.0f, 0.0f);
	renderer->drawRect(viewportXywh[0], viewportXywh[1], viewportXywh[2], viewportXywh[3]);

	renderer->setGlobalColor(1.0f, 1.0f, 1.0f);
	renderer->setBlendMode(BlendMode_Alpha);

	// Draw the splash screen.
	splash->bind();
	renderer->drawTexturedRect(splashx, splashy, width, height);

	renderer->setFont(extraTextFont);
	const QFontMetrics fontMetrics(extraTextFont);
	renderer->drawText(TextParams(splashx + extraTextPos[0], 
	                              splashy + extraTextPos[1] - fontMetrics.height() - 1, 
	                              extraText));

	renderer->swapBuffers();
}
