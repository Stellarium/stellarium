/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include "stelapp.h"

#include "viewport_distorter.h"
#include "stel_utility.h"
#include "callbacks.hpp"
#include "stel_command_interface.h"
#include "stel_ui.h"
#include "stelmodulemgr.h"
#include "stelfontmgr.h"
#include "stellocalemgr.h"
#include "stelskyculturemgr.h"
#include "hip_star_mgr.h"

// Initialize static variables
StelApp* StelApp::singleton = NULL;

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT) :
		frame(0), timefr(0), timeBase(0), fps(0), maxfps(10000.f),  FlagTimePause(0),
		is_mouse_moving_horiz(false), is_mouse_moving_vert(false), draw_mode(StelApp::DM_NONE),
		initialized(0)
{
	// Can't create 2 StelApp instances
	assert(!singleton);
	singleton = this;

	configDir = CDIR;
	localeDir = LDIR;
	dataDir = DATA_ROOT+"/data";
	rootDir = DATA_ROOT + "/";
	
	// Set textures directory
	s_texture::set_texDir(rootDir + "textures/");
	
	localeMgr = new StelLocaleMgr();
	fontManager = new StelFontMgr(getDataFilePath("fontmap.dat"));
	skyCultureMgr = new StelSkyCultureMgr(getDataFilePath("sky_cultures"));
	
	moduleMgr = new StelModuleMgr();
	
	core = new StelCore(LDIR, DATA_ROOT, boost::callback<void, string>(this, &StelApp::recordCommand));
	ui = new StelUI(core, this);
	commander = new StelCommandInterface(core, this);
	scripts = new ScriptMgr(commander, dataDir);
	time_multiplier = 1;
	distorter = 0;
	init();
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	SDL_FreeCursor(Cursor);
	delete ui;
	delete scripts;
	delete commander;
	delete core;
	if (distorter) delete distorter;
	delete skyCultureMgr;
	delete localeMgr;
	delete fontManager;
}

/*************************************************************************
 Get the configuration file path.
*************************************************************************/
string StelApp::getConfigFilePath(void) const
{
	return configDir + "config.ini";
}

/*************************************************************************
 Get the full path to a data file. This method will try to find the file 
 in all valid data directories until it finds it.
*************************************************************************/
string StelApp::getDataFilePath(const string& dataFileName) const
{
	// Absolute path if starts by '/'
	if (dataFileName!="" && dataFileName[0]=='/')
		return dataFileName;
	return dataDir + "/" + dataFileName;
}

/*************************************************************************
 Get the full path to a texture file. This method will try to find the file in all valid data 
*************************************************************************/
string StelApp::getTextureFilePath(const string& textureFileName) const
{
	return rootDir + "textures/" + textureFileName;
}


void StelApp::setViewPortDistorterType(const string &type)
{
	if (distorter)
	{
		if (distorter->getType() == type) return;
		delete distorter;
		distorter = 0;
	}
	distorter = ViewportDistorter::create(type,screenW,screenH);
	InitParser conf;
	conf.load(configDir + "config.ini");
	distorter->init(conf);
}

