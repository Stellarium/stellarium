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

stel_core::stel_core() : screen_W(800), screen_H(600), bppMode(16), Fullscreen(0), initialized(0), navigation(NULL), selected_object(NULL), hip_stars(NULL), asterisms(NULL),
nebulas(NULL), atmosphere(NULL), tone_converter(NULL)
{
	TextureDir[0] = 0;
    ConfigDir[0] = 0;
    DataDir[0] = 0;
}

stel_core::~stel_core()
{
	if (navigation) delete navigation;
	if (selected_object) delete selected_object;
	if (hip_stars) delete hip_stars;
	if (asterisms) delete asterisms;
	if (nebulas) delete nebulas;
	if (atmosphere) delete atmosphere;
	if (tone_converter) delete tone_converter;
	if (ui) delete ui;
}


// Update all the objects in function of the time
void stel_core::update(int delta_time)
{
	static int frame, timefr, timeBase;
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
	navigation->update_vision_vector(delta_time);		// Direction of vision

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
	else DrawMilkyWay();

	// Init the depth buffer which is used by the planets drawing operations
	glClear(GL_DEPTH_BUFFER_BIT);

	// Draw the nebula if they are visible
	if (FlagNebula && (!FlagAtmosphere || sky_brightness<0.1)) nebulas->Draw(FlagNebulaName, du);

	// Draw all the constellations
	if (FlagConstellationDrawing) asterisms->Draw();

	// Draw the hipparcos stars
	if (FlagStars && (!FlagAtmosphere || sky_brightness<0.2)) hip_stars->Draw(StarScale, StarTwinkleAmount, FlagStarName, MaxMagStarName, du);

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
	if (FlagPlanets) Sun->draw(FlagPlanetsHintDrawing, du);

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
    if (FlagHorizon && FlagGround) DrawDecor(2);

	// Draw the ground
    if (FlagGround) DrawGround();

	// Draw the fog
    if (FlagFog) DrawFog();

	// Daw the cardinal points
    if (FlagCardinalPoints) DrawCardinaux();

    // ---- 2D Displays
    ui->draw();
}




void stel_core::init(void)
{
    hip_stars = new Hip_Star_mgr();
    asterisms = new Constellation_mgr();
    nebulas   = new Nebula_mgr();

	// Temporary strings for file names
    char tempName[255];
    char tempName2[255];
    char tempName3[255];
    char tempName4[255];

	// Set textures directory and suffix
	s_texture::set_texDir(TextureDir);
	s_texture::set_suffix(".png");

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

	// Compute planets data
    Sun->compute_position(navigation->get_JDay());

	// Precalculation for the grids drawing TODO will be in a class
    InitMeriParal();

	// Create atmosphere renderer
	atmosphere=new stel_atmosphere();

	// Create tone reproductor
	tone_converter=new tone_reproductor();

	// initialisation of the User Interface
    ui->init();
}


void stel_core::load_config()
{
    char tempName[255];
    char tempName2[255];

    strcpy(tempName,ConfigDir);
    strcat(tempName,"config.txt");
    strcpy(tempName2,ConfigDir);
    strcat(tempName2,"location.txt");

    printf("Loading configuration file... (%s)\n",tempName);

    loadConfig(tempName,tempName2);  // Load the params from config.txt

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
    texIds[11]= new s_texture("nadir");
    texIds[12]= new s_texture("pointeur2");
    texIds[25]= new s_texture("etoile32x32");
    texIds[26]= new s_texture("pointeur4");
    texIds[27]= new s_texture("pointeur5");

    switch (LandscapeNumber)
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
        exit(1);
    }

    if (nebulas->ReadTexture()==0)
	printf("Error while loading messier Texture\n");
}



// find and select the "nearest" object from earth equatorial position
stel_object * stel_core::find_stel_object(Vec3d v)
{
	stel_object * sobj = NULL;

	if (FlagPlanets) sobj = Sun->search(v);
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
