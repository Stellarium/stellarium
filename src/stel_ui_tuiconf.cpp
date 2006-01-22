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

// Class which handles a stellarium User Interface in Text mode

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include "stel_ui.h"
#include "stellastro.h"
#include "sky_localizer.h"

// Draw simple gravity text ui.
void StelUI::draw_gravity_ui(void)
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
		wostringstream os;

		if (core->FlagUTC_Time)
		{
			os << core->observatory->get_printable_date_UTC(jd) << L" " <<
			core->observatory->get_printable_time_UTC(jd) << L" (UTC)";
		}
		else
		{
			os << core->observatory->get_printable_date_local(jd) << L" " <<
			core->observatory->get_printable_time_local(jd);
		}

		if (core->FlagShowFov) os << L" fov " << setprecision(3) << core->projection->get_visible_fov();
		if (core->FlagShowFps) os << L"  FPS " << core->fps;

		glColor3f(0.5,1,0.5);
		core->projection->print_gravity180(baseFont, x-shift + 30, y-shift + 38, os.str(), 0);
	}

	if (core->selected_object && core->FlagShowTuiShortObjInfo)
	{
	    wstring info = L"";
		info = core->selected_object->get_short_info_string(core->navigation);
		if (core->selected_object->get_type()==StelObject::STEL_OBJECT_NEBULA) glColor3fv(core->NebulaLabelColor[draw_mode]);
		if (core->selected_object->get_type()==StelObject::STEL_OBJECT_PLANET)glColor3fv(core->PlanetNamesColor[draw_mode]);
		if (core->selected_object->get_type()==StelObject::STEL_OBJECT_STAR) glColor3fv(core->selected_object->get_RGB());
		core->projection->print_gravity180(baseFont, x+shift - 30, y+shift - 38, info, 0);
	}
}

