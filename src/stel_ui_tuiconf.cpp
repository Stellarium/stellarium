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


// Draw simple gravity text ui.
void stel_ui::draw_gravity_ui(void)
{
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    static char str[255];
	ln_date d;

	int x = core->projection->view_left() + core->projection->viewW()/2;
	int y = core->projection->view_bottom() + core->projection->viewH()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(x,y));

	if (core->FlagShowTuiDateTime)
	{
		if (core->FlagUTC_Time) get_date(core->navigation->get_JDay(),&d);
		else get_date(core->navigation->get_JDay()+core->navigation->get_time_zone()*JD_HOUR,&d);

		if (core->FlagUTC_Time) sprintf(str,
		"%.2d/%.2d/%.4d %.2d:%.2d:%.2d (UTC)",d.days,d.months,d.years,d.hours,d.minutes,(int)d.seconds);
		else sprintf(str,"%.2d/%.2d/%.4d %.2d:%.2d:%.2d FPS:%4.2f",
			d.days,d.months,d.years,d.hours,d.minutes,(int)d.seconds, core->fps);

		glColor3f(0.1,0.9,0.1);
		core->projection->print_gravity180(spaceFont, x-shift + 10, y-shift + 10, str);
	}

	if (core->selected_object && core->FlagShowTuiShortInfo)
	{
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
	tui_time_settmz = new s_tui::Action_item("2.1 Set Time Zone: ");
	tui_time_skytime = new s_tui::Time_item("2.2 Sky Time: ");
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_presetskytime = new s_tui::Time_item("2.3 Preset Sky Time: ");
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_actual = new s_tui::Action_item("2.4 Set Time to actual: ");
	tui_time_startuptime = new s_tui::Boolean_item(false, "2.5 Sky Time At Start-up: ", "Actual","Preset");
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_time_displayformat = new s_tui::Boolean_item(false, "2.6 Time Display Format: ", "12h","24h");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &stel_ui::tui_cb1));
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_actual);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);

	// 3. Constellations
	tui_constellation_lines = new s_tui::Boolean_item(false, "3.2 Lines: ", "Yes","No");
	tui_constellation_art = new s_tui::Boolean_item(false, "3.3 Art: ", "Yes","No");
	tui_menu_constellation->addComponent(tui_constellation_lines);
	tui_menu_constellation->addComponent(tui_constellation_art);

	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, "4.1 Show: ", "Yes","No");
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, "4.2 Maximum Magnitude to Label: ");
	tui_stars_twinkle = new s_tui::Boolean_item(false, "4.3 Twinkle: ", "Yes","No");
	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);

	// 5. Labels
	tui_label_stars = new s_tui::Boolean_item(false, "5.1 Stars: ", "Yes","No");
	tui_label_constellations = new s_tui::Boolean_item(false, "5.2 Constellations: ", "Yes","No");
	tui_label_nebulas = new s_tui::Boolean_item(false, "5.3 Nebulas: ", "Yes","No");
	tui_label_planets = new s_tui::Boolean_item(false, "5.4 Planets: ", "Yes","No");
	tui_label_timeinfo = new s_tui::Boolean_item(false, "5.5 Time & Object Info: ", "Yes","No");
	tui_menu_labels->addComponent(tui_label_stars);
	tui_menu_labels->addComponent(tui_label_constellations);
	tui_menu_labels->addComponent(tui_label_nebulas);
	tui_menu_labels->addComponent(tui_label_planets);
	tui_menu_labels->addComponent(tui_label_timeinfo);

	// 6. Reference Points
	tui_refpoints_cardinalpoints = new s_tui::Boolean_item(false, "6.1 Cardinal Points: ", "Yes","No");
	tui_refpoints_ecliptic = new s_tui::Boolean_item(false, "6.2 Ecliptic: ", "Yes","No");
	tui_refpoints_equator = new s_tui::Boolean_item(false, "6.3 Equator: ", "Yes","No");
	tui_refpoints_equatorialgrid = new s_tui::Boolean_item(false, "6.4 Equatorial Grid: ", "Yes","No");
	tui_refpoints_azimutalgrid = new s_tui::Boolean_item(false, "6.5 Azimutal Grid: ", "Yes","No");
	tui_menu_refpoints->addComponent(tui_refpoints_cardinalpoints);
	tui_menu_refpoints->addComponent(tui_refpoints_ecliptic);
	tui_menu_refpoints->addComponent(tui_refpoints_equator);
	tui_menu_refpoints->addComponent(tui_refpoints_equatorialgrid);
	tui_menu_refpoints->addComponent(tui_refpoints_azimutalgrid);

	// 7. Effects
	tui_effect_atmosphere = new s_tui::Boolean_item(false, "7.2 Atmopshere: ", "Yes","No");
	tui_menu_effects->addComponent(tui_effect_atmosphere);

	// 8. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item("8.1 Load Default Configuration: ");
	tui_admin_savedefault = new s_tui::ActionConfirm_item("8.2 Save Current Configuration as Default: ");
	tui_admin_setlocal = new s_tui::Action_item("8.3 Set Locale: ");
	tui_admin_updateme = new s_tui::Action_item("8.4 Update me via Internet: ");
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

void stel_ui::tui_cb1(void)
{
	core->navigation->set_latitude(tui_location_latitude->getValue());
	core->navigation->set_longitude(tui_location_longitude->getValue());
	core->navigation->set_altitude(tui_location_altitude->getValue());
	core->navigation->set_JDay(tui_time_skytime->getJDay());

}