string StelApp::getViewPortDistorterType(void) const
{
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

	// Load language codes
	Translator::initIso639_1LanguageCodes(getDataFilePath("iso639-1.utf8"));
	
	// Initialize video device and other sdl parameters
	InitParser conf;
	conf.load(configDir + "config.ini");

	// Main section
	string version = conf.get_str("main:version");

	if (version!=string(VERSION))
	{

		std::istringstream istr(version);
		char tmp;
		int v1 =0;
		int v2 =0;
		istr >> v1 >> tmp >> v2;

		// Config versions less than 0.6.0 are not supported, otherwise we will try to use it
		if( v1 == 0 && v2 < 6 )
		{

			// The config file is too old to try an importation
			printf("The current config file is from a version too old for parameters to be imported (%s).\nIt will be replaced by the default config file.\n", version.empty() ? "<0.6.0" : version.c_str());
			system( (string("cp -f ") + dataDir + "default_config.ini " + getConfigFilePath()).c_str() );
			conf.load(configDir + "config.ini");  // Read new config!

		}
		else
		{
			cout << "Attempting to use an existing older config file." << endl;
		}
	}

	// don't mess with SDL init if already initialized earlier
	if(!initialized)
	{
		screenW = conf.get_int("video:screen_w");
		screenH = conf.get_int("video:screen_h");
		initSDL(screenW, screenH, conf.get_int("video:bbp_mode"), conf.get_boolean("video:fullscreen"), getDataFilePath("icon.bmp"));
	}

	// Clear screen, this fixes a strange artifact at loading time in the upper top corner.
	glClear(GL_COLOR_BUFFER_BIT);

	maxfps 				= conf.get_double ("video","maximum_fps",10000);
	minfps 				= conf.get_double ("video","minimum_fps",10000);

	scripts->set_allow_ui( conf.get_boolean("gui","flag_script_allow_ui",0) );

	localeMgr->init(conf);
	skyCultureMgr->init(conf);

	core->init(conf);

	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	FlagEnableMoveMouse	= conf.get_boolean("navigation","flag_enable_move_mouse",1);
	MouseZoom			= conf.get_int("navigation","mouse_zoom",30);

	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		core->getNavigation()->setJDay(PresetSkyTime - localeMgr->get_GMT_shift(PresetSkyTime) * JD_HOUR);
	else core->setTimeNow();

	// initialisation of the User Interface

	// TODO: Need way to update settings from config without reinitializing whole gui
	ui->init(conf);

	if(!initialized) ui->init_tui();  // don't reinit tui since probably called from there
	else ui->localizeTui();  // update translations/fonts as needed

	// Initialisation of the color scheme
	draw_mode = StelApp::DM_NONE;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();

	if (distorter == 0)
	{
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
		else if (x == core->getProjection()->getViewportWidth() - 1)
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
		else if (y == core->getProjection()->getViewportHeight() - 1)
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
#ifdef MACOSX
		if ( key == SDLK_LEFT || 
								key == SDLK_RIGHT || 
								key == SDLK_UP || 
								key == SDLK_DOWN || 
								key == SDLK_BACKSPACE ) {
							if (ui->handle_keys_tui(key, tuiv)) return 1;
							if (ui->handle_keys(key, mod, key, state)) return 1;
								}
#endif
	s_tui::S_TUI_VALUE tuiv;
	if (state == s_gui::S_GUI_PRESSED) tuiv = s_tui::S_TUI_PRESSED;
	else tuiv = s_tui::S_TUI_RELEASED;
	if (ui->FlagShowTuiMenu)
	{

		if (state==S_GUI_PRESSED && unicode=='m')
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
		core->setColorScheme(getConfigFilePath(), "night_color");
		ui->setColorScheme(getConfigFilePath(), "night_color");
	}
	draw_mode=DM_NIGHT;
}

//! Set flag for activating chart vision mode
// ["color" section name used for easier backward compatibility for older configs - Rob]
void StelApp::setVisionModeNormal(void)
{
	if (!getVisionModeNormal())
	{
		core->setColorScheme(getConfigFilePath(), "color");
		ui->setColorScheme(getConfigFilePath(), "color");
	}
	draw_mode=DM_NORMAL;
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
	conf.set_double("viewing:moon_scale", ssmgr->getMoonScale());
	conf.set_boolean("viewing:flag_equatorial_grid", core->getFlagEquatorGrid());
	conf.set_boolean("viewing:flag_azimutal_grid", core->getFlagAzimutalGrid());
	conf.set_boolean("viewing:flag_equator_line", core->getFlagEquatorLine());
	conf.set_boolean("viewing:flag_ecliptic_line", core->getFlagEclipticLine());
	conf.set_boolean("viewing:flag_cardinal_points", core->getFlagCardinalsPoints());
	conf.set_boolean("viewing:flag_meridian_line", core->getFlagMeridianLine());
	conf.set_boolean("viewing:flag_moon_scaled", ssmgr->getFlagMoonScale());

	// Landscape section
	conf.set_boolean("landscape:flag_landscape", core->getFlagLandscape());
	conf.set_boolean("landscape:flag_atmosphere", core->getFlagAtmosphere());
	conf.set_boolean("landscape:flag_fog", core->getFlagFog());
	//	conf.set_double ("viewing:atmosphere_fade_duration", core->getAtmosphereFadeDuration());

	// Star section
	HipStarMgr* smgr = (HipStarMgr*)moduleMgr->getModule("stars");
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
	conf.set_str    ("color:azimuthal_color", StelUtils::vec3f_to_str(core->getColorAzimutalGrid()));
	conf.set_str    ("color:equatorial_color", StelUtils::vec3f_to_str(core->getColorEquatorGrid()));
	conf.set_str    ("color:equator_color", StelUtils::vec3f_to_str(core->getColorEquatorLine()));
	conf.set_str    ("color:ecliptic_color", StelUtils::vec3f_to_str(core->getColorEclipticLine()));
	conf.set_str    ("color:meridian_color", StelUtils::vec3f_to_str(core->getColorMeridianLine()));
	conf.set_str	("color:nebula_label_color", StelUtils::vec3f_to_str(nmgr->getNamesColor()));
	conf.set_str	("color:nebula_circle_color", StelUtils::vec3f_to_str(nmgr->getCirclesColor()));
	conf.set_str    ("color:cardinal_color", StelUtils::vec3f_to_str(core->getColorCardinalPoints()));
	conf.set_str    ("color:planet_names_color", StelUtils::vec3f_to_str(ssmgr->getNamesColor()));
	conf.set_str    ("color:planet_orbits_color", StelUtils::vec3f_to_str(ssmgr->getOrbitsColor()));
	conf.set_str    ("color:object_trails_color", StelUtils::vec3f_to_str(ssmgr->getTrailsColor()));

	// gui section
	conf.set_double("gui:mouse_cursor_timeout",ui->getMouseCursorTimeout());

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
	conf.set_boolean("astro:flag_object_trails", ssmgr->getFlagTrails());
	conf.set_boolean("astro:flag_bright_nebulae", nmgr->getFlagBright());
	conf.set_boolean("astro:flag_nebula", nmgr->getFlagShow());
	conf.set_boolean("astro:flag_nebula_name", nmgr->getFlagHints());
	conf.set_double("astro:max_mag_nebula_name", nmgr->getMaxMagHints());
	conf.set_boolean("astro:flag_planets", ssmgr->getFlagPlanets());
	conf.set_boolean("astro:flag_planets_hints", ssmgr->getFlagHints());
	conf.set_boolean("astro:flag_planets_orbits", ssmgr->getFlagOrbits());

	conf.set_boolean("astro:flag_milky_way", core->getFlagMilkyWay());
	conf.set_double("astro:milky_way_intensity", core->getMilkyWayIntensity());

	// Get landscape and other observatory info
	// TODO: shouldn't observator already know what section to save in?
	(core->getObservatory()).setConf(conf, "init_location");

	conf.save(confFile);

}


void StelApp::recordCommand(string commandline)
{
	scripts->record_command(commandline);
}


// Update translations and font everywhere in the program
void StelApp::updateAppLanguage()
{
	// update translations and font in tui
	ui->localizeTui();
}

// Update translations and font for sky everywhere in the program
void StelApp::updateSkyLanguage()
{
	core->updateSkyLanguage();
}

// Update and reload sky culture informations everywhere in the program
void StelApp::updateSkyCulture()
{
	core->updateSkyCulture();
}
