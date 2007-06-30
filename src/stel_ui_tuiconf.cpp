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
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "SolarSystem.hpp"
#include "NebulaMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "script_mgr.h"
#include "stel_command_interface.h"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "MovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "Navigator.hpp"
#include "Observer.hpp"
#include "StelFileMgr.hpp"
#include "InitParser.hpp"

// Draw simple gravity text ui.
void StelUI::draw_gravity_ui(void)
{
	// Normal transparency mode
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	int x = core->getProjection()->getViewportPosX() + core->getProjection()->getViewportWidth()/2;
	int y = core->getProjection()->getViewportPosY() + core->getProjection()->getViewportHeight()/2;
	//	int shift = (int)(M_SQRT2 / 2 * MY_MIN(core->getViewportWidth()/2, core->getViewportHeight()/2));
	int shift = MY_MIN(core->getProjection()->getViewportWidth()/2, core->getProjection()->getViewportHeight()/2);

	if (FlagShowTuiDateTime)
	{
		double jd = core->getNavigation()->getJDay();
		wostringstream os;

		os << app->getLocaleMgr().get_printable_date_local(jd) << L" " << app->getLocaleMgr().get_printable_time_local(jd);

		// label location if not on earth
		if(core->getObservatory()->getHomePlanetEnglishName() != "Earth") {
			os << L" " << _(core->getObservatory()->getHomePlanetEnglishName());
		}

		if (FlagShowFov) os << L" fov " << setprecision(3) << core->getProjection()->getFov();
		if (FlagShowFps) os << L"  FPS " << app->getFps();

		glColor3f(0.5,1,0.5);
		core->getProjection()->drawText(tuiFont, x - shift + 38, y - 38, os.str(), 0, 0, 0, false);
	}

	if (app->getStelObjectMgr().getWasSelected() && FlagShowTuiShortObjInfo)
	{
	    wstring info = app->getStelObjectMgr().getSelectedObject()[0]->getShortInfoString(core->getNavigation());
		glColor3fv(app->getStelObjectMgr().getSelectedObject()[0]->getInfoColor());
		core->getProjection()->drawText(tuiFont, x + shift - 38, y + 38, info, 0, 0, 0, false);
	}
}

// Create all the components of the text user interface
// should be safe to call more than once but not recommended
// since lose states - try localizeTui() instead
void StelUI::init_tui(void)
{
	LandscapeMgr* lmgr = (LandscapeMgr*)app->getModuleMgr().getModule("landscape");
	
	// Menu root branch
	ScriptDirectoryRead = 0;

	// If already initialized before, delete existing objects
	if(tui_root) delete tui_root;
	tui_root = new s_tui::Branch();

	// Submenus
	tui_menu_location = new s_tui::MenuBranch(wstring(L"1. ") );
	tui_menu_time = new s_tui::MenuBranch(wstring(L"2. ") );
	tui_menu_general = new s_tui::MenuBranch(wstring(L"3. ") );
	tui_menu_stars = new s_tui::MenuBranch(wstring(L"4. ") );
	tui_menu_colors = new s_tui::MenuBranch(wstring(L"5. ") );
	tui_menu_effects = new s_tui::MenuBranch(wstring(L"6. ") );
	tui_menu_scripts = new s_tui::MenuBranch(wstring(L"7. ") );
	tui_menu_administration = new s_tui::MenuBranch(wstring(L"8. ") );

	tui_root->addComponent(tui_menu_location);
	tui_root->addComponent(tui_menu_time);
	tui_root->addComponent(tui_menu_general);	
	tui_root->addComponent(tui_menu_stars);
	tui_root->addComponent(tui_menu_colors);
	tui_root->addComponent(tui_menu_effects);
	tui_root->addComponent(tui_menu_scripts);
	tui_root->addComponent(tui_menu_administration);

	// 1. Location
	tui_location_latitude = new s_tui::Decimal_item(-90., 90., 0.,wstring(L"1.1 ") );
	tui_location_latitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_setlocation));
	tui_location_longitude = new s_tui::Decimal_item(-180., 180., 0.,wstring(L"1.2 ") );
	tui_location_longitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_setlocation));
	tui_location_altitude = new s_tui::Integer_item(-500, 10000, 0,wstring(L"1.3 ") );
	tui_location_altitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_setlocation));

	// Home planet only changed if hit enter to accept because
	// switching planet instantaneously as select is hard on a planetarium audience
	tui_location_planet = new s_tui::MultiSet2_item<wstring>(wstring(L"1.4 ") );
	SolarSystem* ssmgr = (SolarSystem*)app->getModuleMgr().getModule("ssystem");
	tui_location_planet->addItemList(ssmgr->getPlanetHashString());
	//	tui_location_planet->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_location_change_planet));
	tui_location_planet->set_OnTriggerCallback(callback<void>(this, &StelUI::tui_cb_location_change_planet));

	tui_menu_location->addComponent(tui_location_latitude);
	tui_menu_location->addComponent(tui_location_longitude);
	tui_menu_location->addComponent(tui_location_altitude);
	tui_menu_location->addComponent(tui_location_planet);

	// 2. Time
	tui_time_skytime = new s_tui::Time_item(wstring(L"2.1 ") );
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_sky_time));
	try
	{
		tui_time_settmz = new s_tui::Time_zone_item(app->getFileMgr().findFile("data/zone.tab"), wstring(L"2.2 "));
	}
	catch(exception &e)
	{
		cerr << "ERROR locating zone file: " << e.what() << endl;
	}
	tui_time_settmz->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimezone));
	tui_time_settmz->settz(app->getLocaleMgr().get_custom_tz_name());

	tui_time_day_key = new s_tui::MultiSet2_item<wstring>(wstring(L"2.2 ") );
	tui_time_day_key->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_day_key));

	tui_time_presetskytime = new s_tui::Time_item(wstring(L"2.3 ") );
	tui_time_presetskytime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb1));
	tui_time_startuptime = new s_tui::MultiSet2_item<wstring>(wstring(L"2.4 ") );
	tui_time_startuptime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb1));
	tui_time_displayformat = new s_tui::MultiSet_item<wstring>(wstring(L"2.5 ") );
	tui_time_displayformat->addItem(L"24h");
	tui_time_displayformat->addItem(L"12h");
	tui_time_displayformat->addItem(L"system_default");
	tui_time_displayformat->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimedisplayformat));
	tui_time_dateformat = new s_tui::MultiSet_item<wstring>(wstring(L"2.6 ") );
	tui_time_dateformat->addItem(L"yyyymmdd");
	tui_time_dateformat->addItem(L"ddmmyyyy");
	tui_time_dateformat->addItem(L"mmddyyyy");
	tui_time_dateformat->addItem(L"system_default");
	tui_time_dateformat->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimedisplayformat));

	tui_menu_time->addComponent(tui_time_skytime);
	tui_menu_time->addComponent(tui_time_settmz);
	tui_menu_time->addComponent(tui_time_day_key);
	tui_menu_time->addComponent(tui_time_presetskytime);
	tui_menu_time->addComponent(tui_time_startuptime);
	tui_menu_time->addComponent(tui_time_displayformat);
	tui_menu_time->addComponent(tui_time_dateformat);

	// 3. General settings

	// sky culture goes here
	tui_general_sky_culture = new s_tui::MultiSet2_item<wstring>(wstring(L"3.1 ") );
	tui_general_sky_culture->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_general_change_sky_culture));
	tui_menu_general->addComponent(tui_general_sky_culture);

	tui_general_sky_locale = new s_tui::MultiSet_item<wstring>(wstring(L"3.2 ") );
	tui_general_sky_locale->addItemList(Translator::getAvailableLanguagesNamesNative(app->getFileMgr().getLocaleDir()));

	tui_general_sky_locale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_general_change_sky_locale));
	tui_menu_general->addComponent(tui_general_sky_locale);


	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, wstring(L"4.1 ") );
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_magscale = new s_tui::Decimal_item(0,30, 1, wstring(L"4.2 "), 0.1 );
	tui_star_magscale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, wstring(L"4.3 ") );
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_stars_twinkle = new s_tui::Decimal_item(0., 1., 0.3, wstring(L"4.4 "), 0.1);
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_limitingmag = new s_tui::Decimal_item(0., 7., 6.5, wstring(L"4.5 "), 0.1);
	tui_star_limitingmag->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));

	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_magscale);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);
