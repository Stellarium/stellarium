/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

// I'm trying to comment the source but my english isn't perfect, so be 
//tolerant and please correct all the mistakes you find. ;)


#include "stellarium.h"
#include "draw.h"
#include "constellation.h"
#include "constellation_mgr.h"
#include "nebula.h"
#include "nebula_mgr.h"
#include "s_texture.h"
#include "stellarium_ui.h"
#include "s_gui.h"
#include "hip_star_mgr.h"
#include "shooting.h"
#include "stel_config.h"
#include "solarsystem.h"
#include "navigator.h"
#include "stel_object.h"

#include "SDL.h"

using namespace std;

navigator navigation;
stel_object * selected_object=NULL;
stellariumParams global;
Hip_Star_mgr * HipVouteCeleste;       // Class to manage the Hipparcos catalog
Constellation_mgr * ConstellCeleste;  // Constellation boundary and name
Nebula_mgr * messiers;                // Class to manage the messier objects
s_texture * texIds[200];              // Common Textures

/*ShootingStar * TheShooting = NULL;*/

// Globals
bool isProgramLooping;	// Wether the Program Must Go On In The Main Loop
S_AppStatus AppStatus;	// Holds The Runtime Data Of The Application


static int timeAtmosphere=0;

// ***************  Print a beautiful console logo !! ******************
void drawIntro(void)
{
    printf("\n");
    printf("    -----------------------------------------\n");
    printf("    | ## ### ### #   #   ### ###  #  # # # #|\n");
    printf("    | #   #  ##  #   #   ### ##   #  # # ###|\n");
    printf("    |##   #  ### ### ### # # # #  #  ### # #|\n");
    printf("    |                     %s|\n",APP_NAME);
    printf("    -----------------------------------------\n\n");
    printf("Copyright (C) 2003 Fabien Chereau\n\n");
    printf("Please send bug report & comments to stellarium@free.fr\n");
    printf("and check last version on http://stellarium.free.fr\n\n");
};

// ************************  Main display loop  ************************
// Execute all the drawing function in the correct order from the
// furthest to closest objects
void Draw(int delta_time)
{
	// Init openGL viewing with fov, screen size and clip planes
	navigation.init_project_matrix(global.X_Resolution,global.Y_Resolution,0.00000001,1000);

    // Set openGL drawings in equatorial coordinates
    navigation.switch_to_earth_equatorial();

    DrawMilkyWay();                  // Draw the milky way --> init the buffers

    if (global.FlagNebula && (!global.FlagAtmosphere || global.SkyBrightness<0.1))
		messiers->Draw();            // Draw the Messiers Objects
    if (global.FlagConstellationDrawing)
        ConstellCeleste->Draw();     // Draw all the constellations
    if ((!global.FlagAtmosphere && global.FlagStars) ||
        (global.SkyBrightness<0.2 && global.FlagStars))
    {
		HipVouteCeleste->Draw();    // Draw the stars
    }

    if (global.FlagAtmosphere && global.SkyBrightness>0)
		DrawAtmosphere2();	// Draw the atmosphere

    //Sun->DrawMoonDaylight();

    if (global.FlagEquatorialGrid)
    {
		DrawMeridiens();             			// Draw the meridian lines
        DrawParallels();             			// Draw the parallel lines
    }
    if (global.FlagEquator)	DrawEquator();   	// Draw the celestial equator line
    if (global.FlagEcliptic) DrawEcliptic(); 	// Draw the ecliptic line
    if (global.FlagConstellationName) ConstellCeleste->DrawName();	// Draw the constellations's names
    if (selected_object) selected_object->draw_pointer(delta_time);			// Draw the pointer

	navigation.switch_to_heliocentric();
	if (global.FlagPlanets || global.FlagPlanetsHintDrawing) Sun->draw();

	// Set openGL drawings in local coordinates i.e. generally altazimuthal coordinates
	navigation.switch_to_local();

    if (global.FlagAzimutalGrid)
    {
		DrawMeridiensAzimut();       // Draw the "Altazimuthal meridian" lines
        DrawParallelsAzimut();       // Draw the "Altazimuthal parallel" lines
    }
    if (global.FlagAtmosphere)       // Calc the atmosphere
    {
    	// Draw atmosphere every second frame because it's slow....
        if (++timeAtmosphere>1 && global.SkyBrightness>0)
        {
	    	timeAtmosphere=0;
            CalcAtmosphere();
        }
    }
    if (global.FlagHorizon && global.FlagGround)
        DrawDecor(2);                               // Draw the mountains
    if (global.FlagGround) DrawGround();            // Draw the ground
    if (global.FlagFog) DrawFog();                  // Draw the fog
    if (global.FlagCardinalPoints) DrawCardinaux(); // Daw the cardinal points

    // ---- 2D Displays
    renderUi();

}

