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

extern s_texture * texIds[200];            // Common Textures TODO : Remove that

stel_core::stel_core() : screen_W(800), screen_H(600), bppMode(16), Fullscreen(0), initialized(0), navigation(NULL), selected_object(NULL), hip_stars(NULL), asterisms(NULL),
nebulas(NULL), atmosphere(NULL), tone_converter(NULL), du(NULL), conf(NULL), frame(0), timefr(0), timeBase(0)
{
	TextureDir[0] = 0;
    ConfigDir[0] = 0;
    DataDir[0] = 0;

	navigation = new navigator();
}

stel_core::~stel_core()
{
	if (navigation) delete navigation;
	if (selected_object) delete selected_object;
	if (asterisms) delete asterisms;
	if (hip_stars) delete hip_stars;
	if (nebulas) delete nebulas;
	if (atmosphere) delete atmosphere;
	if (tone_converter) delete tone_converter;
	printf("Core destructor\n");
	//if (ui) delete ui;
}

// Set the main data, textures and configuration directories
void stel_core::set_directories(const char * DDIR, const char * TDIR, const char * CDIR)
{
	strncpy(TextureDir, TDIR, sizeof(TextureDir));
	strncpy(ConfigDir, CDIR, sizeof(ConfigDir));
	strncpy(DataDir, DDIR, sizeof(DataDir));
}

// Set the 2 config files names.
void stel_core::set_config_files(const char * _config_file, const char * _location_file)
{
	strncpy(config_file, _config_file, strlen(_config_file) +1);
	strncpy(location_file, _location_file, strlen(_location_file) +1);
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

	Sun->compute_position(navigation->get_JDay());		// Position of sun and all the satellites (ie planets)
	Sun->compute_trans_matrix(navigation->get_JDay());	// Matrix for sun and all the satellites (ie planets)
	navigation->update_transform_matrices();			// Transform matrices between coordinates systems
 	navigation->update_vision_vector(delta_time, selected_object);		// Direction of vision

	// Set the common variables used by the draw functions
	du->set_params(navigation->get_fov(), screen_W, screen_H);

	// Update info about selected object
	if (selected_object) selected_object->update();

	// Compute the sun position in local coordinate
	Vec3d temp(0.,0.,0.);
	Vec3d sunPos = navigation->helio_to_local(&temp);
	sunPos.normalize();

	// Compute the atmosphere color
	if (FlagAtmosphere)
	{
		navigation->switch_to_local();
		atmosphere->compute_color(FlagGround, sunPos, tone_converter, du);
	}

	// compute global sky brightness TODO : function to include in skylight.cpp correctly made
	sky_brightness=asin(sunPos[2])+0.1;
	if (sky_brightness<0) sky_brightness=0;
}

// Execute all the drawing functions
void stel_core::draw(int delta_time)
{
	// Init openGL viewing with fov, screen size and clip planes
	navigation->init_project_matrix(screen_W,screen_H,0.00001 ,40 );

    // Set openGL drawings in equatorial coordinates
    navigation->switch_to_earth_equatorial();

	// Draw the milky way. If not activated, need at least to clear the color buffer
	if (!FlagMilkyWay) glClear(GL_COLOR_BUFFER_BIT);
	else DrawMilkyWay(sky_brightness);

	// Init the depth buffer which is used by the planets drawing operations
	glClear(GL_DEPTH_BUFFER_BIT);

	// Draw the nebula if they are visible
	if (FlagNebula && (!FlagAtmosphere || sky_brightness<0.1)) nebulas->Draw(FlagNebulaName, du);

	// Draw all the constellations
	if (FlagConstellationDrawing) asterisms->Draw();

	// Draw the hipparcos stars
	// convert.... TODO implicit convertion
	Vec3d tempv = navigation->get_equ_vision();
	Vec3f temp(tempv[0],tempv[1],tempv[2]);
	if (FlagStars && (!FlagAtmosphere || sky_brightness<0.2))
		hip_stars->Draw(StarScale, StarTwinkleAmount, FlagStarName, MaxMagStarName, temp, du);

	// Draw the atmosphere
	if (FlagAtmosphere)	atmosphere->draw(du);

	// Draw the equatorial grid
	// TODO : make a nice class for grid wit parameters like numbering and custom color/frequency
    if (FlagEquatorialGrid)
	{
		DrawMeridiens();				// Draw the meridian lines
        DrawParallels();				// Draw the parallel lines
	}

	// TODO : make a nice class for lines management
    if (FlagEquator) DrawEquator();		// Draw the celestial equator line
    if (FlagEcliptic) DrawEcliptic();	// Draw the ecliptic line

	// Draw the constellations's names
    if (FlagConstellationName) asterisms->DrawName(du);

	// Draw the pointer on the currently selected object
    if (selected_object) selected_object->draw_pointer(delta_time, du);

	// Set openGL drawings in heliocentric coordinates
	navigation->switch_to_heliocentric();

	// Draw the planets
	// TODO : manage FlagPlanetsHintDrawing
	if (FlagPlanets) Sun->draw(FlagPlanetsHintDrawing, du, navigation);

	// Set openGL drawings in local coordinates i.e. generally altazimuthal coordinates
	navigation->switch_to_local();

	// Draw the altazimutal grid
	// TODO : make a nice class for grid wit parameters like numbering and custom color/frequency
    if (FlagAzimutalGrid)
	{
		DrawMeridiensAzimut();		// Draw the "Altazimuthal meridian" lines
        DrawParallelsAzimut();		// Draw the "Altazimuthal parallel" lines
	}

	// Draw the mountains
	// TODO custom decor type
    if (FlagHorizon && FlagGround) DrawDecor(2, sky_brightness);

	// Draw the ground
    if (FlagGround) DrawGround(sky_brightness);

	// Draw the fog
    if (FlagFog) DrawFog(sky_brightness);

	// Daw the cardinal points
    if (FlagCardinalPoints) DrawCardinaux(du);

    // ---- 2D Displays
    ui->draw();
}