// TODO	tui_menu_stars->addComponent(tui_star_limitingmag);


	// 5. Colors
	tui_colors_const_line_color = new s_tui::Vector_item(wstring(L"5.1 "));
	tui_colors_const_line_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_const_line_color);

	tui_colors_const_label_color = new s_tui::Vector_item(wstring(L"5.2 "));
	tui_colors_const_label_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_const_label_color);

	tui_colors_const_art_intensity = new s_tui::Decimal_item(0,1,1,wstring(L"5.3 "),0.05);
	tui_colors_const_art_intensity->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_const_art_intensity);

	tui_colors_const_boundary_color = new s_tui::Vector_item(wstring(L"5.4 "));
	tui_colors_const_boundary_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_const_boundary_color);

	tui_colors_cardinal_color = new s_tui::Vector_item(wstring(L"5.5 "));
	tui_colors_cardinal_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_cardinal_color);

	tui_colors_planet_names_color = new s_tui::Vector_item(wstring(L"5.6 "));
	tui_colors_planet_names_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_planet_names_color);

	tui_colors_planet_orbits_color = new s_tui::Vector_item(wstring(L"5.7 "));
	tui_colors_planet_orbits_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_planet_orbits_color);

	tui_colors_object_trails_color = new s_tui::Vector_item(wstring(L"5.8 "));
	tui_colors_object_trails_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_object_trails_color);

	tui_colors_meridian_color = new s_tui::Vector_item(wstring(L"5.9 "));
	tui_colors_meridian_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_meridian_color);

	tui_colors_azimuthal_color = new s_tui::Vector_item(wstring(L"5.10 "));
	tui_colors_azimuthal_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_azimuthal_color);

	tui_colors_equatorial_color = new s_tui::Vector_item(wstring(L"5.11 "));
	tui_colors_equatorial_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_equatorial_color);

	tui_colors_equator_color = new s_tui::Vector_item(wstring(L"5.12 "));
	tui_colors_equator_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_equator_color);

	tui_colors_ecliptic_color = new s_tui::Vector_item(wstring(L"5.13 "));
	tui_colors_ecliptic_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_ecliptic_color);

	tui_colors_nebula_label_color = new s_tui::Vector_item(wstring(L"5.14 "));
	tui_colors_nebula_label_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_nebula_label_color);

	tui_colors_nebula_circle_color = new s_tui::Vector_item(wstring(L"5.15 "));
	tui_colors_nebula_circle_color->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_change_color));
	tui_menu_colors->addComponent(tui_colors_nebula_circle_color);


	// 5. Effects
	tui_effect_light_pollution = new s_tui::Decimal_item(0, 30, 1, wstring(L"5.9 "), 0.1 );
	tui_effect_light_pollution->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_light_pollution);

	tui_effect_landscape = new s_tui::MultiSet_item<wstring>(wstring(L"5.1 ") );
	tui_effect_landscape->addItemList(StelUtils::stringToWstring(lmgr->getLandscapeNames()));

	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);

	tui_effect_manual_zoom = new s_tui::Boolean_item(false, wstring(L"5.2 ") );
	tui_effect_manual_zoom->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_manual_zoom);

	tui_effect_pointobj = new s_tui::Boolean_item(false, wstring(L"5.3 ") );
	tui_effect_pointobj->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_pointobj);

	tui_effect_object_scale = new s_tui::Decimal_item(0, 25, 1, wstring(L"5.4 "), 0.05);
	tui_effect_object_scale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_object_scale);

	tui_effect_milkyway_intensity = new s_tui::Decimal_item(0, 100, 1, wstring(L"5.5 "), .1);
	tui_effect_milkyway_intensity->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects_milkyway_intensity));
	tui_menu_effects->addComponent(tui_effect_milkyway_intensity);

	tui_effect_nebulae_label_magnitude = new s_tui::Decimal_item(0, 100, 1, wstring(L"5.6 "), .5);
	tui_effect_nebulae_label_magnitude->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects_nebulae_label_magnitude));
	tui_menu_effects->addComponent(tui_effect_nebulae_label_magnitude);

	tui_effect_zoom_duration = new s_tui::Decimal_item(1, 10, 2, wstring(L"5.7 ") );
	tui_effect_zoom_duration->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_zoom_duration);

	tui_effect_cursor_timeout = new s_tui::Decimal_item(0, 60, 1, wstring(L"5.8 ") );
	tui_effect_cursor_timeout->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_cursor_timeout);

	tui_effect_landscape_sets_location = new s_tui::Boolean_item(lmgr->getFlagLandscapeSetsLocation(), wstring(L"6.10 "));
	tui_effect_landscape_sets_location->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_landscape_sets_location);

	
	// 6. Scripts
	tui_scripts_local = new s_tui::MultiSet_item<wstring>(wstring(L"6.1 ") );
	//	tui_scripts_local->addItemList(wstring(TUI_SCRIPT_MSG) + wstring(L"\n") 
	//			       + StelUtils::stringToWstring(scripts->get_script_list(core->getDataDir() + "scripts/"))); 
	tui_scripts_local->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_scripts_local));
	tui_menu_scripts->addComponent(tui_scripts_local);

	tui_scripts_removeable = new s_tui::MultiSet_item<wstring>(wstring(L"6.2 ") );
	//	tui_scripts_removeable->addItem(_(TUI_SCRIPT_MSG));
	tui_scripts_removeable->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_scripts_removeable));
	tui_menu_scripts->addComponent(tui_scripts_removeable);


	// 7. Administration
	tui_admin_loaddefault = new s_tui::ActionConfirm_item(wstring(L"7.1 ") );
	tui_admin_loaddefault->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_load_default));
	tui_admin_savedefault = new s_tui::ActionConfirm_item(wstring(L"7.2 ") );
	tui_admin_savedefault->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_save_default));
	tui_admin_shutdown = new s_tui::ActionConfirm_item(wstring(L"7.3 ") );
	tui_admin_shutdown->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_shutdown));
	tui_admin_updateme = new s_tui::ActionConfirm_item(wstring(L"7.4 ") );
	tui_admin_updateme->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_updateme));
	tui_menu_administration->addComponent(tui_admin_loaddefault);
	tui_menu_administration->addComponent(tui_admin_savedefault);
	tui_menu_administration->addComponent(tui_admin_shutdown);
	tui_menu_administration->addComponent(tui_admin_updateme);

	tui_admin_setlocale = new s_tui::MultiSet_item<wstring>(L"7.5 ");
	tui_admin_setlocale->addItemList(Translator::getAvailableLanguagesNamesNative(app->getFileMgr().getLocaleDir()));
	tui_admin_setlocale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_set_locale));
	tui_menu_administration->addComponent(tui_admin_setlocale);

	// Now add in translated labels
	localizeTui();

}