// Create all the components of the text user interface
void StelUI::init_tui(void)
{
	// Menu root branch
	LocaleChanged=0;
	ScriptDirectoryRead = 0;
	tui_root = new s_tui::Branch();

	// Submenus
	s_tui::MenuBranch* tui_menu_location = new s_tui::MenuBranch(wstring(L"1. ") + _("Set Location "));
	s_tui::MenuBranch* tui_menu_time = new s_tui::MenuBranch(wstring(L"2. ") + _("Set Time "));
	s_tui::MenuBranch* tui_menu_general = new s_tui::MenuBranch(wstring(L"3. ") + _("General "));
	s_tui::MenuBranch* tui_menu_stars = new s_tui::MenuBranch(wstring(L"4. ") + _("Stars "));
	s_tui::MenuBranch* tui_menu_colors = new s_tui::MenuBranch(wstring(L"5. ") + _("Colors "));
	s_tui::MenuBranch* tui_menu_effects = new s_tui::MenuBranch(wstring(L"5. ") + _("Effects "));
	s_tui::MenuBranch* tui_menu_scripts = new s_tui::MenuBranch(wstring(L"6. ") + _("Scripts "));
	s_tui::MenuBranch* tui_menu_administration = new s_tui::MenuBranch(wstring(L"7. ") + _("Administration "));
	tui_root->addComponent(tui_menu_location);
	tui_root->addComponent(tui_menu_time);
	tui_root->addComponent(tui_menu_general);	
	tui_root->addComponent(tui_menu_stars);
	tui_root->addComponent(tui_menu_colors);
	tui_root->addComponent(tui_menu_effects);
	tui_root->addComponent(tui_menu_scripts);
	tui_root->addComponent(tui_menu_administration);

	// 1. Location
	tui_location_latitude = new s_tui::Decimal_item(-90., 90., 0.,wstring(L"1.1 ") + _("Latitude: "));
	tui_location_latitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_setlocation));
	tui_location_longitude = new s_tui::Decimal_item(-180., 180., 0.,wstring(L"1.2 ") + _("Longitude: "));
	tui_location_longitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_setlocation));
	tui_location_altitude = new s_tui::Integer_item(-500, 10000, 0,wstring(L"1.3 ") + _("Altitude (m): "));
	tui_location_altitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_setlocation));
	tui_menu_location->addComponent(tui_location_latitude);
	tui_menu_location->addComponent(tui_location_longitude);
	tui_menu_location->addComponent(tui_location_altitude);

	// 2. Time
	tui_time_skytime = new s_tui::Time_item(wstring(L"2.1 ") + _("Sky Time: "));
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_sky_time));
	tui_time_settmz = new s_tui::Time_zone_item(core->getDataDir() + "zone.tab", wstring(L"2.2 ") + _("Set Time Zone: "));
	tui_time_settmz->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimezone));
	tui_time_settmz->settz(core->observatory->get_custom_tz_name());
	tui_time_presetskytime = new s_tui::Time_item(wstring(L"2.3 ") + _("Preset Sky Time: "));
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb1));
	tui_time_startuptime = new s_tui::MultiSet_item<wstring>(wstring(L"2.4 ") + _("Sky Time At Start-up: "));
	tui_time_startuptime->addItem(L"Actual");
	tui_time_startuptime->addItem(L"Preset");
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb1));
	tui_time_displayformat = new s_tui::MultiSet_item<wstring>(wstring(L"2.5 ") + _("Time Display Format: "));
	tui_time_displayformat->addItem(L"24h");
	tui_time_displayformat->addItem(L"12h");
	tui_time_displayformat->addItem(L"system_default");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimedisplayformat));
	tui_time_dateformat = new s_tui::MultiSet_item<wstring>(wstring(L"2.6 ") + _("Date Display Format: "));
	tui_time_dateformat->addItem(L"yyyymmdd");
	tui_time_dateformat->addItem(L"ddmmyyyy");
	tui_time_dateformat->addItem(L"mmddyyyy");
	tui_time_dateformat->addItem(L"system_default");
	tui_time_dateformat->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimedisplayformat));

	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);
	tui_menu_time->addComponent(tui_time_dateformat);

	// 3. General settings

	// sky culture goes here
	tui_general_sky_culture = new s_tui::MultiSet_item<wstring>(wstring(L"3.1 ") + _("Sky Culture: "));
	tui_general_sky_culture->addItemList(core->skyloc->get_sky_culture_list());  // human readable names
	tui_general_sky_culture->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_general_change_sky_culture));
	tui_menu_general->addComponent(tui_general_sky_culture);

	tui_general_sky_locale = new s_tui::MultiSet2_item<wstring>(wstring(L"3.2 ") + _("Sky Language: "));
	//tui_general_sky_locale->addItemList(core->skyloc->get_sky_locale_list());

	tui_general_sky_locale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_general_change_sky_locale));
	tui_menu_general->addComponent(tui_general_sky_locale);


	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, wstring(L"4.1 ") + _("Show: "), _("Yes"),_("No"));
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_magscale = new s_tui::Decimal_item(1,30, 1, wstring(L"4.2 ") + _("Star Magnitude Multiplier: "));
	tui_star_magscale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, wstring(L"4.3 ") + _("Maximum Magnitude to Label: "));
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_stars_twinkle = new s_tui::Decimal_item(0., 1., 0.3, wstring(L"4.4 ") + _("Twinkling: "), 0.1);
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));

	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_magscale);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);

	// 5. Colors
	tui_colors_const_line_color = new s_tui::Vector_item(wstring(L"5.1 ") + _("Constellation lines: "));
	tui_colors_const_line_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_const_line_color);

	tui_colors_const_label_color = new s_tui::Vector_item(wstring(L"5.2 ") + _("Constellation labels: "));
	tui_colors_const_label_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_const_label_color);

	tui_colors_cardinal_color = new s_tui::Vector_item(wstring(L"5.3 ") + _("Cardinal points: "));
	tui_colors_cardinal_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_cardinal_color);


	


	// 5. Effects
	tui_effect_landscape = new s_tui::MultiSet_item<wstring>(wstring(L"5.1 ") + _("Landscape: "));
	tui_effect_landscape->addItemList(StelUtility::stringToWstring(Landscape::get_file_content(core->getDataDir() + "landscapes.ini")));

	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);

	tui_effect_manual_zoom = new s_tui::Boolean_item(false, wstring(L"5.2 ") + _("Manual zoom: "), _("Yes"),_("No"));
	tui_effect_manual_zoom->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_manual_zoom);

	tui_effect_pointobj = new s_tui::Boolean_item(false, wstring(L"5.3 ") + _("Object Sizing Rule: "), _("Point"),_("Magnitude"));
	tui_effect_pointobj->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_pointobj);

	tui_effect_object_scale = new s_tui::Decimal_item(0, 25, 1, wstring(L"5.4 ") + _("Magnitude Sizing Multiplier: "), 0.1);
	tui_effect_object_scale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_object_scale);

	tui_effect_milkyway_intensity = new s_tui::Decimal_item(0, 100, 1, wstring(L"5.5 ") + _("Milky Way intensity: "), .5);
	tui_effect_milkyway_intensity->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects_milkyway_intensity));
	tui_menu_effects->addComponent(tui_effect_milkyway_intensity);

	tui_effect_nebulae_label_magnitude = new s_tui::Decimal_item(0, 100, 1, wstring(L"5.6 ") + _("Maximum Nebula Magnitude to Label: "), .5);
	tui_effect_nebulae_label_magnitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects_nebulae_label_magnitude));
	tui_menu_effects->addComponent(tui_effect_nebulae_label_magnitude);

	tui_effect_zoom_duration = new s_tui::Decimal_item(1, 10, 2, wstring(L"5.7 ") + _("Zoom Duration: "));
	tui_effect_zoom_duration->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_zoom_duration);

	tui_effect_cursor_timeout = new s_tui::Decimal_item(0, 60, 1, wstring(L"5.8 ") + _("Cursor Timeout: "));
	tui_effect_cursor_timeout->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_cursor_timeout);


	// 6. Scripts
	tui_scripts_local = new s_tui::MultiSet_item<wstring>(wstring(L"6.1 ") + _("Local Script: "));
	tui_scripts_local->addItemList(TUI_SCRIPT_MSG + wstring(L"\n") 
				       + StelUtility::stringToWstring(core->scripts->get_script_list(core->getDataDir() + "scripts/"))); 
	tui_scripts_local->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_scripts_local));
	tui_menu_scripts->addComponent(tui_scripts_local);

	tui_scripts_removeable = new s_tui::MultiSet_item<wstring>(wstring(L"6.2 ") + _("CD/DVD Script: "));
	//	tui_scripts_removeable->addItem("Arrow down to load list.");
	tui_scripts_removeable->addItem(TUI_SCRIPT_MSG);
	tui_scripts_removeable->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_scripts_removeable));
	tui_menu_scripts->addComponent(tui_scripts_removeable);


	// 7. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item(wstring(L"7.1 ") + _("Load Default Configuration: "));
	tui_admin_loaddefault->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_load_default));
	tui_admin_savedefault = new s_tui::ActionConfirm_item(wstring(L"7.2 ") + _("Save Current Configuration as Default: "));
	tui_admin_savedefault->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_save_default));
	tui_admin_updateme = new s_tui::Action_item(wstring(L"7.3 ") + _("Update me via Internet: "));
	tui_admin_updateme->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_updateme));
	tui_menu_administration->addComponent(tui_admin_loaddefault);
	tui_menu_administration->addComponent(tui_admin_savedefault);
	tui_menu_administration->addComponent(tui_admin_updateme);

	tui_admin_setlocale = new s_tui::MultiSet_item<wstring>(L"7.3 Set Locale: ");
	// Should be defined elsewhere...
	tui_admin_setlocale->addItem(L"en_US");
	tui_admin_setlocale->addItem(L"fr_FR");
	tui_admin_setlocale->addItem(L"nl_NL");
	tui_admin_setlocale->addItem(L"es_ES");
	tui_admin_setlocale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_set_locale));
	tui_menu_administration->addComponent(tui_admin_setlocale);


	tui_admin_voffset = new s_tui::Integer_item(-10,10,0, wstring(L"7.4 ") + _("N-S Centering Offset: "));
	tui_admin_voffset->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_admin_change_viewport));
	tui_menu_administration->addComponent(tui_admin_voffset);

	tui_admin_hoffset = new s_tui::Integer_item(-10,10,0, wstring(L"7.5 ") + _("E-W Centering Offset: "));
	tui_admin_hoffset->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_admin_change_viewport));
	tui_menu_administration->addComponent(tui_admin_hoffset);


}

