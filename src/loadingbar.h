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

#include "projector.h"
#include "s_font.h"

// Class which is used to display loading bar
class LoadingBar
{
public:
	// Create and initialise
	LoadingBar(Projector* prj, float font_size, const string&  splash_tex, 
		int screenw, int screenh, const wstring& extraTextString=L"", float extraTextSize = 30.f, 
		float extraTextPosx = 0.f, float extraTextPosy = 0.f);
	virtual ~LoadingBar();
	void SetMessage(wstring m) {message=m;}
	void Draw(float val);
private:
	wstring message;
	Projector* prj;
	int splashx, splashy, barx, bary, width, height, barwidth, barheight;
	s_font& barfont;
	s_font& extraTextFont;
	s_texture* splash;
	wstring extraText;
	Vec2f extraTextPos;
	int timeCounter;
};

#endif //_LOADINGBAR_H_
