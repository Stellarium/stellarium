//
// C++ Implementation: stelapp
//
// Description: 
//
//
// Author: Fabien Chereau <stellarium@free.fr>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stelapp.h"

#include "viewport_distorter.h"
#include "stellastro.h"
#include "stel_utility.h"

StelApp::StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT) : 
		frame(0), timefr(0), timeBase(0), fps(0), maxfps(10000.f),  FlagTimePause(0), 
		is_mouse_moving_horiz(false), is_mouse_moving_vert(false), draw_mode(StelApp::DM_NONE),
		initialized(0), GMT_shift(0)
{
	configDir = CDIR;
	SelectedScript = SelectedScriptDirectory = "";
	core = new StelCore(LDIR, DATA_ROOT);
	ui = new StelUI(core, this);
	commander = new StelCommandInterface(core, this);
	scripts = new ScriptMgr(commander, core->getDataDir());
	time_multiplier = 1;
    distorter = 0;
}


StelApp::~StelApp()
{
	SDL_FreeCursor(Cursor);
	delete ui;
	delete scripts;
	delete commander;
	delete core;
    if (distorter) delete distorter;
}

void StelApp::setViewPortDistorterType(const string &type) {
  if (distorter) {
    if (distorter->getType() == type) return;
    delete distorter;
    distorter = 0;
  }
  distorter = ViewportDistorter::create(type,screenW,screenH,core);
  InitParser conf;
  conf.load(configDir + "config.ini");
  distorter->init(conf);
}

string StelApp::getViewPortDistorterType(void) const {
  if (distorter) return distorter->getType();
  return "none";
}


void StelApp::quit(void)
{
	static SDL_Event Q;						// Send a SDL_QUIT event
	Q.type = SDL_QUIT;						// To the SDL event queue
	if(SDL_PushEvent(&Q) == -1)				// Try to send the event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
		exit(-1);
	}
}

void StelApp::init(void)
{
	Translator::initSystemLanguage();

	// Initialize video device and other sdl parameters
	InitParser conf;
	conf.load(configDir + "config.ini");
	
	// Main section
	string version = conf.get_str("main:version");

	if (version!=string(VERSION)) {

		std::istringstream istr(version);
		char tmp;
		int v1 =0;
		int v2 =0;
		istr >> v1 >> tmp >> v2;

		// Config versions less than 0.6.0 are not supported, otherwise we will try to use it
		if( v1 == 0 && v2 < 6 ) {

			// The config file is too old to try an importation
			printf("The current config file is from a version too old for parameters to be imported (%s).\nIt will be replaced by the default config file.\n", version.empty() ? "<0.6.0" : version.c_str());
			system( (string("cp -f ") + core->getDataRoot() + "/data/default_config.ini " + getConfigFile()).c_str() );
			conf.load(configDir + "config.ini");  // Read new config!

		} else {
			cout << "Attempting to use an existing older config file." << endl;
		}
	}

	// don't mess with SDL init if already initialized earlier
	if(!initialized) {
		screenW = conf.get_int("video:screen_w");
		screenH = conf.get_int("video:screen_h");
		initSDL(screenW, screenH, conf.get_int("video:bbp_mode"), conf.get_boolean("video:fullscreen"), core->getDataDir() + "/icon.bmp");	
	}

	// Clear screen, this fixes a strange artifact at loading time in the upper top corner.
	glClear(GL_COLOR_BUFFER_BIT);
	
	maxfps 				= conf.get_double ("video","maximum_fps",10000);
	minfps 				= conf.get_double ("video","minimum_fps",10);
	string appLocaleName = conf.get_str("localization", "app_locale", "system");
	time_format = string_to_s_time_format(conf.get_str("localization:time_display_format"));
	date_format = string_to_s_date_format(conf.get_str("localization:date_display_format"));	
	setAppLanguage(appLocaleName);
	scripts->set_allow_ui( conf.get_boolean("gui","flag_script_allow_ui",0) );

	string tzstr = conf.get_str("localization:time_zone");
	if (tzstr == "system_default")
	{
		time_zone_mode = S_TZ_SYSTEM_DEFAULT;
		// Set the program global intern timezones variables from the system locale
		tzset();
	}
	else
	{
		if (tzstr == "gmt+x") // TODO : handle GMT+X timezones form
		{
			time_zone_mode = S_TZ_GMT_SHIFT;
			// GMT_shift = x;
		}
		else
		{
			// We have a custom time zone name
			time_zone_mode = S_TZ_CUSTOM;
			set_custom_tz_name(tzstr);
		}
	}

	core->init(conf);

	string tmpstr = conf.get_str("projection:viewport");
	if (tmpstr=="maximized") core->setMaximizedViewport(screenW, screenH);
	else
		if (tmpstr=="square" || tmpstr=="disk") {
			core->setSquareViewport(screenW, screenH, 
									conf.get_int("video:horizontal_offset"), conf.get_int("video:horizontal_offset"));
			if (tmpstr=="disk") core->setViewportMaskDisk();

		} else {

			cerr << "ERROR : Unknown viewport type : " << tmpstr << endl;
			exit(-1);
		}	

	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	FlagEnableMoveMouse	= conf.get_boolean("navigation","flag_enable_move_mouse",1);
	MouseZoom			= conf.get_int("navigation","mouse_zoom",30);	
	
	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		core->setJDay(PresetSkyTime - get_GMT_shift(PresetSkyTime) * JD_HOUR);
	else core->setTimeNow();

	// initialisation of the User Interface
	
	// TODO: Need way to update settings from config without reinitializing whole gui
	ui->init(conf);
	
	if(!initialized) ui->init_tui();  // don't reinit tui since probably called from there
	else ui->localizeTui();  // update translations/fonts as needed

	// Initialisation of the color scheme
	draw_mode = StelApp::DM_NONE;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_chart")) setVisionModeChart();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();
	
    if (distorter == 0) {
      setViewPortDistorterType(conf.get_str("video","distorter","none"));
    }

	// play startup script, if available
	if(scripts) scripts->play_startup_script();

	initialized = 1;
}

