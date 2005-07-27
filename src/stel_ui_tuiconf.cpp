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

// Class which handles a stellarium User Interface in Text mode

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include "stel_ui.h"
#include "stellastro.h"
#include "sky_localizer.h"

// Draw simple gravity text ui.
void stel_ui::draw_gravity_ui(void)
{
	// Normal transparency mode
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	int x = core->projection->view_left() + core->projection->viewW()/2;
	int y = core->projection->view_bottom() + core->projection->viewH()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(core->projection->viewW()/2, core->projection->viewH()/2));

	if (core->FlagShowTuiDateTime)
	{
		double jd = core->navigation->get_JDay();
		ostringstream os;

		if (core->FlagUTC_Time)
		{
			os << core->observatory->get_printable_date_UTC(jd) << " " <<
			core->observatory->get_printable_time_UTC(jd) << " (UTC)";
		}
		else
		{
			os << core->observatory->get_printable_date_local(jd) << " " <<
			core->observatory->get_printable_time_local(jd);
		}

		if (core->FlagShowFov) os << " fov " << setprecision(3) << core->projection->get_visible_fov();
		if (core->FlagShowFps) os << "  FPS " << core->fps;

		glColor3f(0.5,1,0.5);
		core->projection->print_gravity180(spaceFont, x-shift + 30, y-shift + 38, os.str(), 0);
	}

	if (core->selected_object && core->FlagShowTuiShortObjInfo)
	{
	    static char str[255];	// TODO use c++ string for get_short_info_string() func
		core->selected_object->get_short_info_string(str, core->navigation);
		if (core->selected_object->get_type()==STEL_OBJECT_NEBULA) glColor3fv(core->NebulaLabelColor);
		if (core->selected_object->get_type()==STEL_OBJECT_PLANET) glColor3fv(core->PlanetNamesColor);
		if (core->selected_object->get_type()==STEL_OBJECT_STAR) glColor3fv(core->selected_object->get_RGB());
		core->projection->print_gravity180(spaceFont, x+shift - 30, y+shift - 38, str, 0);
	}
}

