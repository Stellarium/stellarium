/*
 * Copyright (C) 2003 Fabien Chéreau
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

// Main class for stellarium
// Manage all the objects to be used in the program

#include "stel_core.h"
#include "stellastro.h"
#include "draw.h"

stel_core::stel_core() : screen_W(800), screen_H(600), bppMode(16), Fullscreen(0),
	navigation(NULL), observatory(NULL), projection(NULL), selected_object(NULL), hip_stars(NULL), asterisms(NULL),
	nebulas(NULL), atmosphere(NULL), tone_converter(NULL), selected_constellation(NULL),
	frame(0), timefr(0), timeBase(0), deltaFov(0.), deltaAlt(0.), deltaAz(0.),
	move_speed(0.001), FlagTimePause(0)
{
	ProjectorType = PERSPECTIVE_PROJECTOR;
}

stel_core::~stel_core()
{
	if (navigation) delete navigation;
	if (projection) delete projection;
	if (asterisms) delete asterisms;
	if (hip_stars) delete hip_stars;
	if (nebulas) delete nebulas;
	if (equ_grid) delete equ_grid;
	if (azi_grid) delete azi_grid;
	if (equator_line) delete equator_line;
	if (ecliptic_line) delete ecliptic_line;
	if (cardinals_points) delete cardinals_points;
	if (milky_way) delete milky_way;
	if (atmosphere) delete atmosphere;
	if (tone_converter) delete tone_converter;
	if (ssystem) delete ssystem;
	if (ui) delete ui;
}

// Set the main data, textures and configuration directories
void stel_core::set_directories(const string& DDIR, const string& TDIR, const string& CDIR, const string& DATA_ROOT)
{
	TextureDir = TDIR;
	ConfigDir = CDIR;
	DataDir = DDIR;
	DataRoot = DATA_ROOT;
}

// Set the 2 config files names.
void stel_core::set_config_files(const string& _config_file, const string& _location_file)
{
	config_file = _config_file;
	location_file = _location_file;
}


void stel_core::init(void)
{
	// Set textures directory and suffix
	s_texture::set_texDir(TextureDir);
	s_texture::set_suffix(".png");

	observatory = new Observator();
	observatory->load(ConfigDir + config_file, "init_location");

	navigation = new navigator(observatory);
	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		navigation->set_JDay(PresetSkyTime - observatory->get_GMT_shift(PresetSkyTime) * JD_HOUR);
	else navigation->set_JDay(get_julian_from_sys());
	navigation->set_local_vision(InitViewPos);

    hip_stars = new Hip_Star_mgr();
    asterisms = new Constellation_mgr();
    nebulas   = new Nebula_mgr();
	ssystem = new SolarSystem();
	atmosphere = new stel_atmosphere();
	tone_converter = new tone_reproductor();
	equ_grid = new SkyGrid(EQUATORIAL);
	azi_grid = new SkyGrid(ALTAZIMUTAL);
	equator_line = new SkyLine(EQUATOR);
	ecliptic_line = new SkyLine(ECLIPTIC);
	switch (ProjectorType)
	{
	case PERSPECTIVE_PROJECTOR :
		projection = new Projector(screen_W, screen_H, InitFov);
		break;
	case FISHEYE_PROJECTOR :
		projection = new Fisheye_projector(screen_W, screen_H, InitFov);
		break;
	default :
		projection = new Projector(screen_W, screen_H, InitFov);
		break;
	}

	cardinals_points = new Cardinals(DataDir + "spacefont.txt", "spacefont");
	milky_way = new MilkyWay("voielactee256x256");

    // Load hipparcos stars & names
    hip_stars->load(DataDir + "spacefont.txt", DataDir + "hipparcos.fab",
		DataDir + "commonname.fab",	DataDir + "name.fab");

	// Load constellations
    asterisms->load(DataDir + "spacefont.txt", DataDir + "constellationship.fab", DataDir + "constellationsart.fab", hip_stars);

	// Load the nebulas data TODO : add NGC objects
    nebulas->read(DataDir + "spacefont.txt", DataDir + "messier.fab");

	// Create and init the solar system
	ssystem->init(DataDir + "spacefont.txt", DataDir + "ssystem.ini");

	landscape = Landscape::create_from_file(DataDir + "landscapes.ini", observatory->get_landscape_name());

	// Load the pointer textures
	stel_object::init_textures();

	// initialisation of the User Interface
	ui = new stel_ui(this);
    ui->init();

	ui->init_tui();

	if (FlagAtmosphere) tone_converter->set_world_adaptation_luminance(40000.f);
	else tone_converter->set_world_adaptation_luminance(3.75f);

	// Make the viewport as big as possible
	projection->set_screen_size(screen_W, screen_H);
	projection->set_fov(InitFov);

	switch (ViewportType)
	{
		case MAXIMIZED : projection->maximize_viewport(); break;
		case SQUARE : projection->set_square_viewport(); break;
		case DISK : projection->set_disk_viewport(); break;
		default : projection->maximize_viewport(); break;
	}

	// Set the default moon scaling
	if (FlagInitMoonScaled) ssystem->get_moon()->set_sphere_scale(moon_scale);

	// Compute planets data and init viewing position
	// Position of sun and all the satellites (ie planets)
	ssystem->compute_positions(navigation->get_JDay());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->compute_trans_matrices(navigation->get_JDay());

	// Compute transform matrices between coordinates systems
	navigation->update_transform_matrices((ssystem->get_earth())->get_ecliptic_pos());
	navigation->update_model_view_mat();

	glClear(GL_COLOR_BUFFER_BIT);

}


// Update all the objects in function of the time
void stel_core::update(int delta_time)
{
	frame++;
    timefr+=delta_time;
    if (timefr-timeBase > 1000)
    {
		fps=frame*1000.0/(timefr-timeBase);				// Calc the FPS rate
        frame = 0;
        timeBase+=1000;
    }

    // Update the position of observation and time etc...
	navigation->update_time(delta_time);

	// Position of sun and all the satellites (ie planets)
	ssystem->compute_positions(navigation->get_JDay());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->compute_trans_matrices(navigation->get_JDay());


	// Transform matrices between coordinates systems
	navigation->update_transform_matrices((ssystem->get_earth())->get_ecliptic_pos());
	// Direction of vision
	navigation->update_vision_vector(delta_time, selected_object);
	// Field of view
	projection->update_auto_zoom(delta_time);

	// Move the view direction and/or fov
	update_move(delta_time);

	// Update info about selected object
	if (selected_object) selected_object->update();

	// compute global sky brightness TODO : make this more "scientifically"
	// Compute the sun position in local coordinate
	Vec3d temp(0.,0.,0.);
	Vec3d sunPos = navigation->helio_to_local(temp);
	sunPos.normalize();
	if (FlagAtmosphere) sky_brightness = sunPos[2];
	else sky_brightness = 0.1;
	if (sky_brightness<0) sky_brightness=0;
	landscape->set_sky_brightness(sky_brightness);

	ui->update();
	if (FlagShowTui) ui->tui_update_widgets();
}

// Execute all the drawing functions
void stel_core::draw(int delta_time)
{
	// Init openGL viewing with fov, screen size and clip planes
	projection->set_clipping_planes(0.0005 ,50);

	// Give the updated standard projection matrices to the projector
	projection->set_modelview_matrices(	navigation->get_earth_equ_to_eye_mat(),
										navigation->get_helio_to_eye_mat(),
										navigation->get_local_to_eye_mat());

    // Set openGL drawings in equatorial coordinates
    navigation->switch_to_earth_equatorial();

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw the milky way. If not activated, need at least to clear the color buffer
	if (!FlagMilkyWay) glClear(GL_COLOR_BUFFER_BIT);
	else milky_way->draw(tone_converter, projection, navigation);

	// Draw all the constellations
	if (FlagConstellationDrawing)
	{
		if (FlagConstellationPick && selected_constellation)
			selected_constellation->draw(projection);
		else asterisms->draw(projection);
	}

	// Draw the constellations art
	if (FlagConstellationArt)
	{
		if (FlagConstellationPick && selected_constellation)
			selected_constellation->draw_art(projection);
		else asterisms->draw_art(projection);
	}

	// Draw the constellations's names
	if (FlagConstellationName)
	{
		if (FlagConstellationPick && selected_constellation)
			asterisms->draw_one_name(projection, selected_constellation, FlagGravityLabels);
		else asterisms->draw_names(projection, FlagGravityLabels);
	}

	// Draw the nebula if they are visible
	if (FlagNebula && (!FlagAtmosphere || sky_brightness<0.1))
		nebulas->draw(FlagNebulaName, projection, tone_converter, FlagGravityLabels);

	// Draw the hipparcos stars
	Vec3d tempv = navigation->get_equ_vision();
	Vec3f temp(tempv[0],tempv[1],tempv[2]);

	if (FlagStars && (!FlagAtmosphere || sky_brightness<0.1))
	{
		hip_stars->draw_point(StarScale, StarMagScale, FlagStarTwinkle ? StarTwinkleAmount : 0.f, FlagStarName,
		MaxMagStarName, temp, tone_converter, projection, FlagGravityLabels);
	}

	// Draw the equatorial grid
	if (FlagEquatorialGrid) equ_grid->draw(projection);
	// Draw the altazimutal grid
    if (FlagAzimutalGrid) azi_grid->draw(projection);

	// Draw the celestial equator line
    if (FlagEquatorLine) equator_line->draw(projection);
	// Draw the ecliptic line
    if (FlagEclipticLine) ecliptic_line->draw(projection);

	// Draw the pointer on the currently selected object
    if (selected_object) selected_object->draw_pointer(delta_time, projection, navigation);

	// Draw the planets
	if (FlagPlanets) ssystem->draw(FlagPlanetsHints, projection, navigation, tone_converter,
		FlagGravityLabels, FlagPointStar);

	// Set openGL drawings in local coordinates i.e. generally altazimuthal coordinates
	navigation->switch_to_local();

	// Compute the sun position in local coordinate
	Vec3d temp2(0.,0.,0.);
	Vec3d sunPos = navigation->helio_to_local(temp2);
	sunPos.normalize();

	// Compute the moon position in local coordinate
	temp2 = ssystem->get_moon()->get_heliocentric_ecliptic_pos();
	Vec3d moonPos = navigation->helio_to_local(temp2);
	moonPos.normalize();

	// Compute the atmosphere color
	if (FlagAtmosphere)
	{
		//navigation->switch_to_local();
		atmosphere->compute_color(navigation->get_JDay(), sunPos, moonPos,
		 	ssystem->get_moon()->get_phase(ssystem->get_earth()->get_heliocentric_ecliptic_pos()),
		 	tone_converter, projection, observatory->get_latitude(), observatory->get_altitude(),
			15.f, 40.f);	// Temperature = 15°c, relative humidity = 40%
	}

	// Draw the atmosphere
	if (FlagAtmosphere)	atmosphere->draw(projection);

	// Draw the landscape
	landscape->draw(tone_converter, projection, navigation,	FlagFog, FlagHorizon && FlagGround, FlagGround);

	// Daw the cardinal points
    if (FlagCardinalPoints) cardinals_points->draw(projection, FlagGravityLabels);

	projection->draw_viewport_shape();

    // Draw the Graphical ui and the Text ui
    ui->draw();
	if (FlagShowTui) ui->draw_tui();
	ui->draw_gravity_ui();
}

void stel_core::set_landscape(const string& new_landscape_name)
{
	if (new_landscape_name.empty() || new_landscape_name==observatory->get_landscape_name()) return;
	if (landscape) delete landscape;
	landscape = NULL;
	landscape = Landscape::create_from_file(DataDir + "landscapes.ini", new_landscape_name);
	observatory->set_landscape_name(new_landscape_name);
}

void stel_core::load_config(void)
{
	init_parser conf;
	conf.load(ConfigDir + config_file);

	// Main section
	string version = conf.get_str("main:version");
	if (version!=string(VERSION))
	{
		// The config file is too old to try an importation
		cout << "The current config file is from a version too old (" <<
			(version.empty() ? "<0.6.0" : version) << ")." << endl;
		cout << "It will be replaced by the default config file." << endl;
		system( (string("cp -f ") + DataRoot + "/config/default_config.txt " + ConfigDir + config_file).c_str() );
	}

	// Actually load the config file
	load_config_from(ConfigDir + config_file);

	if (observatory) observatory->load(ConfigDir + config_file, "init_location");
}

void stel_core::save_config(void)
{
	// The config file is supposed to be valid and from the correct stellarium version.
	// This is normally the case if the program is running.
	save_config_to(ConfigDir + config_file);

	if (observatory) observatory->save(ConfigDir + config_file, "init_location");
}

void stel_core::load_config_from(const string& confFile)
{
    cout << "Loading configuration file " << confFile << " ..." << endl;
	init_parser conf;
	conf.load(confFile);

	// Main section (check for version mismatch)
	string version = conf.get_str("main:version");
	if (version!=string(VERSION))
	{
		cout << "ERROR : The current config file is from a version too old (" <<
			(version.empty() ? "<0.6.0" : version) << ")." << endl;
		exit(-1);
	}

	// Video Section
	Fullscreen			= conf.get_boolean("video:fullscreen");
	screen_W			= conf.get_int	   ("video:screen_w");
	screen_H			= conf.get_int	   ("video:screen_h");
	bppMode				= conf.get_int    ("video:bbp_mode");

	// Projector
	string tmpstr = conf.get_str("projection:type");
	if (tmpstr=="perspective") ProjectorType = PERSPECTIVE_PROJECTOR;
	else
	{
		if (tmpstr=="fisheye") ProjectorType = FISHEYE_PROJECTOR;
		else
		{
			cout << "ERROR : Unknown projector type : " << tmpstr << endl;
			exit(-1);
		}
	}

	tmpstr = conf.get_str("projection:viewport");
	if (tmpstr=="maximized") ViewportType = MAXIMIZED;
	else
	if (tmpstr=="square") ViewportType = SQUARE;
	else
	{
		if (tmpstr=="disk") ViewportType = DISK;
		else
		{
			cout << "ERROR : Unknown viewport type : " << tmpstr << endl;
			exit(-1);
		}
	}

	// Star section
	StarScale			= conf.get_double ("stars:star_scale");
	StarMagScale		= conf.get_double ("stars:star_mag_scale");
	StarTwinkleAmount	= conf.get_double ("stars:star_twinkle_amount");
	MaxMagStarName		= conf.get_double ("stars:max_mag_star_name");
	FlagStarTwinkle		= conf.get_boolean("stars:flag_star_twinkle");
	FlagPointStar		= conf.get_boolean("stars:flag_point_star");

	// Ui section
	FlagShowFps			= conf.get_boolean("gui:flag_show_fps");
	FlagMenu			= conf.get_boolean("gui:flag_menu");
	FlagHelp			= conf.get_boolean("gui:flag_help");
	FlagInfos			= conf.get_boolean("gui:flag_infos");
	FlagShowTopBar		= conf.get_boolean("gui:flag_show_topbar");
	FlagShowTime		= conf.get_boolean("gui:flag_show_time");
	FlagShowDate		= conf.get_boolean("gui:flag_show_date");
	FlagShowAppName		= conf.get_boolean("gui:flag_show_appname");
	FlagShowFov			= conf.get_boolean("gui:flag_show_fov");
	FlagShowSelectedObjectInfos = conf.get_boolean("gui:flag_show_selected_object_info");
	GuiBaseColor		= str_to_vec3f(conf.get_str("gui:gui_base_color").c_str());
	GuiTextColor		= str_to_vec3f(conf.get_str("gui:gui_text_color").c_str());

	// Text ui section
	FlagShowTui = conf.get_boolean("tui:flag_show_tui");
	FlagShowTuiDateTime = conf.get_boolean("tui:flag_show_tui_datetime");
	FlagShowTuiShortInfo = conf.get_boolean("tui:flag_show_tui_short_obj_info");

	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	FlagEnableZoomKeys	= conf.get_boolean("navigation:flag_enable_zoom_keys");
	FlagEnableMoveKeys	= conf.get_boolean("navigation:flag_enable_move_keys");
	InitFov				= conf.get_double ("navigation","init_fov",60.);
	InitViewPos 		= str_to_vec3f(conf.get_str("navigation:init_view_pos").c_str());
	auto_move_duration	= conf.get_double ("navigation","auto_move_duration",1.5);
	FlagUTC_Time		= conf.get_boolean("navigation:flag_utc_time");

	// Landscape section
	FlagGround			= conf.get_boolean("landscape:flag_ground");
	FlagHorizon			= conf.get_boolean("landscape:flag_horizon");
	FlagFog				= conf.get_boolean("landscape:flag_fog");
	FlagAtmosphere		= conf.get_boolean("landscape:flag_atmosphere");

	// Viewing section
	FlagConstellationDrawing= conf.get_boolean("viewing:flag_constellation_drawing");
	FlagConstellationName	= conf.get_boolean("viewing:flag_constellation_name");
	FlagConstellationArt	= conf.get_boolean("viewing:flag_constellation_art");
	FlagConstellationPick	= conf.get_boolean("viewing:flag_constellation_pick");
	FlagAzimutalGrid		= conf.get_boolean("viewing:flag_azimutal_grid");
	FlagEquatorialGrid		= conf.get_boolean("viewing:flag_equatorial_grid");
	FlagEquatorLine			= conf.get_boolean("viewing:flag_equator_line");
	FlagEclipticLine		= conf.get_boolean("viewing:flag_ecliptic_line");
	FlagCardinalPoints		= conf.get_boolean("viewing:flag_cardinal_points");
	FlagGravityLabels		= conf.get_boolean("viewing:flag_gravity_labels");
	FlagInitMoonScaled		= conf.get_boolean("viewing:flag_init_moon_scaled");
	moon_scale				= conf.get_double ("viewing","moon_scale",4.);
	ConstellationCulture	= conf.get_str("viewing:constellation_culture");

	// Astro section
	FlagStars				= conf.get_boolean("astro:flag_stars");
	FlagStarName			= conf.get_boolean("astro:flag_star_name");
	FlagPlanets				= conf.get_boolean("astro:flag_planets");
	FlagPlanetsHints		= conf.get_boolean("astro:flag_planets_hints");
	FlagNebula				= conf.get_boolean("astro:flag_nebula");
	FlagNebulaName			= conf.get_boolean("astro:flag_nebula_name");
	FlagMilkyWay			= conf.get_boolean("astro:flag_milky_way");
}

void stel_core::save_config_to(const string& confFile)
{
    cout << "Saving configuration file " << confFile << " ..." << endl;
	init_parser conf;

	// Main section
	conf.set_str	("main:version", string(VERSION));

	// Video Section
	conf.set_boolean("video:fullscreen", Fullscreen);
	conf.set_int	("video:screen_w", screen_W);
	conf.set_int	("video:screen_h", screen_H);
	conf.set_int	("video:bbp_mode", bppMode);

	// Projector
	string tmpstr;
	switch (ProjectorType)
	{
		case PERSPECTIVE_PROJECTOR : tmpstr="perspective";	break;
		case FISHEYE_PROJECTOR : tmpstr="fisheye";		break;
		default : tmpstr="perspective";
	}
	conf.set_str	("projection:type",tmpstr);

	switch (ViewportType)
	{
		case MAXIMIZED : tmpstr="maximized";	break;
		case SQUARE : tmpstr="square";	break;
		case DISK : tmpstr="disk";		break;
		default : tmpstr="maximized";
	}
	conf.set_str	("projection:viewport", tmpstr);

	// Star section
	conf.set_double ("stars:star_scale", StarScale);
	conf.set_double ("stars:star_mag_scale", StarMagScale);
	conf.set_double ("stars:star_twinkle_amount", StarTwinkleAmount);
	conf.set_double ("stars:max_mag_star_name", MaxMagStarName);
	conf.set_boolean("stars:flag_star_twinkle", FlagStarTwinkle);
	conf.set_boolean("stars:flag_point_star", FlagPointStar);

	// Ui section
	conf.set_boolean("gui:flag_show_fps" ,FlagShowFps);
	conf.set_boolean("gui:flag_menu", FlagMenu);
	conf.set_boolean("gui:flag_help", FlagHelp);
	conf.set_boolean("gui:flag_infos", FlagInfos);
	conf.set_boolean("gui:flag_show_topbar", FlagShowTopBar);
	conf.set_boolean("gui:flag_show_time", FlagShowTime);
	conf.set_boolean("gui:flag_show_date", FlagShowDate);
	conf.set_boolean("gui:flag_show_appname", FlagShowAppName);
	conf.set_boolean("gui:flag_show_fov", FlagShowFov);
	conf.set_boolean("gui:flag_show_selected_object_info", FlagShowSelectedObjectInfos);
	conf.set_str	("gui:gui_base_color", vec3f_to_str(GuiBaseColor));
	conf.set_str	("gui:gui_text_color", vec3f_to_str(GuiTextColor));

	// Text ui section
	conf.set_boolean("tui:flag_show_tui", false);
	conf.set_boolean("tui:flag_show_tui_datetime", FlagShowTuiDateTime);
	conf.set_boolean("tui:flag_show_tui_short_obj_info", FlagShowTuiShortInfo);

	// Navigation section
	conf.set_double ("navigation:preset_sky_time", PresetSkyTime);
	conf.set_str	("navigation:startup_time_mode", StartupTimeMode);
	conf.set_boolean("navigation:flag_enable_zoom_keys", FlagEnableZoomKeys);
	conf.set_boolean("navigation:flag_enable_move_keys", FlagEnableMoveKeys);
	conf.set_double ("navigation:init_fov", InitFov);
	conf.set_str	("navigation:init_view_pos", vec3f_to_str(InitViewPos));
	conf.set_double ("navigation:auto_move_duration", auto_move_duration);
	conf.set_boolean("navigation:flag_utc_time", FlagUTC_Time);

	// Landscape section
	conf.set_boolean("landscape:flag_ground", FlagGround);
	conf.set_boolean("landscape:flag_horizon", FlagHorizon);
	conf.set_boolean("landscape:flag_fog", FlagFog);
	conf.set_boolean("landscape:flag_atmosphere", FlagAtmosphere);

	// Viewing section
	conf.set_boolean("viewing:flag_constellation_drawing", FlagConstellationDrawing);
	conf.set_boolean("viewing:flag_constellation_name", FlagConstellationName);
	conf.set_boolean("viewing:flag_constellation_art", FlagConstellationArt);
	conf.set_boolean("viewing:flag_constellation_pick", FlagConstellationPick);
	conf.set_boolean("viewing:flag_azimutal_grid", FlagAzimutalGrid);
	conf.set_boolean("viewing:flag_equatorial_grid", FlagEquatorialGrid);
	conf.set_boolean("viewing:flag_equator_line", FlagEquatorLine);
	conf.set_boolean("viewing:flag_ecliptic_line", FlagEclipticLine);
	conf.set_boolean("viewing:flag_cardinal_points", FlagCardinalPoints);
	conf.set_boolean("viewing:flag_gravity_labels", FlagGravityLabels);
	conf.set_boolean("viewing:flag_init_moon_scaled", FlagInitMoonScaled);
	conf.set_double ("viewing:moon_scale", moon_scale);
	conf.set_str	("viewing:constellation_culture", ConstellationCulture);

	// Astro section
	conf.set_boolean("astro:flag_stars", FlagStars);
	conf.set_boolean("astro:flag_star_name", FlagStarName);
	conf.set_boolean("astro:flag_planets", FlagPlanets);
	conf.set_boolean("astro:flag_planets_hints", FlagPlanetsHints);
	conf.set_boolean("astro:flag_nebula", FlagNebula);
	conf.set_boolean("astro:flag_nebula_name", FlagNebulaName);
	conf.set_boolean("astro:flag_milky_way", FlagMilkyWay);

	conf.save(confFile);

}


// Handle mouse clics
int stel_core::handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button)
{
	return ui->handle_clic(x, y, state, button);
}

// Handle mouse move
int stel_core::handle_move(int x, int y)
{
	return ui->handle_move(x, y);
}

// Handle key press and release
int stel_core::handle_keys(SDLKey key, s_gui::S_GUI_VALUE state)
{
	s_tui::S_TUI_VALUE tuiv;
	if (state == s_gui::S_GUI_PRESSED) tuiv = s_tui::S_TUI_PRESSED;
	else tuiv = s_tui::S_TUI_RELEASED;
	if (FlagShowTui)
	{

		if (ui->handle_keys_tui(key, tuiv)) return 1;
		if (state==S_GUI_PRESSED && key==SDLK_m)
		{
			FlagShowTui = false;
			return 1;
		}
		return 1;
	}

	if (ui->handle_keys(key, state)) return 1;

	if (state == S_GUI_PRESSED)
	{
   		// Direction and zoom deplacements
   		if (key==SDLK_LEFT) turn_left(1);
   		if (key==SDLK_RIGHT) turn_right(1);
   		if (key==SDLK_UP)
		{
			if (SDL_GetModState() & KMOD_CTRL) zoom_in(1);
			else turn_up(1);
		}
   		if (key==SDLK_DOWN)
		{
			if (SDL_GetModState() & KMOD_CTRL) zoom_out(1);
			else turn_down(1);
		}
   		if (key==SDLK_PAGEUP) zoom_in(1);
   		if (key==SDLK_PAGEDOWN) zoom_out(1);
	}
	else
	{
	    // When a deplacement key is released stop mooving
   		if (key==SDLK_LEFT) turn_left(0);
		if (key==SDLK_RIGHT) turn_right(0);
		if (SDL_GetModState() & KMOD_CTRL)
		{
			if (key==SDLK_UP) zoom_in(0);
			if (key==SDLK_DOWN) zoom_out(0);
		}
		else
		{
			if (key==SDLK_UP) turn_up(0);
			if (key==SDLK_DOWN) turn_down(0);
		}
		if (key==SDLK_PAGEUP) zoom_in(0);
		if (key==SDLK_PAGEDOWN) zoom_out(0);
	}
	return 0;
}

void stel_core::turn_right(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = 1;
		navigation->set_flag_traking(0);
	}
	else deltaAz = 0;
}

void stel_core::turn_left(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = -1;
		navigation->set_flag_traking(0);
	}
	else deltaAz = 0;
}

void stel_core::turn_up(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = 1;
		navigation->set_flag_traking(0);
	}
	else deltaAlt = 0;
}

void stel_core::turn_down(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = -1;
		navigation->set_flag_traking(0);
	}
	else deltaAlt = 0;
}

void stel_core::zoom_in(int s)
{
	if (FlagEnableZoomKeys) deltaFov = -1*(s!=0);
}

void stel_core::zoom_out(int s)
{
	if (FlagEnableZoomKeys) deltaFov = (s!=0);
}


// Increment/decrement smoothly the vision field and position
void stel_core::update_move(int delta_time)
{
	// the more it is zoomed, the more the mooving speed is low (in angle)
    double depl=move_speed*delta_time*projection->get_fov();
    if (deltaAz<0)
    {
		deltaAz = -depl/30;
		if (deltaAz<-0.2) deltaAz = -0.2;
    }
    else
    {
		if (deltaAz>0)
        {
			deltaAz = (depl/30);
			if (deltaAz>0.2) deltaAz = 0.2;
        }
    }
    if (deltaAlt<0)
    {
		deltaAlt = -depl/30;
		if (deltaAlt<-0.2) deltaAlt = -0.2;
    }
    else
    {
		if (deltaAlt>0)
        {
			deltaAlt = depl/30;
			if (deltaAlt>0.2) deltaAlt = 0.2;
        }
    }

    if (deltaFov<0)
    {
		deltaFov = -depl*5;
		if (deltaFov<-20) deltaFov = -20;
    }
    else
    {
		if (deltaFov>0)
        {
			deltaFov = depl*5;
			if (deltaFov>20) deltaFov = 20;
        }
    }

	projection->change_fov(deltaFov);
	navigation->update_move(deltaAz, deltaAlt);
}

void stel_core::set_screen_size(int w, int h)
{
	if (w==screen_W && h==screen_H) return;
    screen_W = w;
    screen_H = h;

	projection->set_screen_size(screen_W, screen_H);
}

// find and select the "nearest" object from earth equatorial position
stel_object * stel_core::find_stel_object(const Vec3d& v) const
{
	stel_object * sobj = NULL;

	if (FlagPlanets) sobj = ssystem->search(v, navigation, projection);
	if (sobj) return sobj;

	Vec3f u=Vec3f(v[0],v[1],v[2]);

	sobj = nebulas->search(u);
	if (sobj) return sobj;

	if (FlagStars) sobj = hip_stars->search(u);

	return sobj;
}


// find and select the "nearest" object from screen position
stel_object * stel_core::find_stel_object(int x, int y) const
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);
	return find_stel_object(v);
}

// Find and select in a "clever" way an object
stel_object * stel_core::clever_find(const Vec3d& v) const
{
	stel_object * sobj = NULL;
	vector<stel_object*> candidates;
	vector<stel_object*> temp;
	Vec3d winpos;

	// Field of view for a 30 pixel diameter circle on screen
	float fov_around = projection->get_fov()/MY_MIN(projection->viewW(), projection->viewH()) * 30.f;

	float xpos, ypos;
	projection->project_earth_equ(v, winpos);
	xpos = winpos[0];
	ypos = winpos[1];

	// Collect the planets inside the range
	if (FlagPlanets)
	{
		temp = ssystem->search_around(v, fov_around, navigation, projection);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// The nebulas inside the range
	if (FlagNebula)
	{
		temp = nebulas->search_around(v, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// And the stars inside the range
	if (FlagStars)
	{
		temp = hip_stars->search_around(v, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	float best_object_value;
	best_object_value = 100000.f;
	vector<stel_object*>::iterator iter = candidates.begin();
    while (iter != candidates.end())
    {
		projection->project_earth_equ((*iter)->get_earth_equ_pos(navigation), winpos);
		float distance = sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]));
		float mag = (*iter)->get_mag(navigation);
		if ((*iter)->get_type()==STEL_OBJECT_NEBULA) mag -= 7.f;
		if (distance + mag < best_object_value)
		{
			best_object_value = distance + mag;
			sobj = *iter;
		}
        iter++;
    }

	return sobj;
}

stel_object * stel_core::clever_find(int x, int y) const
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);
	return clever_find(v);
}

// Goto the given object
void stel_core::goto_stel_object(const stel_object* obj, float move_duration) const
{
	if (!obj) return;
	navigation->move_to(obj->get_earth_equ_pos(navigation), move_duration);
}


// Go and zoom temporary to the selected object. Old position is reverted by calling the function again
void stel_core::auto_zoom_in(float move_duration)
{
	if (!selected_object) return;

	navigation->set_flag_traking(true);
	goto_stel_object(selected_object, move_duration);
	float satfov = selected_object->get_satellites_fov(navigation);
	float closefov = selected_object->get_close_fov(navigation);

	if (satfov>0. && projection->get_fov()>satfov) projection->zoom_to(satfov, move_duration);
	else if (projection->get_fov()>closefov) projection->zoom_to(closefov, move_duration);
}

// Unzoom to the old position is reverted by calling the function again
void stel_core::auto_zoom_out(float move_duration)
{
	if (!selected_object)
	{
		projection->zoom_to(InitFov, move_duration);
		navigation->move_to(InitViewPos, move_duration, true);
		navigation->set_flag_traking(false);
		return;
	}

	float satfov = selected_object->get_satellites_fov(navigation);

	if (projection->get_fov()<=satfov*0.9 && satfov>0.)
	{
		projection->zoom_to(satfov, move_duration);
		return;
	}

	projection->zoom_to(InitFov, move_duration);
	navigation->move_to(InitViewPos, move_duration, true);
	navigation->set_flag_traking(false);

}