void stel_core::init(void)
{
	// Set textures directory and suffix
	s_texture::set_texDir(TextureDir);
	s_texture::set_suffix(".png");

    hip_stars = new Hip_Star_mgr();
    asterisms = new Constellation_mgr();
    nebulas   = new Nebula_mgr();
	// Create atmosphere renderer
	atmosphere=new stel_atmosphere();
	// Create tone reproductor
	tone_converter=new tone_reproductor();
	du = new draw_utility();

	// Temporary strings for file names
    char tempName[255];
    char tempName2[255];
    char tempName3[255];
    char tempName4[255];

    // Load hipparcos stars & names
    strcpy(tempName,DataDir);
    strcat(tempName,"hipparcos.fab");
    strcpy(tempName2,DataDir);
    strcat(tempName2,"commonname.fab");
    strcpy(tempName3,DataDir);
    strcat(tempName3,"name.fab");
    strcpy(tempName4,DataDir);
    strcat(tempName4,"spacefont.txt");
    hip_stars->Load(tempName4, tempName,tempName2,tempName3);

	// Load constellations
    strcpy(tempName,DataDir);
    strcat(tempName,"constellationship.fab");
    strcpy(tempName2,DataDir);
    strcat(tempName2,"spacefont.txt");
    asterisms->Load(tempName2,tempName,hip_stars);

	// Load the nebulas data TODO : add NGC objects
    strcpy(tempName,DataDir);
    strcat(tempName,"messier.fab");
    strcpy(tempName2,DataDir);
    strcat(tempName2,"spacefont.txt");
    nebulas->Read(tempName2, tempName);

	// Create and init the solar system TODO : use a class
    strcpy(tempName,DataDir);
    strcat(tempName,"spacefont.txt");
	InitSolarSystem(tempName);

	// Load the common used textures TODO : will be removed
    load_base_textures();

	// Precalculation for the grids drawing TODO will be in a class
    InitMeriParal();

	// initialisation of the User Interface
	ui = new stel_ui(this);
    ui->init();

	// Compute planets data
    Sun->compute_position(navigation->get_JDay());

}


