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

#include "stel_ui.h"
#include "stel_utility.h"

using namespace s_gui;

Component* StelUI::createConfigWindow(void)
{
	config_win = new StdBtWin(_("Configuration"));
	config_win->setOpaque(opaqueGUI);
	config_win->reshape(300,200,400,390);
	config_win->setVisible(core->FlagConfig);

	config_tab_ctr = new TabContainer();
	config_tab_ctr->setSize(config_win->getSize());

	// The current drawing position
	int x,y;
	x=70; y=15;

	// Rendering options
	FilledContainer* tab_render = new FilledContainer();
	tab_render->setSize(config_tab_ctr->getSize());

	s_texture* starp = new s_texture("halo");
	Picture* pstar = new Picture(starp, x-50, y+5, 32, 32);
	tab_render->addComponent(pstar);

	stars_cbx = new LabeledCheckBox(core->FlagStars, _("Stars"));
	stars_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(stars_cbx);
	stars_cbx->setPos(x,y); y+=15;

	star_names_cbx = new LabeledCheckBox(core->FlagStarName, _("Star Names. Up to mag :"));
	star_names_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(star_names_cbx);
	star_names_cbx->setPos(x,y);

	max_mag_star_name = new FloatIncDec(courierFont, tex_up, tex_down, -1.5, 9,
		core->MaxMagStarName, 0.5);
	max_mag_star_name->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(max_mag_star_name);
	max_mag_star_name->setPos(x + 220,y);

	y+=15;

	star_twinkle_cbx = new LabeledCheckBox(core->FlagStarTwinkle, _("Star Twinkle. Amount :"));
	star_twinkle_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(star_twinkle_cbx);
	star_twinkle_cbx->setPos(x,y);

	star_twinkle_amount = new FloatIncDec(courierFont, tex_up, tex_down, 0, 0.6,
		core->StarTwinkleAmount, 0.1);
	star_twinkle_amount->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(star_twinkle_amount);
	star_twinkle_amount->setPos(x + 220,y);

	y+=30;

	s_texture* constellp = new s_texture("bt_constellations");
	Picture* pconstell = new Picture(constellp, x-50, y+5, 32, 32);
	tab_render->addComponent(pconstell);

	constellation_cbx = new LabeledCheckBox(false, _("Constellations"));
	constellation_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(constellation_cbx);
	constellation_cbx->setPos(x,y); y+=15;

	constellation_name_cbx = new LabeledCheckBox(false, _("Constellations Names"));
	constellation_name_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(constellation_name_cbx);
	constellation_name_cbx->setPos(x,y); y+=15;

	sel_constellation_cbx = new LabeledCheckBox(false, _("Selected Constellation Only"));
	sel_constellation_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(sel_constellation_cbx);
	sel_constellation_cbx->setPos(x,y);

	y+=25;

	s_texture* nebp = new s_texture("bt_nebula");
	Picture* pneb = new Picture(nebp, x-50, y, 32, 32);
	tab_render->addComponent(pneb);

	y+=15;

	nebulas_names_cbx = new LabeledCheckBox(false, _("Nebulas Names. Up to mag :"));
	nebulas_names_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(nebulas_names_cbx);
	nebulas_names_cbx->setPos(x,y);

	max_mag_nebula_name = new FloatIncDec(courierFont, tex_up, tex_down, 0, 12,
		core->MaxMagNebulaName, 0.5);
	max_mag_nebula_name->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(max_mag_nebula_name);
	max_mag_nebula_name->setPos(x + 220,y);

	y+=30;

	s_texture* planp = new s_texture("bt_planet");
	Picture* pplan = new Picture(planp, x-50, y, 32, 32);
	tab_render->addComponent(pplan);

	planets_cbx = new LabeledCheckBox(false, _("Planets"));
	planets_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables2));
	tab_render->addComponent(planets_cbx);
	planets_cbx->setPos(x,y);

	moon_x4_cbx = new LabeledCheckBox(false, _("Moon Scale"));
	moon_x4_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(moon_x4_cbx);
	moon_x4_cbx->setPos(x + 150,y);

	y+=15;

	planets_hints_cbx = new LabeledCheckBox(false, _("Planets Hints"));
	planets_hints_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(planets_hints_cbx);
	planets_hints_cbx->setPos(x,y);

	y+=25;

	s_texture* gridp = new s_texture("bt_grid");
	Picture* pgrid = new Picture(gridp, x-50, y, 32, 32);
	tab_render->addComponent(pgrid);

	equator_grid_cbx = new LabeledCheckBox(false, _("Equatorial Grid"));
	equator_grid_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(equator_grid_cbx);
	equator_grid_cbx->setPos(x,y); y+=15;

	azimuth_grid_cbx = new LabeledCheckBox(false, _("Azimuthal Grid"));
	azimuth_grid_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(azimuth_grid_cbx);
	azimuth_grid_cbx->setPos(x,y); y-=15;

	equator_cbx = new LabeledCheckBox(false, _("Equator Line"));
	equator_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(equator_cbx);
	equator_cbx->setPos(x + 150,y); y+=15;

	ecliptic_cbx = new LabeledCheckBox(false, _("Ecliptic Line"));
	ecliptic_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(ecliptic_cbx);
	ecliptic_cbx->setPos(x + 150,y);

	y+=25;

	s_texture* groundp = new s_texture("bt_ground");
	Picture* pground = new Picture(groundp, x-50, y, 32, 32);
	tab_render->addComponent(pground);

	ground_cbx = new LabeledCheckBox(false, _("Ground"));
	ground_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(ground_cbx);
	ground_cbx->setPos(x,y);

	cardinal_cbx = new LabeledCheckBox(false, _("Cardinal Points"));
	cardinal_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(cardinal_cbx);
	cardinal_cbx->setPos(x + 150,y); y+=15;

	atmosphere_cbx = new LabeledCheckBox(false, _("Atmosphere"));
	atmosphere_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(atmosphere_cbx);
	atmosphere_cbx->setPos(x,y);

	fog_cbx = new LabeledCheckBox(false, _("Fog"));
	fog_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateConfigVariables));
	tab_render->addComponent(fog_cbx);
	fog_cbx->setPos(x + 150,y); y+=22;

	LabeledButton* render_save_bt = new LabeledButton(_("Save as default"));
	render_save_bt->setOnPressCallback(callback<void>(this, &StelUI::saveRenderOptions));
	tab_render->addComponent(render_save_bt);
	render_save_bt->setPos(120,y+22);
	render_save_bt->setSize(170,25); y+=20;

	// Date & Time options
	FilledContainer* tab_time = new FilledContainer();
	tab_time->setSize(config_tab_ctr->getSize());

	x=10; y=10;

	Label* tclbl = new Label(_("\1 Current Time :"));
	tclbl->setPos(x,y); y+=20;
	tab_time->addComponent(tclbl);

	time_current = new Time_item(courierFont, tex_up, tex_down);
	time_current->setOnChangeTimeCallback(callback<void>(this, &StelUI::setCurrentTimeFromConfig));
	tab_time->addComponent(time_current);
	time_current->setPos(50,y); y+=80;

	Label* tzbl = new Label(_("\1 Time Zone :"));
	tzbl->setPos(x,y); y+=20;
	tab_time->addComponent(tzbl);

	Label* system_tz_lbl = new Label(_("\1 Using System Default Time Zone"));
	tab_time->addComponent(system_tz_lbl);
	system_tz_lbl->setPos(50 ,y); y+=20;
	string tmpl("(" + core->observatory->get_time_zone_name_from_system(core->navigation->get_JDay()) + ")");
	
	for (unsigned int i=0;i<tmpl.size();i++) {if ((unsigned int)tmpl[i]<0 || (unsigned int)tmpl[i]>255) tmpl[i]='*';}
	system_tz_lbl2 = new Label(tmpl);
	tab_time->addComponent(system_tz_lbl2);
	
	system_tz_lbl2->setPos(70 ,y); y+=30;

	Label* time_speed_lbl = new Label(_("\1 Time speed : "));
	tab_time->addComponent(time_speed_lbl);
	time_speed_lbl->setPos(x ,y); y+=20;

	time_speed_lbl2 = new Label("\1 Current Time Speed is XX sec/sec.");
	tab_time->addComponent(time_speed_lbl2);
	time_speed_lbl2->setPos(50 ,y); y+=30;

	TextLabel* ts_lbl = new TextLabel(_("Use key J and L to decrease and increase\n   time speed.\nUse key K to return to real time speed."));
	tab_time->addComponent(ts_lbl);
	ts_lbl->setPos(50 ,y); y+=30;

	// Location options
	FilledContainer* tab_location = new FilledContainer();
	tab_location->setSize(config_tab_ctr->getSize());

	x=5; y=5;
	// TODO: specify the earthmap in the ini
	s_texture *earth = new s_texture("earthmap");
	s_texture *pointertex = new s_texture("pointeur1");
	s_texture *citytex = new s_texture("city");
	earth_map = new MapPicture(earth, pointertex, citytex, x,y,tab_location->getSizex()-10, 250);
	earth_map->setOnPressCallback(callback<void>(this, &StelUI::setObserverPositionFromMap));
	earth_map->setOnNearestCityCallback(callback<void>(this, &StelUI::setCityFromMap));
	tab_location->addComponent(earth_map);
	y+=earth_map->getSizey();
	earth_map->set_font(core->MapFontSize, core->DataDir + core->BaseFontName);
	load_cities(core->DataDir + "cities.fab");
	
	y += 5;
	Label * lblcursor = new Label(_("Cursor : "));
	lblcursor->setPos(20, y+1);
	lblMapLocation = new Label();
	lblMapLocation->setPos(100, y+1);
	
	Label * lblloc = new Label(_("Selected : "));
	lblloc->setPos(20, y+21);
	lblMapPointer = new Label(core->observatory->get_name());
	lblMapPointer->setPos(100, y+21);
	
	Label * lbllong = new Label(_("Longitude : "));
	lbllong->setPos(20, y+41);
	long_incdec	= new FloatIncDec(courierFont, tex_up, tex_down, -180, 180, 0, 1.f/60);
	long_incdec->setSizex(135);
	long_incdec->setFormat(FORMAT_LONGITUDE);
	long_incdec->setOnPressCallback(callback<void>(this, &StelUI::setObserverPositionFromIncDec));
	long_incdec->setPos(100,y+40);

	Label * lbllat = new Label(_("Latitude : "));
	lbllat->setPos(20, y+61);
	lat_incdec	= new FloatIncDec(courierFont, tex_up, tex_down, -90, 90, 0, 1.f/60);
	lat_incdec->setFormat(FORMAT_LATITUDE);
	lat_incdec->setSizex(135);
	lat_incdec->setOnPressCallback(callback<void>(this, &StelUI::setObserverPositionFromIncDec));
	lat_incdec->setPos(100,y+60);

	Label * lblalt = new Label(_("Altitude : "));
	lblalt->setPos(20, y+81);
	alt_incdec	= new IntIncDec(courierFont, tex_up, tex_down, 0, 2000, 0, 10);
	alt_incdec->setSizex(135);
	alt_incdec->setOnPressCallback(callback<void>(this, &StelUI::setObserverPositionFromIncDec));
	alt_incdec->setPos(100,y+80);

	LabeledButton* location_save_bt = new LabeledButton(_("Save location"));
	location_save_bt->setOnPressCallback(callback<void>(this, &StelUI::saveObserverPosition));
	location_save_bt->setPos(250,y+70);
	location_save_bt->setSize(120,25);

	tab_location->addComponent(lblcursor);
	tab_location->addComponent(lblloc);
	tab_location->addComponent(lblMapLocation);
	tab_location->addComponent(lblMapPointer);
	tab_location->addComponent(lbllong);
	tab_location->addComponent(lbllat);
	tab_location->addComponent(lblalt);
	tab_location->addComponent(long_incdec);
	tab_location->addComponent(lat_incdec);
	tab_location->addComponent(alt_incdec);
	tab_location->addComponent(location_save_bt);

	// Video Options
	FilledContainer* tab_video = new FilledContainer();
	tab_video->setSize(config_tab_ctr->getSize());

	x=10; y=10;
	Label * lblvideo1 = new Label(_("\1 Projection :"));
	lblvideo1->setPos(x, y);
	tab_video->addComponent(lblvideo1);

	x=50; y+=20;

	fisheye_projection_cbx = new LabeledCheckBox(core->projection->get_type()==Projector::FISHEYE_PROJECTOR, _("Fisheye Projection Mode"));
	fisheye_projection_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateVideoVariables));
	tab_video->addComponent(fisheye_projection_cbx);
	fisheye_projection_cbx->setPos(x,y); y+=15;

	disk_viewport_cbx = new LabeledCheckBox(core->projection->get_viewport_type()==Projector::DISK, _("Disk Viewport"));
	disk_viewport_cbx->setOnPressCallback(callback<void>(this, &StelUI::updateVideoVariables));
	tab_video->addComponent(disk_viewport_cbx);
	disk_viewport_cbx->setPos(x,y); y+=35;

	Label * lblvideo2 = new Label(_("\1 Screen Resolution :"));
	lblvideo2->setPos(10, y);
	tab_video->addComponent(lblvideo2); y+=20;

	Label * lblvideo3 = new Label(_("Restart program for"));
	Label * lblvideo4 = new Label(_("change to apply."));
	lblvideo3->setPos(200, y+25);
	lblvideo4->setPos(200, y+40);
	tab_video->addComponent(lblvideo3);
	tab_video->addComponent(lblvideo4);

	screen_size_sl = new StringList();
	screen_size_sl->setPos(x,y);
	screen_size_sl->addItem("640x480");
	screen_size_sl->addItem("800x600");
	screen_size_sl->addItem("1024x768");
	screen_size_sl->addItem("1280x800");
	screen_size_sl->addItem("1280x1024");
	screen_size_sl->addItem("1400x1050");
	screen_size_sl->addItem("1600x1200");
	screen_size_sl->adjustSize();
	char vs[1000];
	sprintf(vs, "%dx%d", core->screen_W, core->screen_H);
	screen_size_sl->setValue(vs);
	tab_video->addComponent(screen_size_sl);

	y+=100;

	snprintf(vs, 999, "%sconfig.ini", core->ConfigDir.c_str());
	Label * lblvideo5 = new Label(_("For unlisted screen resolution, edit the file :"));
	Label * lblvideo6 = new Label(string(vs));
	lblvideo5->setPos(30, y+25);
	lblvideo6->setPos(30, y+40);
	tab_video->addComponent(lblvideo5);
	tab_video->addComponent(lblvideo6);

	y+=80;

	LabeledButton* video_save_bt = new LabeledButton(_("Save as default"));
	video_save_bt->setOnPressCallback(callback<void>(this, &StelUI::setVideoOption));
	tab_video->addComponent(video_save_bt);
	video_save_bt->setPos(120,y+22);
	video_save_bt->setSize(170,25); y+=20;

	// Landscapes option
	FilledContainer* tab_landscapes = new FilledContainer();
	tab_landscapes->setSize(config_tab_ctr->getSize());

	x=10; y=10;
	Label * lbllandscapes1 = new Label(_("\1 Choose landscapes :"));
	lbllandscapes1->setPos(x, y);
	tab_landscapes->addComponent(lbllandscapes1);

	x=50; y+=20;

	landscape_sl = new StringList();
	landscape_sl->setPos(x,y);
	landscape_sl->addItemList(Landscape::get_file_content(core->DataDir + "landscapes.ini"));
	landscape_sl->adjustSize();
	sprintf(vs, "%s", core->observatory->get_landscape_name().c_str());
	landscape_sl->setValue(vs);
	landscape_sl->setOnPressCallback(callback<void>(this, &StelUI::setLandscape));
	tab_landscapes->addComponent(landscape_sl);

	y+=150;	

	Label * lbllandscape1 = new Label(_("Please save option in tab \"Location\""));
	Label * lbllandscape2 = new Label(_("to save current landscape as the default landscape."));
	lbllandscape1->setPos(30, y+25);
	lbllandscape2->setPos(30, y+40);
	tab_landscapes->addComponent(lbllandscape1);
	tab_landscapes->addComponent(lbllandscape2);

	// Global window
	config_tab_ctr->setTexture(flipBaseTex);
	config_tab_ctr->addTab(tab_time, _("Date & Time"));
	config_tab_ctr->addTab(tab_location, _("Location"));
	config_tab_ctr->addTab(tab_landscapes, _("Landscapes"));
	config_tab_ctr->addTab(tab_video, _("Video"));
	config_tab_ctr->addTab(tab_render, _("Rendering"));
	config_win->addComponent(config_tab_ctr);
	config_win->setOnHideBtCallback(callback<void>(this, &StelUI::config_win_hideBtCallback));

	return config_win;
}

