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

#include "stel_ui.h"
#include "stellastro.h"
#include <iostream>
#include <iomanip>

// Draw simple gravity text ui.
void stel_ui::draw_gravity_ui(void)
{
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

		os << " fov " << setprecision(3) << core->projection->get_fov() << "  FPS " << core->fps;

		glColor3f(0.1,0.9,0.1);
		core->projection->print_gravity180(spaceFont, x-shift + 10, y-shift + 10, os.str());
	}

	if (core->selected_object && core->FlagShowTuiShortInfo)
	{
	    static char str[255];	// TODO use c++ string for get_short_info_string() func
		core->selected_object->get_short_info_string(str, core->navigation);
		if (core->selected_object->get_type()==STEL_OBJECT_NEBULA) glColor3f(0.4f,0.5f,0.8f);
		if (core->selected_object->get_type()==STEL_OBJECT_PLANET) glColor3f(1.0f,0.3f,0.3f);
		if (core->selected_object->get_type()==STEL_OBJECT_STAR) glColor3fv(core->selected_object->get_RGB());
		core->projection->print_gravity180(spaceFont, x+shift - 10, y+shift - 10, str);
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
	s_tui::MenuBranch* tui_menu_constellation = new s_tui::MenuBranch("3. Constellations ");
	s_tui::MenuBranch* tui_menu_stars = new s_tui::MenuBranch("4. Stars ");
	s_tui::MenuBranch* tui_menu_labels = new s_tui::MenuBranch("5. Labels ");
	s_tui::MenuBranch* tui_menu_refpoints = new s_tui::MenuBranch("6. Reference Points ");
	s_tui::MenuBranch* tui_menu_effects = new s_tui::MenuBranch("7. Effects ");
	s_tui::MenuBranch* tui_menu_administration = new s_tui::MenuBranch("8. Administration ");
	tui_root->addComponent(tui_menu_location);
	tui_root->addComponent(tui_menu_time);
	tui_root->addComponent(tui_menu_constellation);
	tui_root->addComponent(tui_menu_stars);
	tui_root->addComponent(tui_menu_labels);
	tui_root->addComponent(tui_menu_refpoints);
	tui_root->addComponent(tui_menu_effects);
	tui_root->addComponent(tui_menu_administration);

	// 1. Location
	tui_location_latitude = new s_tui::Decimal_item(-90., 90., 0., "1.1 Latitude: ");
	tui_location_latitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_location_longitude = new s_tui::Decimal_item(-180., 180., 0., "1.2 Longitude: ");
	tui_location_longitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_location_altitude = new s_tui::Integer_item(-500, 10000, 0, "1.3 Altitude: ");
	tui_location_altitude->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_location->addComponent(tui_location_latitude);
	tui_menu_location->addComponent(tui_location_longitude);
	tui_menu_location->addComponent(tui_location_altitude);

	// 2. Time
	tui_time_settmz = new s_tui::MultiSet_item<string>("2.1 Set Time Zone: ");
	tui_time_settmz->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_settimezone));
	tui_time_settmz->addItem("Paris");
	tui_time_settmz->addItem("New York");
	tui_time_skytime = new s_tui::Time_item("2.2 Sky Time: ");
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_presetskytime = new s_tui::Time_item("2.3 Preset Sky Time: ");
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_actual = new s_tui::Action_item("2.4 Set Time to actual: ");
	tui_time_actual->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_actualtime));
	tui_time_startuptime = new s_tui::MultiSet_item<string>("2.5 Sky Time At Start-up: ");
	tui_time_startuptime->addItem("Actual");
	tui_time_startuptime->addItem("Preset");
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_displayformat = new s_tui::MultiSet_item<string>("2.6 Time Display Format: ");
	tui_time_displayformat->addItem("24h");
	tui_time_displayformat->addItem("12h");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_actual);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);

	// 3. Constellations
	tui_constellation_culture = new s_tui::MultiSet_item<string>("3.1 Culture: ");
	tui_constellation_culture->addItem("Occidental");
	tui_constellation_culture->addItem("Old Greek");
	tui_constellation_culture->addItem("Chinese");
	tui_constellation_lines = new s_tui::Boolean_item(false, "3.2 Lines: ", "Yes","No");
	tui_constellation_lines->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_constellation_art = new s_tui::Boolean_item(false, "3.3 Art: ", "Yes","No");
	tui_constellation_art->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_constellation->addComponent(tui_constellation_culture);
	tui_menu_constellation->addComponent(tui_constellation_lines);
	tui_menu_constellation->addComponent(tui_constellation_art);

	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, "4.1 Show: ", "Yes","No");
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, "4.2 Maximum Magnitude to Label: ");
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_stars_twinkle = new s_tui::Boolean_item(false, "4.3 Twinkle: ", "Yes","No");
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);

	// 5. Labels
	tui_label_stars = new s_tui::Boolean_item(false, "5.1 Stars: ", "Yes","No");
	tui_label_stars->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_label_constellations = new s_tui::Boolean_item(false, "5.2 Constellations: ", "Yes","No");
	tui_label_constellations->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_label_nebulas = new s_tui::Boolean_item(false, "5.3 Nebulas: ", "Yes","No");
	tui_label_nebulas->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_label_planets = new s_tui::Boolean_item(false, "5.4 Planets: ", "Yes","No");
	tui_label_planets->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_label_timeinfo = new s_tui::Boolean_item(false, "5.5 Time & Object Info: ", "Yes","No");
	tui_label_timeinfo->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_labels->addComponent(tui_label_stars);
	tui_menu_labels->addComponent(tui_label_constellations);
	tui_menu_labels->addComponent(tui_label_nebulas);
	tui_menu_labels->addComponent(tui_label_planets);
	tui_menu_labels->addComponent(tui_label_timeinfo);

	// 6. Reference Points
	tui_refpoints_cardinalpoints = new s_tui::Boolean_item(false, "6.1 Cardinal Points: ", "Yes","No");
	tui_refpoints_cardinalpoints->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_refpoints_ecliptic = new s_tui::Boolean_item(false, "6.2 Ecliptic: ", "Yes","No");
	tui_refpoints_ecliptic->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_refpoints_equator = new s_tui::Boolean_item(false, "6.3 Equator: ", "Yes","No");
	tui_refpoints_equator->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_refpoints_equatorialgrid = new s_tui::Boolean_item(false, "6.4 Equatorial Grid: ", "Yes","No");
	tui_refpoints_equatorialgrid->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_refpoints_azimutalgrid = new s_tui::Boolean_item(false, "6.5 Azimutal Grid: ", "Yes","No");
	tui_refpoints_azimutalgrid->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_refpoints->addComponent(tui_refpoints_cardinalpoints);
	tui_menu_refpoints->addComponent(tui_refpoints_ecliptic);
	tui_menu_refpoints->addComponent(tui_refpoints_equator);
	tui_menu_refpoints->addComponent(tui_refpoints_equatorialgrid);
	tui_menu_refpoints->addComponent(tui_refpoints_azimutalgrid);

	// 7. Effects
	tui_effect_landscape = new s_tui::MultiSet_item<string>("7.1 Landscape: ");
	tui_effect_landscape->addItemList(Landscape::get_file_content(core->DataDir + "landscapes.ini"));
	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);
	tui_effect_atmosphere = new s_tui::Boolean_item(false, "7.2 Atmopshere: ", "Yes","No");
	tui_effect_atmosphere->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_effects->addComponent(tui_effect_atmosphere);

	// 8. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item("8.1 Load Default Configuration: ");
	tui_admin_loaddefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_load_default));
	tui_admin_savedefault = new s_tui::ActionConfirm_item("8.2 Save Current Configuration as Default: ");
	tui_admin_savedefault->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_save_default));
	tui_admin_setlocal = new s_tui::MultiSet_item<string>("8.3 Set Locale: ");
	tui_admin_setlocal->addItem("fr_FR");
	tui_admin_setlocal->addItem("en_EN");
	tui_admin_setlocal->addItem("en_US");
	tui_admin_setlocal->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb_admin_set_locale));
	tui_admin_updateme = new s_tui::Action_item("8.4 Update me via Internet: ");
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

	int x = core->projection->view_left() + core->projection->viewW()/2;
	int y = core->projection->view_bottom() + core->projection->viewH()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(x,y));

	if (tui_root)
	{
		glColor3f(0.1,0.9,0.1);
		core->projection->print_gravity180(spaceFont, x+shift - 10, y-shift + 10,
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

	// 3. Constellation
	core->ConstellationCulture 	= tui_constellation_culture->getCurrent();
	core->FlagConstellationDrawing 	= tui_constellation_lines->getValue();
	core->FlagConstellationArt 	= tui_constellation_art->getValue();

	// 4. Stars
	core->FlagStars 			= tui_stars_show->getValue();
	core->MaxMagStarName 		= tui_star_labelmaxmag->getValue();
	core->FlagStarTwinkle 		= tui_stars_twinkle->getValue();

	// 5. Labels
	core->FlagStarName 			= tui_label_stars->getValue();
	core->FlagConstellationName = tui_label_constellations->getValue();
	core->FlagNebulaName 		= tui_label_nebulas->getValue();
	core->FlagNebulaCircle 		= tui_label_nebulas->getValue();
	core->FlagPlanetsHints		= tui_label_planets->getValue();
	core->FlagShowTuiDateTime	= tui_label_timeinfo->getValue();
	core->FlagShowTuiShortInfo	= tui_label_timeinfo->getValue();

	// 6. Reference points
	core->FlagCardinalPoints	= tui_refpoints_cardinalpoints->getValue();
	core->FlagEclipticLine		= tui_refpoints_ecliptic->getValue();
	core->FlagEquatorLine		= tui_refpoints_equator->getValue();
	core->FlagEquatorialGrid	= tui_refpoints_equatorialgrid->getValue();
	core->FlagAzimutalGrid		= tui_refpoints_azimutalgrid->getValue();

	// 7. Effect
	core->FlagAtmosphere		= tui_effect_atmosphere->getValue();

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

	// 3. Constellation
	tui_constellation_culture->setCurrent(core->ConstellationCulture);
	tui_constellation_lines->setValue(core->FlagConstellationDrawing);
	tui_constellation_art->setValue(core->FlagConstellationArt);

	// 4. Stars
	tui_stars_show->setValue(core->FlagStars);
	tui_star_labelmaxmag->setValue(core->MaxMagStarName);
	tui_stars_twinkle->setValue(core->FlagStarTwinkle);

	// 5. Labels
	tui_label_stars->setValue(core->FlagStarName);
	tui_label_constellations->setValue(core->FlagConstellationName);
	tui_label_nebulas->setValue(core->FlagNebulaName);
	tui_label_nebulas->setValue(core->FlagNebulaCircle);
	tui_label_planets->setValue(core->FlagPlanetsHints);
	tui_label_timeinfo->setValue(core->FlagShowTuiDateTime);
	tui_label_timeinfo->setValue(core->FlagShowTuiShortInfo);

	// 6. Reference points
	tui_refpoints_cardinalpoints->setValue(core->FlagCardinalPoints);
	tui_refpoints_ecliptic->setValue(core->FlagEclipticLine);
	tui_refpoints_equator->setValue(core->FlagEquatorLine);
	tui_refpoints_equatorialgrid->setValue(core->FlagEquatorialGrid);
	tui_refpoints_azimutalgrid->setValue(core->FlagAzimutalGrid);

	// 7. Effect
	tui_effect_atmosphere->setValue(core->FlagAtmosphere);
}

// Launch script to set time zone in the system locales
void stel_ui::tui_cb_settimezone(void)
{
	//	TODO
}

// Launch script to set system time
void stel_ui::tui_cb_actualtime(void)
{
	//	TODO
}

// 8. Administration actions functions

// Load default configuration
void stel_ui::tui_cb_admin_load_default(void)
{
	core->load_config();
}

// Load default configuration
void stel_ui::tui_cb_admin_save_default(void)
{
	core->save_config();
}

// Set locale parameter (LANG)
void stel_ui::tui_cb_admin_set_locale(void)
{
	//	TODO
	// Use localeconv() from clocal header
}

// Launch script for internet update
void stel_ui::tui_cb_admin_updateme(void)
{
	//	TODO
}

// Set a new landscape skin
void stel_ui::tui_cb_tui_effect_change_landscape(void)
{
	core->set_landscape(tui_effect_landscape->getCurrent());
}