// Display the tui
void StelUI::draw_tui(void)
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
		core->projection->print_gravity180(baseFont, x+shift - 30, y-shift + 38,
						   s_tui::stop_active + tui_root->getString(), 0);

	}
}

int StelUI::handle_keys_tui(Uint16 key, s_tui::S_TUI_VALUE state)
{
	return tui_root->onKey(key, state);
}

// Update all the core parameters with values taken from the tui widgets
void StelUI::tui_cb1(void)
{
	// 2. Date & Time
	core->PresetSkyTime 		= tui_time_presetskytime->getJDay();
	core->StartupTimeMode 		= StelUtility::wstringToString(tui_time_startuptime->getCurrent());

}

// Update all the tui widgets with values taken from the core parameters
void StelUI::tui_update_widgets(void)
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
	tui_time_startuptime->setCurrent(StelUtility::stringToWstring(core->StartupTimeMode));
	tui_time_displayformat->setCurrent(StelUtility::stringToWstring(core->observatory->get_time_format_str()));
	tui_time_dateformat->setCurrent(StelUtility::stringToWstring(core->observatory->get_date_format_str()));

	// 3. general
	tui_general_sky_culture->setValue(core->skyloc->convert_directory_to_sky_culture(core->SkyCulture));
	tui_general_sky_locale->setValue(StelUtility::stringToWstring(core->skyTranslator.getLocaleName()));

	// 4. Stars
	tui_stars_show->setValue(core->getFlagStars());
	tui_star_labelmaxmag->setValue(core->getMaxMagStarName());
	tui_stars_twinkle->setValue(core->getStarTwinkleAmount());
	tui_star_magscale->setValue(core->getStarMagScale());

	// 5. Colors
	tui_colors_const_line_color->setVector(core->asterisms->getLineColor());
	tui_colors_const_label_color->setVector(core->asterisms->getLabelColor());
	tui_colors_cardinal_color->setVector(core->cardinals_points->get_color());

	// 5. effects
	tui_effect_landscape->setValue(StelUtility::stringToWstring(core->observatory->get_landscape_name()));
	tui_effect_pointobj->setValue(core->getFlagPointStar());
	tui_effect_zoom_duration->setValue(core->auto_move_duration);
	tui_effect_manual_zoom->setValue(core->FlagManualZoom);
	tui_effect_object_scale->setValue(core->getStarScale());
	tui_effect_milkyway_intensity->setValue(core->milky_way->get_intensity());
	tui_effect_cursor_timeout->setValue(core->MouseCursorTimeout);
	tui_effect_nebulae_label_magnitude->setValue(core->MaxMagNebulaName);


	// 6. Scripts
	// each fresh time enter needs to reset to select message
	if(core->SelectedScript=="") {
		tui_scripts_local->setCurrent(TUI_SCRIPT_MSG);
		
		if(ScriptDirectoryRead) {
			tui_scripts_removeable->setCurrent(TUI_SCRIPT_MSG);
		} else {
			// no directory mounted, so put up message
			tui_scripts_removeable->replaceItemList(L"Arrow down to load list.",0);
		}
	}

	// 7. admin
	//tui_admin_setlocale->setValue(core->UILocale);
	tui_admin_voffset->setValue(core->getViewportVerticalOffset());
	tui_admin_hoffset->setValue(core->getViewportHorizontalOffset());

}

