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
#include "s_tui.h"

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

	void draw_gravity_ui(void);	// Draw simple gravity text ui.

	// Handle mouse clics
	int handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);
	// Handle mouse move
	int handle_move(int x, int y);
	// Handle key press and release
	int handle_keys(SDLKey key, S_GUI_VALUE state);

	// Text UI
	void init_tui(void);
	void draw_tui(void);		// Display the tui
	int handle_keys_tui(SDLKey key, s_tui::S_TUI_VALUE state);
	// Update all the tui widgets with values taken from the core parameters
	void tui_update_widgets(void);

private:
	stel_core * core;		// The main core can be access because stel_ui is a friend class

	s_font * spaceFont;		// The standard font
	s_font * courierFont;	// The standard fixed size font
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
	FlagButton * bt_flag_constellation_draw;
	FlagButton * bt_flag_constellation_name;
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
	void cb(void);
	void bt_flag_ctrOnMouseInOut(void);
	void cbr(void);
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

	// The window containing the help info
	StdBtWin * help_win;
	TextLabel * help_txtlbl;
	Component* createHelpWindow(void);
	void help_win_hideBtCallback(void);

	// The window managing the configuration
	StdBtWin* config_win;
	Component* createConfigWindow(void);
	void config_win_hideBtCallback(void);

	TabContainer * config_tab_ctr;

	void updateConfigVariables(void);
	void updateConfigForm(void);

	////////////////////////////////////////////////////////////////////////////
	// Text UI components
	s_tui::Branch* tui_root;

	// 1. Location
	s_tui::Decimal_item* tui_location_latitude;
	s_tui::Decimal_item* tui_location_longitude;
	s_tui::Integer_item* tui_location_altitude;

	// 2. Time & Date
	s_tui::MultiSet_item<string>* tui_time_settmz;
	s_tui::Time_item2* tui_time_skytime;
	s_tui::Time_item* tui_time_presetskytime;
	s_tui::Time_item* tui_time_actual;
	s_tui::MultiSet_item<string>* tui_time_startuptime;
	s_tui::MultiSet_item<string>* tui_time_displayformat;

	// 3. Stars
	s_tui::Boolean_item* tui_stars_show;
	s_tui::Decimal_item* tui_star_labelmaxmag;
	s_tui::Boolean_item* tui_stars_twinkle;

	// 4. Effect
	s_tui::MultiSet_item<string>* tui_effect_landscape;

	// 5. Administration
	s_tui::ActionConfirm_item* tui_admin_loaddefault;
	s_tui::ActionConfirm_item* tui_admin_savedefault;
	s_tui::MultiSet_item<string>* tui_admin_setlocal;
	s_tui::Action_item* tui_admin_updateme;

	// Tui Callbacks
	void tui_cb1(void);						// Update all the core flags and params from the tui
	void tui_cb_settimezone(void);			// Set time zone
	void tui_cb_actualtime(void);			// Set sky time as current
	void tui_cb_settimedisplayformat(void);	// Set 12/24h format
	void tui_cb_admin_load_default(void);	// Load default configuration
	void tui_cb_admin_save_default(void);	// Save default configuration
	void tui_cb_admin_set_locale(void);		// Set locale parameter (LANG)
	void tui_cb_admin_updateme(void);		// Launch script for internet update
	void tui_cb_tui_effect_change_landscape(void);	// Select a new landscape skin

	// Parse a file of type /usr/share/zoneinfo/zone.tab
	s_tui::MultiSet_item<string>* stel_ui::create_tree_from_time_zone_file(const string& zonetab);
};

#endif  //_STEL_UI_H
