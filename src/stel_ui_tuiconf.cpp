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

	int x = core->getViewportPosX() + core->getViewportWidth()/2;
	int y = core->getViewportPosY() + core->getViewportHeight()/2;
	//	int shift = (int)(M_SQRT2 / 2 * MY_MIN(core->getViewportWidth()/2, core->getViewportHeight()/2));
	int shift = MY_MIN(core->getViewportWidth()/2, core->getViewportHeight()/2);

	if (FlagShowTuiDateTime)
	{
		double jd = core->getJDay();
		wostringstream os;

		os << core->getObservatory().get_printable_date_local(jd) << L" " <<
		core->getObservatory().get_printable_time_local(jd);

		// label location if not on earth
		if(core->getObservatory().getHomePlanetEnglishName() != "Earth") {
			os << L" " << _(core->getObservatory().getHomePlanetEnglishName());
		}

		if (FlagShowFov) os << L" fov " << setprecision(3) << core->getFov();
		if (FlagShowFps) os << L"  FPS " << app->fps;

		glColor3f(0.5,1,0.5);
		core->printGravity(tuiFont, x - shift + 38, y - 38, os.str(), 0);
	}

	if (core->getFlagHasSelected() && FlagShowTuiShortObjInfo)
	{
	    wstring info = core->getSelectedObjectShortInfo();
		glColor3fv(core->getSelectedObjectInfoColor());
		core->printGravity(tuiFont, x + shift - 38, 
						   y + 38, info, 0);
	}
}

// Create all the components of the text user interface
// should be safe to call more than once but not recommended
// since lose states - try localizeTui() instead
void StelUI::init_tui(void)
{
	// Menu root branch
	ScriptDirectoryRead = 0;

	// If already initialized before, delete existing objects
	if(tui_root) delete tui_root;
	if(tuiFont) delete tuiFont;

	// Load standard font based on app locale
	string fontFile, tmpstr;
	float fontScale, tmpfloat;

	core->getFontForLocale(app->getAppLanguage(), fontFile, fontScale, tmpstr, tmpfloat);

	tuiFont = new s_font(BaseFontSize*fontScale, fontFile);
	if (!tuiFont)
	{
		printf("ERROR WHILE CREATING FONT\n");
		exit(-1);
	}

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

	// experimental
	tui_location_planet = new s_tui::MultiSet2_item<wstring>(wstring(L"1.4 ") );
	tui_location_planet->addItemList(core->getPlanetHashString());
	tui_location_planet->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_location_change_planet));

	tui_menu_location->addComponent(tui_location_latitude);
	tui_menu_location->addComponent(tui_location_longitude);
	tui_menu_location->addComponent(tui_location_altitude);
	tui_menu_location->addComponent(tui_location_planet);

	// 2. Time
	tui_time_skytime = new s_tui::Time_item(wstring(L"2.1 ") );
	tui_time_skytime->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_sky_time));
	tui_time_settmz = new s_tui::Time_zone_item(core->getDataDir() + "zone.tab", wstring(L"2.2 ") );
	tui_time_settmz->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_settimezone));
	tui_time_settmz->settz(core->getObservatory().get_custom_tz_name());
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
	tui_general_sky_locale->addItemList(StelUtility::stringToWstring(Translator::getAvailableLanguagesCodes(core->getLocaleDir())));

	tui_general_sky_locale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_general_change_sky_locale));
	tui_menu_general->addComponent(tui_general_sky_locale);


	// 4. Stars
	tui_stars_show = new s_tui::Boolean_item(false, wstring(L"4.1 ") );
	tui_stars_show->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_magscale = new s_tui::Decimal_item(1,30, 1, wstring(L"4.2 ") );
	tui_star_magscale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_star_labelmaxmag = new s_tui::Decimal_item(-1.5, 10., 2, wstring(L"4.3 ") );
	tui_star_labelmaxmag->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));
	tui_stars_twinkle = new s_tui::Decimal_item(0., 1., 0.3, wstring(L"4.4 "), 0.1);
	tui_stars_twinkle->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_stars));

	tui_menu_stars->addComponent(tui_stars_show);
	tui_menu_stars->addComponent(tui_star_magscale);
	tui_menu_stars->addComponent(tui_star_labelmaxmag);
	tui_menu_stars->addComponent(tui_stars_twinkle);

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
	tui_effect_landscape = new s_tui::MultiSet_item<wstring>(wstring(L"5.1 ") );
	tui_effect_landscape->addItemList(StelUtility::stringToWstring(Landscape::get_file_content(core->getDataDir() + "landscapes.ini")));

	tui_effect_landscape->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_tui_effect_change_landscape));
	tui_menu_effects->addComponent(tui_effect_landscape);

	tui_effect_manual_zoom = new s_tui::Boolean_item(false, wstring(L"5.2 ") );
	tui_effect_manual_zoom->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_manual_zoom);

	tui_effect_pointobj = new s_tui::Boolean_item(false, wstring(L"5.3 ") );
	tui_effect_pointobj->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_pointobj);

	tui_effect_object_scale = new s_tui::Decimal_item(0, 25, 1, wstring(L"5.4 ") + _("Magnitude Sizing Multiplier: "), 0.1);
	tui_effect_object_scale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_effects));
	tui_menu_effects->addComponent(tui_effect_object_scale);

	tui_effect_milkyway_intensity = new s_tui::Decimal_item(0, 100, 1, wstring(L"5.5 "), .5);
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


	// 6. Scripts
	tui_scripts_local = new s_tui::MultiSet_item<wstring>(wstring(L"6.1 ") );
	//	tui_scripts_local->addItemList(wstring(TUI_SCRIPT_MSG) + wstring(L"\n") 
	//			       + StelUtility::stringToWstring(app->scripts->get_script_list(core->getDataDir() + "scripts/"))); 
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
	tui_admin_updateme = new s_tui::Action_item(wstring(L"7.3 ") );
	tui_admin_updateme->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_updateme));
	tui_menu_administration->addComponent(tui_admin_loaddefault);
	tui_menu_administration->addComponent(tui_admin_savedefault);
	tui_menu_administration->addComponent(tui_admin_updateme);

	tui_admin_setlocale = new s_tui::MultiSet_item<wstring>(L"7.4 ");
	tui_admin_setlocale->addItemList(StelUtility::stringToWstring(Translator::getAvailableLanguagesCodes(core->getLocaleDir())));
	tui_admin_setlocale->set_OnChangeCallback(callback<void>(this, &StelUI::tui_cb_admin_set_locale));
	tui_menu_administration->addComponent(tui_admin_setlocale);

	// Now add in translated labels
	localizeTui();

}