void StelUI::load_cities(const string & fileName)
{
	string name, state, country;
	float time;

	char linetmp[200];
	char cname[50],cstate[50],ccountry[50],clat[20],clon[20],ctime[20];
	int showatzoom;
	int alt;
	char tmpstr[2000];
	int total = 0;

	cout << _("Loading Cities data...");
	FILE *fic = fopen(fileName.c_str(), "r");
	if (!fic)
	{
		printf("Can't open %s\n", fileName.c_str());
		return; // no art, but still loaded constellation data
	}

	// determine total number to be loaded for percent complete display
	while (fgets(tmpstr, 2000, fic)) {++total;}
	rewind(fic);

//	LoadingBar lb(core->projection, core->LoadingBarSize, core->BaseFontPngName, core->BaseFontTxtName, "logo24bits", core->screen_W, core->screen_H);

	int line = 0;
	while (!feof(fic))
	{
/*
		if (!(line%100) || (line == total-1))
		{
			sprintf(tmpstr, _("Loading city data: %d/%d"), line+1, total);
			lb.SetMessage(tmpstr);
			lb.Draw((float)(line+1)/total);
		}
*/
		fgets(linetmp, 100, fic);
		if (linetmp[0] != '#')
		{
			if (sscanf(linetmp, "%s %s %s %s %s %d %s %d\n", cname, cstate, ccountry, clat, 
				clon, &alt, ctime, &showatzoom) != 8)
			{
				if (feof(fic))
				{
					// Empty city file
					fclose(fic);
					return;		
				}
				printf("ERROR while loading city data in line %d\n", line);
				assert(0);
			}
			
			name = cname;
		    for (string::size_type i=0;i<name.length();++i)
			{
				if (name[i]=='_') name[i]=' ';
			}	
			state = cstate;
		    for (string::size_type i=0;i<state.length();++i)
			{
				if (state[i]=='_') state[i]=' ';
			}
			country = ccountry;
		    for (string::size_type i=0;i<country.length();++i)
			{
				if (country[i]=='_') country[i]=' ';
			}	
			if (ctime[0] == 'x')
				time = 0;
			else 
				sscanf(ctime,"%f",&time);
			earth_map->addCity(name, state, country, get_dec_angle(clon), 
				get_dec_angle(clat), time, showatzoom, alt);
		}
		line++;
	}
	fclose(fic);
	cout << "(" << line << " cities loaded)" << endl;
}