void StelApp::update(int delta_time)
{
	++frame;
	timefr+=delta_time;
	if (timefr-timeBase > 1000)
	{
		fps=frame*1000.0/(timefr-timeBase);				// Calc the FPS rate
		frame = 0;
		timeBase+=1000;
	}

	// change time rate if needed to fast forward scripts
	delta_time *= time_multiplier;

	// keep audio position updated if changing time multiplier
	if(!scripts->is_paused()) commander->update(delta_time);

	// run command from a running script
	scripts->update(delta_time);
	
	ui->gui_update_widgets(delta_time);
	ui->tui_update_widgets();

	if(!scripts->is_paused()) core->getImageMgr()->update(delta_time);
	
	core->update(delta_time);
}

//! Main drawinf function called at each frame
double StelApp::draw(int delta_time)
{
	// Render all the main objects of stellarium
	double squaredDistance = core->draw(delta_time);

	// Draw the Graphical ui and the Text ui
	ui->draw();

    distorter->distort();

    return squaredDistance;
}

//! @brief Set the application locale. This apply to GUI, console messages etc..
void StelApp::setAppLanguage(const std::string& newAppLocaleName)
{
	// Update the translator with new locale name
	Translator::globalTranslator = Translator(PACKAGE, core->getLocaleDir(), newAppLocaleName);
	cout << "Application locale is " << Translator::globalTranslator.getLocaleName() << endl;

	// update translations and font in tui
	ui->localizeTui();

	// TODO: GUI needs to be reinitialized to load new translations and/or fonts
}

// Handle mouse clics
int StelApp::handleClick(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
    distorter->distortXY(x,y);
	return ui->handle_clic(x, y, button, state);
}

// Handle mouse move
int StelApp::handleMove(int x, int y)
{
    distorter->distortXY(x,y);
	// Turn if the mouse is at the edge of the screen.
	// unless config asks otherwise
	if(FlagEnableMoveMouse)
	{
		if (x == 0)
		{
			core->turn_left(1);
			is_mouse_moving_horiz = true;
		}
		else if (x == core->getViewportWidth() - 1)
		{
			core->turn_right(1);
			is_mouse_moving_horiz = true;
		}
		else if (is_mouse_moving_horiz)
		{
			core->turn_left(0);
			is_mouse_moving_horiz = false;
		}

		if (y == 0)
		{
			core->turn_up(1);
			is_mouse_moving_vert = true;
		}
		else if (y == core->getViewportHeight() - 1)
		{
			core->turn_down(1);
			is_mouse_moving_vert = true;
		}
		else if (is_mouse_moving_vert)
		{
			core->turn_up(0);
			is_mouse_moving_vert = false;
		}
	}

	return ui->handle_move(x, y);

}

