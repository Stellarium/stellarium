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

StelApp::StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT) : 
		frame(0), timefr(0), timeBase(0), fps(0), maxfps(10000.f),  FlagTimePause(0), 
		is_mouse_moving_horiz(false), is_mouse_moving_vert(false), draw_mode(StelApp::DM_NONE),
		initialized(0)
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
  distorter = ViewportDistorter::create(type,screenW,screenH);
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
			wcout << _("Attempting to use an existing older config file.") << endl;
		}
	}

	// don't mess with SDL init if already initialized earlier
	if(!initialized) {
		screenW = conf.get_int("video:screen_w");
		screenH = conf.get_int("video:screen_h");
		initSDL(screenW, screenH, conf.get_int("video:bbp_mode"), conf.get_boolean("video:fullscreen"), core->getDataDir() + "/icon.bmp");	
	}

	maxfps 				= conf.get_double ("video","maximum_fps",10000);
	string appLocaleName = conf.get_str("localization", "app_locale", "system");
	setAppLanguage(appLocaleName);
	scripts->set_allow_ui( conf.get_boolean("gui","flag_script_allow_ui",0) );

	core->init(conf);

	string tmpstr = conf.get_str("projection:viewport");
	if (tmpstr=="maximized") core->setMaximizedViewport(screenW, screenH);
	else
		if (tmpstr=="square") core->setSquareViewport(screenW, screenH, conf.get_int("video:horizontal_offset"), conf.get_int("video:horizontal_offset"));
		else
		{
			if (tmpstr=="disk") core->getViewportMaskDisk();
			else
			{
				cerr << "ERROR : Unknown viewport type : " << tmpstr << endl;
				exit(-1);
			}
		}	

	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	FlagEnableMoveMouse	= conf.get_boolean("navigation","flag_enable_move_mouse",1);
	MouseZoom			= conf.get_int("navigation","mouse_zoom",30);
	
	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		core->setJDay(PresetSkyTime - core->getObservatory().get_GMT_shift(PresetSkyTime) * JD_HOUR);
	
	// initialisation of the User Interface
	ui->init(conf);
	ui->init_tui();
	
	// Initialisation of the color scheme
	draw_mode = StelApp::DM_NONE;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_chart")) setVisionModeChart();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();
	
    if (distorter == 0) {
      setViewPortDistorterType(conf.get_str("video","distorter","none"));
    }
    
    core->setTimeNow();

	// play startup script, if available
	if(scripts) scripts->play_startup_script();

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
void StelApp::draw(int delta_time)
{
	// Render all the main objects of stellarium
	core->draw(delta_time);

	// Draw the Graphical ui and the Text ui
	ui->draw();

    distorter->distort();
}

//! @brief Set the application locale. This apply to GUI, console messages etc..
void StelApp::setAppLanguage(const std::string& newAppLocaleName)
{
	// Update the translator with new locale name
	Translator::globalTranslator = Translator(PACKAGE, core->getLocaleDir(), newAppLocaleName);
	cout << "Application locale is " << Translator::globalTranslator.getLocaleName() << endl;
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
int StelApp::handleKeys(Uint16 key, s_gui::S_GUI_VALUE state)
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
		if (ui->handle_keys_tui(key, tuiv)) return 1;
		return 1;
	}

	if (ui->handle_keys(key, state)) return 1;

	if (state == S_GUI_PRESSED)
	{
		// Direction and zoom deplacements
		if (key==SDLK_LEFT) core->turn_left(1);
		if (key==SDLK_RIGHT) core->turn_right(1);
		if (key==SDLK_UP)
		{
			if (SDL_GetModState() & KMOD_CTRL) core->zoom_in(1);
			else core->turn_up(1);
		}
		if (key==SDLK_DOWN)
		{
			if (SDL_GetModState() & KMOD_CTRL) core->zoom_out(1);
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
		if (SDL_GetModState() & KMOD_CTRL)
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

	conf.set_int    ("video:horizontal_offset", core->getViewportHorizontalOffset());
	conf.set_int    ("video:vertical_offset", core->getViewportVerticalOffset());

	// localization section
	conf.set_str    ("localization:sky_culture", core->getSkyCultureDir());
	conf.set_str    ("localization:sky_locale", core->getSkyLanguage());

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