// Search window

Component* StelUI::createSearchWindow(void)
{
	// The current drawing position
	int xi,yi;
	xi=20; yi=15;

	int x,y;
	x=70; y=yi+5;
	
//	int h = 120;
	int h = 300;

     // Bring up dialog
	search_win = new StdBtWin(_("Object Search"));
	search_win->setOpaque(opaqueGUI);
	search_win->reshape(300,200,400,h);
	search_win->setVisible(core->FlagSearch);

	search_tab_ctr = new TabContainer();
	search_tab_ctr->setSize(search_win->getSizex(),search_win->getSizey()- 20);

	lblSearchMessage = new Label("");
	lblSearchMessage->setPos(10, search_win->getSizey()-15);

	// stars
	FilledContainer* tab_stars = new FilledContainer();
	tab_stars->setSize(search_tab_ctr->getSize());

	s_texture* starp = new s_texture("halo");
	Picture* pstar = new Picture(starp, xi, yi, 32, 32);
	tab_stars->addComponent(pstar);

	Label * lblstars1 = new Label(_("Star No./Name"));
	lblstars1->setPos(x, y+5);
	tab_stars->addComponent(lblstars1);

	Label * lblstars2 = new Label(_("eg. 60718 (aCrux) or Canopus"));
	Label * lblstars3 = new Label(_("HP 60718, HD 108248, SAO 251904"));
	lblstars2->setPos(x+100, y+35);
	lblstars3->setPos(x+100, y+51);
	tab_stars->addComponent(lblstars2);
	tab_stars->addComponent(lblstars3);

	star_edit = new EditBox("");
	star_edit->setOnReturnKeyCallback(callback<void>(this, &StelUI::doStarSearch));
	star_edit->setOnAutoCompleteCallback(callback<void>(this, &StelUI::showStarAutoComplete));
	tab_stars->addComponent(star_edit);
	star_edit->setPos(x+100,y);
	star_edit->setSize(170,25);

    // constellation
	FilledContainer* tab_constellations = new FilledContainer();
	tab_constellations->setSize(search_tab_ctr->getSize());

	s_texture* constellp = new s_texture("bt_constellations");
	Picture* pconstell = new Picture(constellp, xi, yi, 32, 32);
	tab_constellations->addComponent(pconstell);

	Label * lblconst1 = new Label(_("Constellation"));
	lblconst1->setPos(x, y+5);
	tab_constellations->addComponent(lblconst1);

	Label * lblconst2 = new Label(_("eg. Crux"));
	lblconst2->setPos(x+100, y+35);
	tab_constellations->addComponent(lblconst2);

	constellation_edit = new EditBox("");
	constellation_edit->setOnReturnKeyCallback(callback<void>(this, &StelUI::doConstellationSearch));
	constellation_edit->setOnAutoCompleteCallback(callback<void>(this, &StelUI::showConstellationAutoComplete));
	tab_constellations->addComponent(constellation_edit);
	constellation_edit->setPos(x+100,y);
	constellation_edit->setSize(170,25);
	
    // nebula
	FilledContainer* tab_nebula = new FilledContainer();
	tab_nebula->setSize(search_tab_ctr->getSize());
 
	s_texture* nebp = new s_texture("bt_nebula");
	Picture* pneb = new Picture(nebp, xi, yi, 32, 32);
	tab_nebula->addComponent(pneb);

	Label * lblnebula1 = new Label(_("Nebula"));
	lblnebula1->setPos(x, y+5);
	tab_nebula->addComponent(lblnebula1);

	Label * lblnebula2 = new Label(_("eg. M83, NGC 7009, IC 2118" ));
	Label * lblnebula3 = new Label(_("CW 113, SH 40" ));
	lblnebula2->setPos(x+100, y+35);
	lblnebula3->setPos(x+100, y+51);
	tab_nebula->addComponent(lblnebula2);
	tab_nebula->addComponent(lblnebula3);

	nebula_edit = new EditBox("");
	nebula_edit->setOnReturnKeyCallback(callback<void>(this, &StelUI::doNebulaSearch));
	tab_nebula->addComponent(nebula_edit);
	nebula_edit->setPos(x+100,y);
	nebula_edit->setSize(170,25);

    // controls
	FilledContainer* tab_example = new FilledContainer();
	tab_example->setSize(search_tab_ctr->getSize());

	// example horizontal cursorbar
	CursorBar *chbar = new CursorBar(false, -.6,1.7);
	tab_example->addComponent(chbar);
	chbar->setPos(60,10);
	chbar->setSize(150,20);
	
	// example horizontal cursorbar
    CursorBar *cvbar = new CursorBar(true, 5,15);
	cvbar->setPos(30,30);
	cvbar->setSize(20,120);
	tab_example->addComponent(cvbar);

	// example vscrollbar
	ScrollBar *vbar = new ScrollBar(true, 5,1);
	vbar->setPos(320,20);
	vbar->setSize(20,140);
	tab_example->addComponent(vbar);

	// example hscrollbar
	ScrollBar *hbar = new ScrollBar(false,12,3);
	hbar->setPos(80,160);
	hbar->setSize(240,20);
	tab_example->addComponent(hbar);

	listBox = new ListBox(6);
	listBox->setPos(60,40);
	listBox->setSizex(200);
	listBox->setOnChangeCallback(callback<void>(this, &StelUI::listBoxChanged));
	tab_example->addComponent(listBox);

	// example editbox
	EditBox *editbox = new EditBox();
	editbox->setPos(20,180);
	editbox->setSize(250,25);
	tab_example->addComponent(editbox);

	// example messageboxes and inputboxes
	LabeledButton *button1 = new LabeledButton("MsgBox 1");
	button1->setPos(20,220);
	button1->setSize(80,25);
	button1->setOnPressCallback(callback<void>(this, &StelUI::msgbox1));
	tab_example->addComponent(button1);

	LabeledButton *button2 = new LabeledButton("MsgBox 2");
	button2->setPos(115,220);
	button2->setSize(80,25);
	button2->setOnPressCallback(callback<void>(this, &StelUI::msgbox2));
	tab_example->addComponent(button2);

	LabeledButton *button3 = new LabeledButton("MsgBox 3");
	button3->setPos(205,220);
	button3->setSize(80,25);
	button3->setOnPressCallback(callback<void>(this, &StelUI::msgbox3));
	tab_example->addComponent(button3);

	LabeledButton *button4 = new LabeledButton("Inputbox");
	button4->setPos(300,220);
	button4->setSize(80,25);
	button4->setOnPressCallback(callback<void>(this, &StelUI::inputbox1));
	tab_example->addComponent(button4);

    // Planets
	FilledContainer* tab_planets = new FilledContainer();
	tab_planets->setSize(search_tab_ctr->getSize());

	s_texture* planp = new s_texture("bt_planet");
	Picture* pplan = new Picture(planp, xi, yi, 32, 32);
	tab_planets->addComponent(pplan);

	Label * lblplanet1 = new Label(_("Planet/Moon"));
	lblplanet1->setPos(x, y+5);
	tab_planets->addComponent(lblplanet1);

	Label * lblplanet2 = new Label(_("eg. Pluto or Io"));
	lblplanet2->setPos(x+100, y+35);
	tab_planets->addComponent(lblplanet2);

	planet_edit = new EditBox("");
	planet_edit->setOnReturnKeyCallback(callback<void>(this, &StelUI::doPlanetSearch));
	planet_edit->setOnAutoCompleteCallback(callback<void>(this, &StelUI::showPlanetAutoComplete));
	tab_planets->addComponent(planet_edit);
	planet_edit->setPos(x+100,y);
	planet_edit->setSize(170,25);
	
	// Search tab
	search_tab_ctr->setTexture(flipBaseTex);
	search_tab_ctr->addTab(tab_stars, _("Stars"));
	search_tab_ctr->addTab(tab_constellations, _("Constellation"));
	search_tab_ctr->addTab(tab_nebula, _("Nebula"));
	search_tab_ctr->addTab(tab_planets, _("Planets & Moons"));
	//search_tab_ctr->addTab(tab_example, _("Widgets"));
    search_win->addComponent(search_tab_ctr);
	search_win->addComponent(lblSearchMessage);
	search_win->setOnHideBtCallback(callback<void>(this, &StelUI::search_win_hideBtCallback));

	return search_win;

}