// Update fonts, labels and lists for a new app locale
void StelUI::localizeTui(void)
{
	ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("script_mgr");
	
	cout << "Localizing TUI for locale: " << app->getLocaleMgr().getAppLanguage() << endl;

	if(!tui_root) return;  // not initialized yet

	tui_menu_location->setLabel(wstring(L"1. ") + _("Set Location "));
	tui_menu_time->setLabel(wstring(L"2. ") + _("Set Time "));
	tui_menu_general->setLabel(wstring(L"3. ") + _("General "));
	tui_menu_stars->setLabel(wstring(L"4. ") + _("Stars "));
	tui_menu_colors->setLabel(wstring(L"5. ") + _("Colors "));
	tui_menu_effects->setLabel(wstring(L"6. ") + _("Effects "));
	tui_menu_scripts->setLabel(wstring(L"7. ") + _("Scripts "));
	tui_menu_administration->setLabel(wstring(L"8. ") + _("Administration "));

	// 1. Location
	tui_location_latitude->setLabel(wstring(L"1.1 ") + _("Latitude: "));
	tui_location_longitude->setLabel(wstring(L"1.2 ") + _("Longitude: "));
	tui_location_altitude->setLabel(wstring(L"1.3 ") + _("Altitude (m): "));
	tui_location_planet->setLabel(wstring(L"1.4 ") + _("Solar System Body: "));
	SolarSystem* ssmgr = (SolarSystem*)app->getModuleMgr().getModule("ssystem");
	tui_location_planet->replaceItemList(ssmgr->getPlanetHashString(),0);

	// 2. Time
	tui_time_skytime->setLabel(wstring(L"2.1 ") + _("Sky Time: "));
	tui_time_settmz->setLabel(wstring(L"2.2 ") + _("Set Time Zone: "));
	tui_time_day_key->setLabel(wstring(L"2.3 ") + _("Day keys: "));
	tui_time_day_key->replaceItemList(_("Calendar") + wstring(L"\ncalendar\n") 
					  + _("Sidereal") + wstring(L"\nsidereal\n"), 0);
	tui_time_presetskytime->setLabel(wstring(L"2.4 ") + _("Preset Sky Time: "));
	tui_time_startuptime->setLabel(wstring(L"2.5 ") + _("Sky Time At Start-up: "));
	tui_time_startuptime->replaceItemList(_("Actual Time") + wstring(L"\nActual\n") 
										  + _("Preset Time") + wstring(L"\nPreset\n"), 0);
	tui_time_displayformat->setLabel(wstring(L"2.6 ") + _("Time Display Format: "));
	tui_time_dateformat->setLabel(wstring(L"2.7 ") + _("Date Display Format: "));

	// 3. General settings

	// sky culture goes here
	tui_general_sky_culture->setLabel(wstring(L"3.1 ") + _("Sky Culture: "));
	tui_general_sky_culture->replaceItemList(app->getSkyCultureMgr().getSkyCultureHash(), 0);  // human readable names
	// wcout << "sky culture list is: " << core->getSkyCultureHash() << endl;

	tui_general_sky_locale->setLabel(wstring(L"3.2 ") + _("Sky Language: "));

	// 4. Stars
	tui_stars_show->setLabel(wstring(L"4.1 ") + _("Show: "), _("Yes"),_("No"));
	tui_star_magscale->setLabel(wstring(L"4.2 ") + _("Star Value Multiplier: "));
	tui_star_labelmaxmag->setLabel(wstring(L"4.3 ") + _("Maximum Magnitude to Label: "));
	tui_stars_twinkle->setLabel(wstring(L"4.4 ") + _("Twinkling: "));
	tui_star_limitingmag->setLabel(wstring(L"4.5 ") + _("Limiting Magnitude: "));

	// 5. Colors
	tui_colors_const_line_color->setLabel(wstring(L"5.1 ") + _("Constellation Lines") + L": ");
	tui_colors_const_label_color->setLabel(wstring(L"5.2 ") + _("Constellation Names")+L": ");
	tui_colors_const_art_intensity->setLabel(wstring(L"5.3 ") + _("Constellation Art Intensity") + L": ");
	tui_colors_const_boundary_color->setLabel(wstring(L"5.4 ") + _("Constellation Boundaries") + L": ");
	tui_colors_cardinal_color->setLabel(wstring(L"5.5 ") + _("Cardinal Points")+ L": ");
	tui_colors_planet_names_color->setLabel(wstring(L"5.6 ") + _("Planet Names") + L": ");
	tui_colors_planet_orbits_color->setLabel(wstring(L"5.7 ") + _("Planet Orbits") + L": ");
	tui_colors_object_trails_color->setLabel(wstring(L"5.8 ") + _("Planet Trails") + L": ");
	tui_colors_meridian_color->setLabel(wstring(L"5.9 ") + _("Meridian Line") + L": ");
	tui_colors_azimuthal_color->setLabel(wstring(L"5.10 ") + _("Azimuthal Grid") + L": ");
	tui_colors_equatorial_color->setLabel(wstring(L"5.11 ") + _("Equatorial Grid") + L": ");
	tui_colors_equator_color->setLabel(wstring(L"5.12 ") + _("Equator Line") + L": ");
	tui_colors_ecliptic_color->setLabel(wstring(L"5.13 ") + _("Ecliptic Line") + L": ");
	tui_colors_nebula_label_color->setLabel(wstring(L"5.14 ") + _("Nebula Names") + L": ");
	tui_colors_nebula_circle_color->setLabel(wstring(L"5.15 ") + _("Nebula Circles") + L": ");


	// 6. Effects
	tui_effect_light_pollution->setLabel(wstring(L"6.1 ") + _("Light Pollution Luminance: "));
	tui_effect_landscape->setLabel(wstring(L"6.2 ") + _("Landscape: "));
	tui_effect_manual_zoom->setLabel(wstring(L"6.3 ") + _("Manual zoom: "), _("Yes"),_("No"));
	tui_effect_pointobj->setLabel(wstring(L"6.4 ") + _("Object Sizing Rule: "), _("Point"),_("Magnitude"));
	tui_effect_object_scale->setLabel(wstring(L"6.5 ") + _("Magnitude Scaling Multiplier: "));
	tui_effect_milkyway_intensity->setLabel(wstring(L"6.6 ") + _("Milky Way intensity: "));
	tui_effect_nebulae_label_magnitude->setLabel(wstring(L"6.7 ") + _("Maximum Nebula Magnitude to Label: "));
	tui_effect_zoom_duration->setLabel(wstring(L"6.8 ") + _("Zoom Duration: "));
	tui_effect_cursor_timeout->setLabel(wstring(L"6.9 ") + _("Cursor Timeout: "));
	tui_effect_landscape_sets_location->setLabel(wstring(L"6.10 ") + _("Setting Landscape Sets Location: "), _("Yes"),_("No"));

	// 7. Scripts
	tui_scripts_local->setLabel(wstring(L"7.1 ") + _("Local Script: "));
	tui_scripts_local->replaceItemList(_(TUI_SCRIPT_MSG) + wstring(L"\n") 
			+ StelUtils::stringToWstring(scripts->get_script_list("scripts")), 0); 
	tui_scripts_removeable->setLabel(wstring(L"7.2 ") + _("CD/DVD Script: "));
	tui_scripts_removeable->replaceItemList(_(TUI_SCRIPT_MSG), 0);

	// 8. Administration
	tui_admin_loaddefault->setLabel(wstring(L"8.1 ") + _("Load Default Configuration: "));
	tui_admin_savedefault->setLabel(wstring(L"8.2 ") + _("Save Current Configuration as Default: "));
	tui_admin_shutdown->setLabel(wstring(L"8.3 ") + _("Shut Down: "));
	tui_admin_updateme->setLabel(wstring(L"8.4 ") + _("Update me via Internet: "));
	tui_admin_setlocale->setLabel(wstring(L"8.5 ") + _("Set UI Locale: "));
}