// Handle key press and release
int StelApp::handleKeys(SDLKey key, SDLMod mod,
                        Uint16 unicode, s_gui::S_GUI_VALUE state)
{
	s_tui::S_TUI_VALUE tuiv;
	if (state == s_gui::S_GUI_PRESSED) tuiv = s_tui::S_TUI_PRESSED;
	else tuiv = s_tui::S_TUI_RELEASED;
	if (ui->FlagShowTuiMenu)
	{

		if (state==S_GUI_PRESSED && key==SDLK_m)
		{
			// leave tui menu
			ui->FlagShowTuiMenu = false;

			// If selected a script in tui, run that now
			if(SelectedScript!="")
				commander->execute_command("script action play filename " +  SelectedScript
				                           + " path " + SelectedScriptDirectory);

			// clear out now
			SelectedScriptDirectory = SelectedScript = "";
			return 1;
		}
		if (ui->handle_keys_tui(unicode, tuiv)) return 1;
		return 1;
	}

	if (ui->handle_keys(key, mod, unicode, state)) return 1;

	if (state == S_GUI_PRESSED)
	{
		// Direction and zoom deplacements
		if (key==SDLK_LEFT) core->turn_left(1);
		if (key==SDLK_RIGHT) core->turn_right(1);
		if (key==SDLK_UP)
		{
			if (mod & KMOD_CTRL) core->zoom_in(1);
			else core->turn_up(1);
		}
		if (key==SDLK_DOWN)
		{
			if (mod & KMOD_CTRL) core->zoom_out(1);
			else core->turn_down(1);
		}
		if (key==SDLK_PAGEUP) core->zoom_in(1);
		if (key==SDLK_PAGEDOWN) core->zoom_out(1);
	}
	else
	{
		// When a deplacement key is released stop mooving
		if (key==SDLK_LEFT) core->turn_left(0);
		if (key==SDLK_RIGHT) core->turn_right(0);
		if (mod & KMOD_CTRL)
		{
			if (key==SDLK_UP) core->zoom_in(0);
			if (key==SDLK_DOWN) core->zoom_out(0);
		}
		else
		{
			if (key==SDLK_UP) core->turn_up(0);
			if (key==SDLK_DOWN) core->turn_down(0);
		}
		if (key==SDLK_PAGEUP) core->zoom_in(0);
		if (key==SDLK_PAGEDOWN) core->zoom_out(0);
	}
	return 0;
}


//! Set the drawing mode in 2D for drawing in the full screen
void StelApp::set2DfullscreenProjection(void) const
{
	glViewport(0, 0, screenW, screenH);
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
    glPushMatrix();						// store previous matrix
    glLoadIdentity();
    gluOrtho2D(	0, screenW,
				0, screenH);			// set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode
    glPushMatrix();
    glLoadIdentity();	
}
	
//! Restore previous projection mode
void StelApp::restoreFrom2DfullscreenProjection(void) const
{
    glMatrixMode(GL_PROJECTION);		// Restore previous matrix
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();	
}

//! Set flag for activating night vision mode
void StelApp::setVisionModeNight(void)
{
	if (!getVisionModeNight())
	{
		core->setColorScheme(getConfigFile(), "night_color");
		ui->setColorScheme(getConfigFile(), "night_color");
	}
	draw_mode=DM_NIGHT;
}

//! Set flag for activating chart vision mode
void StelApp::setVisionModeChart(void)
{
	if (!getVisionModeChart())
	{
		core->setColorScheme(getConfigFile(), "chart_color");
		ui->setColorScheme(getConfigFile(), "chart_color");
	}
	draw_mode=DM_CHART;
}

//! Set flag for activating chart vision mode 
// ["color" section name used for easier backward compatibility for older configs - Rob]
void StelApp::setVisionModeNormal(void)
{
	if (!getVisionModeNormal())
	{
		core->setColorScheme(getConfigFile(), "color");
		ui->setColorScheme(getConfigFile(), "color");
	}
	draw_mode=DM_NORMAL;
}	

double StelApp::getMouseCursorTimeout() 
{
	return ui->getMouseCursorTimeout(); 
}

// For use by TUI - saves all current settings
// TODO: Put in stel_core?