void stel_core::load_config(void)
{
    char tempName[255];
    char tempName2[255];

    strcpy(tempName,ConfigDir);
    strcat(tempName,config_file);
    strcpy(tempName2,ConfigDir);
    strcat(tempName2,location_file);

	navigation->load_position(tempName2);

    printf("Loading configuration file... (%s)\n",tempName);

	if (conf) delete (conf);
	conf = new init_parser(tempName);
	conf->load();

	// Main section
	char * version = NULL;
	if (conf->get_str("main:version")) version = strdup(conf->get_str("main:version"));

	// Video Section
	Fullscreen			= conf->get_boolean("video:fullscreen");
	screen_W			= conf->get_int	   ("video:screen_w");
	screen_H			= conf->get_int	   ("video:screen_h");
	bppMode				= conf->get_int    ("video:bbp_mode");

	// Star section
	StarScale			= conf->get_double ("stars:star_scale");
	StarTwinkleAmount	= conf->get_double ("stars:star_twinkle_amount");
	MaxMagStarName		= conf->get_double ("stars:max_mag_star_name");

	// Ui section
	FlagShowFps			= conf->get_boolean("gui:flag_show_fps");
	FlagMenu			= conf->get_boolean("gui:flag_menu");
	FlagHelp			= conf->get_boolean("gui:flag_help");
	FlagInfos			= conf->get_boolean("gui:flag_infos");
	FlagUTC_Time		= conf->get_boolean("gui:flag_utc_time");
	GuiBaseColor		= str_to_vec3f(conf->get_str("gui:gui_base_color"));
	GuiTextColor		= str_to_vec3f(conf->get_str("gui:gui_text_color"));

	// Navigation section
	navigation->set_flag_lock_equ_pos(conf->get_boolean("navigation:flag_lock_equ_pos"));
    // init the time parameters with current time and date
	const ln_date * pDate = str_to_date(conf->get_str("navigation:date"),conf->get_str("navigation:time"));
	if (pDate) navigation->set_JDay(get_julian_day(pDate));
	else navigation->set_JDay(get_julian_from_sys());

	// Landscape section
	landscape_number	= conf->get_int    ("landscape:landscape_number");
	FlagGround			= conf->get_boolean("landscape:flag_ground");
	FlagHorizon			= conf->get_boolean("landscape:flag_horizon");
	FlagFog				= conf->get_boolean("landscape:flag_fog");
	FlagAtmosphere		= conf->get_boolean("landscape:flag_atmosphere");

	// Viewing section
	FlagConstellationDrawing= conf->get_boolean("viewing:flag_constellation_drawing");
	FlagConstellationName	= conf->get_boolean("viewing:flag_constellation_name");
	FlagAzimutalGrid		= conf->get_boolean("viewing:flag_azimutal_grid");
	FlagEquatorialGrid		= conf->get_boolean("viewing:flag_equatorial_grid");
	FlagEquator				= conf->get_boolean("viewing:flag_equator");
	FlagEcliptic			= conf->get_boolean("viewing:flag_ecliptic");
	FlagCardinalPoints		= conf->get_boolean("viewing:flag_cardinal_points");

	// Astro section
	FlagStars				= conf->get_boolean("astro:flag_stars");
	FlagStarName			= conf->get_boolean("astro:flag_star_name");
	FlagPlanets				= conf->get_boolean("astro:flag_planets");
	FlagPlanetsHintDrawing	= conf->get_boolean("astro:flag_planets_hints");
	FlagNebula				= conf->get_boolean("astro:flag_nebula");
	FlagNebulaName			= conf->get_boolean("astro:flag_nebula_name");
	FlagMilkyWay			= conf->get_boolean("astro:flag_milky_way");

	//conf->save();
}


// Load the textures "for non object oriented stuff" TODO : remove that
void stel_core::load_base_textures(void)
{
    printf("Loading common textures...\n");
    texIds[2] = new s_texture("voielactee256x256",TEX_LOAD_TYPE_PNG_SOLID);
    texIds[3] = new s_texture("fog",TEX_LOAD_TYPE_PNG_REPEAT);
    texIds[6] = new s_texture("n");
    texIds[7] = new s_texture("s");
    texIds[8] = new s_texture("e");
    texIds[9] = new s_texture("w");
    texIds[10]= new s_texture("zenith");

    texIds[12]= new s_texture("pointeur2");
    texIds[25]= new s_texture("etoile32x32");
    texIds[26]= new s_texture("pointeur4");
    texIds[27]= new s_texture("pointeur5");
    texIds[11]= new s_texture("nadir");

    switch (landscape_number)
    {
    case 1 :
        texIds[31]= new s_texture("landscapes/sea1",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[32]= new s_texture("landscapes/sea2",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[33]= new s_texture("landscapes/sea3",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[34]= new s_texture("landscapes/sea4",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[1] = new s_texture("landscapes/sea5",TEX_LOAD_TYPE_PNG_SOLID);
        break;
    case 2 :
        texIds[31]= new s_texture("landscapes/mountain1",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[32]= new s_texture("landscapes/mountain2",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[33]= new s_texture("landscapes/mountain3",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[34]= new s_texture("landscapes/mountain4",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[1] = new s_texture("landscapes/mountain5",
				  TEX_LOAD_TYPE_PNG_SOLID);
        break;
    case 3 :
        texIds[31]= new s_texture("landscapes/snowy1",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[32]= new s_texture("landscapes/snowy2",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[33]= new s_texture("landscapes/snowy3",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[34]= new s_texture("landscapes/snowy4",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[1] = new s_texture("landscapes/snowy5",
				  TEX_LOAD_TYPE_PNG_SOLID);
        break;
    default :
        printf("ERROR : Bad landscape number, change it in config.txt\n");
        exit(-1);
    }

}



// find and select the "nearest" object from earth equatorial position
stel_object * stel_core::find_stel_object(Vec3d v)
{
	stel_object * sobj = NULL;

	if (FlagPlanets) sobj = Sun->search(v, navigation);
	if (sobj) return sobj;

	Vec3f u=Vec3f(v[0],v[1],v[2]);

	sobj = nebulas->search(u);
	if (sobj) return sobj;

	if (FlagStars) sobj = hip_stars->search(u);

	return sobj;
}


// find and select the "nearest" object from screen position
stel_object * stel_core::find_stel_object(int x, int y)
{
    glPushMatrix();
    navigation->switch_to_earth_equatorial();
	Vec3d v = du->unproject((double)x,(double)y);
    glPopMatrix();

	return find_stel_object(v);
}