void StelUI::msgbox1(void)
{
	dialog_win->MessageBox("Stellarium",string("This is a 1 button message (OK only)\nWith an Alert Icon\nAnd an extra line"), 
		BT_OK + BT_ICON_ALERT, "1 button test");
}

void StelUI::msgbox2(void)
{
	dialog_win->MessageBox("Stellarium","This is a 2 button message (OK & CANCEL)\nWith a Question Icon", 
		BT_OK + BT_CANCEL + BT_ICON_QUESTION, "2 button test");
}

void StelUI::msgbox3(void)
{
	dialog_win->MessageBox("Stellarium","This is a 2 button message (OK & CANCEL)\nWith a Blank Icon", 
		BT_OK + BT_CANCEL + BT_ICON_BLANK, "2 button test");
}

void StelUI::inputbox1(void)
{
	dialog_win->InputBox("Stellarium","Input a string into the EditBox class below.", "input test");
}

void StelUI::dialogCallback(void)
{
	string lastID = dialog_win->getLastID();
	int lastButton = dialog_win->getLastButton();
	string lastInput = dialog_win->getLastInput();
	int lastType = dialog_win->getLastType();

	if (lastID == "observatory name")
	{
		if (lastButton != BT_OK || lastInput == "")
			lastInput = UNKNOWN_OBSERVATORY;
		doSaveObserverPosition(lastInput);
		setCityFromMap();
	}
	else if (lastID != "")
	{
		string msg = lastID;
		msg += " returned btn: ";
	
		if (lastButton == BT_OK) msg += "BT_OK";
		else if (lastButton == BT_YES) msg += "BT_YES";
		else if (lastButton == BT_NO) msg += "BT_NO";
		else if (lastButton == BT_CANCEL) msg += "BT_CANCEL";
	
		if (lastType == STDDLGWIN_MSG) dialog_win->MessageBox("Stellarium",msg, BT_OK);
		else if (lastType == STDDLGWIN_INPUT) dialog_win->MessageBox("Stellarium",msg + " inp: " + lastInput, BT_OK);
	}		
}

