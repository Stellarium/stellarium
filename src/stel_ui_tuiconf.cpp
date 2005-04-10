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
	tui_root = new s_tui::Branch();

	// Submenus
	s_tui::MenuBranch* tui_menu_location = new s_tui::MenuBranch("1. Set Location ");
	s_tui::MenuBranch* tui_menu_time = new s_tui::MenuBranch("2. Set Time ");
	s_tui::MenuBranch* tui_menu_general = new s_tui::MenuBranch("3. General ");
	s_tui::MenuBranch* tui_menu_stars = new s_tui::MenuBranch("4. Stars ");
	s_tui::MenuBranch* tui_menu_effects = new s_tui::MenuBranch("5. Effects ");
	s_tui::MenuBranch* tui_menu_scripts = new s_tui::MenuBranch("6. Scripts ");
	s_tui::MenuBranch* tui_menu_administration = new s_tui::MenuBranch("7. Administration ");
	tui_root->addComponent(tui_menu_location);
	tui_root->addComponent(tui_menu_time);
	tui_root->addComponent(tui_menu_general);	
	tui_root->addComponent(tui_menu_stars);
	tui_root->addComponent(tui_menu_effects);
	tui_root->addComponent(tui_menu_scripts);
	tui_root->addComponent(tui_menu_administration);

	// 1. Location
	tui_location_latitude = new s_tui::Decimal_item(-90., 90., 0., "1.1 Latitude: ");
	tui_location_latitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_location_longitude = new s_tui::Decimal_item(-180., 180., 0., "1.2 Longitude: ");
	tui_location_longitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_location_altitude = new s_tui::Integer_item(-500, 10000, 0, "1.3 Altitude (m): ");
	tui_location_altitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_location->addComponent(tui_location_latitude);
	tui_menu_location->addComponent(tui_location_longitude);
	tui_menu_location->addComponent(tui_location_altitude);

	// 2. Time
	tui_time_skytime = new s_tui::Time_item("2.1 Sky Time: ");
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_settmz = new s_tui::Time_zone_item(core->DataDir + "zone.tab", "2.2 Set Time Zone: ");
	tui_time_settmz->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimezone));
	tui_time_settmz->settz(core->observatory->get_custom_tz_name());
	tui_time_presetskytime = new s_tui::Time_item("2.3 Preset Sky Time: ");
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_startuptime = new s_tui::MultiSet_item<string>("2.4 Sky Time At Start-up: ");
	tui_time_startuptime->addItem("Actual");
	tui_time_startuptime->addItem("Preset");
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_displayformat = new s_tui::MultiSet_item<string>("2.5 Time Display Format: ");
	tui_time_displayformat->addItem("24h");
	tui_time_displayformat->addItem("12h");
	tui_time_displayformat->addItem("system_default");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimedisplayformat));

	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);

	// 3. General settings

	// sky culture goes here
	tui_general_sky_culture = new s_tui::MultiSet_item<string>("3.1 Sky Culture: ");
	tui_general_sky_culture->addItemList(core->skyloc->get_sky_culture_list());  // human readable names
	tui_general_sky_culture->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_general_change_sky_culture));
	tui_menu_general->addComponent(tui_general_sky_culture);

	tui_general_sky_locale = new s_tui::MultiSet_item<string>("3.2 Sky Language: ");
	tui_general_sky_locale->addItemList(core->skyloc->get_sky_locale_list());  // human readable names

	tui_general_sky_locale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_general_change_sky_locale));
	tui_menu_general->addComponent(tui_general_sky_locale);


	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, "4.1 Show: ", "Yes","No");
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_star_magscale = new s_tui::Decimal_item(1,30, 1, "4.2 Star Magnitude Multiplier: ");
	tui_star_magscale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, "4.3 Maximum Magnitude to Label: ");
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_stars_twinkle = new s_tui::Decimal_item(0., 1., 0.3, "4.4 Twinkling: ", 0.1);
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));

	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_magscale);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);


	// 5. Effects
	tui_effect_landscape = new s_tui::MultiSet_item<string>("5.1 Landscape: ");
	tui_effect_landscape->addItemList(Landscape::get_file_content(core->DataDir + "landscapes.ini"));
	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);

	tui_effect_manual_zoom = new s_tui::Boolean_item(false, "5.2 Manual zoom: ", "Yes","No");
	tui_effect_manual_zoom->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_effects->addComponent(tui_effect_manual_zoom);

	tui_effect_pointobj = new s_tui::Boolean_item(false, "5.3 Object Sizing Rule: ", "Point","Magnitude");
	tui_effect_pointobj->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_effects->addComponent(tui_effect_pointobj);

	tui_effect_object_scale = new s_tui::Decimal_item(0, 25, 1, "5.4 Object Size Multiplier: ");
	tui_effect_object_scale->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_effects->addComponent(tui_effect_object_scale);

	tui_effect_zoom_duration = new s_tui::Decimal_item(1, 10, 2, "5.5 Zoom duration: ");
	tui_effect_zoom_duration->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_effects->addComponent(tui_effect_zoom_duration);


	// 6. Scripts
	tui_scripts_local = new s_tui::MultiSet_item<string>("6.1 Local Script: ");
	tui_scripts_local->addItemList(TUI_SCRIPT_MSG + string("\n") 
				       + core->scripts->get_script_list(core->DataDir + "scripts/")); 
	tui_scripts_local->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_scripts_local));
	tui_menu_scripts->addComponent(tui_scripts_local);

	tui_scripts_removeable = new s_tui::MultiSet_item<string>("6.2 CD/DVD Script: ");
	//	tui_scripts_removeable->addItem("Arrow down to load list.");
	tui_scripts_removeable->addItem(TUI_SCRIPT_MSG);
	tui_scripts_removeable->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_scripts_removeable));
	tui_menu_scripts->addComponent(tui_scripts_removeable);


	// 7. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item("7.1 Load Default Configuration: ");
	tui_admin_loaddefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_load_default));
	tui_admin_savedefault = new s_tui::ActionConfirm_item("7.2 Save Current Configuration as Default: ");
	tui_admin_savedefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_save_default));
	/*tui_admin_setlocal = new s_tui::MultiSet_item<string>("7.3 Set Locale: ");
	tui_admin_setlocal->addItem("fr_FR");
	tui_admin_setlocal->addItem("en_EN");
	tui_admin_setlocal->addItem("en_US");
	tui_admin_setlocal->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_set_locale));*/
	tui_admin_updateme = new s_tui::Action_item("7.3 Update me via Internet: ");
	tui_admin_updateme->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_updateme));
	tui_menu_administration->addComponent(tui_admin_loaddefault);
	tui_menu_administration->addComponent(tui_admin_savedefault);
	//tui_menu_administration->addComponent(tui_admin_setlocal);
	tui_menu_administration->addComponent(tui_admin_updateme);

	tui_admin_voffset = new s_tui::Integer_item(-10,10,0, "7.4 N-S Centering Offset: ");
	tui_admin_voffset->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_admin_change_viewport));
	tui_menu_administration->addComponent(tui_admin_voffset);

	tui_admin_hoffset = new s_tui::Integer_item(-10,10,0, "7.5 E-W Centering Offset: ");
	tui_admin_hoffset->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_admin_change_viewport));
	tui_menu_administration->addComponent(tui_admin_hoffset);


}