// Display the tui
void StelUI::draw_tui(void)
{

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	int x = core->getProjection()->getViewportPosX() + core->getProjection()->getViewportWidth()/2;
	int y = core->getProjection()->getViewportPosY() + core->getProjection()->getViewportHeight()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(core->getProjection()->getViewportWidth()/2, core->getProjection()->getViewportHeight()/2));
	
	if(!core->getProjection()->getFlagGravityLabels()) {
		// for horizontal tui move to left edge of screen kludge
		shift = 0;
		x = core->getProjection()->getViewportPosX() + int(0.1*core->getProjection()->getViewportWidth());
		y = core->getProjection()->getViewportPosY() + int(0.1*core->getProjection()->getViewportHeight());
	}

	if (tui_root)
	{
		glColor3f(0.5,1,0.5);
		core->getProjection()->drawText(tuiFont, x+shift - 30, y-shift + 38, s_tui::stop_active + tui_root->getString(), 0, 0, 0, false);
	}
}

int StelUI::handle_keys_tui(Uint16 key, Uint8 state)
{
	if (!FlagShowTuiMenu)
	{
		return 0;
	}
	if (state==Stel_KEYDOWN && key=='m')
	{
		// leave tui menu
		FlagShowTuiMenu = false;

		// If selected a script in tui, run that now
		if(SelectedScript!="")
		{
			// build up a command string
			string cmd;
			if (SelectedScriptDirectory!="")
				cmd = "script action play filename \"" +  SelectedScript + "\" path \"" + SelectedScriptDirectory + "/scripts/\"";
			else
			{
				try
				{
					string theParent = app->getFileMgr().findFile("scripts/" + SelectedScript);
					theParent = app->getFileMgr().dirName(theParent);
					cmd = "script action play filename \"" + SelectedScript 
						+ "\" path \"" + theParent + "/\"";
				}
				catch(exception& e)
				{
					cerr << "ERROR while executing script " << SelectedScript << ": " << e.what() << endl;
				}
			}
			
			// now execute the command
			StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
			if ( cmd != "" )
				commander->execute_command(cmd);
			else
				cerr << "ERROR while executing script" << endl;
		}

		// clear out now
		SelectedScriptDirectory = SelectedScript = "";
		return 1;
	}
	tui_root->onKey(key, state);
	return 1;
}