// Launch script to set time zone in the system locales
// TODO : this works only if the system manages the TZ environment
// variables of the form "Europe/Paris". This is not the case on windows
// so everything migth have to be re-done internaly :(
void StelUI::tui_cb_settimezone(void)
{
	// Don't call the script anymore coz it's pointless
	// system( ( core->getDataDir() + "script_set_time_zone " + tui_time_settmz->getCurrent() ).c_str() );
	core->observatory->set_custom_tz_name(tui_time_settmz->gettz());
}

// Set time format mode
void StelUI::tui_cb_settimedisplayformat(void)
{
	core->observatory->set_time_format_str(StelUtility::wstringToString(tui_time_displayformat->getCurrent()));
	core->observatory->set_date_format_str(StelUtility::wstringToString(tui_time_dateformat->getCurrent()));
}

// 7. Administration actions functions

// Load default configuration
void StelUI::tui_cb_admin_load_default(void)
{

	core->commander->execute_command("configuration action reload");

}

// Save to default configuration
void StelUI::tui_cb_admin_save_default(void)
{
	core->saveConfig();
	core->observatory->save(core->getConfigDir() + core->config_file, "init_location");
	system( ( core->getDataDir() + "script_save_config " ).c_str() );
}

// Launch script for internet update
void StelUI::tui_cb_admin_updateme(void)
{
	system( ( core->getDataDir() + "script_internet_update" ).c_str() );
}

// Set a new landscape skin
void StelUI::tui_cb_tui_effect_change_landscape(void)
{
	//	core->set_landscape(tui_effect_landscape->getCurrent());
	core->commander->execute_command(string("set landscape_name " +  StelUtility::wstringToString(tui_effect_landscape->getCurrent())));
}


