/*
 * Copyright (C) 2003 Fabien Ch�eau
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
	nebulas(NULL), atmosphere(NULL), tone_converter(NULL), FlagHelp(false), FlagInfos(false),
	FlagConfig(false), FlagShowTuiMenu(0), 
	frame(0), timefr(0), timeBase(0), maxfps(10000.f), deltaFov(0.), deltaAlt(0.), deltaAz(0.),
	move_speed(0.001), FlagTimePause(0), is_mouse_moving_horiz(false), is_mouse_moving_vert(false)
{
	ProjectorType = PERSPECTIVE_PROJECTOR;
	SelectedScript = SelectedScriptDirectory = "";
	ScriptRemoveableDiskMounted = 0;
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
	delete landscape; landscape = NULL;
	delete observatory; observatory = NULL;
	delete skyloc; skyloc = NULL;
	if (milky_way) delete milky_way;
	delete meteors; meteors = NULL;
	if (atmosphere) delete atmosphere;
	if (tone_converter) delete tone_converter;
	if (ssystem) delete ssystem;
	if (ui) delete ui;
	if (scripts) delete scripts;
	if (commander) delete commander;
	if(script_images) delete script_images;

	stel_object::delete_textures(); // Load the pointer textures 
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
void stel_core::set_config_files(const string& _config_file)
{
	config_file = _config_file;
}


void stel_core::init(void)
{
	// Set textures directory and suffix
	s_texture::set_texDir(TextureDir);
	s_texture::set_suffix(".png");

	commander = new StelCommandInterface(this);
	scripts = new ScriptMgr(commander);
	script_images = new ImageMgr();

	observatory = new Observator();
	observatory->load(ConfigDir + config_file, "init_location");

	navigation = new navigator(observatory);
	navigation->set_viewing_mode(ViewingMode);
	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		navigation->set_JDay(PresetSkyTime - observatory->get_GMT_shift(PresetSkyTime) * JD_HOUR);
	else navigation->set_JDay(get_julian_from_sys());
	navigation->set_local_vision(InitViewPos);


	switch (ProjectorType)
	{
	case PERSPECTIVE_PROJECTOR :
		projection = new Projector(screen_W, screen_H, InitFov);
		break;
	case FISHEYE_PROJECTOR :
		projection = new Fisheye_projector(screen_W, screen_H, InitFov, DistortionFunction);
		break;
	default :
		projection = new Projector(screen_W, screen_H, InitFov);
		break;
	}

	// Make the viewport as big as possible
	projection->set_screen_size(screen_W, screen_H);
	projection->set_fov(InitFov);

	projection->set_viewport_offset(horizontalOffset, verticalOffset);
	projection->set_viewport_type(ViewportType);

	// get ready for sky localization
	skyloc = new Sky_localizer(DataDir);

	// Load hipparcos stars & names
	hip_stars = new Hip_Star_mgr(
		DataDir + "hipparcos.fab",
		DataDir + "name.fab",
		DataDir + "star_names." + SkyLocale + ".fab",
		DataDir + "spacefont.txt" );

	nebulas   = new Nebula_mgr(NebulaLabelColor, NebulaCircleColor);

	// Create and init the solar system
	ssystem = new SolarSystem(DataDir, SkyLocale, "spacefont.txt", PlanetNamesColor, PlanetOrbitsColor );
	ssystem->set_object_scale(StarScale);
	ssystem->set_trail_color(ObjectTrailsColor);
	if(FlagObjectTrails) ssystem->start_trails();

	atmosphere = new stel_atmosphere();
	atmosphere->set_fade_duration(AtmosphereFadeDuration);

	tone_converter = new tone_reproductor();

	equ_grid = new SkyGrid(EQUATORIAL, EquatorialColor, DataDir + "spacefont.txt", "spacefont");
	azi_grid = new SkyGrid(ALTAZIMUTAL, AzimuthalColor, DataDir + "spacefont.txt", "spacefont");
	equator_line = new SkyLine(EQUATOR, EquatorColor);
	ecliptic_line = new SkyLine(ECLIPTIC, EclipticColor);


	cardinals_points = new Cardinals(DataDir + "spacefont.txt", "spacefont");
	cardinals_points->set_color(CardinalColor);

	milky_way = new MilkyWay("milkyway");
	meteors = new Meteor_mgr(10, 60);

	landscape = Landscape::create_from_file(DataDir + "landscapes.ini", observatory->get_landscape_name());

	// Load the pointer textures
	stel_object::init_textures();

	// initialisation of the User Interface
	ui = new stel_ui(this);
	ui->init();

	ui->init_tui();

	tone_converter->set_world_adaptation_luminance(3.75f + atmosphere->get_fade_intensity()*40000.f);

	// Set the default moon scaling
	if (FlagInitMoonScaled) ssystem->get_moon()->set_sphere_scale(MoonScale);

	// Compute planets data and init viewing position
	// Position of sun and all the satellites (ie planets)
	ssystem->compute_positions(navigation->get_JDay());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->compute_trans_matrices(navigation->get_JDay());

	// Compute transform matrices between coordinates systems
	navigation->update_transform_matrices((ssystem->get_earth())->get_ecliptic_pos());
	navigation->update_model_view_mat();

	// Load the nebulas data TODO : add NGC objects
	projection->set_orthographic_projection();
	
	// Load constellations
	asterisms = new Constellation_mgr(DataDir, SkyCulture, SkyLocale, hip_stars, "spacefont.txt", screen_W/2-150, screen_H/2-20, ConstLinesColor, ConstNamesColor);
	asterisms->set_art_intensity(ConstellationArtIntensity);
	asterisms->set_art_fade_duration(ConstellationArtFadeDuration);
	

	nebulas->read(DataDir + "spacefont.txt", DataDir + "messier.fab", screen_W/2-150, screen_H/2-20);
	projection->reset_perspective_projection();
	
	selected_planet=NULL;	// Fix a bug on macosX! Thanks Fumio!
		
	// could load a startup script
	commander->execute_command("script action play filename ./scripts/startup.sts");
}

void stel_core::quit(void)
{
	static SDL_Event Q;						// Send a SDL_QUIT event
	Q.type = SDL_QUIT;						// To the SDL event queue
	if(SDL_PushEvent(&Q) == -1)				// Try to send the event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
		exit(-1);
	}
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

    // run command from a running script
    scripts->update(delta_time);

    // Update the position of observation and time etc...
    observatory->update(delta_time);
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

	// planet trails (call after nav is updated)
	ssystem->update_trails(navigation);

	// Move the view direction and/or fov
	update_move(delta_time);

	// Update info about selected object
	if (selected_object) selected_object->update();

	// Update faders
	asterisms->update(delta_time);
	atmosphere->update(delta_time);
	
	// Compute the sun position in local coordinate
	Vec3d temp(0.,0.,0.);
	Vec3d sunPos = navigation->helio_to_local(temp);
	sunPos.normalize();

	// Compute the moon position in local coordinate
	temp = ssystem->get_moon()->get_heliocentric_ecliptic_pos();
	Vec3d moonPos = navigation->helio_to_local(temp);
	moonPos.normalize();

	// Compute the atmosphere color and intensity 
	atmosphere->compute_color(navigation->get_JDay(), sunPos, moonPos,
				  ssystem->get_moon()->get_phase(ssystem->get_earth()->get_heliocentric_ecliptic_pos()),
				  tone_converter, projection, observatory->get_latitude(), observatory->get_altitude(),
				  15.f, 40.f);	// Temperature = 15c, relative humidity = 40%
	tone_converter->set_world_adaptation_luminance(atmosphere->get_world_adaptation_luminance());
	
	// compute global sky brightness TODO : make this more "scientifically"
	sky_brightness = sunPos[2] * atmosphere->get_fade_intensity();
	if( sky_brightness < 0 )
	{
		sky_brightness = 0;
	}
	else if (sky_brightness<0.1) sky_brightness=0.1;

	landscape->set_sky_brightness(sky_brightness);

	ui->gui_update_widgets();

	if (FlagShowGravityUi || FlagShowTuiMenu) ui->tui_update_widgets();

	if(!scripts->is_paused()) script_images->update(delta_time);
}

// Execute all the drawing functions
void stel_core::draw(int delta_time)
{
	// Init openGL viewing with fov, screen size and clip planes
	projection->set_clipping_planes(0.0005 ,50);

	// Give the updated standard projection matrices to the projector
	projection->set_modelview_matrices(	navigation->get_earth_equ_to_eye_mat(),
						navigation->get_helio_to_eye_mat(),
						navigation->get_local_to_eye_mat(),
						navigation->get_prec_earth_equ_to_eye_mat());

	// Set openGL drawings in equatorial coordinates
	navigation->switch_to_earth_equatorial();

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw the milky way. If not activated, need at least to clear the color buffer
	if (FlagMilkyWay)
	{
		milky_way->draw(tone_converter, projection, navigation);
	}
	else
	{
	  glClear(GL_COLOR_BUFFER_BIT);
	}

	// Draw all the constellations
	asterisms->show_lines(FlagConstellationDrawing);
	asterisms->show_art(FlagConstellationArt);
	asterisms->show_names(FlagConstellationName);
	asterisms->draw(projection);
	asterisms->draw_art(projection, navigation);
	asterisms->draw_names(projection, FlagGravityLabels);

	// Draw the nebula if they are visible
	if (FlagNebula && sky_brightness<0.11)
		nebulas->draw(FlagNebulaName, projection, navigation, tone_converter,
			FlagGravityLabels, MaxMagNebulaName, FlagBrightNebulae);

	// Draw the hipparcos stars
	Vec3d tempv = navigation->get_prec_equ_vision();
	Vec3f temp(tempv[0],tempv[1],tempv[2]);

	if (FlagStars && sky_brightness<=0.11)
	{
		if (FlagPointStar) hip_stars->draw_point(StarScale, StarMagScale,
			FlagStarTwinkle ? StarTwinkleAmount : 0.f, FlagStarName,
			MaxMagStarName, temp, tone_converter, projection, FlagGravityLabels);
		else hip_stars->draw(StarScale, StarMagScale,
			FlagStarTwinkle ? StarTwinkleAmount : 0.f, FlagStarName,
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
	if (FlagPlanets) {

	  // Selecting Sun doesn't turn off orbits, becuase it's orbit is the ecliptic (separate control)
	  // plus makes viewing orbits easier when center on Sun
	  if( FlagPlanetsOrbits && selected_planet && selected_planet->get_name() != "Sun") {
	    // draw orbit only for selected planet
	    selected_planet->draw_orbit(navigation, projection);
	  }

	  ssystem->draw(FlagPlanetsHints, projection, navigation, tone_converter,
			FlagGravityLabels, FlagPointStar, 
			FlagPlanetsOrbits && (selected_planet==NULL || selected_planet->get_name() == "Sun"),
			FlagObjectTrails);

	}

	// Set openGL drawings in local coordinates i.e. generally altazimuthal coordinates
	navigation->switch_to_local();

	// Draw meteors
	meteors->update(projection, navigation, tone_converter, delta_time);

	if(!FlagAtmosphere || sky_brightness<0.01) {
	  projection->set_orthographic_projection(); 
	  meteors->draw(projection, navigation);
	  projection->reset_perspective_projection(); 
	}

	// Draw the atmosphere
	atmosphere->show(FlagAtmosphere);
	atmosphere->draw(projection, delta_time);

	// Draw the landscape
	landscape->draw(tone_converter, projection, navigation,	FlagFog, FlagHorizon && FlagGround, FlagGround);

	// Daw the cardinal points
	if (FlagCardinalPoints) cardinals_points->draw(projection, observatory->get_latitude(), FlagGravityLabels );

	// draw images loaded by a script
	projection->set_orthographic_projection(); 
	script_images->draw(screen_W, screen_H, projection->viewW(), projection->viewH());  
	projection->reset_perspective_projection(); 

	projection->draw_viewport_shape();

	// Draw the Graphical ui and the Text ui
	ui->draw();

	if (FlagShowGravityUi) ui->draw_gravity_ui();
	if (FlagShowTuiMenu) ui->draw_tui();


}

void stel_core::set_landscape(const string& new_landscape_name)
{
    //	if (new_landscape_name.empty() || new_landscape_name==observatory->get_landscape_name()) return;
    if (new_landscape_name.empty()) return;
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
		if (version>="0.6.0" && version != string(VERSION))
		{
			printf(_("The current config file is from a previous version (>=0.6.0).\nPrevious options will be imported in the new config file.\n"));
			
			// Store temporarily the previous observator parameters
			Observator tempobs;
			tempobs.load(ConfigDir + config_file, "init_location");
			
			// Set the new landscape though
			tempobs.set_landscape_name("Guereins");
			
			load_config_from(ConfigDir + config_file);			
			// We just imported previous parameters (from >=0.6.0)
			save_config_to(ConfigDir + config_file);
			tempobs.save(ConfigDir + config_file, "init_location");

			load_config_from(ConfigDir + config_file);
		}
		else
		{
			// The config file is too old to try an importation
			printf(_("The current config file is from a version too old for parameters to be imported (%s).\nIt will be replaced by the default config file.\n"), version.empty() ? "<0.6.0" : version.c_str());
			system( (string("cp -f ") + DataRoot + "/config/default_config.ini " + ConfigDir + config_file).c_str() );
			
			// Actually load the config file
			load_config_from(ConfigDir + config_file);
			return;
		}
	}
	
	// Versions match, there was no pblms
	load_config_from(ConfigDir + config_file);
	
}

void stel_core::save_config(void)
{
	// The config file is supposed to be valid and from the correct stellarium version.
	// This is normally the case if the program is running.
	save_config_to(ConfigDir + config_file);
}

void stel_core::load_config_from(const string& confFile)
{
    cout << _("Loading configuration file ") << confFile << " ..." << endl;
	init_parser conf;
	conf.load(confFile);

	// Main section (check for version mismatch)
	string version = conf.get_str("main:version");
	if (version!=string(VERSION) && version<"0.6.0")
	{
		cout << _("ERROR : The current config file is from a different version (") <<
			(version.empty() ? "<0.6.0" : version) << ")." << endl;
		exit(-1);
	}

	// Video Section
	Fullscreen			= conf.get_boolean("video:fullscreen");
	screen_W			= conf.get_int	  ("video:screen_w");
	screen_H			= conf.get_int	  ("video:screen_h");
	bppMode				= conf.get_int    ("video:bbp_mode");
	horizontalOffset	= conf.get_int    ("video:horizontal_offset");
	verticalOffset		= conf.get_int    ("video:vertical_offset");
	maxfps 				= conf.get_double ("video","maximum_fps",10000);

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

	// -1 is default fisheye linear distortion
	DistortionFunction 	= conf.get_int("projection", "distortion_function", -1);

	// localization section
	SkyCulture = conf.get_str("localization", "sky_culture", "western");
	SkyLocale = conf.get_str("localization", "sky_locale", "eng");
	// default sky is western, english (actually only planet names are english!)

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
	FlagShowSelectedObjectInfo = conf.get_boolean("gui:flag_show_selected_object_info");
	GuiBaseColor		= str_to_vec3f(conf.get_str("gui:gui_base_color").c_str());
	GuiTextColor		= str_to_vec3f(conf.get_str("gui:gui_text_color").c_str());
	BaseFontSize		= conf.get_double ("gui","base_font_size",15);
	
	// Colors
	AzimuthalColor		= str_to_vec3f(conf.get_str("color:azimuthal_color").c_str());
	EquatorialColor		= str_to_vec3f(conf.get_str("color:equatorial_color").c_str());
	EquatorColor		= str_to_vec3f(conf.get_str("color:equator_color").c_str());
	EclipticColor		= str_to_vec3f(conf.get_str("color:ecliptic_color").c_str());
	ConstLinesColor		= str_to_vec3f(conf.get_str("color:const_lines_color").c_str());
	ConstNamesColor		= str_to_vec3f(conf.get_str("color:const_names_color").c_str());
	NebulaLabelColor	= str_to_vec3f(conf.get_str("color:nebula_label_color").c_str());
	NebulaCircleColor	= str_to_vec3f(conf.get_str("color:nebula_circle_color").c_str());
	CardinalColor 		= str_to_vec3f(conf.get_str("color:cardinal_color").c_str());
	PlanetNamesColor	= str_to_vec3f(conf.get_str("color:planet_names_color").c_str());
	PlanetOrbitsColor	= str_to_vec3f(conf.get_str("color", "planet_orbits_color", ".6,1,1").c_str());
	ObjectTrailsColor	= str_to_vec3f(conf.get_str("color", "object_trails_color", "1,0.7,0").c_str());

	// Text ui section
	FlagEnableTuiMenu = conf.get_boolean("tui:flag_enable_tui_menu");
	FlagShowGravityUi = conf.get_boolean("tui:flag_show_gravity_ui");
	FlagShowTuiDateTime = conf.get_boolean("tui:flag_show_tui_datetime");
	FlagShowTuiShortObjInfo = conf.get_boolean("tui:flag_show_tui_short_obj_info");

	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	FlagEnableZoomKeys	= conf.get_boolean("navigation:flag_enable_zoom_keys");
	FlagManualZoom		= conf.get_boolean("navigation:flag_manual_zoom");
	FlagEnableMoveKeys	= conf.get_boolean("navigation:flag_enable_move_keys");
	FlagEnableMoveMouse	= conf.get_boolean("navigation","flag_enable_move_mouse",1);
	InitFov	                = conf.get_double ("navigation","init_fov",60.);
	InitViewPos 		= str_to_vec3f(conf.get_str("navigation:init_view_pos").c_str());
	auto_move_duration	= conf.get_double ("navigation","auto_move_duration",1.5);
	FlagUTC_Time		= conf.get_boolean("navigation:flag_utc_time");
	MouseZoom		= conf.get_int("navigation","mouse_zoom",30);

	// Viewing Mode
	tmpstr = conf.get_str("navigation:viewing_mode");
	if (tmpstr=="equator") ViewingMode = VIEW_EQUATOR;
	else
	{
		if (tmpstr=="horizon") ViewingMode = VIEW_HORIZON;
		else
		{
			cout << "ERROR : Unknown viewing mode type : " << tmpstr << endl;
			ViewingMode = VIEW_HORIZON;
		}
	}

	// Landscape section
	FlagGround			= conf.get_boolean("landscape:flag_ground");
	FlagHorizon			= conf.get_boolean("landscape:flag_horizon");
	FlagFog				= conf.get_boolean("landscape:flag_fog");
	FlagAtmosphere		= conf.get_boolean("landscape:flag_atmosphere");
	AtmosphereFadeDuration  = conf.get_double("landscape","atmosphere_fade_duration",1.5);

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
	MoonScale				= conf.get_double ("viewing","moon_scale",5.);
	ConstellationArtIntensity       = conf.get_double("viewing","constellation_art_intensity", 0.5);
	ConstellationArtFadeDuration    = conf.get_double("viewing","constellation_art_fade_duration",2.);
	
	// Astro section
	FlagStars				= conf.get_boolean("astro:flag_stars");
	FlagStarName			= conf.get_boolean("astro:flag_star_name");
	FlagPlanets				= conf.get_boolean("astro:flag_planets");
	FlagPlanetsHints		= conf.get_boolean("astro:flag_planets_hints");
	FlagPlanetsOrbits		= conf.get_boolean("astro:flag_planets_orbits");
	FlagObjectTrails		= conf.get_boolean("astro", "flag_object_trails", 0);
	FlagNebula				= conf.get_boolean("astro:flag_nebula");
	FlagNebulaName			= conf.get_boolean("astro:flag_nebula_name");
	MaxMagNebulaName		= conf.get_double("astro:max_mag_nebula_name");
	FlagMilkyWay			= conf.get_boolean("astro:flag_milky_way");
	FlagBrightNebulae		= conf.get_boolean("astro:flag_bright_nebulae");
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
	conf.set_int    ("video:horizontal_offset", horizontalOffset);
	conf.set_int    ("video:vertical_offset", verticalOffset);
	conf.set_double ("video:maximum_fps", maxfps);
	
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
	conf.set_int    ("projection:distortion_function", DistortionFunction);

	// localization section
	conf.set_str    ("localization:sky_culture", SkyCulture);
	conf.set_str    ("localization:sky_locale", SkyLocale);

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
	conf.set_boolean("gui:flag_show_selected_object_info", FlagShowSelectedObjectInfo);
	conf.set_str	("gui:gui_base_color", vec3f_to_str(GuiBaseColor));
	conf.set_str	("gui:gui_text_color", vec3f_to_str(GuiTextColor));
	conf.set_double ("gui:base_font_size", BaseFontSize);
	
	// Colors
	conf.set_str    ("color:azimuthal_color", vec3f_to_str(AzimuthalColor));
	conf.set_str    ("color:equatorial_color", vec3f_to_str(EquatorialColor));
	conf.set_str    ("color:equator_color", vec3f_to_str(EquatorColor));
	conf.set_str    ("color:ecliptic_color", vec3f_to_str(EclipticColor));
	conf.set_str    ("color:const_lines_color", vec3f_to_str(ConstLinesColor));
	conf.set_str    ("color:const_names_color", vec3f_to_str(ConstNamesColor));
	conf.set_str	("color:nebula_label_color", vec3f_to_str(NebulaLabelColor));
	conf.set_str	("color:nebula_circle_color", vec3f_to_str(NebulaCircleColor));
	conf.set_str    ("color:cardinal_color", vec3f_to_str(CardinalColor));
	conf.set_str    ("color:planet_names_color", vec3f_to_str(PlanetNamesColor));
	conf.set_str    ("color:planet_orbits_color", vec3f_to_str(PlanetOrbitsColor));
	conf.set_str    ("color:object_trails_color", vec3f_to_str(ObjectTrailsColor));

	// Text ui section
	conf.set_boolean("tui:flag_enable_tui_menu", FlagEnableTuiMenu);
	conf.set_boolean("tui:flag_show_gravity_ui", FlagShowGravityUi);
	conf.set_boolean("tui:flag_show_tui_datetime", FlagShowTuiDateTime);
	conf.set_boolean("tui:flag_show_tui_short_obj_info", FlagShowTuiShortObjInfo);

	// Navigation section
	conf.set_double ("navigation:preset_sky_time", PresetSkyTime);
	conf.set_str	("navigation:startup_time_mode", StartupTimeMode);
	conf.set_boolean("navigation:flag_enable_zoom_keys", FlagEnableZoomKeys);
	conf.set_boolean("navigation:flag_manual_zoom", FlagManualZoom);
	conf.set_boolean("navigation:flag_enable_move_keys", FlagEnableMoveKeys);
	conf.set_boolean("navigation:flag_enable_move_mouse", FlagEnableMoveMouse);
	conf.set_double ("navigation:init_fov", InitFov);
	conf.set_str	("navigation:init_view_pos", vec3f_to_str(InitViewPos));
	conf.set_double ("navigation:auto_move_duration", auto_move_duration);
	conf.set_boolean("navigation:flag_utc_time", FlagUTC_Time);
	conf.set_int("navigation:mouse_zoom", MouseZoom);
	switch (ViewingMode)
	{
		case VIEW_HORIZON : tmpstr="horizon";	break;
		case VIEW_EQUATOR : tmpstr="equator";		break;
		default : tmpstr="horizon";
	}
	conf.set_str	("navigation:viewing_mode",tmpstr);

	// Landscape section
	conf.set_boolean("landscape:flag_ground", FlagGround);
	conf.set_boolean("landscape:flag_horizon", FlagHorizon);
	conf.set_boolean("landscape:flag_fog", FlagFog);
	conf.set_boolean("landscape:flag_atmosphere", FlagAtmosphere);
	conf.set_double ("viewing:atmosphere_fade_duration", AtmosphereFadeDuration);

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
	conf.set_double ("viewing:moon_scale", MoonScale);
	conf.set_double ("viewing:constellation_art_intensity", ConstellationArtIntensity);
	conf.set_double ("viewing:constellation_art_fade_duration", ConstellationArtFadeDuration);


	// Astro section
	conf.set_boolean("astro:flag_stars", FlagStars);
	conf.set_boolean("astro:flag_star_name", FlagStarName);
	conf.set_boolean("astro:flag_planets", FlagPlanets);
	conf.set_boolean("astro:flag_planets_hints", FlagPlanetsHints);
	conf.set_boolean("astro:flag_planets_orbits", FlagPlanetsOrbits);
	conf.set_boolean("astro:flag_object_trails", FlagObjectTrails);
	conf.set_boolean("astro:flag_nebula", FlagNebula);
	conf.set_boolean("astro:flag_nebula_name", FlagNebulaName);
	conf.set_double("astro:max_mag_nebula_name", MaxMagNebulaName);
	conf.set_boolean("astro:flag_milky_way", FlagMilkyWay);
	conf.set_boolean("astro:flag_bright_nebulae", FlagBrightNebulae);

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
  // Turn if the mouse is at the edge of the screen.
  // unless config asks otherwise
  if(FlagEnableMoveMouse) {
    if (x == 0)
      {
	turn_left(1);
	is_mouse_moving_horiz = true;
      }
    else if (x == screen_W - 1)
      {
	turn_right(1);
	is_mouse_moving_horiz = true;
      }
    else if (is_mouse_moving_horiz)
      {
	turn_left(0);
	is_mouse_moving_horiz = false;
      }

    if (y == 0)
      {
	turn_up(1);
	is_mouse_moving_vert = true;
      }
    else if (y == screen_H - 1)
      {	
	turn_down(1);
	is_mouse_moving_vert = true;
      }
    else if (is_mouse_moving_vert)
      {
	turn_up(0);
	is_mouse_moving_vert = false;
      }
  }

  return ui->handle_move(x, y);

}

// Handle key press and release
int stel_core::handle_keys(SDLKey key, s_gui::S_GUI_VALUE state)
{
	s_tui::S_TUI_VALUE tuiv;
	if (state == s_gui::S_GUI_PRESSED) tuiv = s_tui::S_TUI_PRESSED;
	else tuiv = s_tui::S_TUI_RELEASED;
	if (FlagShowTuiMenu)
	{
		if (state==S_GUI_PRESSED && key==SDLK_m)
		{
		  // leave tui menu
		  FlagShowTuiMenu = false;

		  // If selected a script in tui, run that now
		  if(SelectedScript!="") commander->execute_command("script action play filename " + SelectedScriptDirectory + SelectedScript + " path " + SelectedScriptDirectory);
		  else {
		    // TODO unmount disk...
		    ScriptRemoveableDiskMounted = 0;
		  }
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
		navigation->set_flag_lock_equ_pos(0);
	}
	else deltaAz = 0;
}

void stel_core::turn_left(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = -1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);
	}
	else deltaAz = 0;
}

void stel_core::turn_up(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = 1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);
	}
	else deltaAlt = 0;
}

void stel_core::turn_down(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = -1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);
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
		if (deltaFov<-0.15*projection->get_fov()) deltaFov = -0.15*projection->get_fov();
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

	Vec3f u = navigation->earth_equ_to_prec_earth_equ(v);

	//	Vec3f u=Vec3f(v[0],v[1],v[2]);

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

	// nebulas and stars used precessed equ coords
	Vec3d p = navigation->earth_equ_to_prec_earth_equ(v);

	// The nebulas inside the range
	if (FlagNebula)
	{
		temp = nebulas->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// And the stars inside the range
	if (FlagStars)
	{
		temp = hip_stars->search_around(p, fov_around);
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
		if ((*iter)->get_type()==STEL_OBJECT_NEBULA) {
		  if( FlagNebulaName ) {
		    // make very easy to select if labeled
		    mag = -1;
		  } else {
		    mag -= 9.f;
		  }
		}
		if ((*iter)->get_type()==STEL_OBJECT_PLANET) {
		  if( FlagPlanetsHints ) {
		    // easy to select, especially pluto
		    mag -= 15.f;
		  } else {
		    mag -= 8.f;
		  }
		}
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

// Go and zoom to the selected object.
void stel_core::auto_zoom_in(float move_duration)
{
        float manual_move_duration;

	if (!selected_object) return;
  
	if (!navigation->get_flag_traking()) {
	  navigation->set_flag_traking(true);
	  navigation->move_to(selected_object->get_earth_equ_pos(navigation), move_duration, false, 1);
	  manual_move_duration = move_duration;
	} else {
	  // faster zoom in manual zoom mode once object is centered
	  manual_move_duration = move_duration*.66f;
	}

	if( FlagManualZoom ) {
	  // if manual zoom mode, user can zoom in incrementally
	  float newfov = projection->get_fov()*0.5f;
	  projection->zoom_to(newfov, manual_move_duration);

	} else {
	  float satfov = selected_object->get_satellites_fov(navigation);
	  float closefov = selected_object->get_close_fov(navigation);

	  if (satfov>0. && projection->get_fov()*0.9>satfov) projection->zoom_to(satfov, move_duration);
	  else if (projection->get_fov()>closefov) projection->zoom_to(closefov, move_duration);
	}
}

// Unzoom and go to the init position
void stel_core::auto_zoom_out(float move_duration)
{
	if (!selected_object)
	{
		projection->zoom_to(InitFov, move_duration);
		navigation->move_to(InitViewPos, move_duration, true, -1);
		navigation->set_flag_traking(false);
		navigation->set_flag_lock_equ_pos(0);
		return;
	}

	// If the selected object has satellites, unzoom to satellites view
	float satfov = selected_object->get_satellites_fov(navigation);
	if (projection->get_fov()<=satfov*0.9 && satfov>0.)
	{
		projection->zoom_to(satfov, move_duration);
		return;
	}

	// If the selected object is part of a planet subsystem (other than sun),
	// unzoom to subsystem view
	if (selected_object->get_type() == STEL_OBJECT_PLANET && selected_object!=ssystem->get_sun() && ((planet*)selected_object)->get_parent()!=ssystem->get_sun())
	{
		float satfov = ((planet*)selected_object)->get_parent()->get_satellites_fov(navigation);
		if (projection->get_fov()<=satfov*0.9 && satfov>0.)
		{
			projection->zoom_to(satfov, move_duration);
			return;
		}
	}

	projection->zoom_to(InitFov, move_duration);
	navigation->move_to(InitViewPos, move_duration, true, -1);
	navigation->set_flag_traking(false);
	navigation->set_flag_lock_equ_pos(0);

}

// this really belongs elsewhere
int stel_core::set_sky_culture(string _culture_dir)
{
  if(SkyCulture == _culture_dir) return 1;

  if(!projection) return 0;  // objects not initialized yet, will be loaded in init

  // percent complete bar only draws in 2d mode
  projection->set_orthographic_projection();
  if( asterisms->set_sky_culture(_culture_dir, DataDir + "spacefont.txt", screen_W/2-150, screen_H/2-20) ) {

    SkyCulture = _culture_dir;

    // as constellations have changed, clear out any selection and retest for match!
    if (selected_object && selected_object->get_type()==STEL_OBJECT_STAR)
	{
      asterisms->set_selected(asterisms->is_star_in((Hip_Star*)selected_object));
    }
	else
	{
      asterisms->set_selected(NULL);
    }

    projection->reset_perspective_projection();
    return 1;
  }
  projection->reset_perspective_projection();
  return 0;

}


// this really belongs elsewhere
void stel_core::set_sky_locale(string _locale)
{
  //  if(SkyLocale == _locale) return;  (not valid as culture can change and must reload)

  if( !hip_stars ) return; // objects not initialized yet

  SkyLocale = _locale;
  hip_stars->load_common_names(DataDir + "star_names." + _locale + ".fab");
  ssystem->set_sky_locale(_locale);
  asterisms->set_sky_locale(_locale);

}

// set a core flag
// if caller is not trusted, some flags can't be changed 
// newval is new value of flag changed
int stel_core::set_flag(string name, string value, bool &newval, bool trusted) {

  // value can be "on", "off", or "toggle"
  if(value == "toggle") {

    if(trusted) {
      if(name=="enable_zoom_keys") newval = (FlagEnableZoomKeys = !FlagEnableZoomKeys);
      else if(name=="enable_move_keys") newval = (FlagEnableMoveKeys = !FlagEnableMoveKeys);
      else if(name=="enable_move_mouse") newval = (FlagEnableMoveMouse = !FlagEnableMoveMouse);
    }


    if(name=="star_twinkle") newval = (FlagStarTwinkle = !FlagStarTwinkle);
    else if(name=="point_star") newval = (FlagPointStar = !FlagPointStar);
    else if(name=="menu") newval = (FlagMenu = !FlagMenu);
    else if(name=="help") newval = (FlagHelp = !FlagHelp);
    else if(name=="infos") newval = (FlagInfos = !FlagInfos);
    else if(name=="show_topbar") newval = (FlagShowTopBar = !FlagShowTopBar);
    else if(name=="show_time") newval = (FlagShowTime = !FlagShowTime);
    else if(name=="show_date") newval = (FlagShowDate = !FlagShowDate);
    else if(name=="show_appname") newval = (FlagShowAppName = !FlagShowAppName);
    else if(name=="show_selected_object_info") newval = (FlagShowSelectedObjectInfo = !FlagShowSelectedObjectInfo);
    else if(name=="show_fps") newval = (FlagShowFps = !FlagShowFps);
    else if(name=="show_fov") newval = (FlagShowFov = !FlagShowFov);
    else if(name=="enable_tui_menu") newval = (FlagEnableTuiMenu = !FlagEnableTuiMenu);
    else if(name=="show_gravity_ui") newval = (FlagShowGravityUi = !FlagShowGravityUi);
    else if(name=="show_tui_datetime") newval = (FlagShowTuiDateTime = !FlagShowTuiDateTime);
    else if(name=="show_tui_short_obj_info") newval = (FlagShowTuiShortObjInfo = !FlagShowTuiShortObjInfo);
    else if(name=="utc_time") newval = (FlagUTC_Time = !FlagUTC_Time);
    else if(name=="manual_zoom") newval = (FlagManualZoom = !FlagManualZoom);
    else if(name=="ground") newval = (FlagGround = !FlagGround);
    else if(name=="horizon") newval = (FlagHorizon = !FlagHorizon);
    else if(name=="fog") newval = (FlagFog = !FlagFog);
    else if(name=="atmosphere") newval = (FlagAtmosphere = !FlagAtmosphere);
    else if(name=="constellation_drawing") newval = (FlagConstellationDrawing = !FlagConstellationDrawing);
    else if(name=="constellation_name") newval = (FlagConstellationName = !FlagConstellationName);
    else if(name=="constellation_art") newval = (FlagConstellationArt = !FlagConstellationArt);
    else if(name=="constellation_pick") newval = (FlagConstellationPick = !FlagConstellationPick);
    else if(name=="azimutal_grid") newval = (FlagAzimutalGrid = !FlagAzimutalGrid);
    else if(name=="equatorial_grid") newval = (FlagEquatorialGrid = !FlagEquatorialGrid);
    else if(name=="equator_line") newval = (FlagEquatorLine = !FlagEquatorLine);
    else if(name=="ecliptic_line") newval = (FlagEclipticLine = !FlagEclipticLine);
    else if(name=="cardinal_points") newval = (FlagCardinalPoints = !FlagCardinalPoints);
    else if(name=="gravity_labels") newval = (FlagGravityLabels = !FlagGravityLabels);
    else if(name=="init_moon_scaled") {
      if(newval = (FlagInitMoonScaled = !FlagInitMoonScaled)) 
	ssystem->get_moon()->set_sphere_scale(MoonScale);
      else ssystem->get_moon()->set_sphere_scale(1.);
    }
    else if(name=="stars") newval = (FlagStars = !FlagStars);
    else if(name=="star_name") newval = (FlagStarName = !FlagStarName);
    else if(name=="planets") newval = (FlagPlanets = !FlagPlanets);
    else if(name=="planets_hints") newval = (FlagPlanetsHints = !FlagPlanetsHints);
    else if(name=="planets_orbits") newval = (FlagPlanetsOrbits = !FlagPlanetsOrbits);
    else if(name=="nebula") newval = (FlagNebula = !FlagNebula);
    else if(name=="nebula_name") newval = (FlagNebulaName = !FlagNebulaName);
    else if(name=="milky_way") newval = (FlagMilkyWay = !FlagMilkyWay);
    else if(name=="bright_nebulae") newval = (FlagBrightNebulae = !FlagBrightNebulae);
    else if(name=="object_trails") newval = (FlagObjectTrails = !FlagObjectTrails);
    else return(0);  // no matching flag found

  } else {

    newval = (value == "on" || value == "1");

    if(trusted) {
      if(name=="enable_zoom_keys") FlagEnableZoomKeys = newval;
      else if(name=="enable_move_keys") FlagEnableMoveKeys = newval;
      else if(name=="enable_move_mouse") FlagEnableMoveMouse = newval;
    }

    if(name=="star_twinkle") FlagStarTwinkle = newval;
    else if(name=="point_star") FlagPointStar = newval;
    else if(name=="menu") FlagMenu = newval;
    else if(name=="help") FlagHelp = newval;
    else if(name=="infos") FlagInfos = newval;
    else if(name=="show_topbar") FlagShowTopBar = newval;
    else if(name=="show_time") FlagShowTime = newval;
    else if(name=="show_date") FlagShowDate = newval;
    else if(name=="show_appname") FlagShowAppName = newval;
    else if(name=="show_selected_object_info") FlagShowSelectedObjectInfo = newval;
    else if(name=="show_fps") FlagShowFps = newval;
    else if(name=="show_fov") FlagShowFov = newval;
    else if(name=="enable_tui_menu") FlagEnableTuiMenu = newval;
    else if(name=="show_gravity_ui") FlagShowGravityUi = newval;
    else if(name=="show_tui_datetime") FlagShowTuiDateTime = newval;
    else if(name=="show_tui_short_obj_info") FlagShowTuiShortObjInfo = newval;
    else if(name=="utc_time") FlagUTC_Time = newval;
    else if(name=="manual_zoom") FlagManualZoom = newval;
    else if(name=="ground") FlagGround = newval;
    else if(name=="horizon") FlagHorizon = newval;
    else if(name=="fog") FlagFog = newval;
    else if(name=="atmosphere") FlagAtmosphere = newval;
    else if(name=="constellation_drawing") FlagConstellationDrawing = newval;
    else if(name=="constellation_name") FlagConstellationName = newval;
    else if(name=="constellation_art") FlagConstellationArt = newval;
    else if(name=="constellation_pick") FlagConstellationPick = newval;
    else if(name=="azimutal_grid") FlagAzimutalGrid = newval;
    else if(name=="equatorial_grid") FlagEquatorialGrid = newval;
    else if(name=="equator_line") FlagEquatorLine = newval;
    else if(name=="ecliptic_line") FlagEclipticLine = newval;
    else if(name=="cardinal_points") FlagCardinalPoints = newval;
    else if(name=="gravity_labels") FlagGravityLabels = newval;
    else if(name=="init_moon_scaled") {
      if((FlagInitMoonScaled = newval)) ssystem->get_moon()->set_sphere_scale(MoonScale);
      else ssystem->get_moon()->set_sphere_scale(1.);
    }
    else if(name=="stars") FlagStars = newval;
    else if(name=="star_name") FlagStarName = newval;
    else if(name=="planets") FlagPlanets = newval;
    else if(name=="planets_hints") FlagPlanetsHints = newval;
    else if(name=="planets_orbits") FlagPlanetsOrbits = newval;
    else if(name=="nebula") FlagNebula = newval;
    else if(name=="nebula_name") FlagNebulaName = newval;
    else if(name=="milky_way") FlagMilkyWay = newval;
    else if(name=="bright_nebulae") FlagBrightNebulae = newval;
    else if(name=="object_trails") FlagObjectTrails = newval;
    else return(0);  // no matching flag found

  }


  return(1);  // everything worked 

}


/*

// until migration is complete, temporary method to load up setting mgr with config
// needed for easy reset of settings after script completes

void stel_core::populate_setting_mgr(SettingMgr *smgr) {

  smgr->set("version", VERSION);
  smgr->set("fullscreen", Fullscreen);
  smgr->set("screen_w", screen_W);
  smgr->set("screen_h", screen_H);
  smgr->set("bpp_mode", bppMode);
  smgr->set("horizontal_offset", horizontalOffset);
  smgr->set("vertical_offset", verticalOffset);
  smgr->set("maxfps", maxfps);
  smgr->set("projector_type", ProjectorType);
  smgr->set("viewport", ViewportType);
  smgr->set("distortion_function", DistortionFunction);
  smgr->set("sky_culture", SkyCulture);
  smgr->set("sky_locale", SkyLocale);
  smgr->set("star_scale", StarScale);
  smgr->set("star_mag_scale", StarMagScale);
  smgr->set("star_twinkle_amount", StarTwinkleAmount);
  smgr->set("max_mag_star_name", MaxMagStarName);
  smgr->set("flag_star_twinkle", FlagStarTwinkle);
  smgr->set("flag_point_star", FlagPointStar);
  smgr->set("flag_show_fps", FlagShowFps);
  smgr->set("flag_menu", FlagMenu);
  smgr->set("flag_help", FlagHelp);
  smgr->set("flag_infos", FlagInfos);
  smgr->set("flag_show_top_bar", FlagShowTopBar);
  smgr->set("flag_show_time", FlagShowTime);
  smgr->set("flag_show_date", FlagShowDate);
  smgr->set("flag_show_app_name", FlagShowAppName);
  smgr->set("flag_show_fov", FlagShowFov);
  smgr->set("flag_show_selected_object_info", FlagShowSelectedObjectInfo);
  smgr->set("gui_base_color", GuiBaseColor);
  smgr->set("gui_text_color", GuiTextColor);
  smgr->set("base_font_size", BaseFontSize);
  smgr->set("azimuthal_color", AzimuthalColor);
  smgr->set("equatorial_color", EquatorialColor);
  smgr->set("equator_color", EquatorColor);
  smgr->set("ecliptic_color", EclipticColor);
  smgr->set("const_lines_color", ConstLinesColor);
  smgr->set("const_names_color", ConstNamesColor);
  smgr->set("nebula_label_color", NebulaLabelColor);
  smgr->set("nebula_circle_color", NebulaCircleColor);
  smgr->set("cardinal_color", CardinalColor);
  smgr->set("planet_names_color", PlanetNamesColor);
  smgr->set("planet_orbits_color", PlanetOrbitsColor);
  smgr->set("object_trails_color", ObjectTrailsColor);
  smgr->set("flag_enable_tui_menu", FlagEnableTuiMenu);
  smgr->set("flag_show_gravity_ui", FlagShowGravityUi);
  smgr->set("flag_show_tui_date_time", FlagShowTuiDateTime);
  smgr->set("flag_show_tui_short_obj_info", FlagShowTuiShortObjInfo);
  smgr->set("preset_sky_time", PresetSkyTime);
  smgr->set("startup_time_mode", StartupTimeMode);
  smgr->set("flag_enable_zoom_keys", FlagEnableZoomKeys);
  smgr->set("flag_manual_zoom", FlagManualZoom);
  smgr->set("flag_enable_move_keys", FlagEnableMoveKeys);
  smgr->set("flag_enable_move_mouse", FlagEnableMoveMouse);
  smgr->set("init_fov", InitFov);
  smgr->set("init_view_pos", InitViewPos);
  smgr->set("auto_move_duration", auto_move_duration);
  smgr->set("flag_utc_time", FlagUTC_Time);
  smgr->set("mouse_zoom", MouseZoom);
  smgr->set("viewing_mode", ViewingMode);
  smgr->set("flag_ground", FlagGround);
  smgr->set("flag_horizon", FlagHorizon);
  smgr->set("flag_fog", FlagFog);
  smgr->set("flag_atmosphere", FlagAtmosphere);
  smgr->set("atmosphere_fade_duration", AtmosphereFadeDuration);
  smgr->set("flag_constellation_drawing", FlagConstellationDrawing);
  smgr->set("flag_constellation_name", FlagConstellationName);
  smgr->set("flag_constellation_art", FlagConstellationArt);
  smgr->set("flag_constellation_pick", FlagConstellationPick);
  smgr->set("flag_azimutal_grid", FlagAzimutalGrid);
  smgr->set("flag_equatorial_grid", FlagEquatorialGrid);
  smgr->set("flag_equator_line", FlagEquatorLine);
  smgr->set("flag_ecliptic_line", FlagEclipticLine);
  smgr->set("flag_cardinal_points", FlagCardinalPoints);
  smgr->set("flag_gravity_labels", FlagGravityLabels);
  smgr->set("flag_init_moon_scaled", FlagInitMoonScaled);
  smgr->set("moon_scale", MoonScale);
  smgr->set("constellation_art_intensity", ConstellationArtIntensity);
  smgr->set("constellation_art_fade_duration", ConstellationArtFadeDuration);
  smgr->set("flag_stars", FlagStars);
  smgr->set("flag_star_name", FlagStarName);
  smgr->set("flag_planets", FlagPlanets);
  smgr->set("flag_planets_hints", FlagPlanetsHints);
  smgr->set("flag_planets_orbits", FlagPlanetsOrbits);
  smgr->set("flag_object_trails", FlagObjectTrails);
  smgr->set("flag_nebula", FlagNebula);
  smgr->set("flag_nebula_name", FlagNebulaName);
  smgr->set("max_mag_nebula_name", MaxMagNebulaName);
  smgr->set("flag_milky_way", FlagMilkyWay);
  smgr->set("flag_bright_nebulae", FlagBrightNebulae);

}

*/