void StelUI::listBoxChanged(void)
{
	int value = listBox->getValue();
	string objectName = listBox->getItem(value);
    string command = string("select planet " + objectName);
    string error = string("Planet '" + objectName + "' not found");
	doSearchCommand(command, error);
}

void StelUI::hideSearchMessage(void)
{
       lblSearchMessage->setLabel("");
       lblSearchMessage->adjustSize();
}

void StelUI::showSearchMessage(string _message)
{
       lblSearchMessage->setLabel(_message);
       lblSearchMessage->adjustSize();
       int x1 = search_win->getSizex();
       int x2 = lblSearchMessage->getSizex();
       lblSearchMessage->setPos((x1-x2)/2,search_win->getSizey()-17);
}

void StelUI::doSearchCommand(string _command, string _error)
{
	if(!core->commander->execute_command(_command))
        return;

    if (core->selected_object)
    {
        gotoObject();
        hideSearchMessage();
    }    
    else
        showSearchMessage(_error);
}

void StelUI::doNebulaSearch(void)
{
    string objectName = nebula_edit->getText();
    string originalName = objectName;
    
    transform(objectName.begin(), objectName.end(), objectName.begin(), (int(*)(int))toupper);
    for (string::size_type i=0;i<objectName.length();++i)
	{
		if (objectName[i]==' ') objectName[i]='_';
	}

    string command = string("select nebula " + objectName);
    string error = string("Nebula '" + originalName + "' not found");
    
    doSearchCommand(command, error);
    nebula_edit->clearText();
}

