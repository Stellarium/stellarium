/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#define TUI_SCRIPT_MSG "Select and exit to run."

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
	void gui_update_widgets(int delta_time);		// Update changing values

	void draw_gravity_ui(void);	// Draw simple gravity text ui.

	// Handle mouse clics
	int handle_clic(Uint16 x, Uint16 y, S_GUI_VALUE button, S_GUI_VALUE state);
	// Handle mouse move
	int handle_move(int x, int y);
	// Handle key press and release
	int handle_keys(Uint16 key, S_GUI_VALUE state);

	// Text UI
	void init_tui(void);
	void draw_tui(void);		// Display the tui
	int handle_keys_tui(Uint16 key, s_tui::S_TUI_VALUE state);
	// Update all the tui widgets with values taken from the core parameters
	void tui_update_widgets(void);
	void show_message(string _message, int _time_out=0);
    void gotoObject(void);
    void setConstellationAutoComplete(vector<string> _autoComplete ) { constellation_edit->setAutoCompleteOptions(_autoComplete);}
    void setPlanetAutoComplete(vector<string> _autoComplete ) { planet_edit->setAutoCompleteOptions(_autoComplete);}
    void setStarAutoComplete(vector<string> _autoComplete ) { star_edit->setAutoCompleteOptions(_autoComplete);}
    void setListNames(vector<string> _names ) { listBox->addItems(_names);}
    void setTitleObservatoryName(const string& name);
    string getTitleWithAltitude(void);
    bool isInitialised(void) { return initialised; }
    void changeLocale(void) { desktop->changeLocale(); }