// Create all the components of the text user interface
void stel_ui::init_tui(void)
{
	// Menu root branch
	LocaleChanged=0;
	tui_root = new s_tui::Branch();

	// Submenus
	s_tui::MenuBranch* tui_menu_location = new s_tui::MenuBranch(string("1. ") + _("Set Location "));
	s_tui::MenuBranch* tui_menu_time = new s_tui::MenuBranch(string("2. ") + _("Set Time "));
	s_tui::MenuBranch* tui_menu_general = new s_tui::MenuBranch(string("3. ") + _("General "));
	s_tui::MenuBranch* tui_menu_stars = new s_tui::MenuBranch(string("4. ") + _("Stars "));
	s_tui::MenuBranch* tui_menu_effects = new s_tui::MenuBranch(string("5. ") + _("Effects "));
	s_tui::MenuBranch* tui_menu_scripts = new s_tui::MenuBranch(string("6. ") + _("Scripts "));
	s_tui::MenuBranch* tui_menu_administration = new s_tui::MenuBranch(string("7. ") + _("Administration "));
	tui_root->addComponent(tui_menu_location);
	tui_root->addComponent(tui_menu_time);
	tui_root->addComponent(tui_menu_general);	
	tui_root->addComponent(tui_menu_stars);
	tui_root->addComponent(tui_menu_effects);
	tui_root->addComponent(tui_menu_scripts);
	tui_root->addComponent(tui_menu_administration);

	// 1. Location
	tui_location_latitude = new s_tui::Decimal_item(-90., 90., 0.,string("1.1 ") + _("Latitude: "));
	tui_location_latitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_setlocation));
	tui_location_longitude = new s_tui::Decimal_item(-180., 180., 0.,string("1.2 ") + _("Longitude: "));
	tui_location_longitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_setlocation));
	tui_location_altitude = new s_tui::Integer_item(-500, 10000, 0,string("1.3 ") + _("Altitude (m): "));
	tui_location_altitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_setlocation));
	tui_menu_location->addComponent(tui_location_latitude);
	tui_menu_location->addComponent(tui_location_longitude);
	tui_menu_location->addComponent(tui_location_altitude);

	// 2. Time
	tui_time_skytime = new s_tui::Time_item(string("2.1 ") + _("Sky Time: "));
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_sky_time));
	tui_time_settmz = new s_tui::Time_zone_item(core->DataDir + "zone.tab", string("2.2 ") + _("Set Time Zone: "));
	tui_time_settmz->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimezone));
	tui_time_settmz->settz(core->observatory->get_custom_tz_name());
	tui_time_presetskytime = new s_tui::Time_item(string("2.3 ") + _("Preset Sky Time: "));
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_startuptime = new s_tui::MultiSet_item<string>(string("2.4 ") + _("Sky Time At Start-up: "));
	tui_time_startuptime->addItem("Actual");
	tui_time_startuptime->addItem("Preset");
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_displayformat = new s_tui::MultiSet_item<string>(string("2.5 ") + _("Time Display Format: "));
	tui_time_displayformat->addItem("24h");
	tui_time_displayformat->addItem("12h");
	tui_time_displayformat->addItem("system_default");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimedisplayformat));
	tui_time_dateformat = new s_tui::MultiSet_item<string>(string("2.6 ") + _("Date Display Format: "));
	tui_time_dateformat->addItem("yyyymmdd");
	tui_time_dateformat->addItem("ddmmyyyy");
	tui_time_dateformat->addItem("mmddyyyy");
	tui_time_dateformat->addItem("system_default");
	tui_time_dateformat->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimedisplayformat));

	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);
	tui_menu_time->addComponent(tui_time_dateformat);

	// 3. General settings

	// sky culture goes here
	tui_general_sky_culture = new s_tui::MultiSet_item<string>(string("3.1 ") + _("Sky Culture: "));
	tui_general_sky_culture->addItemList(core->skyloc->get_sky_culture_list());  // human readable names
	tui_general_sky_culture->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_general_change_sky_culture));
	tui_menu_general->addComponent(tui_general_sky_culture);

	tui_general_sky_locale = new s_tui::MultiSet2_item<string>(string("3.2 ") + _("Sky Language: "));
	tui_general_sky_locale->addItemList(core->skyloc->get_sky_locale_list());

	tui_general_sky_locale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_general_change_sky_locale));
	tui_menu_general->addComponent(tui_general_sky_locale);


	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, string("4.1 ") + _("Show: "), _("Yes"),_("No"));
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_stars));
	tui_star_magscale = new s_tui::Decimal_item(1,30, 1, string("4.2 ") + _("Star Magnitude Multiplier: "));
	tui_star_magscale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_stars));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, string("4.3 ") + _("Maximum Magnitude to Label: "));
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_stars));
	tui_stars_twinkle = new s_tui::Decimal_item(0., 1., 0.3, string("4.4 ") + _("Twinkling: "), 0.1);
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_stars));

	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_magscale);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);


	// 5. Effects
	tui_effect_landscape = new s_tui::MultiSet_item<string>(string("5.1 ") + _("Landscape: "));
	tui_effect_landscape->addItemList(Landscape::get_file_content(core->DataDir + "landscapes.ini"));

	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);

	tui_effect_manual_zoom = new s_tui::Boolean_item(false, string("5.2 ") + _("Manual zoom: "), _("Yes"),_("No"));
	tui_effect_manual_zoom->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_manual_zoom);

	tui_effect_pointobj = new s_tui::Boolean_item(false, string("5.3 ") + _("Object Sizing Rule: "), _("Point"),_("Magnitude"));
	tui_effect_pointobj->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_pointobj);

	tui_effect_object_scale = new s_tui::Decimal_item(0, 25, 1, string("5.4 ") + _("Magnitude Sizing Multiplier: "), 0.1);
	tui_effect_object_scale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_object_scale);

	tui_effect_milkyway_intensity = new s_tui::Decimal_item(0, 10, 1, string("5.5 ") + _("Milky Way intensity: "), .5);
	tui_effect_milkyway_intensity->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_effects_milkyway_intensity));
	tui_menu_effects->addComponent(tui_effect_milkyway_intensity);

	tui_effect_zoom_duration = new s_tui::Decimal_item(1, 10, 2, string("5.6 ") + _("Zoom duration: "));
	tui_effect_zoom_duration->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_zoom_duration);


	// 6. Scripts
	tui_scripts_local = new s_tui::MultiSet_item<string>(string("6.1 ") + _("Local Script: "));
	tui_scripts_local->addItemList(TUI_SCRIPT_MSG + string("\n") 
				       + core->scripts->get_script_list(core->DataDir + "scripts/")); 
	tui_scripts_local->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_scripts_local));
	tui_menu_scripts->addComponent(tui_scripts_local);

	tui_scripts_removeable = new s_tui::MultiSet_item<string>(string("6.2 ") + _("CD/DVD Script: "));
	//	tui_scripts_removeable->addItem("Arrow down to load list.");
	tui_scripts_removeable->addItem(TUI_SCRIPT_MSG);
	tui_scripts_removeable->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_scripts_removeable));
	tui_menu_scripts->addComponent(tui_scripts_removeable);


	// 7. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item(string("7.1 ") + _("Load Default Configuration: "));
	tui_admin_loaddefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_load_default));
	tui_admin_savedefault = new s_tui::ActionConfirm_item(string("7.2 ") + _("Save Current Configuration as Default: "));
	tui_admin_savedefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_save_default));
	tui_admin_updateme = new s_tui::Action_item(string("7.3 ") + _("Update me via Internet: "));
	tui_admin_updateme->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_updateme));
	tui_menu_administration->addComponent(tui_admin_loaddefault);
	tui_menu_administration->addComponent(tui_admin_savedefault);
	tui_menu_administration->addComponent(tui_admin_updateme);

	tui_admin_setlocale = new s_tui::MultiSet_item<string>("7.3 Set Locale: ");
	// Should be defined elsewhere...
	tui_admin_setlocale->addItem("en_US");
	tui_admin_setlocale->addItem("fr_FR");
	tui_admin_setlocale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_set_locale));
	tui_menu_administration->addComponent(tui_admin_setlocale);


	tui_admin_voffset = new s_tui::Integer_item(-10,10,0, string("7.4 ") + _("N-S Centering Offset: "));
	tui_admin_voffset->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_admin_change_viewport));
	tui_menu_administration->addComponent(tui_admin_voffset);

	tui_admin_hoffset = new s_tui::Integer_item(-10,10,0, string("7.5 ") + _("E-W Centering Offset: "));
	tui_admin_hoffset->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_admin_change_viewport));
	tui_menu_administration->addComponent(tui_admin_hoffset);


}