// Update all the core parameters with values taken from the tui widgets
void StelUI::tui_cb1(void)
{
	// 2. Date & Time
	core->getNavigation()->setPresetSkyTime(tui_time_presetskytime->getJDay());
	core->getNavigation()->setStartupTimeMode(StelUtils::wstringToString(tui_time_startuptime->getCurrent()));
}

// Update all the tui widgets with values taken from the core parameters
void StelUI::tui_update_widgets(void)
{
	if (!initialised)
		return;

	if (!FlagShowTuiMenu) return;
	
	StarMgr* smgr = (StarMgr*)app->getModuleMgr().getModule("stars");
	ConstellationMgr* cmgr = (ConstellationMgr*)app->getModuleMgr().getModule("constellations");
	NebulaMgr* nmgr = (NebulaMgr*)app->getModuleMgr().getModule("nebulas");
	SolarSystem* ssmgr = (SolarSystem*)app->getModuleMgr().getModule("ssystem");
	MilkyWay* mw = (MilkyWay*)app->getModuleMgr().getModule("milkyway");
	LandscapeMgr* lmgr = (LandscapeMgr*)app->getModuleMgr().getModule("landscape");
	GridLinesMgr* grlmgr = (GridLinesMgr*)app->getModuleMgr().getModule("gridlines");
	MovementMgr* mvmgr = (MovementMgr*)app->getModuleMgr().getModule("movements");
	
	// 1. Location
	tui_location_latitude->setValue(core->getObservatory()->get_latitude());
	tui_location_longitude->setValue(core->getObservatory()->get_longitude());
	tui_location_altitude->setValue(core->getObservatory()->get_altitude());


	// 2. Date & Time
	tui_time_skytime->setJDay(core->getNavigation()->getJDay() + app->getLocaleMgr().get_GMT_shift(core->getNavigation()->getJDay())*JD_HOUR);
	tui_time_settmz->settz(app->getLocaleMgr().get_custom_tz_name());
	tui_time_day_key->setCurrent(StelUtils::stringToWstring(getDayKeyMode()));
	tui_time_presetskytime->setJDay(core->getNavigation()->getPresetSkyTime());
	tui_time_startuptime->setCurrent(StelUtils::stringToWstring(core->getNavigation()->getStartupTimeMode()));
	tui_time_displayformat->setCurrent(StelUtils::stringToWstring(app->getLocaleMgr().get_time_format_str()));
	tui_time_dateformat->setCurrent(StelUtils::stringToWstring(app->getLocaleMgr().get_date_format_str()));

	// 3. general
	tui_general_sky_culture->setValue(StelUtils::stringToWstring(app->getSkyCultureMgr().getSkyCultureDir()));
	tui_general_sky_locale->setValue(StelUtils::stringToWstring(app->getLocaleMgr().getSkyLanguage()));

	// 4. Stars
	tui_stars_show->setValue(smgr->getFlagStars());
	tui_star_labelmaxmag->setValue(smgr->getMaxMagName());
	tui_stars_twinkle->setValue(smgr->getTwinkleAmount());
	tui_star_magscale->setValue(smgr->getMagScale());
// TODO	tui_star_limitingmag->setValue(core->getStarLimitingMag());


	// 5. Colors
	tui_colors_const_line_color->setVector(cmgr->getLinesColor());
	tui_colors_const_label_color->setVector(cmgr->getNamesColor());
	tui_colors_const_art_intensity->setValue(cmgr->getArtIntensity());
	tui_colors_const_boundary_color->setVector(cmgr->getBoundariesColor());
	tui_colors_cardinal_color->setVector(lmgr->getColorCardinalPoints());
	tui_colors_planet_names_color->setVector(ssmgr->getNamesColor());
	tui_colors_planet_orbits_color->setVector(ssmgr->getOrbitsColor());
	tui_colors_object_trails_color->setVector(ssmgr->getTrailsColor());
	tui_colors_meridian_color->setVector(grlmgr->getColorMeridianLine());
	tui_colors_azimuthal_color->setVector(grlmgr->getColorAzimutalGrid());
	tui_colors_equatorial_color->setVector(grlmgr->getColorEquatorGrid());
	tui_colors_equator_color->setVector(grlmgr->getColorEquatorLine());
	tui_colors_ecliptic_color->setVector(grlmgr->getColorEclipticLine());
	tui_colors_nebula_label_color->setVector(nmgr->getNamesColor());
	tui_colors_nebula_circle_color->setVector(nmgr->getCirclesColor());


	// 6. effects
	tui_effect_light_pollution->setValue(lmgr->getAtmosphereLightPollutionLuminance());
	tui_effect_landscape->setValue(lmgr->getLandscapeName());
	tui_effect_pointobj->setValue(smgr->getFlagPointStar());
	tui_effect_zoom_duration->setValue(mvmgr->getAutomoveDuration());
	tui_effect_manual_zoom->setValue(mvmgr->getFlagManualAutoZoom());
	tui_effect_object_scale->setValue(smgr->getScale());
	tui_effect_milkyway_intensity->setValue(mw->getIntensity());
	tui_effect_cursor_timeout->setValue(MouseCursorTimeout);
	tui_effect_nebulae_label_magnitude->setValue(nmgr->getMaxMagHints());
	tui_effect_landscape_sets_location->setValue(lmgr->getFlagLandscapeSetsLocation());


	// 7. Scripts
	// each fresh time enter needs to reset to select message
	if(SelectedScript=="") {
		tui_scripts_local->setCurrent(_(TUI_SCRIPT_MSG));
		
		if(ScriptDirectoryRead) {
			tui_scripts_removeable->setCurrent(_(TUI_SCRIPT_MSG));
		} else {
			// no directory mounted, so put up message
			tui_scripts_removeable->replaceItemList(_("Arrow down to load list."),0);
		}
	}

	// 8. admin
	tui_admin_setlocale->setValue( StelUtils::stringToWstring(app->getLocaleMgr().getAppLanguage()) );

}

// Launch script to set time zone in the system locales
// TODO : this works only if the system manages the TZ environment
// variables of the form "Europe/Paris". This is not the case on windows
// so everything migth have to be re-done internaly :(
void StelUI::tui_cb_settimezone(void)
{
	// Don't call the script anymore coz it's pointless
	// system( ( core->getDataDir() + "script_set_time_zone " + tui_time_settmz->getCurrent() ).c_str() );
	app->getLocaleMgr().set_custom_tz_name(tui_time_settmz->gettz());
}