private:
	stel_core * core;		// The main core can be accessed because stel_ui is a friend class
	bool initialised;
	s_font * baseFont;		// The standard font
	s_font * courierFont;	// The standard fixed size font
	s_texture * baseTex;	// The standard fill texture
	s_texture * flipBaseTex;	// The standard fill texture
	s_texture * tex_up;		// Up arrow texture
	s_texture * tex_down;	// Down arrow texture

	Container * desktop;	// The container which contains everything
	bool opaqueGUI;
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
	FlagButton * bt_flag_constellation_art;
	FlagButton * bt_flag_azimuth_grid;
	FlagButton * bt_flag_equator_grid;
	FlagButton * bt_flag_ground;
	FlagButton * bt_flag_cardinals;
	FlagButton * bt_flag_atmosphere;
	FlagButton * bt_flag_nebula_name;
	FlagButton * bt_flag_help;
	FlagButton * bt_flag_equatorial_mode;
	FlagButton * bt_flag_config;
	FlagButton * bt_flag_night;
	FlagButton * bt_flag_search;
	EditBox * bt_script;
	FlagButton * bt_flag_goto;
	FlagButton * bt_flag_quit;

    void cbEditScriptInOut(void);
    void cbEditScriptPress(void);
    void cbEditScriptExecute(void);
    void cbEditScriptKey(void);
    void cbEditScriptWordCount(void);

	Component* createFlagButtons(void);
	void cb(void);
	void bt_flag_ctrOnMouseInOut(void);
	void cbr(void);

	// Tile control buttons
	FilledContainer * bt_time_control_ctr;
	LabeledButton * bt_dec_time_speed;
	LabeledButton * bt_real_time_speed;
	LabeledButton * bt_inc_time_speed;
	LabeledButton * bt_time_now;
	Component* createTimeControlButtons(void);
	void bt_time_control_ctrOnMouseInOut(void);
	void bt_dec_time_speed_cb(void);
	void bt_real_time_speed_cb(void);
	void bt_inc_time_speed_cb(void);
	void bt_time_now_cb(void);
	void tcbr(void);
		
	// The dynamic information about the button under the mouse
	Label * bt_flag_help_lbl;
	Label * bt_flag_time_control_lbl;

	// The TextLabel displaying the infos about the selected object
	Container * info_select_ctr;
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

	// window for transient messages
	StdTransBtWin * message_win;
	TextLabel * message_txtlbl;

	// The window managing the configuration
	StdBtWin* config_win;
	Component* createConfigWindow(void);
	void config_win_hideBtCallback(void);

	TabContainer * config_tab_ctr;

	void load_cities(const string & fileName);
	vector<City*> cities;

	// The window managing the search - Tony
	StdBtWin* search_win;
	TabContainer * search_tab_ctr;
	Component* createSearchWindow(void);
	void search_win_hideBtCallback(void);
    void showSearchMessage(string _error); 
    void hideSearchMessage(void); 
    void doSearchCommand(string _command, string _error);

	// standard dialogs
	StdDlgWin* dialog_win;
	void dialogCallback(void);
	
	// Rendering options
	LabeledCheckBox* stars_cbx;
	LabeledCheckBox* star_names_cbx;
	FloatIncDec* max_mag_star_name;
	LabeledCheckBox* star_twinkle_cbx;
	FloatIncDec* star_twinkle_amount;
	LabeledCheckBox* constellation_cbx;
	LabeledCheckBox* constellation_name_cbx;
	LabeledCheckBox* sel_constellation_cbx;
	LabeledCheckBox* nebulas_names_cbx;
	FloatIncDec* max_mag_nebula_name;
	LabeledCheckBox* planets_cbx;
	LabeledCheckBox* planets_hints_cbx;
	LabeledCheckBox* moon_x4_cbx;
	LabeledCheckBox* equator_grid_cbx;
	LabeledCheckBox* azimuth_grid_cbx;
	LabeledCheckBox* equator_cbx;
	LabeledCheckBox* ecliptic_cbx;
	LabeledCheckBox* ground_cbx;
	LabeledCheckBox* cardinal_cbx;
	LabeledCheckBox* atmosphere_cbx;
	LabeledCheckBox* fog_cbx;
	void saveRenderOptions(void);

	// Location options
	bool waitOnLocation;
	MapPicture* earth_map;
	Label *lblMapLocation;
	Label *lblMapPointer;
	FloatIncDec* lat_incdec, * long_incdec;
	IntIncDec* alt_incdec;
	void setObserverPositionFromMap(void);
	void setCityFromMap(void);
	void setObserverPositionFromIncDec(void);
	void doSaveObserverPosition(const string &name);
	void saveObserverPosition(void); // callback to the MapPicture change

	// Date & Time options
	Time_item* time_current;
	LabeledCheckBox* system_tz_cbx;
	void setCurrentTimeFromConfig(void);

	Time_zone_item* tzselector;
	Label* system_tz_lbl2;
	Label* time_speed_lbl2;
	void setTimeZone(void);

	// Video Options
	LabeledCheckBox* fisheye_projection_cbx;
	LabeledCheckBox* disk_viewport_cbx;
	StringList* screen_size_sl;
	void setVideoOption(void);
	void updateVideoVariables(void);

	void updateConfigVariables(void);
	void updateConfigVariables2(void);
	void updateConfigForm(void);

	bool LocaleChanged;  // flag to watch for need to rebuild TUI

	// Landscape options
	StringList* landscape_sl;
	void setLandscape(void);
	void saveLandscapeOption(void);

	// Mouse control options
	bool is_dragging, has_dragged;
	int previous_x, previous_y;
	
    // Search options
    EditBox* nebula_edit;
    void doNebulaSearch(void);
    EditBox* star_edit;
    void showStarAutoComplete(void);
    void doStarSearch(void);
    EditBox* constellation_edit;
    void doConstellationSearch(void);
    void showConstellationAutoComplete(void);
    EditBox* planet_edit;
    void doPlanetSearch(void);
    void showPlanetAutoComplete(void);
    Label *lblSearchMessage;
	
	// Example tab of widgets
	ListBox *listBox;
	void msgbox1(void);
	void msgbox2(void);
	void msgbox3(void);
	void inputbox1(void);
	void listBoxChanged(void);
	
	////////////////////////////////////////////////////////////////////////////
	// Text UI components
	s_tui::Branch* tui_root;

	// 1. Location
	s_tui::Decimal_item* tui_location_latitude;
	s_tui::Decimal_item* tui_location_longitude;
	s_tui::Integer_item* tui_location_altitude;

	// 2. Time & Date
	s_tui::Time_zone_item* tui_time_settmz;
	s_tui::Time_item* tui_time_skytime;
	s_tui::Time_item* tui_time_presetskytime;
	s_tui::MultiSet_item<string>* tui_time_startuptime;
	s_tui::MultiSet_item<string>* tui_time_displayformat;
	s_tui::MultiSet_item<string>* tui_time_dateformat;

	// 3. General
	s_tui::MultiSet_item<string>* tui_general_sky_culture;
	s_tui::MultiSet2_item<string>* tui_general_sky_locale;

	// 4. Stars
	s_tui::Boolean_item* tui_stars_show;
	s_tui::Decimal_item* tui_star_labelmaxmag;
	s_tui::Decimal_item* tui_stars_twinkle;
	s_tui::Decimal_item* tui_star_magscale;
	
	// 5. Effects
	s_tui::MultiSet_item<string>* tui_effect_landscape;
	s_tui::Boolean_item* tui_effect_pointobj;
	s_tui::Decimal_item* tui_effect_object_scale;
	s_tui::Decimal_item* tui_effect_zoom_duration;
	s_tui::Decimal_item* tui_effect_milkyway_intensity;
	s_tui::Decimal_item* tui_effect_nebulae_label_magnitude;
	s_tui::Boolean_item* tui_effect_manual_zoom;
	s_tui::Decimal_item* tui_effect_cursor_timeout;

	// 6. Scripts
	s_tui::MultiSet_item<string>* tui_scripts_local;
	s_tui::MultiSet_item<string>* tui_scripts_removeable;
	bool flag_scripts_removeable_disk_mounted;  // is the removeable disk for scripts mounted?

	// 7. Administration
	s_tui::ActionConfirm_item* tui_admin_loaddefault;
	s_tui::ActionConfirm_item* tui_admin_savedefault;
	s_tui::Action_item* tui_admin_updateme;
	s_tui::MultiSet_item<string>* tui_admin_setlocale;
	s_tui::Integer_item* tui_admin_voffset;
	s_tui::Integer_item* tui_admin_hoffset;

	// Tui Callbacks
	void tui_cb1(void);						// Update all the core flags and params from the tui
	void tui_cb_settimezone(void);			// Set time zone
	void tui_cb_settimedisplayformat(void);	// Set 12/24h format
	void tui_cb_admin_load_default(void);	// Load default configuration
	void tui_cb_admin_save_default(void);	// Save default configuration
	void tui_cb_admin_set_locale(void);		// Set locale for UI (not sky)
	void tui_cb_admin_updateme(void);		// Launch script for internet update
	void tui_cb_tui_effect_change_landscape(void);	// Select a new landscape skin
	void tui_cb_tui_general_change_sky_culture(void);  // select new sky culture
	void tui_cb_tui_general_change_sky_locale(void);  // select new sky locale
	void tui_cb_tui_admin_change_viewport(void);    // Set viewport offset
	void tui_cb_scripts_removeable(void);    // changed removeable disk script selection
	void tui_cb_scripts_local();             // changed local script selection
	void tui_cb_effects_milkyway_intensity();        // change milky way intensity
	void tui_cb_effects_nebulae_label_magnitude();    // change nebula label limiting magnitude
	void tui_cb_setlocation();        // change observer position
	void tui_cb_stars();        // change star parameters
	void tui_cb_effects();        // change effect parameters
	void tui_cb_sky_time();        // change effect parameters

	// Parse a file of type /usr/share/zoneinfo/zone.tab
	s_tui::MultiSet_item<string>* stel_ui::create_tree_from_time_zone_file(const string& zonetab);

	bool ScriptDirectoryRead;
	double MouseTimeLeft;  // for cursor timeout (seconds)
};

#endif  //_STEL_UI_H