void StelApp::saveCurrentConfig(const string& confFile)
{

	// No longer resaves everything, just settings user can change through UI

    cout << "Saving configuration file " << confFile << " ..." << endl;
	InitParser conf;
	conf.load(confFile);

	// Main section
	conf.set_str	("main:version", string(VERSION));

	// localization section
	conf.set_str    ("localization:sky_culture", core->getSkyCultureDir());
	conf.set_str    ("localization:sky_locale", core->getSkyLanguage());
	conf.set_str    ("localization:app_locale", getAppLanguage());
	conf.set_str	("localization:time_display_format", get_time_format_str());
	conf.set_str	("localization:date_display_format", get_date_format_str());
	if (time_zone_mode == S_TZ_CUSTOM)
	{
		conf.set_str("localization:time_zone", custom_tz_name);
	}
	if (time_zone_mode == S_TZ_SYSTEM_DEFAULT)
	{
		conf.set_str("localization:time_zone", "system_default");
	}
	if (time_zone_mode == S_TZ_GMT_SHIFT)
	{
		conf.set_str("localization:time_zone", "gmt+x");
	}
		
	// viewing section
	conf.set_boolean("viewing:flag_constellation_drawing", core->getFlagConstellationLines());
	conf.set_boolean("viewing:flag_constellation_name", core->getFlagConstellationNames());
	conf.set_boolean("viewing:flag_constellation_art", core->getFlagConstellationArt());
	conf.set_boolean("viewing:flag_constellation_boundaries", core->getFlagConstellationBoundaries());
	conf.set_boolean("viewing:flag_constellation_pick", core->getFlagConstellationIsolateSelected());
	conf.set_double("viewing:moon_scale", core->getMoonScale());
	//conf.set_boolean("viewing:use_common_names", FlagUseCommonNames);
	conf.set_boolean("viewing:flag_equatorial_grid", core->getFlagEquatorGrid());
	conf.set_boolean("viewing:flag_azimutal_grid", core->getFlagAzimutalGrid());
	conf.set_boolean("viewing:flag_equator_line", core->getFlagEquatorLine());
	conf.set_boolean("viewing:flag_ecliptic_line", core->getFlagEclipticLine());
	conf.set_boolean("viewing:flag_cardinal_points", core->getFlagCardinalsPoints());
	conf.set_boolean("viewing:flag_meridian_line", core->getFlagMeridianLine());
	conf.set_boolean("viewing:flag_moon_scaled", core->getFlagMoonScaled());
	conf.set_double ("viewing:constellation_art_intensity", core->getConstellationArtIntensity());
	conf.set_double ("viewing:constellation_art_fade_duration", core->getConstellationArtFadeDuration());

	// Landscape section
	conf.set_boolean("landscape:flag_landscape", core->getFlagLandscape());
	conf.set_boolean("landscape:flag_atmosphere", core->getFlagAtmosphere());
	conf.set_boolean("landscape:flag_fog", core->getFlagFog());
	//	conf.set_double ("viewing:atmosphere_fade_duration", core->getAtmosphereFadeDuration());

	// Star section
	conf.set_double ("stars:star_scale", core->getStarScale());
	conf.set_double ("stars:star_mag_scale", core->getStarMagScale());
	conf.set_boolean("stars:flag_point_star", core->getFlagPointStar());
	conf.set_double("stars:max_mag_star_name", core->getMaxMagStarName());
	conf.set_boolean("stars:flag_star_twinkle", core->getFlagStarTwinkle());
	conf.set_double("stars:star_twinkle_amount", core->getStarTwinkleAmount());
	//	conf.set_double("stars:star_limiting_mag", hip_stars->core->get_limiting_mag());

	// Color section
	conf.set_str    ("color:azimuthal_color", StelUtility::vec3f_to_str(core->getColorAzimutalGrid()));
	conf.set_str    ("color:equatorial_color", StelUtility::vec3f_to_str(core->getColorEquatorGrid()));
	conf.set_str    ("color:equator_color", StelUtility::vec3f_to_str(core->getColorEquatorLine()));
	conf.set_str    ("color:ecliptic_color", StelUtility::vec3f_to_str(core->getColorEclipticLine()));
	conf.set_str    ("color:meridian_color", StelUtility::vec3f_to_str(core->getColorMeridianLine()));
	conf.set_str    ("color:const_lines_color", StelUtility::vec3f_to_str(core->getColorConstellationLine()));
	conf.set_str    ("color:const_names_color", StelUtility::vec3f_to_str(core->getColorConstellationNames()));
	conf.set_str    ("color:const_boundary_color", StelUtility::vec3f_to_str(core->getColorConstellationBoundaries()));
	conf.set_str	("color:nebula_label_color", StelUtility::vec3f_to_str(core->getColorNebulaLabels()));
	conf.set_str	("color:nebula_circle_color", StelUtility::vec3f_to_str(core->getColorNebulaCircle()));
	conf.set_str    ("color:cardinal_color", StelUtility::vec3f_to_str(core->getColorCardinalPoints()));
	conf.set_str    ("color:planet_names_color", StelUtility::vec3f_to_str(core->getColorPlanetsNames()));
	conf.set_str    ("color:planet_orbits_color", StelUtility::vec3f_to_str(core->getColorPlanetsOrbits()));
	conf.set_str    ("color:object_trails_color", StelUtility::vec3f_to_str(core->getColorPlanetsTrails()));
	//  Are these used?
	//	conf.set_str    ("color:star_label_color", StelUtility::vec3f_to_str(core->getColorStarNames()));
	//  conf.set_str    ("color:star_circle_color", StelUtility::vec3f_to_str(core->getColorStarCircles()));

	// gui section
	conf.set_double("gui:mouse_cursor_timeout",getMouseCursorTimeout());

	// not user settable yet
	// conf.set_str	("gui:gui_base_color", StelUtility::vec3f_to_str(GuiBaseColor));
	// conf.set_str	("gui:gui_text_color", StelUtility::vec3f_to_str(GuiTextColor));
	// conf.set_double ("gui:base_font_size", BaseFontSize);

	// Text ui section
	conf.set_boolean("tui:flag_show_gravity_ui", ui->getFlagShowGravityUi());
	conf.set_boolean("tui:flag_show_tui_datetime", ui->getFlagShowTuiDateTime());
	conf.set_boolean("tui:flag_show_tui_short_obj_info", ui->getFlagShowTuiShortObjInfo());

	// Navigation section
	conf.set_boolean("navigation:flag_manual_zoom", core->getFlagManualAutoZoom());
	conf.set_double ("navigation:auto_move_duration", core->getAutoMoveDuration());
	conf.set_double ("navigation:zoom_speed", core->getZoomSpeed());
	conf.set_double ("navigation:preset_sky_time", PresetSkyTime);
	conf.set_str	("navigation:startup_time_mode", StartupTimeMode);

	// Astro section
	conf.set_boolean("astro:flag_object_trails", core->getFlagPlanetsTrails());
	conf.set_boolean("astro:flag_bright_nebulae", core->getFlagBrightNebulae());
	conf.set_boolean("astro:flag_stars", core->getFlagStars());
	conf.set_boolean("astro:flag_star_name", core->getFlagStarName());
	conf.set_boolean("astro:flag_nebula", core->getFlagNebula());
	conf.set_boolean("astro:flag_nebula_name", core->getFlagNebulaHints());
	conf.set_double("astro:max_mag_nebula_name", core->getNebulaMaxMagHints());
	conf.set_boolean("astro:flag_planets", core->getFlagPlanets());
	conf.set_boolean("astro:flag_planets_hints", core->getFlagPlanetsHints());
	conf.set_boolean("astro:flag_planets_orbits", core->getFlagPlanetsOrbits());

	conf.set_boolean("astro:flag_milky_way", core->getFlagMilkyWay());
	conf.set_double("astro:milky_way_intensity", core->getMilkyWayIntensity());

	// Get landscape and other observatory info
	// TODO: shouldn't observator already know what section to save in?
	(core->getObservatory()).setConf(conf, "init_location");

	conf.save(confFile);

}


