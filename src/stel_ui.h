/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

// Class which handles a stellarium User Interface

#ifndef _STEL_UI_H
#define _STEL_UI_H

#include "stellarium.h"
#include "stel_core.h"

#include "s_font.h"
#include "s_gui.h"
#include "s_gui_window.h"

// Predeclaration of the stel_core class
class stel_core;

class stel_ui
{
public:
	stel_ui(stel_core *);	// Create a stellarium ui. Need to call init() before use
    virtual ~stel_ui();		// Delete the ui

	void init(void);		// Initialize the ui.

	void draw();			// Display the ui

	// TODO : for the following functions, get rid of the SDL macro def and types
	// need the creation of an interface between s_gui and SDL

	// Handle mouse clics
	void handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);

	// Handle mouse move
	void handle_move(int x, int y);

	// Handle key press and release
	bool handle_keys(SDLKey key, int state);

private:
	stel_core * core;

	s_font * spaceFont;		// The font used in the GraphicContext (=Skin)
	GraphicsContext * gc;	// The graphic context used to give the widget their drawing style
	Container * Base;		// The container which contains everything (=the desktop)

	void updateStandardWidgets(void);
	void updateInfoSelectString(void);

	// Flags buttons (the buttons in the bottom left corner)
	FilledContainer * ContainerBtFlags;		// The containers for the button
	Textured_Button * BtConstellationsDraw;
	Textured_Button * BtConstellationsName;
	Textured_Button * BtAzimutalGrid;
	Textured_Button * BtEquatorialGrid;
	Textured_Button * BtGround;
	Textured_Button * BtCardinalPoints;
	Textured_Button * BtAtmosphere;
	Textured_Button * BtNebula;
	Textured_Button * BtHelp;
	Textured_Button * BtFollowEarth;
	Textured_Button * BtConfig;
	Label * btLegend;	// The dynamic information about the button under the mouse

	// The TextLabel displaying the infos about the selected object
	FilledTextLabel * InfoSelectLabel;

	// The top bar containing the main infos (date, time, fps etc...)
	FilledContainer * TopWindowsInfos;
	Label * DateLabel;
	Label * HourLabel;
	Label * FPSLabel;
	Label * AppNameLabel;
	Label * FOVLabel;

	// The window containing the help info
	StdBtWin * HelpWin;
	TextLabel * HelpTextLabel;

	// The window containing the info (licence)
	StdBtWin * InfoWin;
	TextLabel * InfoTextLabel;

	// The window containing the time controls
	StdBtWin * TimeControlWin;
	Container * TimeControlContainer;
	Labeled_Button * TimeRW;
	Labeled_Button * TimeFRW;
	Labeled_Button * TimeF;
	Labeled_Button * TimeNow;
	Labeled_Button * TimeFF;
	Labeled_Button * TimeReal;
	Labeled_Button * TimePause;


	// Configuration window for stellarium
	// All the function related with the configuration window are in
	// the file stel_ui_conf.cpp

	// The window containing the configuration options
	StdBtWin * ConfigWin;

	// Star configuration subsection
	FilledContainer * StarConfigContainer;
	Labeled_Button * ToggleStarName;
	Label * StarLabel;
	Label * StarNameLabel;
	Label * StarNameMagLabel;
	CursorBar * ChangeStarDrawNameBar;	// Cursor for the star name/magnitude
	Label * StarTwinkleLabel;
	CursorBar * ChangeStarTwinkleBar;	// Cursor for the star name/magnitude
	Label * StarScaleLabel;
	CursorBar * ChangeStarScaleBar;

	// Landscape configuration subsection
	FilledContainer * LandscapeConfigContainer;
	Label * LandscapeLabel;
	Labeled_Button * ToggleGround;
	Labeled_Button * ToggleFog;
	Labeled_Button * ToggleAtmosphere;
	Labeled_Button * ToggleMilkyWay;

	// Location configuration subsection
	FilledContainer * LocationConfigContainer;
	Label * LocationLabel;
	CursorBar * LatitudeBar;
	CursorBar * LongitudeBar;
	CursorBar * AltitudeBar;
	CursorBar * TimeZoneBar;
	Label * LatitudeLabel;
	Label * LongitudeLabel;
	Label * AltitudeLabel;
	Label * TimeZoneLabel;
	Labeled_Button * SaveLocation;
	ClickablePicture * EarthMap;

	// CALLBACKS
	void BtFlagsOnClicCallBack(guiValue button,Component * bt);
	void BtFlagsOnMouseOverCallBack(guiValue event,Component * bt);
	void HelpWinHideCallback(void);
	void InfoWinHideCallback(void);
	void TimeControlWinHideCallback(void);
	void TimeControlBtOnClicCallback(guiValue button,Component * caller);

};

#endif  //_STEL_UI_H