// Set time format mode
void StelUI::tui_cb_settimedisplayformat(void)
{
	app->getLocaleMgr().set_time_format_str(StelUtils::wstringToString(tui_time_displayformat->getCurrent()));
	app->getLocaleMgr().set_date_format_str(StelUtils::wstringToString(tui_time_dateformat->getCurrent()));
}

// 7. Administration actions functions

// Load default configuration
void StelUI::tui_cb_admin_load_default(void)
{
	//app->init();
	tuiUpdateIndependentWidgets(); 
}

// Save to default configuration
void StelUI::tui_cb_admin_save_default(void)
{
	try
	{
		saveCurrentConfig(app->getConfigFilePath());
	}
	catch(exception& e)
	{
		cerr << "ERROR: could not save config.ini file: " << e.what() << endl;
	}

	try
	{
		system( (app->getFileMgr().findFile("data/script_save_config ") ).c_str() );
	}
	catch(exception& e)
	{
		cerr << "ERROR while calling script_save_config: " << e.what() << endl;
	}
}

// Launch script for internet update
void StelUI::tui_cb_admin_updateme(void)
{
	try
	{
		system( ( app->getFileMgr().findFile("data/script_internet_update" )).c_str() );
	}
	catch(exception& e)
	{
		cerr << "ERROR while calling script_internet_update: " << e.what() << endl;
	}
}


// Launch script for shutdown, then exit
void StelUI::tui_cb_admin_shutdown(void)
{
	try
	{
		system( ( app->getFileMgr().findFile("data/script_shutdown" )).c_str() );
	}
	catch(exception& e)
	{
		cerr << "ERROR while calling script_shutdown: " << e.what() << endl;
	}
	app->terminateApplication();
}

// Set a new landscape skin
void StelUI::tui_cb_tui_effect_change_landscape(void)
{
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	commander->execute_command(string("set landscape_name \"" +  StelUtils::wstringToString(tui_effect_landscape->getCurrent()) + "\""));
}


// Set a new sky culture
void StelUI::tui_cb_tui_general_change_sky_culture(void) {
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	//	core->setSkyCulture(tui_general_sky_culture->getCurrent());
	commander->execute_command( string("set sky_culture ") + StelUtils::wstringToString(tui_general_sky_culture->getCurrent()));
}

// Set a new sky locale
void StelUI::tui_cb_tui_general_change_sky_locale(void) {
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	// wcout << "set sky locale to " << tui_general_sky_locale->getCurrent() << endl;
	commander->execute_command( string("set sky_locale " + StelUtils::wstringToString(tui_general_sky_locale->getCurrent())));
}


// callback for changing scripts from removeable media
void StelUI::tui_cb_scripts_removeable() {
  if(!ScriptDirectoryRead) {
  		ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("script_mgr");
	  // read scripts from mounted disk
	  string script_list = scripts->get_script_list(scripts->get_removable_media_path());
	  tui_scripts_removeable->replaceItemList(_(TUI_SCRIPT_MSG) + wstring(L"\n") + StelUtils::stringToWstring(script_list),0);
	  ScriptDirectoryRead = 1;
  } 

  if(tui_scripts_removeable->getCurrent()==_(TUI_SCRIPT_MSG)) {
	  SelectedScript = "";
  } else {
  		ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("script_mgr");
	  SelectedScript = StelUtils::wstringToString(tui_scripts_removeable->getCurrent());
	  SelectedScriptDirectory = scripts->get_removable_media_path();
	  // to avoid confusing user, clear out local script selection as well
	  tui_scripts_local->setCurrent(_(TUI_SCRIPT_MSG));
  } 
}

/****************************************************************************
 callback for changing scripts from local directory
****************************************************************************/
void StelUI::tui_cb_scripts_local() {  
	if(tui_scripts_local->getCurrent()!=_(TUI_SCRIPT_MSG))
	{
    		SelectedScript = StelUtils::wstringToString(tui_scripts_local->getCurrent());
		SelectedScriptDirectory = "";
    		// to reduce confusion for user, clear out removeable script selection as well
    		if(ScriptDirectoryRead) 
			tui_scripts_removeable->setCurrent(_(TUI_SCRIPT_MSG));
  	} 
	else
	{
    		SelectedScript = "";
  	}
}


// change UI locale
void StelUI::tui_cb_admin_set_locale() {

	app->getLocaleMgr().setAppLanguage(StelUtils::wstringToString(tui_admin_setlocale->getCurrent()));
}


void StelUI::tui_cb_effects_milkyway_intensity() {
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	std::ostringstream oss;
	oss << "set milky_way_intensity " << tui_effect_milkyway_intensity->getValue();
	commander->execute_command(oss.str());
}

void StelUI::tui_cb_setlocation() {
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	std::ostringstream oss;
	oss << "moveto lat " << tui_location_latitude->getValue() 
		<< " lon " <<  tui_location_longitude->getValue()
		<< " alt " << tui_location_altitude->getValue();
	commander->execute_command(oss.str());

}


void StelUI::tui_cb_stars()
{
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	
	// 4. Stars
	std::ostringstream oss;

	oss << "flag stars " << tui_stars_show->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "set max_mag_star_name " << tui_star_labelmaxmag->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_twinkle_amount " << tui_stars_twinkle->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_mag_scale " << tui_star_magscale->getValue();
	commander->execute_command(oss.str());
/* TODO
	oss.str("");
	oss << "set star_limiting_mag " << tui_star_limitingmag->getValue();
	commander->execute_command(oss.str());
*/

}

void StelUI::tui_cb_effects()
{
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	
	// 5. effects
	std::ostringstream oss;

	oss << "flag point_star " << tui_effect_pointobj->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "set auto_move_duration " << tui_effect_zoom_duration->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "flag manual_zoom " << tui_effect_manual_zoom->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_scale " << tui_effect_object_scale->getValue();
	commander->execute_command(oss.str());

	MouseCursorTimeout = tui_effect_cursor_timeout->getValue();  // never recorded

	oss.str("");
	oss << "set light_pollution_luminance " << tui_effect_light_pollution->getValue();
	commander->execute_command(oss.str());

	oss.str("");
	oss << "flag landscape_sets_location " << tui_effect_landscape_sets_location->getValue();
	commander->execute_command(oss.str());

}


// set sky time
void StelUI::tui_cb_sky_time()
{
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	std::ostringstream oss;
	oss << "date local " << StelUtils::wstringToString(tui_time_skytime->getDateString());
	commander->execute_command(oss.str());
}