// Display the tui
void stel_ui::draw_tui(void)
{
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

int stel_ui::handle_keys_tui(SDLKey key, s_tui::S_TUI_VALUE state)
{
	return tui_root->onKey(key, state);
}

// Update all the core parameters with values taken from the tui widgets
void stel_ui::tui_cb1(void)
{
	// 1. Location

	core->observatory->set_latitude(tui_location_latitude->getValue());
	core->observatory->set_longitude(tui_location_longitude->getValue());
	core->observatory->set_altitude(tui_location_altitude->getValue());

	// 2. Date & Time
	double skyJDay = tui_time_skytime->getJDay();
	core->navigation->set_JDay(skyJDay -   
				   core->observatory->get_GMT_shift(skyJDay,1)*JD_HOUR);
	core->PresetSkyTime 		= tui_time_presetskytime->getJDay();
	core->StartupTimeMode 		= tui_time_startuptime->getCurrent();

	// 4. Stars
	core->FlagStars 			= tui_stars_show->getValue();
	core->MaxMagStarName 		= tui_star_labelmaxmag->getValue();
	core->StarTwinkleAmount		= tui_stars_twinkle->getValue();
	core->StarMagScale			= tui_star_magscale->getValue();

	// 5. effects
	core->FlagPointStar 		= tui_effect_pointobj->getValue();
	core->auto_move_duration	= tui_effect_zoom_duration->getValue();
	core->FlagManualZoom 		= tui_effect_manual_zoom->getValue();


	std::ostringstream oss;
	oss << "set star_scale " << tui_effect_object_scale->getValue();
	core->commander->execute_command(oss.str());

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

	// 3. general
	tui_general_sky_culture->setValue(core->skyloc->convert_directory_to_sky_culture(core->SkyCulture));
	tui_general_sky_locale->setValue(core->skyloc->convert_locale_to_name(core->SkyLocale));  // human readable names

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
}

// 7. Administration actions functions

// Load default configuration
void stel_ui::tui_cb_admin_load_default(void)
{
	core->load_config();

	// believe this is redundant : yes but this must be here and not in stel_core.cpp
	if(core->asterisms)
	{
		core->asterisms->set_art_intensity(core->ConstellationArtIntensity);
		core->asterisms->set_art_fade_duration(core->ConstellationArtFadeDuration);
	}
	if (!core->FlagAtmosphere && core->tone_converter)
		core->tone_converter->set_world_adaptation_luminance(3.75f);
	if (core->atmosphere) core->atmosphere->set_fade_duration(core->AtmosphereFadeDuration);
	core->observatory->load(core->ConfigDir + core->config_file, "init_location");
	core->set_landscape(core->observatory->get_landscape_name());
	
	if (core->StartupTimeMode=="preset" || core->StartupTimeMode=="Preset")
	{
		core->navigation->set_JDay(core->PresetSkyTime -
			core->observatory->get_GMT_shift(core->PresetSkyTime) * JD_HOUR);
	}
	else
	{
		core->navigation->set_JDay(get_julian_from_sys());
	}
	if(core->FlagObjectTrails && core->ssystem) core->ssystem->start_trails();
	else  core->ssystem->end_trails();

	system( ( core->DataDir + "script_load_config " ).c_str() );
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
	core->set_landscape(tui_effect_landscape->getCurrent());
}


// Set a new sky culture
void stel_ui::tui_cb_tui_general_change_sky_culture(void) {

  core->set_sky_culture(core->skyloc->convert_sky_culture_to_directory(tui_general_sky_culture->getCurrent()));

}

// Set a new sky locale
void stel_ui::tui_cb_tui_general_change_sky_locale(void) {

  string locale = core->skyloc->convert_name_to_locale(tui_general_sky_locale->getCurrent());
  core->set_sky_locale(locale);
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
    // TODO: make sure disk is mounted here

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