// ************************  On resize  *******************************
void ResizeGL(int w, int h)
{
    if (!h || (w==global.X_Resolution && h==global.Y_Resolution)) return;
    global.X_Resolution = w;
    global.Y_Resolution = h;
	clearUi();
	initUi();
    glViewport(0, 0, global.X_Resolution, global.Y_Resolution);
	navigation.init_project_matrix(global.X_Resolution,global.Y_Resolution,1,10000);
}


// ************************  Initialisation  ******************************
void loadCommonTextures(void)
{
    printf("Loading common textures...\n");
    texIds[2] = new s_texture("voielactee256x256",TEX_LOAD_TYPE_PNG_SOLID);
    texIds[3] = new s_texture("fog",TEX_LOAD_TYPE_PNG_REPEAT);
    texIds[4] = new s_texture("ciel");
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
    texIds[30]= new s_texture("spacefont");

    switch (global.LandscapeNumber)
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

    texIds[47]= new s_texture("saturneAnneaux128x128",TEX_LOAD_TYPE_PNG_ALPHA);
    texIds[48]= new s_texture("halo");
    texIds[50]= new s_texture("haloLune");
    if (messiers->ReadTexture()==0)
	printf("Error while loading messier Texture\n");
}


void TerminateApplication(void)				// Terminate The Application
{
	static SDL_Event Q;						// We're Sending A SDL_QUIT Event
	Q.type = SDL_QUIT;						// To The SDL Event Queue
	if(SDL_PushEvent(&Q) == -1)				// Try Send The Event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
		exit(1);
	}
	return;
}

// *******************  Handle time  **************************
void Update(Uint32 delta_time)
{
	static int frame, timefr, timeBase;
	frame++;
    timefr+=delta_time;
    if (timefr-timeBase > 1000)
    {
		global.Fps=frame*1000.0/(timefr-timeBase);     // Calc the PFS rate
        frame = 0;
        timeBase+=1000;
    }

    // Update the position of observation and time etc...
	navigation.update_time(delta_time);
	Sun->compute_position(navigation.get_JDay());
	Sun->compute_trans_matrix(navigation.get_JDay());
	navigation.update_transform_matrices();
	navigation.update_vision_vector(delta_time);


    // compute sky brightness
    if (global.FlagAtmosphere)
    {
        Vec3d sunPos = Sun->get_ecliptic_pos();
        sunPos.normalize();
        global.SkyBrightness=asin(sunPos[1])+0.1;
        if (global.SkyBrightness<0) global.SkyBrightness=0;
    }
    else
    {
        global.SkyBrightness=0;
    }
	if (selected_object) selected_object->update();
	return;
}

// ********************  Handle keys  **********************
void pressKey(Uint8 *keys)
{
    // Direction and zoom deplacements
    if(keys[SDLK_LEFT])
	{
		navigation.turn_left(1);
	}
    if(keys[SDLK_RIGHT])
	{
		navigation.turn_right(1);
	}
    if(keys[SDLK_UP])
	{
		if (SDL_GetModState() & KMOD_CTRL)
		{
			navigation.zoom_in(1);
		}
		else
		{	navigation.turn_up(1);
		}
	}
    if(keys[SDLK_DOWN])
	{
		if (SDL_GetModState() & KMOD_CTRL)
		{
			navigation.zoom_out(1);
		}
		else
		{
			navigation.turn_down(1);
		}
	}
    if(keys[SDLK_PAGEUP]) navigation.zoom_in(1);
    if(keys[SDLK_PAGEDOWN]) navigation.zoom_out(1);

}

// *******************  Stop mooving and zooming  **********************
void releaseKey(SDLKey key)
{   
    // When a deplacement key is released stop mooving
    if (key==SDLK_LEFT) navigation.turn_left(0);
	if (key==SDLK_RIGHT) navigation.turn_right(0);
	if (SDL_GetModState() & KMOD_CTRL)
	{
		if (key==SDLK_UP) navigation.zoom_in(0);
		if (key==SDLK_DOWN) navigation.zoom_out(0);
	}
	else
	{
		if (key==SDLK_UP) navigation.turn_up(0);
		if (key==SDLK_DOWN) navigation.turn_down(0);
	}

    if (key==SDLK_PAGEUP)  navigation.zoom_in(0);
	if (key==SDLK_PAGEDOWN) navigation.zoom_out(0);
}