// set nebula label limit
void StelUI::tui_cb_effects_nebulae_label_magnitude()
{
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");
	std::ostringstream oss;
	oss << "set max_mag_nebula_name " << tui_effect_nebulae_label_magnitude->getValue();
	commander->execute_command(oss.str());
}


void StelUI::tui_cb_change_color()
{
	ConstellationMgr* cmgr = (ConstellationMgr*)app->getModuleMgr().getModule("constellations");
	NebulaMgr* nmgr = (NebulaMgr*)app->getModuleMgr().getModule("nebulas");
	SolarSystem* ssmgr = (SolarSystem*)app->getModuleMgr().getModule("ssystem");
	LandscapeMgr* lmgr = (LandscapeMgr*)app->getModuleMgr().getModule("landscape");
	GridLinesMgr* grlmgr = (GridLinesMgr*)app->getModuleMgr().getModule("gridlines");
	
	cmgr->setLinesColor( tui_colors_const_line_color->getVector() );
	cmgr->setNamesColor( tui_colors_const_label_color->getVector() );
	cmgr->setArtIntensity(tui_colors_const_art_intensity->getValue() );
	cmgr->setBoundariesColor(tui_colors_const_boundary_color->getVector() );
	
	lmgr->setColorCardinalPoints( tui_colors_cardinal_color->getVector() );
	
	ssmgr->setOrbitsColor(tui_colors_planet_orbits_color->getVector() );
	ssmgr->setNamesColor(tui_colors_planet_names_color->getVector() );
	ssmgr->setTrailsColor(tui_colors_object_trails_color->getVector() );
	grlmgr->setColorAzimutalGrid(tui_colors_azimuthal_color->getVector() );
	grlmgr->setColorEquatorGrid(tui_colors_equatorial_color->getVector() );
	grlmgr->setColorEquatorLine(tui_colors_equator_color->getVector() );
	grlmgr->setColorEclipticLine(tui_colors_ecliptic_color->getVector() );
	grlmgr->setColorMeridianLine(tui_colors_meridian_color->getVector() );
	
	nmgr->setNamesColor(tui_colors_nebula_label_color->getVector() );
	nmgr->setCirclesColor(tui_colors_nebula_circle_color->getVector() );
}


void StelUI::tui_cb_location_change_planet()
{
	//	core->setHomePlanet( StelUtils::wstringToString( tui_location_planet->getCurrent() ) );
	//	wcout << "set home planet " << tui_location_planet->getCurrent() << endl;
	StelCommandInterface* commander = (StelCommandInterface*)StelApp::getInstance().getModuleMgr().getModule("command_interface");	
	commander->execute_command(string("set home_planet \"") + 
									StelUtils::wstringToString( tui_location_planet->getCurrent() ) +
									"\"");
}


void StelUI::tui_cb_day_key() 
{

	setDayKeyMode( StelUtils::wstringToString(tui_time_day_key->getCurrent()) );
	//	cout << "Set from tui value DayKeyMode to " << app->DayKeyMode << endl;
}


// Update widgets that don't always match current settings with current settings
void StelUI::tuiUpdateIndependentWidgets(void) { 

	// Called when open tui menu

	// Since some tui options don't immediately affect actual settings
	// reset those options to the current values now
	// (can not do this in tui_update_widgets)

	tui_location_planet->setValue(StelUtils::stringToWstring(core->getObservatory()->getHomePlanetEnglishName()));

	// Reread local script directory (in case new files)
	ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("script_mgr");
	tui_scripts_local->replaceItemList(_(TUI_SCRIPT_MSG) + wstring(L"\n") 
			+ StelUtils::stringToWstring(scripts->get_script_list("scripts")), 0); 

	// also clear out script lists as media may have changed
	ScriptDirectoryRead = 0;

}