// Update fonts, labels and lists for a new app locale
void StelUI::localizeTui(void)
{

	cout << "Localizing TUI for locale: " << app->getAppLanguage() << endl;
	if(tuiFont) delete tuiFont;

	// Load standard font based on app locale
	string fontFile, tmpstr;
	float fontScale, tmpfloat;

	core->getFontForLocale(app->getAppLanguage(), fontFile, fontScale, tmpstr, tmpfloat);

	tuiFont = new s_font(BaseFontSize*fontScale, fontFile);
	if (!tuiFont)
	{
		printf("ERROR WHILE CREATING FONT\n");
		exit(-1);
	}

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
	tui_location_planet->replaceItemList(core->getPlanetHashString(),0);

	// 2. Time
	tui_time_skytime->setLabel(wstring(L"2.1 ") + _("Sky Time: "));
	tui_time_settmz->setLabel(wstring(L"2.2 ") + _("Set Time Zone: "));
	tui_time_presetskytime->setLabel(wstring(L"2.3 ") + _("Preset Sky Time: "));
	tui_time_startuptime->setLabel(wstring(L"2.4 ") + _("Sky Time At Start-up: "));
	tui_time_startuptime->replaceItemList(_("Actual Time") + wstring(L"\nActual\n") 
										  + _("Preset Time") + wstring(L"\nPreset\n"), 0);
	tui_time_displayformat->setLabel(wstring(L"2.5 ") + _("Time Display Format: "));
	tui_time_dateformat->setLabel(wstring(L"2.6 ") + _("Date Display Format: "));

	// 3. General settings

	// sky culture goes here
	tui_general_sky_culture->setLabel(wstring(L"3.1 ") + _("Sky Culture: "));
	tui_general_sky_culture->replaceItemList(core->getSkyCultureHash(), 0);  // human readable names
	// wcout << "sky culture list is: " << core->getSkyCultureHash() << endl;

	tui_general_sky_locale->setLabel(wstring(L"3.2 ") + _("Sky Language: "));

	// 4. Stars
	tui_stars_show->setLabel(wstring(L"4.1 ") + _("Show: "), _("Yes"),_("No"));
	tui_star_magscale->setLabel(wstring(L"4.2 ") + _("Star Magnitude Multiplier: "));
	tui_star_labelmaxmag->setLabel(wstring(L"4.3 ") + _("Maximum Magnitude to Label: "));
	tui_stars_twinkle->setLabel(wstring(L"4.4 ") + _("Twinkling: "));

	// 5. Colors
	tui_colors_const_line_color->setLabel(wstring(L"5.1 ") + _("Constellation Lines") + L": ");
	tui_colors_const_label_color->setLabel(wstring(L"5.2 ") + _("Constellation Names")+L": ");
	tui_colors_const_art_intensity->setLabel(wstring(L"5.3 ") + _("Constellation Art Intensity") + L": ");
	tui_colors_const_boundary_color->setLabel(wstring(L"5.4 ") + _("Constellation Boundaries") + L": ");
	tui_colors_cardinal_color->setLabel(wstring(L"5.5 ") + _("Cardinal Points")+L": " + L": ");
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
	tui_effect_landscape->setLabel(wstring(L"6.1 ") + _("Landscape: "));
	tui_effect_manual_zoom->setLabel(wstring(L"6.2 ") + _("Manual zoom: "), _("Yes"),_("No"));
	tui_effect_pointobj->setLabel(wstring(L"6.3 ") + _("Object Sizing Rule: "), _("Point"),_("Magnitude"));
	tui_effect_object_scale->setLabel(wstring(L"6.4 ") + _("Magnitude Sizing Multiplier: "));
	tui_effect_milkyway_intensity->setLabel(wstring(L"6.5 ") + _("Milky Way intensity: "));
	tui_effect_nebulae_label_magnitude->setLabel(wstring(L"6.6 ") + _("Maximum Nebula Magnitude to Label: "));
	tui_effect_zoom_duration->setLabel(wstring(L"6.7 ") + _("Zoom Duration: "));
	tui_effect_cursor_timeout->setLabel(wstring(L"6.8 ") + _("Cursor Timeout: "));

	// 7. Scripts
	tui_scripts_local->setLabel(wstring(L"7.1 ") + _("Local Script: "));
	tui_scripts_local->replaceItemList(_(TUI_SCRIPT_MSG) + wstring(L"\n") 
									   + StelUtility::stringToWstring(app->scripts->get_script_list(core->getDataDir() + "scripts/")), 0); 
	tui_scripts_removeable->setLabel(wstring(L"7.2 ") + _("CD/DVD Script: "));
	tui_scripts_removeable->replaceItemList(_(TUI_SCRIPT_MSG), 0);

	// 8. Administration
	tui_admin_loaddefault->setLabel(wstring(L"8.1 ") + _("Load Default Configuration: "));
	tui_admin_savedefault->setLabel(wstring(L"8.2 ") + _("Save Current Configuration as Default: "));
	tui_admin_updateme->setLabel(wstring(L"8.3 ") + _("Update me via Internet: "));
	tui_admin_setlocale->setLabel(wstring(L"8.4 ") + _("Set UI Locale: "));
}