void StelUI::doConstellationSearch(void)
{
    string rawObjectName = constellation_edit->getText();

    string objectName = core->asterisms->get_short_name_by_name(rawObjectName);

    if (objectName != "")
    {
		string command = string("select constellation_star " + objectName);
		string error = string("Constellation '" + objectName + "' not found");

		doSearchCommand(command, error);
    }
    else
        showSearchMessage(string("Constellation " + rawObjectName + " not found"));

	constellation_edit->clearText();
}

void StelUI::showConstellationAutoComplete(void)
{
    showSearchMessage(constellation_edit->getAutoCompleteOptions());
}

void StelUI::doStarSearch(void)
{
    string objectName = star_edit->getText();
    string originalName = objectName;
    unsigned int HP = core->hip_stars->getCommonNameHP(objectName);

    if (HP > 0)
    {
		ostringstream oss;
		oss << "HP_" << HP;
		objectName	= oss.str();
	}
	else
	{
	    transform(objectName.begin(), objectName.end(), objectName.begin(), (int(*)(int))toupper);
    	for (string::size_type i=0;i<objectName.length();++i)
		{
			if (objectName[i]==' ') objectName[i]='_';
		}
	}
    
	string command = string("select star " + objectName);
    string error = string("Star '" + originalName + "' not found");
    
    doSearchCommand(command, error);
    star_edit->clearText();
}

void StelUI::showStarAutoComplete(void)
{
    showSearchMessage(star_edit->getAutoCompleteOptions());
}

void StelUI::doPlanetSearch(void)
{
    string objectName = planet_edit->getText();
    string command = string("select planet " + objectName);
    string error = string("Planet '" + objectName + "' not found");

    planet_edit->clearText();
    
	doSearchCommand(command, error);
}

void StelUI::showPlanetAutoComplete(void)
{
    showSearchMessage(planet_edit->getAutoCompleteOptions());
}