// Set a new sky culture
void StelUI::tui_cb_tui_general_change_sky_culture(void) {

	//  core->set_sky_culture(core->skyloc->convert_sky_culture_to_directory(tui_general_sky_culture->getCurrent()));
	core->commander->execute_command( string("set sky_culture " + 
											 core->skyloc->convert_sky_culture_to_directory(tui_general_sky_culture->getCurrent())));
}

// Set a new sky locale
void StelUI::tui_cb_tui_general_change_sky_locale(void) {

	//	core->set_sky_locale(tui_general_sky_locale->getCurrent());
	core->commander->execute_command( string("set sky_locale " + StelUtility::wstringToString(tui_general_sky_locale->getCurrent())));
}


// callback for viewport centering
void StelUI::tui_cb_tui_admin_change_viewport(void)
{
	core->setViewportVerticalOffset(tui_admin_voffset->getValue());
	core->setViewportHorizontalOffset(tui_admin_hoffset->getValue());
}

// callback for changing scripts from removeable media
void StelUI::tui_cb_scripts_removeable() {
  
  if(!ScriptDirectoryRead) {
	  // read scripts from mounted disk
	  string script_list = core->scripts->get_script_list(SCRIPT_REMOVEABLE_DISK);
	  tui_scripts_removeable->replaceItemList(TUI_SCRIPT_MSG + wstring(L"\n") + StelUtility::stringToWstring(script_list),0);
	  ScriptDirectoryRead = 1;
  } 

  if(tui_scripts_removeable->getCurrent()==TUI_SCRIPT_MSG) {
	  core->SelectedScript = "";
  } else {
	  core->SelectedScript = StelUtility::wstringToString(tui_scripts_removeable->getCurrent());
	  core->SelectedScriptDirectory = SCRIPT_REMOVEABLE_DISK;
	  // to avoid confusing user, clear out local script selection as well
	  tui_scripts_local->setCurrent(TUI_SCRIPT_MSG);
  } 
}


// callback for changing scripts from local directory
void StelUI::tui_cb_scripts_local() {
  
  if(tui_scripts_local->getCurrent()!=TUI_SCRIPT_MSG){
    core->SelectedScript = StelUtility::wstringToString(tui_scripts_local->getCurrent());
    core->SelectedScriptDirectory = core->getDataDir() + "scripts/";
    // to reduce confusion for user, clear out removeable script selection as well
    if(ScriptDirectoryRead) tui_scripts_removeable->setCurrent(TUI_SCRIPT_MSG);
  } else {
    core->SelectedScript = "";
  }
}


// change UI locale
void StelUI::tui_cb_admin_set_locale() {

	// Right now just set for the current session

#if !defined(MACOSX)
	LocaleChanged = 1;  // will reload TUI next draw.  Note that position in TUI is lost...
	core->setAppLanguage(StelUtility::wstringToString(tui_admin_setlocale->getCurrent()));
#endif


}


void StelUI::tui_cb_effects_milkyway_intensity() {

	std::ostringstream oss;
	oss << "set milky_way_intensity " << tui_effect_milkyway_intensity->getValue();
	core->commander->execute_command(oss.str());
}

void StelUI::tui_cb_setlocation() {
	
	std::ostringstream oss;
	oss << "moveto lat " << tui_location_latitude->getValue() 
		<< " lon " <<  tui_location_longitude->getValue()
		<< " alt " << tui_location_altitude->getValue();
	core->commander->execute_command(oss.str());

}


void StelUI::tui_cb_stars()
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

void StelUI::tui_cb_effects()
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

	core->MouseCursorTimeout = tui_effect_cursor_timeout->getValue();  // never recorded

}


// set sky time
void StelUI::tui_cb_sky_time()
{
	std::ostringstream oss;
	oss << "date local " << StelUtility::wstringToString(tui_time_skytime->getDateString());
	core->commander->execute_command(oss.str());
}


// set nebula label limit
void StelUI::tui_cb_effects_nebulae_label_magnitude()
{
	std::ostringstream oss;
	oss << "set max_mag_nebula_name " << tui_effect_nebulae_label_magnitude->getValue();
	core->commander->execute_command(oss.str());
}


void StelUI::tui_cb_change_color()
{
	core->asterisms->setLineColor( tui_colors_const_line_color->getVector() );
	core->asterisms->setLabelColor( tui_colors_const_label_color->getVector() );
	core->cardinals_points->set_color( tui_colors_cardinal_color->getVector() );
}