// Display the tui
void StelUI::draw_tui(void)
{

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	int x = core->getViewportPosX() + core->getViewportWidth()/2;
	int y = core->getViewportPosY() + core->getViewportHeight()/2;
	int shift = (int)(M_SQRT2 / 2 * MY_MIN(core->getViewportWidth()/2, core->getViewportHeight()/2));
	
	if (tui_root)
	{
		glColor3f(0.5,1,0.5);
		core->printGravity(tuiFont, x+shift - 30, y-shift + 38, s_tui::stop_active + tui_root->getString(), 0);
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
	app->PresetSkyTime 		= tui_time_presetskytime->getJDay();
	app->StartupTimeMode 		= StelUtility::wstringToString(tui_time_startuptime->getCurrent());

}

// Update all the tui widgets with values taken from the core parameters
void StelUI::tui_update_widgets(void)
{
	if (!FlagShowTuiMenu) return;
	
	// 1. Location
	tui_location_latitude->setValue(core->getObservatory().get_latitude());
	tui_location_longitude->setValue(core->getObservatory().get_longitude());
	tui_location_altitude->setValue(core->getObservatory().get_altitude());
	tui_location_planet->setValue(StelUtility::stringToWstring(core->getObservatory().getHomePlanetEnglishName()));

	// 2. Date & Time
	tui_time_skytime->setJDay(core->getJDay() + 
				  core->getObservatory().get_GMT_shift(core->getJDay())*JD_HOUR);
	tui_time_settmz->settz(core->getObservatory().get_custom_tz_name());
	tui_time_presetskytime->setJDay(app->PresetSkyTime);
	tui_time_startuptime->setCurrent(StelUtility::stringToWstring(app->StartupTimeMode));
	tui_time_displayformat->setCurrent(StelUtility::stringToWstring(core->getObservatory().get_time_format_str()));
	tui_time_dateformat->setCurrent(StelUtility::stringToWstring(core->getObservatory().get_date_format_str()));

	// 3. general
	tui_general_sky_culture->setValue(StelUtility::stringToWstring(core->getSkyCultureDir()));
	tui_general_sky_locale->setValue(StelUtility::stringToWstring(core->getSkyLanguage()));

	// 4. Stars
	tui_stars_show->setValue(core->getFlagStars());
	tui_star_labelmaxmag->setValue(core->getMaxMagStarName());
	tui_stars_twinkle->setValue(core->getStarTwinkleAmount());
	tui_star_magscale->setValue(core->getStarMagScale());

	// 5. Colors
	tui_colors_const_line_color->setVector(core->getColorConstellationLine());
	tui_colors_const_label_color->setVector(core->getColorConstellationNames());
	tui_colors_cardinal_color->setVector(core->getColorCardinalPoints());
	tui_colors_const_art_intensity->setValue(core->getConstellationArtIntensity());
	tui_colors_const_boundary_color->setVector(core->getColorConstellationBoundaries());
	tui_colors_planet_names_color->setVector(core->getColorPlanetsNames());
	tui_colors_planet_orbits_color->setVector(core->getColorPlanetsOrbits());
	tui_colors_object_trails_color->setVector(core->getColorPlanetsTrails());
	tui_colors_meridian_color->setVector(core->getColorMeridianLine());
	tui_colors_azimuthal_color->setVector(core->getColorAzimutalGrid());
	tui_colors_equatorial_color->setVector(core->getColorEquatorGrid());
	tui_colors_equator_color->setVector(core->getColorEquatorLine());
	tui_colors_ecliptic_color->setVector(core->getColorEclipticLine());
	tui_colors_nebula_label_color->setVector(core->getColorNebulaLabels());
	tui_colors_nebula_circle_color->setVector(core->getColorNebulaCircle());


	// 6. effects
	tui_effect_landscape->setValue(StelUtility::stringToWstring(core->getObservatory().get_landscape_name()));
	tui_effect_pointobj->setValue(core->getFlagPointStar());
	tui_effect_zoom_duration->setValue(core->getAutomoveDuration());
	tui_effect_manual_zoom->setValue(core->getFlagManualAutoZoom());
	tui_effect_object_scale->setValue(core->getStarScale());
	tui_effect_milkyway_intensity->setValue(core->getMilkyWayIntensity());
	tui_effect_cursor_timeout->setValue(MouseCursorTimeout);
	tui_effect_nebulae_label_magnitude->setValue(core->getNebulaMaxMagHints());


	// 7. Scripts
	// each fresh time enter needs to reset to select message
	if(app->SelectedScript=="") {
		tui_scripts_local->setCurrent(_(TUI_SCRIPT_MSG));
		
		if(ScriptDirectoryRead) {
			tui_scripts_removeable->setCurrent(_(TUI_SCRIPT_MSG));
		} else {
			// no directory mounted, so put up message
			tui_scripts_removeable->replaceItemList(_("Arrow down to load list."),0);
		}
	}

	// 8. admin
	tui_admin_setlocale->setValue( StelUtility::stringToWstring(app->getAppLanguage()) );

}

// Launch script to set time zone in the system locales
// TODO : this works only if the system manages the TZ environment
// variables of the form "Europe/Paris". This is not the case on windows
// so everything migth have to be re-done internaly :(
void StelUI::tui_cb_settimezone(void)
{
	// Don't call the script anymore coz it's pointless
	// system( ( core->getDataDir() + "script_set_time_zone " + tui_time_settmz->getCurrent() ).c_str() );
	core->getObservatory().set_custom_tz_name(tui_time_settmz->gettz());
}

// Set time format mode
void StelUI::tui_cb_settimedisplayformat(void)
{
	core->getObservatory().set_time_format_str(StelUtility::wstringToString(tui_time_displayformat->getCurrent()));
	core->getObservatory().set_date_format_str(StelUtility::wstringToString(tui_time_dateformat->getCurrent()));
}

// 7. Administration actions functions

// Load default configuration
void StelUI::tui_cb_admin_load_default(void)
{
	app->init();
}

// Save to default configuration
void StelUI::tui_cb_admin_save_default(void)
{
	app->saveCurrentConfig(app->getConfigFile());

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
	app->commander->execute_command(string("set landscape_name " +  StelUtility::wstringToString(tui_effect_landscape->getCurrent())));
}


// Set a new sky culture
void StelUI::tui_cb_tui_general_change_sky_culture(void) {

	//	core->setSkyCulture(tui_general_sky_culture->getCurrent());
	app->commander->execute_command( string("set sky_culture ") + StelUtility::wstringToString(tui_general_sky_culture->getCurrent()));
}

// Set a new sky locale
void StelUI::tui_cb_tui_general_change_sky_locale(void) {
	// wcout << "set sky locale to " << tui_general_sky_locale->getCurrent() << endl;
	app->commander->execute_command( string("set sky_locale " + StelUtility::wstringToString(tui_general_sky_locale->getCurrent())));
}


#define SCRIPT_REMOVEABLE_DISK "/tmp/scripts/"

// callback for changing scripts from removeable media
void StelUI::tui_cb_scripts_removeable() {
  
  if(!ScriptDirectoryRead) {
	  // read scripts from mounted disk
	  string script_list = app->scripts->get_script_list(SCRIPT_REMOVEABLE_DISK);
	  tui_scripts_removeable->replaceItemList(_(TUI_SCRIPT_MSG) + wstring(L"\n") + StelUtility::stringToWstring(script_list),0);
	  ScriptDirectoryRead = 1;
  } 

  if(tui_scripts_removeable->getCurrent()==_(TUI_SCRIPT_MSG)) {
	  app->SelectedScript = "";
  } else {
	  app->SelectedScript = StelUtility::wstringToString(tui_scripts_removeable->getCurrent());
	  app->SelectedScriptDirectory = SCRIPT_REMOVEABLE_DISK;
	  // to avoid confusing user, clear out local script selection as well
	  tui_scripts_local->setCurrent(_(TUI_SCRIPT_MSG));
  } 
}


// callback for changing scripts from local directory
void StelUI::tui_cb_scripts_local() {
  
	if(tui_scripts_local->getCurrent()!=_(TUI_SCRIPT_MSG)){
    app->SelectedScript = StelUtility::wstringToString(tui_scripts_local->getCurrent());
    app->SelectedScriptDirectory = core->getDataDir() + "scripts/";
    // to reduce confusion for user, clear out removeable script selection as well
    if(ScriptDirectoryRead) tui_scripts_removeable->setCurrent(_(TUI_SCRIPT_MSG));
  } else {
    app->SelectedScript = "";
  }
}


// change UI locale
void StelUI::tui_cb_admin_set_locale() {

	app->setAppLanguage(StelUtility::wstringToString(tui_admin_setlocale->getCurrent()));
}


void StelUI::tui_cb_effects_milkyway_intensity() {

	std::ostringstream oss;
	oss << "set milky_way_intensity " << tui_effect_milkyway_intensity->getValue();
	app->commander->execute_command(oss.str());
}

void StelUI::tui_cb_setlocation() {
	
	std::ostringstream oss;
	oss << "moveto lat " << tui_location_latitude->getValue() 
		<< " lon " <<  tui_location_longitude->getValue()
		<< " alt " << tui_location_altitude->getValue();
	app->commander->execute_command(oss.str());

}


void StelUI::tui_cb_stars()
{
	// 4. Stars
	std::ostringstream oss;

	oss << "flag stars " << tui_stars_show->getValue();
	app->commander->execute_command(oss.str());

	oss.str("");
	oss << "set max_mag_star_name " << tui_star_labelmaxmag->getValue();
	app->commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_twinkle_amount " << tui_stars_twinkle->getValue();
	app->commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_mag_scale " << tui_star_magscale->getValue();
	app->commander->execute_command(oss.str());

}

void StelUI::tui_cb_effects()
{

	// 5. effects
	std::ostringstream oss;

	oss << "flag point_star " << tui_effect_pointobj->getValue();
	app->commander->execute_command(oss.str());

	oss.str("");
	oss << "set auto_move_duration " << tui_effect_zoom_duration->getValue();
	app->commander->execute_command(oss.str());

	oss.str("");
	oss << "flag manual_zoom " << tui_effect_manual_zoom->getValue();
	app->commander->execute_command(oss.str());

	oss.str("");
	oss << "set star_scale " << tui_effect_object_scale->getValue();
	app->commander->execute_command(oss.str());

	MouseCursorTimeout = tui_effect_cursor_timeout->getValue();  // never recorded

}


// set sky time
void StelUI::tui_cb_sky_time()
{
	std::ostringstream oss;
	oss << "date local " << StelUtility::wstringToString(tui_time_skytime->getDateString());
	app->commander->execute_command(oss.str());
}


// set nebula label limit
void StelUI::tui_cb_effects_nebulae_label_magnitude()
{
	std::ostringstream oss;
	oss << "set max_mag_nebula_name " << tui_effect_nebulae_label_magnitude->getValue();
	app->commander->execute_command(oss.str());
}


void StelUI::tui_cb_change_color()
{
	core->setColorConstellationLine( tui_colors_const_line_color->getVector() );
	core->setColorConstellationNames( tui_colors_const_label_color->getVector() );
	core->setColorCardinalPoints( tui_colors_cardinal_color->getVector() );
	core->setConstellationArtIntensity(tui_colors_const_art_intensity->getValue() );
	core->setColorConstellationBoundaries(tui_colors_const_boundary_color->getVector() );
	// core->setColorStarNames(
	// core->setColorStarCircles(
	core->setColorPlanetsOrbits(tui_colors_planet_orbits_color->getVector() );
	core->setColorPlanetsNames(tui_colors_planet_names_color->getVector() );
	core->setColorPlanetsTrails(tui_colors_object_trails_color->getVector() );
	core->setColorAzimutalGrid(tui_colors_azimuthal_color->getVector() );
	core->setColorEquatorGrid(tui_colors_equatorial_color->getVector() );
	core->setColorEquatorLine(tui_colors_equator_color->getVector() );
	core->setColorEclipticLine(tui_colors_ecliptic_color->getVector() );
	core->setColorMeridianLine(tui_colors_meridian_color->getVector() );
	core->setColorNebulaLabels(tui_colors_nebula_label_color->getVector() );
	core->setColorNebulaCircle(tui_colors_nebula_circle_color->getVector() );


}


void StelUI::tui_cb_location_change_planet()
{
	//	core->setHomePlanet( StelUtility::wstringToString( tui_location_planet->getCurrent() ) );
	app->commander->execute_command(string("set home_planet ") + 
									StelUtility::wstringToString( tui_location_planet->getCurrent() ));
}