void StelUI::updateConfigVariables(void)
{
	core->commander->execute_command("flag stars ", stars_cbx->getState());
	core->commander->execute_command("flag star_names ", star_names_cbx->getState());
	core->commander->execute_command("set max_mag_star_name ", max_mag_star_name->getValue());
	core->commander->execute_command("flag star_twinkle ", star_twinkle_cbx->getState());
	core->commander->execute_command("set star_twinkle_amount ", star_twinkle_amount->getValue());
	core->commander->execute_command("flag constellation_drawing ", constellation_cbx->getState());
	core->commander->execute_command("flag constellation_names ", constellation_name_cbx->getState());
	core->commander->execute_command("flag constellation_pick ", sel_constellation_cbx->getState());
	core->commander->execute_command("flag nebula_names ", nebulas_names_cbx->getState());
	core->commander->execute_command("set max_mag_nebula_name ", max_mag_nebula_name->getValue());
	core->commander->execute_command("flag planet_names ", planets_hints_cbx->getState());
	core->commander->execute_command("flag moon_scaled ", moon_x4_cbx->getState());
	core->commander->execute_command("flag equatorial_grid ", equator_grid_cbx->getState());
	core->commander->execute_command("flag azimuthal_grid ", azimuth_grid_cbx->getState());
	core->commander->execute_command("flag equator_line ", equator_cbx->getState());
	core->commander->execute_command("flag ecliptic_line ", ecliptic_cbx->getState());
	core->commander->execute_command("flag landscape ", ground_cbx->getState());
	core->commander->execute_command("flag cardinal_points ", cardinal_cbx->getState());
	core->commander->execute_command("flag atmosphere ", atmosphere_cbx->getState());
	core->commander->execute_command("flag fog ", fog_cbx->getState());
}

void StelUI::updateConfigVariables2(void)
{
	core->commander->execute_command("flag planets ", planets_cbx->getState());
}

void StelUI::setCurrentTimeFromConfig(void)
{
	//	core->navigation->set_JDay(time_current->getJDay() - core->observatory->get_GMT_shift()*JD_HOUR);
	core->commander->execute_command(string("date local " + time_current->getDateString()));
}

void StelUI::setObserverPositionFromMap(void)
{
	std::ostringstream oss;
	oss << "moveto lat " << earth_map->getPointerLatitude() << " lon " << earth_map->getPointerLongitude()
		<< " alt " << earth_map->getPointerAltitude();
	core->commander->execute_command(oss.str());
}

void StelUI::setCityFromMap(void)
{
	waitOnLocation = false;
	lblMapLocation->setLabel(earth_map->getCursorString());
	lblMapPointer->setLabel(earth_map->getPositionString());
}

void StelUI::setObserverPositionFromIncDec(void)
{
	std::ostringstream oss;
	oss << "moveto lat " << lat_incdec->getValue() << " lon " << long_incdec->getValue()
		<< " alt " << alt_incdec->getValue();
	core->commander->execute_command(oss.str());
}

void StelUI::doSaveObserverPosition(const string& name)
{
	string location = name;
    for (string::size_type i=0;i<location.length();++i)
	{
		if (location[i]==' ') location[i]='_';
	}

	std::ostringstream oss;
	oss << "moveto lat " << lat_incdec->getValue() << " lon " << long_incdec->getValue()
		<< " name " << location;
	core->commander->execute_command(oss.str());

	core->observatory->save(core->ConfigDir + core->config_file, "init_location");
	core->ui->setTitleObservatoryName(core->ui->getTitleWithAltitude());
}

void StelUI::saveObserverPosition(void)
{
	string location = earth_map->getPositionString();

	if (location == UNKNOWN_OBSERVATORY)	
		dialog_win->InputBox("Stellarium","Enter observatory name", "observatory name");
	else
		doSaveObserverPosition(location);

}

void StelUI::saveRenderOptions(void)
{
	cout << _("Saving rendering options in file ") << core->ConfigDir + core->config_file << endl;

	InitParser conf;
	conf.load(core->ConfigDir + core->config_file);

	conf.set_boolean("astro:flag_stars", core->FlagStars);
	conf.set_boolean("astro:flag_star_name", core->FlagStarName);
	conf.set_double("stars:max_mag_star_name", core->MaxMagStarName);
	conf.set_boolean("stars:flag_star_twinkle", core->FlagStarTwinkle);
	conf.set_double("stars:star_twinkle_amount", core->StarTwinkleAmount);
	conf.set_boolean("viewing:flag_constellation_drawing", core->constellation_get_flag_lines());
	conf.set_boolean("viewing:flag_constellation_name", core->constellation_get_flag_names());
	conf.set_boolean("viewing:flag_constellation_boundaries", core->constellation_get_flag_boundaries());
	conf.set_boolean("viewing:flag_constellation_pick", core->asterisms->get_flag_isolate_selected());
	conf.set_boolean("astro:flag_nebula", core->FlagNebula);
	conf.set_boolean("astro:flag_nebula_name", core->nebulas->get_flag_hints());
	conf.set_double("astro:max_mag_nebula_name", core->MaxMagNebulaName);
	conf.set_boolean("astro:flag_planets", core->FlagPlanets);
	conf.set_boolean("astro:flag_planets_hints", core->FlagPlanetsHints);
	conf.set_double("viewing:moon_scale", core->ssystem->get_moon()->get_sphere_scale());
	conf.set_boolean("viewing:flag_chart", core->FlagChart);
	conf.set_boolean("viewing:flag_night", core->FlagNight);
	//conf.set_boolean("viewing:use_common_names", core->FlagUseCommonNames);
	conf.set_boolean("viewing:flag_equatorial_grid", core->FlagEquatorialGrid);
	conf.set_boolean("viewing:flag_azimutal_grid", core->FlagAzimutalGrid);
	conf.set_boolean("viewing:flag_equator_line", core->FlagEquatorLine);
	conf.set_boolean("viewing:flag_ecliptic_line", core->FlagEclipticLine);
	conf.set_boolean("landscape:flag_landscape", core->FlagLandscape);
	conf.set_boolean("viewing:flag_cardinal_points", core->cardinals_points->get_flag_show());
	conf.set_boolean("landscape:flag_atmosphere", core->FlagAtmosphere);
	conf.set_boolean("landscape:flag_fog", core->FlagFog);

	conf.save(core->ConfigDir + core->config_file);
}

void StelUI::setTimeZone(void)
{
	core->observatory->set_custom_tz_name(tzselector->gettz());
}

