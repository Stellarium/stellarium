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
#include "stel_atmosphere.h"
#include "tone_reproductor.h"

#include "SDL.h"

using namespace std;

struct AppStatus		// We Use A Struct To Hold Application Runtime Data
{
	bool Visible;		// Is The Application Visible? Or Iconified?
	bool MouseFocus;	// Is The Mouse Cursor In The Application Field?
	bool KeyboardFocus;	// Is The Input Focus On Our Application?
};

// Globals
bool isProgramLooping;	// Wether the Program Must Go On In The Main Loop


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
    printf("Please check last version and send bug report & comments \n");
    printf("on stellarium web page : http://stellarium.free.fr\n\n");
};


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

	sky=new stel_atmosphere();
	eye=new tone_reproductor();

    initUi();                        // initialisation of the User Interface
    return true;		     // Return TRUE (Initialization Successful)
}


// ***************************  InitGL  ********************************
bool InitGL(SDL_Surface *S)  // Any OpenGL Initialization Code Goes Here
{
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    // init the blending function parameters
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    return true;     // Return TRUE (Initialization Successful)
}

// ***************************  Usage  *********************************
void usage(char **argv)
{
	printf("Usage: %s [OPTION] ...\n -v, --version          output version information and exit\n -h, --help             display this help and exit\n", argv[0]);
}


// ***************************  Main  **********************************
int main(int argc, char **argv)
{
    // Check command line arguments
    if (argc == 2)
    {
        if (!(strcmp(argv[1],"--version") && strcmp(argv[1],"-v")))
        {
            printf("%s\n", APP_NAME);
            exit(0);
        }
        if (!(strcmp(argv[1],"--help") && strcmp(argv[1],"-h")))
        {
            usage(argv);
            exit(0);
        }
    }

    if (argc > 1)
    {
        printf("%s: Bad command line argument(s)\n", argv[0]);
        printf("Try `%s --help' for more information.\n", argv[0]);
        exit(1);
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

	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24);

    // We Want A Hardware Surface
    Vflags = SDL_HWSURFACE|SDL_OPENGL;//|SDL_DOUBLEBUF;

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
