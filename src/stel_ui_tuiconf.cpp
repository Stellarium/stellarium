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
#include <map>
#include "stel_ui.h"
#include "stellastro.h"

// Draw simple gravity text ui.
void stel_ui::draw_gravity_ui(void)
{
	// Normal transparency mode
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	int x = core->projection->view_left() + core->projection->viewW()/2;
	int y = core->projection->view_bottom() + core->projection->viewH()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(x,y));

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

		if (core->FlagShowFov) os << " fov " << setprecision(3) << core->projection->get_fov();
		if (core->FlagShowFps) os << "  FPS " << core->fps;

		glColor3f(0.1,0.9,0.1);
		core->projection->print_gravity180(spaceFont, x-shift + 15, y-shift + 19, os.str());
	}

	if (core->selected_object && core->FlagShowTuiShortInfo)
	{
	    static char str[255];	// TODO use c++ string for get_short_info_string() func
		core->selected_object->get_short_info_string(str, core->navigation);
		if (core->selected_object->get_type()==STEL_OBJECT_NEBULA) glColor3f(0.4f,0.5f,0.8f);
		if (core->selected_object->get_type()==STEL_OBJECT_PLANET) glColor3f(1.0f,0.3f,0.3f);
		if (core->selected_object->get_type()==STEL_OBJECT_STAR) glColor3fv(core->selected_object->get_RGB());
		core->projection->print_gravity180(spaceFont, x+shift - 15, y+shift - 19, str);
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
	s_tui::MenuBranch* tui_menu_stars = new s_tui::MenuBranch("3. Stars ");
	s_tui::MenuBranch* tui_menu_effects = new s_tui::MenuBranch("4. Effects ");
	s_tui::MenuBranch* tui_menu_administration = new s_tui::MenuBranch("5. Administration ");
	tui_root->addComponent(tui_menu_location);
	tui_root->addComponent(tui_menu_time);
	tui_root->addComponent(tui_menu_stars);
	tui_root->addComponent(tui_menu_effects);
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
	tui_time_settmz = create_tree_from_time_zone_file(core->DataDir + "zone.tab");
	tui_time_settmz->set_OnTriggerCallback(callback<void>(this, &stel_ui::tui_cb_settimezone));
	tui_time_skytime = new s_tui::Time_item("2.2 Sky Time: ");
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_presetskytime = new s_tui::Time_item("2.3 Preset Sky Time: ");
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_actual = new s_tui::Time_item("2.4 Set Actual Time: ");
	tui_time_actual->setJDay(get_julian_from_sys() + core->observatory->get_GMT_shift(core->PresetSkyTime) * JD_HOUR);
	tui_time_actual->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_actualtime));
	tui_time_startuptime = new s_tui::MultiSet_item<string>("2.5 Sky Time At Start-up: ");
	tui_time_startuptime->addItem("Actual");
	tui_time_startuptime->addItem("Preset");
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_displayformat = new s_tui::MultiSet_item<string>("2.6 Time Display Format: ");
	tui_time_displayformat->addItem("24h");
	tui_time_displayformat->addItem("12h");
	tui_time_displayformat->addItem("system_default");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimedisplayformat));
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_actual);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);

	// 3. Stars
	tui_stars_show = new s_tui::Boolean_item(false, "3.1 Show: ", "Yes","No");
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, "3.2 Maximum Magnitude to Label: ");
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_stars_twinkle = new s_tui::Decimal_item(0., 1., 0.3, "3.3 Twinkling: ", 0.1);
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);

	// 4. Effects
	tui_effect_landscape = new s_tui::MultiSet_item<string>("4.1 Landscape: ");
	tui_effect_landscape->addItemList(Landscape::get_file_content(core->DataDir + "landscapes.ini"));
	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);

	// 5. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item("5.1 Load Default Configuration: ");
	tui_admin_loaddefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_load_default));
	tui_admin_savedefault = new s_tui::ActionConfirm_item("5.2 Save Current Configuration as Default: ");
	tui_admin_savedefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_save_default));
	tui_admin_setlocal = new s_tui::MultiSet_item<string>("5.3 Set Locale: ");
	tui_admin_setlocal->addItem("fr_FR");
	tui_admin_setlocal->addItem("en_EN");
	tui_admin_setlocal->addItem("en_US");
	tui_admin_setlocal->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_set_locale));
	tui_admin_updateme = new s_tui::Action_item("5.4 Update me via Internet: ");
	tui_admin_updateme->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_updateme));
	tui_menu_administration->addComponent(tui_admin_loaddefault);
	tui_menu_administration->addComponent(tui_admin_savedefault);
	tui_menu_administration->addComponent(tui_admin_setlocal);
	tui_menu_administration->addComponent(tui_admin_updateme);

}