void StelUI::setVideoOption(void)
{
	string s = screen_size_sl->getValue();
	int i = s.find("x");
	int w = atoi(s.substr(0,i).c_str());
	int h = atoi(s.substr(i+1,s.size()).c_str());

	cout << _("Saving video size ") << w << "x" << h << _(" in file ") << core->ConfigDir + core->config_file << endl;

	InitParser conf;
	conf.load(core->ConfigDir + core->config_file);

	switch (core->projection->get_type())
	{
		case Projector::FISHEYE_PROJECTOR : conf.set_str("projection:type", "fisheye"); break;
		case Projector::PERSPECTIVE_PROJECTOR :
		default :
			conf.set_str("projection:type", "perspective"); break;
	}

	switch (core->projection->get_viewport_type())
	{
		case Projector::SQUARE : conf.set_str("projection:viewport", "square"); break;
		case Projector::DISK : conf.set_str("projection:viewport", "disk"); break;
		case Projector::MAXIMIZED :
		default :
			conf.set_str("projection:viewport", "maximized"); break;
	}

	conf.set_int("video:screen_w", w);
	conf.set_int("video:screen_h", h);
	conf.save(core->ConfigDir + core->config_file);
}

void StelUI::setLandscape(void)
{
	core->set_landscape(landscape_sl->getValue());
}

void StelUI::updateVideoVariables(void)
{
	if (fisheye_projection_cbx->getState() && core->projection->get_type()!=Projector::FISHEYE_PROJECTOR)
	{
		// Switch to fisheye projection
		FisheyeProjector* p = new FisheyeProjector(*(core->projection));
		delete core->projection;
		core->projection = p;
	}
	if (!fisheye_projection_cbx->getState() && core->projection->get_type()==Projector::FISHEYE_PROJECTOR)
	{
		// Switch to perspective projection
		Projector* p = new Projector(*(core->projection));
		delete core->projection;
		core->projection = p;
		core->projection->set_minmaxfov(0.001, 100.);
	}


	if (disk_viewport_cbx->getState() && core->projection->get_viewport_type()!=Projector::DISK)
	{
		core->projection->set_disk_viewport();
	}
	if (!disk_viewport_cbx->getState() && core->projection->get_viewport_type()==Projector::DISK)
	{
		core->projection->maximize_viewport();
	}

}

void StelUI::updateConfigForm(void)
{
	stars_cbx->setState(core->FlagStars);
	star_names_cbx->setState(core->FlagStarName);
	max_mag_star_name->setValue(core->MaxMagStarName);
	star_twinkle_cbx->setState(core->FlagStarTwinkle);
	star_twinkle_amount->setValue(core->StarTwinkleAmount);
	constellation_cbx->setState(core->constellation_get_flag_lines());
	constellation_name_cbx->setState(core->constellation_get_flag_names());
	sel_constellation_cbx->setState(core->asterisms->get_flag_isolate_selected());
	nebulas_names_cbx->setState(core->nebulas->get_flag_hints());
	max_mag_nebula_name->setValue(core->MaxMagNebulaName);
	planets_cbx->setState(core->FlagPlanets);
	planets_hints_cbx->setState(core->FlagPlanetsHints);
	moon_x4_cbx->setState(core->FlagMoonScaled);
	// TODO: add charting button
	equator_grid_cbx->setState(core->FlagEquatorialGrid);
	azimuth_grid_cbx->setState(core->FlagAzimutalGrid);
	equator_cbx->setState(core->FlagEquatorLine);
	ecliptic_cbx->setState(core->FlagEclipticLine);
	ground_cbx->setState(core->FlagLandscape);
	cardinal_cbx->setState(core->cardinals_points->get_flag_show());
	atmosphere_cbx->setState(core->FlagAtmosphere);
	fog_cbx->setState(core->FlagFog);

	earth_map->setPointerLongitude(core->observatory->get_longitude());
	earth_map->setPointerLatitude(core->observatory->get_latitude());
	long_incdec->setValue(core->observatory->get_longitude());
	lat_incdec->setValue(core->observatory->get_latitude());
	alt_incdec->setValue(core->observatory->get_altitude());
	lblMapLocation->setLabel(earth_map->getCursorString());
	if (!waitOnLocation)
		lblMapPointer->setLabel(earth_map->getPositionString());
	else
	{
		earth_map->findPosition(core->observatory->get_longitude(),core->observatory->get_latitude());
		lblMapPointer->setLabel(earth_map->getPositionString());
		waitOnLocation = false;
	}

	time_current->setJDay(core->navigation->get_JDay() + core->observatory->get_GMT_shift(core->navigation->get_JDay())*JD_HOUR);
	system_tz_lbl2->setLabel("(" +
		 core->observatory->get_time_zone_name_from_system(core->navigation->get_JDay()) + ")");

	static char tempstr[100];
	sprintf(tempstr, "%.1f", core->navigation->get_time_speed()/JD_SECOND);
	time_speed_lbl2->setLabel("\1 Current Time Speed is x" + string(tempstr));

	fisheye_projection_cbx->setState(core->projection->get_type()==Projector::FISHEYE_PROJECTOR);
	disk_viewport_cbx->setState(core->projection->get_viewport_type()==Projector::DISK);
}

void StelUI::config_win_hideBtCallback(void)
{
	core->FlagConfig = false;
	config_win->setVisible(false);
	// for MapPicture - when the dialog appears, this tells the system
	// not to show the city until MapPicture has located the name
	// from the lat and long.
	waitOnLocation = true;
	bt_flag_config->setState(0);
}

void StelUI::search_win_hideBtCallback(void)
{
	core->FlagSearch = false;
	search_win->setVisible(false);
	bt_flag_search->setState(0);
}
