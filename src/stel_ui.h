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
// TODO : get rid of the SDL macro def and types
// need the creation of an interface between s_gui and SDL

#ifndef _STEL_UI_H
#define _STEL_UI_H

#include "stellarium.h"
#include "stel_core.h"
#include "s_gui.h"

// Predeclaration of the stel_core class
class stel_core;

using namespace std;
using namespace s_gui;

class stel_ui
{
public:
	stel_ui(stel_core *);	// Create a stellarium ui. Need to call init() before use
    virtual ~stel_ui();		// Delete the ui
	void init(void);		// Initialize the ui.
	void draw(void);		// Display the ui
	void update(void);		// Update changing values

	// Handle mouse clics
	int handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);
	// Handle mouse move
	int handle_move(int x, int y);
	// Handle key press and release
	int handle_keys(SDLKey key, S_GUI_VALUE state);

private:
	stel_core * core;

	s_font * spaceFont;		// The standard font
	s_texture * baseTex;	// The standard fill texture

	Container * desktop;	// The container which contains everything

	// The top bar containing the main infos (date, time, fps etc...)
	FilledContainer * top_bar_ctr;
	Label * top_bar_date_lbl;
	Label * top_bar_hour_lbl;
	Label * top_bar_fps_lbl;
	Label * top_bar_appName_lbl;
	Label * top_bar_fov_lbl;
	Component* createTopBar(void);
	void updateTopBar(void);

	// Flags buttons (the buttons in the bottom left corner)
	FilledContainer * bt_flag_ctr;		// The container for the button
	FlagButton * bt_flag_asterism_draw;
	FlagButton * bt_flag_asterism_name;
	FlagButton * bt_flag_azimuth_grid;
	FlagButton * bt_flag_equator_grid;
	FlagButton * bt_flag_ground;
	FlagButton * bt_flag_cardinals;
	FlagButton * bt_flag_atmosphere;
	FlagButton * bt_flag_nebula_name;
	FlagButton * bt_flag_help;
	FlagButton * bt_flag_follow_earth;
	FlagButton * bt_flag_config;
	Component* createFlagButtons(void);
	void cb1(void);	void cb2(void);	void cb3(void);	void cb4(void);
	void cb5(void);	void cb6(void);	void cb7(void);	void cb8(void);
	void cb9(void);	void cb10(void);void cb11(void);
	void bt_flag_ctrOnMouseInOut(void);
	void cbr1(void);	void cbr2(void);	void cbr3(void);	void cbr4(void);
	void cbr5(void);	void cbr6(void);	void cbr7(void);	void cbr8(void);
	void cbr9(void);	void cbr10(void);void cbr11(void);
	// The dynamic information about the button under the mouse
	Label * bt_flag_help_lbl;

	// The TextLabel displaying the infos about the selected object
	FilledContainer * info_select_ctr;
	TextLabel * info_select_txtlbl;
	void updateInfoSelectString(void);

	// The window containing the info (licence)
	StdBtWin * licence_win;
	TextLabel * licence_txtlbl;
	Component* createLicenceWindow(void);

};

#endif  //_STEL_UI_H