// Display the tui
void stel_ui::draw_tui(void)
{
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	int x = core->projection->view_left() + core->projection->viewW()/2;
	int y = core->projection->view_bottom() + core->projection->viewH()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(x,y));

	if (tui_root)
	{
		glColor3f(0.1,0.9,0.1);
		core->projection->print_gravity180(spaceFont, x+shift - 15, y-shift + 15,
			s_tui::stop_active + tui_root->getString());
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
	core->navigation->set_JDay(tui_time_skytime->getJDay());
	core->PresetSkyTime 		= tui_time_presetskytime->getJDay();
	core->StartupTimeMode 		= tui_time_startuptime->getCurrent();

	// 3. Stars
	core->FlagStars 			= tui_stars_show->getValue();
	core->MaxMagStarName 		= tui_star_labelmaxmag->getValue();
	core->StarTwinkleAmount		= tui_stars_twinkle->getValue();

}

// Update all the tui widgets with values taken from the core parameters
void stel_ui::tui_update_widgets(void)
{
	// 1. Location
	tui_location_latitude->setValue(core->observatory->get_latitude());
	tui_location_longitude->setValue(core->observatory->get_longitude());
	tui_location_altitude->setValue(core->observatory->get_altitude());

	// 2. Date & Time
	tui_time_skytime->setJDay(core->navigation->get_JDay());
	tui_time_presetskytime->setJDay(core->PresetSkyTime);
	tui_time_startuptime->setCurrent(core->StartupTimeMode);
	tui_time_displayformat->setCurrent(core->observatory->get_time_format_str());

	// 3. Stars
	tui_stars_show->setValue(core->FlagStars);
	tui_star_labelmaxmag->setValue(core->MaxMagStarName);
	tui_stars_twinkle->setValue(core->StarTwinkleAmount);
}

// Launch script to set time zone in the system locales
// TODO : this works only if the system manages the TZ environment
// variables of the form "Europe/Paris". This is not the case on windows
// so everything migth have to be re-done internaly :(
void stel_ui::tui_cb_settimezone(void)
{
	system( ( core->DataDir + "script_set_time_zone " + tui_time_settmz->getCurrent() ).c_str() );
	putenv(strdup((string("TZ=") + tui_time_settmz->getCurrent()).c_str()));
	tzset();
}

// Set time format mode
void stel_ui::tui_cb_settimedisplayformat(void)
{
	core->observatory->set_time_format_str(tui_time_displayformat->getCurrent());
}

// Launch script to set system time to current sky time with the iso 8601 utc time as parameter
// ie in format %Y-%m-%d %H:%M:%S
void stel_ui::tui_cb_actualtime(void)
{
	system( ( core->DataDir + "script_set_system_time " +
		core->observatory->get_ISO8601_time_UTC(tui_time_actual->getJDay()) ).c_str() );
}

// 5. Administration actions functions

// Load default configuration
void stel_ui::tui_cb_admin_load_default(void)
{
	core->load_config();
	system( ( core->DataDir + "script_load_config " ).c_str() );
}

// Load default configuration
void stel_ui::tui_cb_admin_save_default(void)
{
	core->save_config();
	system( ( core->DataDir + "script_save_config " ).c_str() );
}

// Call script to set locale parameter (LANG)
void stel_ui::tui_cb_admin_set_locale(void)
{
	system( ( core->DataDir + "script_set_locale " + tui_admin_setlocal->getCurrent() ).c_str() );
	putenv(strdup((string("LANG=") + tui_admin_setlocal->getCurrent()).c_str()));
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

// Parse a file of type /usr/share/zoneinfo/zone.tab
s_tui::MultiSet_item<string>* stel_ui::create_tree_from_time_zone_file(const string& zonetab)
{
	s_tui::MultiSet_item<string>* retmult = new s_tui::MultiSet_item<string>("2.1 Set Time Zone: ");

	ifstream is(zonetab.c_str());
	char zoneline[256];

	string unused, tzname;

	while (is.getline(zoneline, 256))
	{
		if (zoneline[0]=='#') continue;
		istringstream istr(zoneline);
		istr >> unused >> unused >> tzname;
		retmult->addItem(tzname);
	}

	is.close();

	// Try to detect which time zone is currently used by the system from the TZ environment variable
	char * temp = getenv("TZ");
	string currenttz;
	if (temp!=NULL)
	{
		currenttz = temp;
	}
	if (currenttz.empty())
	{
		if (core->FlagShowTZWarning)
		{
			cout << "The TZ environment variable wasn't set." << endl;
			cout << "The default value in the set time zone menu will be incorrect. The system time zone will be used though.." << endl;
		}
		return retmult;
	}

	if (!retmult->setValue(currenttz))
	{
		cout << "Can't find timezone " << currenttz << " in the time zone list" << endl;
	}
	return retmult;
}