void StelApp::recordCommand(string commandline) {
	scripts->record_command(commandline);
}


// Return a string with the UTC date formated according to the date_format variable
wstring StelApp::get_printable_date_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char date[255];
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT : StelUtility::my_strftime(date, 254, "%x", &time_utc); break;
		case S_DATE_MMDDYYYY : StelUtility::my_strftime(date, 254, "%m/%d/%Y", &time_utc); break;
		case S_DATE_DDMMYYYY : StelUtility::my_strftime(date, 254, "%d/%m/%Y", &time_utc); break;
		case S_DATE_YYYYMMDD : StelUtility::my_strftime(date, 254, "%Y-%m-%d", &time_utc); break;
	}
	return StelUtility::stringToWstring(date);
}

// Return a string with the UTC time formated according to the time_format variable
// TODO : for some locales (french) the %p returns nothing
wstring StelApp::get_printable_time_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char heure[255];
	switch(time_format)
	{
		case S_TIME_SYSTEM_DEFAULT : StelUtility::my_strftime(heure, 254, "%X", &time_utc); break;
		case S_TIME_24H : StelUtility::my_strftime(heure, 254, "%H:%M:%S", &time_utc); break;
		case S_TIME_12H : StelUtility::my_strftime(heure, 254, "%I:%M:%S %p", &time_utc); break;
	}
	return StelUtility::stringToWstring(heure);
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
string StelApp::get_ISO8601_time_local(double JD) const
{
	struct tm time_local;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + StelUtility::get_GMT_shift_from_system(JD)*0.041666666666, &time_local);

	static char isotime[255];
	StelUtility::my_strftime(isotime, 254, "%Y-%m-%d %H:%M:%S", &time_local);
	return isotime;
}