// Display the tui
void stel_ui::draw_tui(void)
{

	// not sure this is best location...
	// If locale has changed, rebuild TUI with new translated text
	if(LocaleChanged) {
		cout << "Reloading TUI due to locale change\n";
		if(tui_root) delete tui_root;
		init_tui();

		// THIS IS A HACK, but lacking time for a better solution:
		// go back to the locale menu, where user left off
		handle_keys_tui(SDLK_UP, s_tui::S_TUI_PRESSED);
		handle_keys_tui(SDLK_UP, s_tui::S_TUI_RELEASED);
		handle_keys_tui(SDLK_RIGHT, s_tui::S_TUI_PRESSED);
		handle_keys_tui(SDLK_RIGHT, s_tui::S_TUI_RELEASED);
		handle_keys_tui(SDLK_DOWN, s_tui::S_TUI_PRESSED);
		handle_keys_tui(SDLK_DOWN, s_tui::S_TUI_RELEASED);
		handle_keys_tui(SDLK_DOWN, s_tui::S_TUI_PRESSED);
		handle_keys_tui(SDLK_DOWN, s_tui::S_TUI_RELEASED);
		handle_keys_tui(SDLK_DOWN, s_tui::S_TUI_PRESSED);
		handle_keys_tui(SDLK_DOWN, s_tui::S_TUI_RELEASED);
		handle_keys_tui(SDLK_RIGHT, s_tui::S_TUI_PRESSED);
		handle_keys_tui(SDLK_RIGHT, s_tui::S_TUI_RELEASED);

	}

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	int x = core->projection->view_left() + core->projection->viewW()/2;
	int y = core->projection->view_bottom() + core->projection->viewH()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(core->projection->viewW()/2, core->projection->viewH()/2));

	if (tui_root)
	{
		glColor3f(0.5,1,0.5);
		core->projection->print_gravity180(spaceFont, x+shift - 30, y-shift + 38,
						   s_tui::stop_active + tui_root->getString(), 0);

	}
}

int stel_ui::handle_keys_tui(Uint16 key, s_tui::S_TUI_VALUE state)
{
	return tui_root->onKey(key, state);
}

// Update all the core parameters with values taken from the tui widgets
void stel_ui::tui_cb1(void)
{

	std::ostringstream oss;

	// 2. Date & Time
	core->PresetSkyTime 		= tui_time_presetskytime->getJDay();
	core->StartupTimeMode 		= tui_time_startuptime->getCurrent();

}