bool InitTimers(Uint32 *C)   // This Is Used To Init All The Timers In Our Application
{
	*C = SDL_GetTicks(); // Hold The Value Of SDL_GetTicks At The Program Init
	return true;	     // Return TRUE (Initialization Successful)
}


bool Initialize(void)	     // Any Application & User Initialization Code Goes Here
{
    AppStatus.Visible	    = true;	 // At The Beginning, Our App Is Visible
    AppStatus.MouseFocus    = true;	 // And Have Both Mouse
    AppStatus.KeyboardFocus = true;	 // And Input Focus

    SDL_EnableKeyRepeat(0, 0); // Disable key repeat

    HipVouteCeleste = new Hip_Star_mgr();
    if (!HipVouteCeleste) exit(-1);

    ConstellCeleste = new Constellation_mgr();
    if (!ConstellCeleste) exit(-1);

    messiers = new Nebula_mgr();
    if (!messiers) exit(-1);

	InitSolarSystem(); // Create and init the solar system
    if (!Sun) exit(-1);

    loadCommonTextures();            // Load the common used textures

    char tempName[255];
    char tempName2[255];
    char tempName3[255];

    // Load hipparcos stars & names
    strcpy(tempName,global.DataDir);
    strcat(tempName,"hipparcos.fab");
    strcpy(tempName2,global.DataDir);
    strcat(tempName2,"commonname.fab");
    strcpy(tempName3,global.DataDir);
    strcat(tempName3,"name.fab");
    HipVouteCeleste->Load(tempName,tempName2,tempName3);

    strcpy(tempName,global.DataDir);
    strcat(tempName,"constellationship.fab");
    ConstellCeleste->Load(tempName,HipVouteCeleste);	// Load constellations
    Sun->compute_position(navigation.get_JDay());		// Compute planet data
    InitMeriParal();                 	// Precalculation for the grids drawing
    InitAtmosphere();

    strcpy(tempName,global.DataDir);
    strcat(tempName,"messier.fab");
    messiers->Read(tempName);        // read the messiers object data
    initUi();                        // initialisation of the User Interface
    return true;		     // Return TRUE (Initialization Successful)
}


// ***************************  InitGL  ********************************
bool InitGL(SDL_Surface *S)  // Any OpenGL Initialization Code Goes Here
{
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glDisable(GL_DEPTH_TEST);
    // init the blending function parameters
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    return true;     // Return TRUE (Initialization Successful)
}