// Return a string with the local date formated according to the date_format variable
wstring StelApp::get_printable_date_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + StelUtility::get_GMT_shift_from_system(JD)*0.041666666666, &time_local);

	static char date[255];
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT : StelUtility::my_strftime(date, 254, "%x", &time_local); break;
		case S_DATE_MMDDYYYY : StelUtility::my_strftime(date, 254, "%m/%d/%Y", &time_local); break;
		case S_DATE_DDMMYYYY : StelUtility::my_strftime(date, 254, "%d/%m/%Y", &time_local); break;
		case S_DATE_YYYYMMDD : StelUtility::my_strftime(date, 254, "%Y-%m-%d", &time_local); break;
	}

	return StelUtility::stringToWstring(date);
}

// Return a string with the local time (according to time_zone_mode variable) formated
// according to the time_format variable
wstring StelApp::get_printable_time_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + StelUtility::get_GMT_shift_from_system(JD)*0.041666666666, &time_local);

	static char heure[255];
	switch(time_format)
	{
		case S_TIME_SYSTEM_DEFAULT : StelUtility::my_strftime(heure, 254, "%X", &time_local); break;
		case S_TIME_24H : StelUtility::my_strftime(heure, 254, "%H:%M:%S", &time_local); break;
		case S_TIME_12H : StelUtility::my_strftime(heure, 254, "%I:%M:%S %p", &time_local); break;
	}
	return StelUtility::stringToWstring(heure);
}

// Convert the time format enum to its associated string and reverse
StelApp::S_TIME_FORMAT StelApp::string_to_s_time_format(const string& tf) const
{
	if (tf == "system_default") return S_TIME_SYSTEM_DEFAULT;
	if (tf == "24h") return S_TIME_24H;
	if (tf == "12h") return S_TIME_12H;
	cout << "ERROR : unrecognized time_display_format : " << tf << " system_default used." << endl;
	return S_TIME_SYSTEM_DEFAULT;
}

string StelApp::s_time_format_to_string(S_TIME_FORMAT tf) const
{
	if (tf == S_TIME_SYSTEM_DEFAULT) return "system_default";
	if (tf == S_TIME_24H) return "24h";
	if (tf == S_TIME_12H) return "12h";
	cout << "ERROR : unrecognized time_display_format value : " << tf << " system_default used." << endl;
	return "system_default";
}

// Convert the date format enum to its associated string and reverse
StelApp::S_DATE_FORMAT StelApp::string_to_s_date_format(const string& df) const
{
	if (df == "system_default") return S_DATE_SYSTEM_DEFAULT;
	if (df == "mmddyyyy") return S_DATE_MMDDYYYY;
	if (df == "ddmmyyyy") return S_DATE_DDMMYYYY;
	if (df == "yyyymmdd") return S_DATE_YYYYMMDD;  // iso8601
	cout << "ERROR : unrecognized date_display_format : " << df << " system_default used." << endl;
	return S_DATE_SYSTEM_DEFAULT;
}

string StelApp::s_date_format_to_string(S_DATE_FORMAT df) const
{
	if (df == S_DATE_SYSTEM_DEFAULT) return "system_default";
	if (df == S_DATE_MMDDYYYY) return "mmddyyyy";
	if (df == S_DATE_DDMMYYYY) return "ddmmyyyy";
	if (df == S_DATE_YYYYMMDD) return "yyyymmdd";
	cout << "ERROR : unrecognized date_display_format value : " << df << " system_default used." << endl;
	return "system_default";
}

void StelApp::set_custom_tz_name(const string& tzname)
{
	custom_tz_name = tzname;
	time_zone_mode = S_TZ_CUSTOM;

	if( custom_tz_name != "")
	{
		// set the TZ environement variable and update c locale stuff
		putenv(strdup((string("TZ=") + custom_tz_name).c_str()));
		tzset();
	}
}

float StelApp::get_GMT_shift(double JD, bool _local) const
{
	if (time_zone_mode == S_TZ_GMT_SHIFT) return GMT_shift;
	else return StelUtility::get_GMT_shift_from_system(JD,_local);
}