// Saves all current settings
void StelUI::saveCurrentConfig(const string& confFile)
{
	// No longer resaves everything, just settings user can change through UI

	StelSkyCultureMgr* skyCultureMgr = &app->getSkyCultureMgr();
	StelLocaleMgr* localeMgr = &app->getLocaleMgr();
	StelModuleMgr* moduleMgr = &app->getModuleMgr();
	GridLinesMgr* grlmgr = (GridLinesMgr*)app->getModuleMgr().getModule("gridlines");
	MovementMgr* mvmgr = (MovementMgr*)app->getModuleMgr().getModule("movements");
	
	cout << "Saving configuration file " << confFile << " ..." << endl;
	InitParser conf;
	conf.load(confFile);

	// Main section
	conf.set_str	("main:version", string(PACKAGE_VERSION));

	// localization section
	conf.set_str    ("localization:sky_culture", skyCultureMgr->getSkyCultureDir());
	conf.set_str    ("localization:sky_locale", localeMgr->getSkyLanguage());
	conf.set_str    ("localization:app_locale", localeMgr->getAppLanguage());
	conf.set_str	("localization:time_display_format", localeMgr->get_time_format_str());
	conf.set_str	("localization:date_display_format", localeMgr->get_date_format_str());
	if (localeMgr->get_tz_format() == StelLocaleMgr::S_TZ_CUSTOM)
	{
		conf.set_str("localization:time_zone", localeMgr->get_custom_tz_name());
	}
	if (localeMgr->get_tz_format() == StelLocaleMgr::S_TZ_SYSTEM_DEFAULT)
	{
		conf.set_str("localization:time_zone", "system_default");
	}
	if (localeMgr->get_tz_format() == StelLocaleMgr::S_TZ_GMT_SHIFT)
	{
		conf.set_str("localization:time_zone", "gmt+x");
	}

	// viewing section
	ConstellationMgr* cmgr = (ConstellationMgr*)moduleMgr->getModule("constellations");
	conf.set_boolean("viewing:flag_constellation_drawing", cmgr->getFlagLines());
	conf.set_boolean("viewing:flag_constellation_name", cmgr->getFlagNames());
	conf.set_boolean("viewing:flag_constellation_art", cmgr->getFlagArt());
	conf.set_boolean("viewing:flag_constellation_boundaries", cmgr->getFlagBoundaries());
	conf.set_boolean("viewing:flag_constellation_pick", cmgr->getFlagIsolateSelected());
	conf.set_double ("viewing:constellation_art_intensity", cmgr->getArtIntensity());
	conf.set_double ("viewing:constellation_art_fade_duration", cmgr->getArtFadeDuration());
	conf.set_str    ("color:const_lines_color", StelUtils::vec3f_to_str(cmgr->getLinesColor()));
	conf.set_str    ("color:const_names_color", StelUtils::vec3f_to_str(cmgr->getNamesColor()));
	conf.set_str    ("color:const_boundary_color", StelUtils::vec3f_to_str(cmgr->getBoundariesColor()));
		
	SolarSystem* ssmgr = (SolarSystem*)moduleMgr->getModule("ssystem");
	LandscapeMgr* lmgr = (LandscapeMgr*)moduleMgr->getModule("landscape");
	conf.set_double("viewing:moon_scale", ssmgr->getMoonScale());
	conf.set_boolean("viewing:flag_equatorial_grid", grlmgr->getFlagEquatorGrid());
	conf.set_boolean("viewing:flag_azimutal_grid", grlmgr->getFlagAzimutalGrid());
	conf.set_boolean("viewing:flag_equator_line", grlmgr->getFlagEquatorLine());
	conf.set_boolean("viewing:flag_ecliptic_line", grlmgr->getFlagEclipticLine());
	conf.set_boolean("viewing:flag_cardinal_points", lmgr->getFlagCardinalsPoints());
	conf.set_boolean("viewing:flag_meridian_line", grlmgr->getFlagMeridianLine());
	conf.set_boolean("viewing:flag_moon_scaled", ssmgr->getFlagMoonScale());
	conf.set_double("viewing:light_pollution_luminance", lmgr->getAtmosphereLightPollutionLuminance());

	// Landscape section
	conf.set_boolean("landscape:flag_landscape", lmgr->getFlagLandscape());
	conf.set_boolean("landscape:flag_atmosphere", lmgr->getFlagAtmosphere());
	conf.set_boolean("landscape:flag_fog", lmgr->getFlagFog());
	//	conf.set_double ("viewing:atmosphere_fade_duration", core->getAtmosphereFadeDuration());

	// Star section
	StarMgr* smgr = (StarMgr*)moduleMgr->getModule("stars");
	conf.set_double ("stars:star_scale", smgr->getScale());
	conf.set_double ("stars:star_mag_scale", smgr->getMagScale());
	conf.set_boolean("stars:flag_point_star", smgr->getFlagPointStar());
	conf.set_double ("stars:max_mag_star_name", smgr->getMaxMagName());
	conf.set_boolean("stars:flag_star_twinkle", smgr->getFlagTwinkle());
	conf.set_double ("stars:star_twinkle_amount", smgr->getTwinkleAmount());
	conf.set_boolean("astro:flag_stars", smgr->getFlagStars());
	conf.set_boolean("astro:flag_star_name", smgr->getFlagNames());

	// Color section
	NebulaMgr* nmgr = (NebulaMgr*)moduleMgr->getModule("nebulas");
	conf.set_str    ("color:azimuthal_color", StelUtils::vec3f_to_str(grlmgr->getColorAzimutalGrid()));
	conf.set_str    ("color:equatorial_color", StelUtils::vec3f_to_str(grlmgr->getColorEquatorGrid()));
	conf.set_str    ("color:equator_color", StelUtils::vec3f_to_str(grlmgr->getColorEquatorLine()));
	conf.set_str    ("color:ecliptic_color", StelUtils::vec3f_to_str(grlmgr->getColorEclipticLine()));
	conf.set_str    ("color:meridian_color", StelUtils::vec3f_to_str(grlmgr->getColorMeridianLine()));
	conf.set_str	("color:nebula_label_color", StelUtils::vec3f_to_str(nmgr->getNamesColor()));
	conf.set_str	("color:nebula_circle_color", StelUtils::vec3f_to_str(nmgr->getCirclesColor()));
	conf.set_str    ("color:cardinal_color", StelUtils::vec3f_to_str(lmgr->getColorCardinalPoints()));
	conf.set_str    ("color:planet_names_color", StelUtils::vec3f_to_str(ssmgr->getNamesColor()));
	conf.set_str    ("color:planet_orbits_color", StelUtils::vec3f_to_str(ssmgr->getOrbitsColor()));
	conf.set_str    ("color:object_trails_color", StelUtils::vec3f_to_str(ssmgr->getTrailsColor()));

	// gui section
	conf.set_double("gui:mouse_cursor_timeout",getMouseCursorTimeout());
	conf.set_str	("gui:day_key_mode", getDayKeyMode());

	// Text ui section
	conf.set_boolean("tui:flag_show_gravity_ui", getFlagShowGravityUi());
	conf.set_boolean("tui:flag_show_tui_datetime", getFlagShowTuiDateTime());
	conf.set_boolean("tui:flag_show_tui_short_obj_info", getFlagShowTuiShortObjInfo());

	// Navigation section
	conf.set_boolean("navigation:flag_manual_zoom", mvmgr->getFlagManualAutoZoom());
	conf.set_double ("navigation:auto_move_duration", mvmgr->getAutoMoveDuration());
	conf.set_double ("navigation:zoom_speed", mvmgr->getZoomSpeed());
	conf.set_double ("navigation:preset_sky_time", core->getNavigation()->getPresetSkyTime());
	conf.set_str	("navigation:startup_time_mode", core->getNavigation()->getStartupTimeMode());

	// Astro section
	conf.set_boolean("astro:flag_object_trails", ssmgr->getFlagTrails());
	conf.set_boolean("astro:flag_bright_nebulae", nmgr->getFlagBright());
	conf.set_boolean("astro:flag_nebula", nmgr->getFlagShow());
	conf.set_boolean("astro:flag_nebula_name", nmgr->getFlagHints());
	conf.set_double("astro:max_mag_nebula_name", nmgr->getMaxMagHints());
	conf.set_boolean("astro:flag_planets", ssmgr->getFlagPlanets());
	conf.set_boolean("astro:flag_planets_hints", ssmgr->getFlagHints());
	conf.set_boolean("astro:flag_planets_orbits", ssmgr->getFlagOrbits());

	MilkyWay* mw = (MilkyWay*)moduleMgr->getModule("milkyway");
	conf.set_boolean("astro:flag_milky_way", mw->getFlagShow());
	conf.set_double("astro:milky_way_intensity", mw->getIntensity());

	// Get landscape and other observatory info
	// TODO: shouldn't observator already know what section to save in?
	core->getObservatory()->setConf(conf, "init_location");

	conf.save(confFile);
}