// ***************************  Main  **********************************
int main(int argc, char **argv)
{   
    
    // Check command line arguments
    if (argc==2 && !( strcmp(argv[1],"--version") && strcmp(argv[1],"-version") && strcmp(argv[1],"--v") && strcmp(argv[1],"-v")))
    {
        printf("%s\n",APP_NAME);
        exit(0);
    }
    
    if (argc>1)
    {
        printf("Bad arguments : possible options are -version or -v\n");
        exit(0);
    }
    
    setDirectories();
    
    drawIntro();        // Print the console logo
    
    char tempName[255];
    char tempName2[255];
    
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"config.txt");
    strcpy(tempName2,global.ConfigDir);
    strcat(tempName2,"location.txt");

    printf("Loading configuration file... (%s)\n",tempName);

    loadConfig(tempName,tempName2);  // Load the params from config.txt

    SDL_Surface *Screen;// The Screen
    SDL_Event	E;		// And Event Used In The Polling Process
    Uint8	*Keys;		// A Pointer To An Array That Will Contain The Keyboard Snapshot
    Uint32	Vflags;		// Our Video Flags
    Uint32	TickCount;	// Used For The Tick Counter
    Uint32	LastCount;	// Used For The Tick Counter
    Screen = NULL;
    Keys = NULL;

    if(SDL_Init(SDL_INIT_VIDEO)<0)  // Init The SDL Library, The VIDEO Subsystem
    {
         printf("Unable to open SDL: %s\n", SDL_GetError() );
         exit(1);
    }

    // Make Sure That SDL_Quit Will Be Called In Case of exit()
    atexit(SDL_Quit);

    // We Want A Hardware Surface
    Vflags = SDL_HWSURFACE|SDL_OPENGL;

	// If fullscreen, set the Flag
    if (global.Fullscreen) Vflags|=SDL_FULLSCREEN;

	// Create the SDL Screen surface
    Screen = SDL_SetVideoMode(global.X_Resolution, global.Y_Resolution, global.bppMode, Vflags);
	if(!Screen)
	{
		printf("sdl: Couldn't set %dx%d video mode: %s!",
		global.X_Resolution, global.Y_Resolution, SDL_GetError());
		exit(-1);
	}

	SDL_WM_SetCaption(APP_NAME, APP_NAME);	// Set The Window Caption
    strcpy(tempName,global.DataDir);
    strcat(tempName,"icon.bmp");
	SDL_WM_SetIcon(SDL_LoadBMP(tempName), NULL);


	if(!InitTimers(&LastCount))			// We Call The Timers Init Function
	{
		printf("Can't init the timers: %s\n", SDL_GetError() );
		exit(-1);
	}

	if(!InitGL(Screen))					// CallThe OpenGL Init Function
	{
		printf("Can't init GL: %s\n", SDL_GetError() );
		exit(-1);
	}

	if(!Initialize())					// Init The Application
	{
		printf("App init failed: %s\n", SDL_GetError() );
		exit(-1);
	}

	isProgramLooping = true;			// Make Our Program Loop

	while(isProgramLooping)				// While It's looping
	{
		if(SDL_PollEvent(&E))			// Fetch The First Event Of The Queue
		{
			switch(E.type)				// And Processing It
			{	
			case SDL_QUIT:
				{
					isProgramLooping = false;
					break;
				}

			case SDL_VIDEORESIZE:
				{
					// Recalculate The OpenGL Scene Data For The New Window
					ResizeGL(E.resize.w, E.resize.h);
					break;
				}

			case SDL_ACTIVEEVENT:
				{
					if(E.active.state & SDL_APPACTIVE)
					{
						// Activity Level Changed (IE: Iconified)
						if(E.active.gain)
						{
							// Activity's Been Gained
							AppStatus.Visible = true;
						}
						else
						{
							AppStatus.Visible = false;
						}
					}
					
					// The Mouse Cursor Has Left/Entered The Window Space?
					if(E.active.state & SDL_APPMOUSEFOCUS)
					{
						if(E.active.gain)	// Entered
						{
							AppStatus.MouseFocus = true;
						}
						else
						{
							AppStatus.MouseFocus = false;
						}
					}

					// The Window Has Gained/Lost Input Focus?
					if(E.active.state & SDL_APPINPUTFOCUS)
					{
						if(E.active.gain)	// Gained
						{
							AppStatus.KeyboardFocus = true;
						}
						else
						{
							AppStatus.KeyboardFocus = false;
						}
					}
					
					break;
				}

			case SDL_MOUSEMOTION:
				{
				  	GuiHandleMove(E.motion.x,E.motion.y);
					break;
				}

			case SDL_MOUSEBUTTONDOWN:
				{
					GuiHandleClic(E.button.x,E.button.y,E.button.state,E.button.button);
					break;
				}
				
			case SDL_MOUSEBUTTONUP:
				{
					GuiHandleClic(E.button.x,E.button.y,E.button.state,E.button.button);
					break;
				}

			case SDL_KEYDOWN:	// Someone Has Pressed A Key?
				{
					// Send the event to the gui and stop if it has been intercepted
					if (!GuiHandleKeys(E.key.keysym.sym,GUI_DOWN))
					{
						// Take A SnapShot Of The Keyboard
						Keys = SDL_GetKeyState(NULL);
						if (Keys[SDLK_F1])
							SDL_WM_ToggleFullScreen(Screen); // Try fullscreen
						else
							pressKey(Keys);
					}
					break;
				}

			case SDL_KEYUP:	   // Someone Has Released A Key?
				{
					releaseKey(E.key.keysym.sym); // Handle the info
				}
				
			}
		}
		else  // No Events To Poll? (SDL_PollEvent()==0?)
		{
			// If The Application Is Not Visible
			if(!AppStatus.Visible)
			{
				// Leave The CPU Alone, Don't Waste Time, Simply Wait For An Event
				SDL_WaitEvent(NULL);
			}
			else
			{
				TickCount = SDL_GetTicks();	// Get Present Ticks
				Update(TickCount-LastCount);// And Update The Motions And Data
				Draw(TickCount-LastCount);  // Do The Drawings!
				LastCount = TickCount;		// Save The Present Tick Probing
				SDL_GL_SwapBuffers();		// And Swap The Buffers
			}
		}
	}

	exit(0);

	return 0;

}