// Update all the tui widgets with values taken from the core parameters
void stel_ui::tui_update_widgets(void)
{
	// 1. Location
	tui_location_latitude->setValue(core->observatory->get_latitude());
	tui_location_longitude->setValue(core->observatory->get_longitude());
	tui_location_altitude->setValue(core->observatory->get_altitude());

	// 2. Date & Time
	tui_time_skytime->setJDay(core->navigation->get_JDay() + 
				  core->observatory->get_GMT_shift(core->navigation->get_JDay())*JD_HOUR);
	tui_time_settmz->settz(core->observatory->get_custom_tz_name());
	tui_time_presetskytime->setJDay(core->PresetSkyTime);
	tui_time_startuptime->setCurrent(core->StartupTimeMode);
	tui_time_displayformat->setCurrent(core->observatory->get_time_format_str());
	tui_time_dateformat->setCurrent(core->observatory->get_date_format_str());

	// 3. general
	tui_general_sky_culture->setValue(core->skyloc->convert_directory_to_sky_culture(core->SkyCulture));
	tui_general_sky_locale->setValue(core->SkyLocale);

	// 4. Stars
	tui_stars_show->setValue(core->FlagStars);
	tui_star_labelmaxmag->setValue(core->MaxMagStarName);
	tui_stars_twinkle->setValue(core->StarTwinkleAmount);
	tui_star_magscale->setValue(core->StarMagScale);

	// 5. effects
	tui_effect_landscape->setValue(core->observatory->get_landscape_name());
	tui_effect_pointobj->setValue(core->FlagPointStar);
	tui_effect_zoom_duration->setValue(core->auto_move_duration);
	tui_effect_manual_zoom->setValue(core->FlagManualZoom);
	tui_effect_object_scale->setValue(core->StarScale);
	tui_effect_milkyway_intensity->setValue(core->milky_way->get_intensity());

	// 6. Scripts
	// each fresh time enter needs to reset to select message
	if(core->SelectedScript=="") {
		tui_scripts_local->setCurrent(TUI_SCRIPT_MSG);
		
		if(core->ScriptRemoveableDiskMounted) {
			tui_scripts_removeable->setCurrent(TUI_SCRIPT_MSG);
		} else {
			// no directory mounted, so put up message
			tui_scripts_removeable->replaceItemList("Arrow down to load list.",0);
		}
	}

	// 7. admin
	tui_admin_setlocale->setValue(core->UILocale);
	tui_admin_voffset->setValue(core->verticalOffset);
	tui_admin_hoffset->setValue(core->horizontalOffset);


}

// Launch script to set time zone in the system locales
// TODO : this works only if the system manages the TZ environment
// variables of the form "Europe/Paris". This is not the case on windows
// so everything migth have to be re-done internaly :(
void stel_ui::tui_cb_settimezone(void)
{
	// Don't call the script anymore coz it's pointless
	// system( ( core->DataDir + "script_set_time_zone " + tui_time_settmz->getCurrent() ).c_str() );
	core->observatory->set_custom_tz_name(tui_time_settmz->gettz());
}

// Set time format mode
void stel_ui::tui_cb_settimedisplayformat(void)
{
	core->observatory->set_time_format_str(tui_time_displayformat->getCurrent());
	core->observatory->set_date_format_str(tui_time_dateformat->getCurrent());
}

// 7. Administration actions functions

// Load default configuration
void stel_ui::tui_cb_admin_load_default(void)
{

	core->commander->execute_command("configuration action reload");

}

// Save to default configuration
void stel_ui::tui_cb_admin_save_default(void)
{
	core->save_config();
	core->observatory->save(core->ConfigDir + core->config_file, "init_location");
	system( ( core->DataDir + "script_save_config " ).c_str() );
}

// Launch script for internet update
void stel_ui::tui_cb_admin_updateme(void)
{
	system( ( core->DataDir + "script_internet_update" ).c_str() );
}

// Set a new landscape skin
void stel_ui::tui_cb_tui_effect_change_landscape(void)
{
	//	core->set_landscape(tui_effect_landscape->getCurrent());
	core->commander->execute_command(string("set landscape_name " +  tui_effect_landscape->getCurrent()));
}


// Set a new sky culture
void stel_ui::tui_cb_tui_general_change_sky_culture(void) {

	//  core->set_sky_culture(core->skyloc->convert_sky_culture_to_directory(tui_general_sky_culture->getCurrent()));
	core->commander->execute_command( string("set sky_culture " + 
											 core->skyloc->convert_sky_culture_to_directory(tui_general_sky_culture->getCurrent())));
}

// Set a new sky locale
void stel_ui::tui_cb_tui_general_change_sky_locale(void) {

	//	core->set_sky_locale(tui_general_sky_locale->getCurrent());
	core->commander->execute_command( string("set sky_locale " + tui_general_sky_locale->getCurrent()));
}


