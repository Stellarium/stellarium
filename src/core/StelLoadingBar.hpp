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

#ifndef _STELLOADINGBAR_HPP_
#define _STELLOADINGBAR_HPP_

#include "StelProjectorType.hpp"
#include "VecMath.hpp"

#include <QString>
#include <QFont>

//! @class StelLoadingBar 
//! This class is used to display loading bar.
//! A StelLoadingBar has a progress bar whose value can be set after the creation
//! of the StelLoadingBar, and a text display area, whose contents may also be
//! modified after the creation of the StelLoadingBar.
//! It can also have an image which is set at object creation time, and some
//! static text (called extra text, which is set at object creation time.
//! Stellarium's splash screen, visible when the program is first started, is
//! a StelLoadingBar.  The image is the logo for the project, the extra text is
//! used to show the application name and version. 
class StelLoadingBar
{
public:
	//! Create and initialise the StelLoadingBar.
	//! @param splashTex the file name of a texture to display with the StelLoadingBar.
	//! @param extraTextString extra text which does not change during the life.
	//! life of the StelLoadingBar. This is used for the application name and version
	//! when the StelLoadingBar is used as a splash screen.
	//! @param extraTextSize the size of the font used for the exta text.
	//! @param extraTextPosx the x position of the extra text.
	//! @param extraTextPosy the y position of the extra text.
	//! @param width the width to use for the texture display.
	//! @param height the height to use for the texture display.
	StelLoadingBar(const QString&  splashTex, const QString& extraTextString="",
			   float extraTextSize = 30.f, float extraTextPosx = 0.f, float extraTextPosy = 0.f,
			   int width=512, int height=512);

	virtual ~StelLoadingBar();
	
	//! Draw the splashscreen.
	void draw(class StelRenderer* renderer);

private:
	QString message;
	int splashx, splashy, width, height;
	QFont extraTextFont;
	class StelTextureNew* splash;
	QString extraText;
	QString splashName;
	Vec4i viewportXywh;
	Vec2f extraTextPos;
};

#endif // _STELLOADINGBAR_HPP_
