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

#ifndef _LOADINGBAR_H_
#define _LOADINGBAR_H_

#include "STextureTypes.hpp"
#include "vecmath.h"

#include <string>

using std::string;
using std::wstring;

class SFont;
class Projector;

//! @class LoadingBar 
//! This class is used to display loading bar.
//! A LoadingBar has a progress bar whose value can be set after the creation
//! of the LoadingBar, and a text display area, whose contents may also be
//! modified after the creation of the LoadingBar.
//! It can also have an image which is set at object creation time, and some
//! static text (called extra text, which is set at object creation time.
//! Stellarium's splash screen, visible when the program is first started, is
//! a LoadingBar.  The image is the logo for the project, the extra text is
//! used to show the application name and version. 
class LoadingBar
{
public:
	//! Create and initialise the LoadingBar.
	//! @param prj the projector used to display the LoadingBar
	//! @param font_size the size of the font to use in the LoadingBar
	//! @param splash_tex the file name of a texture to display with the LoadingBar
	//! @param screenw the screen width
	//! @param screenh the screen height
	//! @param extraTextString extra text which does not change during the life
	//! life of the LoadingBar. This is used for the application name and version
	//! when the LoadingBar is used as a splash screen.
	//! @param extraTextSize the size of the font used for the exta text
	//! @param extraTextPosx the x position of the extra text
	//! @param extraTextPosy the y position of the extra text
	LoadingBar(Projector* prj, float font_size, const string&  splash_tex, 
		int screenw, int screenh, const wstring& extraTextString=L"", float extraTextSize = 30.f, 
		float extraTextPosx = 0.f, float extraTextPosy = 0.f);
	virtual ~LoadingBar();
	
	//! Set the message for the loading bar.
	//! @param m a wstring message to display under the loading bar
	void SetMessage(wstring m) {message=m;}
	
	//! Draw the LoadingBar, setting the value.
	//! @param val the value which the progress bar should display. This is a 
	//! float which should take a value between 0.0 and 1.0.
	void Draw(float val);

	//! Sets the value of clearBuffer flag.
	//! If the clearBuffer flag is set to true, glClear is called 
	//! at the end of each Draw() operation, else it is not.  This
	//! can be useful to prevent flickering when only the LoadingBar
	//! screen is visible.
	//! The default value of the clearBuffer flag is false.
	void setClearBuffer(bool b) {clearBuffer=b;}
	
private:
	wstring message;
	Projector* prj;
	int splashx, splashy, barx, bary, width, height, barwidth, barheight;
	SFont& barfont;
	SFont& extraTextFont;
	STextureSP splash;
	wstring extraText;
	Vec2f extraTextPos;
	double timeCounter;
	bool clearBuffer;
};

#endif //_LOADINGBAR_H_