// callback for viewport centering
void stel_ui::tui_cb_tui_admin_change_viewport(void)
{

  core->verticalOffset            = tui_admin_voffset->getValue();
  core->horizontalOffset          = tui_admin_hoffset->getValue();


  core->projection->set_viewport_offset( core->horizontalOffset, core->verticalOffset);
  core->projection->set_viewport_type( core->ViewportType );

}

// callback for changing scripts from removeable media
void stel_ui::tui_cb_scripts_removeable() {
  
  if(!core->ScriptRemoveableDiskMounted) {
    // mount disk
	system( ( core->DataDir + "script_mount_script_disk " ).c_str() );	  

    cout << "MOUNT DISK for scripts\n";
    // read scripts from mounted disk
    tui_scripts_removeable->replaceItemList(TUI_SCRIPT_MSG + string("\n") + core->scripts->get_script_list(SCRIPT_REMOVEABLE_DISK), 0);   
    core->ScriptRemoveableDiskMounted = 1;
  } 

  if(tui_scripts_removeable->getCurrent()!=TUI_SCRIPT_MSG){
    core->SelectedScript = tui_scripts_removeable->getCurrent();
    core->SelectedScriptDirectory = SCRIPT_REMOVEABLE_DISK;
    // to avoid confusing user, clear out local script selection as well
    tui_scripts_local->setCurrent(TUI_SCRIPT_MSG);
  } else {
    core->SelectedScript = "";
  }
}


// callback for changing scripts from local directory
void stel_ui::tui_cb_scripts_local() {
  
  if(tui_scripts_local->getCurrent()!=TUI_SCRIPT_MSG){
    core->SelectedScript = tui_scripts_local->getCurrent();
    core->SelectedScriptDirectory = core->DataDir + "scripts/";
    // to reduce confusion for user, clear out removeable script selection as well
    if(core->ScriptRemoveableDiskMounted) tui_scripts_removeable->setCurrent(TUI_SCRIPT_MSG);
  } else {
    core->SelectedScript = "";
  }
}


// change UI locale
void stel_ui::tui_cb_admin_set_locale() {

	// Right now just set for the current session

#ifndef MACOSX
	string tmp = string("LC_ALL=" + tui_admin_setlocale->getCurrent());
	putenv((char *)tmp.c_str());

	setlocale (LC_CTYPE, "");
	setlocale (LC_MESSAGES, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	// Now need to update all UI components with new translations!
	
	core->UILocale = tui_admin_setlocale->getCurrent().c_str();
	LocaleChanged = 1;  // will reload TUI next draw.  Note that position in TUI is lost...
#endif


}


void stel_ui::tui_cb_effects_milkyway_intensity() {

	std::ostringstream oss;
	oss << "set milky_way_intensity " << tui_effect_milkyway_intensity->getValue();
	core->commander->execute_command(oss.str());
}

void stel_ui::tui_cb_setlocation() {
	
	std::ostringstream oss;
	oss << "moveto lat " << tui_location_latitude->getValue() 
		<< " lon " <<  tui_location_longitude->getValue()
		<< " alt " << tui_location_altitude->getValue();
	core->commander->execute_command(oss.str());

}


void stel_ui::tui_cb_stars()
{
	// 4. Stars
	std::ostringstream oss;

	oss << "flag stars " << tui_stars_show->getValue();
	core->commander->execute_command(oss.str());

	oss.str("");
	oss << "set max_mag_star_name " << tui_star_labelmaxmag->getValue();
	core->commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_twinkle_amount " << tui_stars_twinkle->getValue();
	core->commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_mag_scale " << tui_star_magscale->getValue();
	core->commander->execute_command(oss.str());

}

void stel_ui::tui_cb_effects()
{

	// 5. effects
	std::ostringstream oss;

	oss << "flag point_star " << tui_effect_pointobj->getValue();
	core->commander->execute_command(oss.str());

	oss.str("");
	oss << "set auto_move_duration " << tui_effect_zoom_duration->getValue();
	core->commander->execute_command(oss.str());

	oss.str("");
	oss << "flag manual_zoom " << tui_effect_manual_zoom->getValue();
	core->commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_scale " << tui_effect_object_scale->getValue();
	core->commander->execute_command(oss.str());

}


// set sky time
void stel_ui::tui_cb_sky_time()
{
	std::ostringstream oss;
	oss << "date local " << tui_time_skytime->getDateString();
	core->commander->execute_command(oss.str());
}
