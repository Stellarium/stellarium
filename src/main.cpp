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
#include "planet_mgr.h"
#include "nebula.h"
#include "nebula_mgr.h"
#include "s_texture.h"
#include "stellarium_ui.h"
#include "navigation.h"
#include "s_gui.h"
#include "hip_star_mgr.h"
#include "shooting.h"
#include "stelconfig.h"

#include <SDL.h>

using namespace std;

stellariumParams global;
Hip_Star_mgr * HipVouteCeleste;       // Class to manage the Hipparcos catalog
Constellation_mgr * ConstellCeleste;  // Constellation boundary and name
Nebula_mgr * messiers;                // Class to manage the messier objects
Planet_mgr * SolarSystem;             // Class to manage the planets
s_texture * texIds[200];              // Common Textures

/*ShootingStar * TheShooting = NULL;*/

// Globals
bool isProgramLooping;														// We're Using This One To Know If The Program Must Go On In The Main Loop
S_AppStatus AppStatus;														// The Struct That Holds The Runtime Data Of The Application


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
void Draw(void)
{  
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(global.Fov, (double)global.X_Resolution / 
		   global.Y_Resolution, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    

    // Execute all the drawing function in the correct order from the more 
    // far to nearest objects

    // ---- Equatorial Coordinates
    Switch_to_equatorial();          // Switch in Equatorial coordinates
    DrawMilkyWay();                  // Draw the milky way --> init the buffers

    if (global.FlagNebula && (!global.FlagAtmosphere || 
			      global.SkyBrightness<0.1)) 
	messiers->Draw();            // Draw the Messiers Objects
    if (global.FlagConstellationDrawing)
        ConstellCeleste->Draw();     // Draw all the constellations
    if ((!global.FlagAtmosphere && global.FlagStars) || 
        (global.SkyBrightness<0.2 && global.FlagStars)) 
    {
	HipVouteCeleste->Draw();    // Draw the stars  
    }

/*
	if (!TheShooting) TheShooting = new ShootingStar();
	if (TheShooting->IsDead())
	{
		delete TheShooting;
		TheShooting = new ShootingStar();
	}
	TheShooting->Draw();
*/
    if (global.FlagPlanets || global.FlagPlanetsHintDrawing) 
	SolarSystem->Draw();         // Draw the planets
    if (global.FlagAtmosphere && global.SkyBrightness>0) 
	DrawAtmosphere2();

    SolarSystem->DrawMoonDaylight();

    if (global.FlagEquatorialGrid)
    { 
	DrawMeridiens();             // Draw the meridian lines
        DrawParallels();             // Draw the parallel lines
    }
    if (global.FlagEquator) 
	DrawEquator();               // Draw the celestial equator line
    if (global.FlagEcliptic) 
	DrawEcliptic();              // Draw the ecliptic line
    if (global.FlagConstellationName)
        ConstellCeleste->DrawName(); // Draw the constellations's names
    if (global.FlagSelect)           // Draw the star pointer
        DrawPointer(global.SelectedObject.XYZ,
		    global.SelectedObject.Size,
		    global.SelectedObject.RGB,
		    global.SelectedObject.type);
    
    // ---- AltAzimutal Coordinates
    Switch_to_altazimutal();         // Switch in AltAzimutal coordinates
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
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(global.Fov, (double) global.X_Resolution / 
		   global.Y_Resolution, 1, 10000);   // Update the ratio
    glMatrixMode(GL_MODELVIEW);
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

void TerminateApplication(void)												// Terminate The Application
{
	static SDL_Event Q;														// We're Sending A SDL_QUIT Event
	Q.type = SDL_QUIT;														// To The SDL Event Queue
	if(SDL_PushEvent(&Q) == -1)												// Try Send The Event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );		// And Eventually Report Errors
		exit(1);															// And Exit
	}
	return;																	// We're Always Making Our Funtions Return
}

// *******************  Handle time  **************************
void Update(Uint32 Milliseconds)
{
    // Update the position of observation and time etc...
    Update_time(Milliseconds, *SolarSystem);
    Update_variables();
	return;
}

// ********************  Handle keys  **********************
void pressKey(Uint8 *keys) 
{   	
    // Direction and zoom deplacements
    if(keys[SDLK_LEFT])
	{
		global.deltaAz = -1;
		global.FlagTraking=false; 
	}
    if(keys[SDLK_RIGHT])
	{
		global.deltaAz =  1;
		global.FlagTraking=false;
	}
    if(keys[SDLK_UP])
	{
		if (SDL_GetModState() & KMOD_CTRL)
		{
			global.deltaFov= -1;
		}
		else
		{	global.deltaAlt =  1;
			global.FlagTraking=false;
		}
	}
    if(keys[SDLK_DOWN])
	{
		if (SDL_GetModState() & KMOD_CTRL)
		{
			global.deltaFov= 1;
		}
		else
		{
			global.deltaAlt = -1;
			global.FlagTraking=false;
		}
	}
    if(keys[SDLK_PAGEUP]) global.deltaFov= -1;
    if(keys[SDLK_PAGEDOWN]) global.deltaFov=  1;

}

// *******************  Stop mooving and zooming  **********************
void releaseKey(SDLKey key)
{   
    // When a deplacement key is released stop mooving
    if (key==SDLK_LEFT || key==SDLK_RIGHT) global.deltaAz = 0;
    if (key==SDLK_UP || key==SDLK_DOWN)
    {
		if (SDL_GetModState() & KMOD_CTRL)
		{
			global.deltaFov = 0;
		}
		else
		{
			global.deltaAlt = 0;
		}
	}
    if (key==SDLK_PAGEUP || key==SDLK_PAGEDOWN)	global.deltaFov = 0;
}

bool InitTimers(Uint32 *C)   // This Is Used To Init All The Timers In Our Application
{
	*C = SDL_GetTicks(); // Hold The Value Of SDL_GetTicks At The Program Init
	return true;	     // Return TRUE (Initialization Successful)
}

bool CreateWindowGL (SDL_Surface *S, int W, int H, int B, Uint32 F)   // This Code Creates Our OpenGL Window
{
	if(!(S = SDL_SetVideoMode(W, H, B, F)))	  // We're Using SDL_SetVideoMode To Create The Window
	{
		return false;// If It Fails, We're Returning False
	}
	ResizeGL(global.X_Resolution, global.Y_Resolution);   // We're Calling Reshape As The Window Is Created
	return true;	     // Return TRUE (Initialization Successful)
}


bool Initialize(void)	     // Any Application & User Initialization Code Goes Here
{
    AppStatus.Visible	    = true;	 // At The Beginning, Our App Is Visible
    AppStatus.MouseFocus    = true;	 // And Have Both Mouse
    AppStatus.KeyboardFocus = true;	 // And Input Focus
  
    SDL_EnableKeyRepeat(0, 0); // Disable key repeat
    
    HipVouteCeleste = new Hip_Star_mgr();
    if (!HipVouteCeleste) exit(1);

    ConstellCeleste = new Constellation_mgr();
    if (!ConstellCeleste) exit(1);

    messiers = new Nebula_mgr();
    if (!messiers) exit(1);

    SolarSystem = new Planet_mgr();
    if (!SolarSystem) exit(1);

    loadCommonTextures();            // Load the common used textures
    SolarSystem->loadTextures();
    
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
    ConstellCeleste->Load(tempName,HipVouteCeleste);     // Load constellations      
    SolarSystem->Compute(global.JDay,global.ThePlace);// Compute planet data
    InitMeriParal();                 // Precalculation for the grids drawing
    InitAtmosphere();
    
    strcpy(tempName,global.DataDir);
    strcat(tempName,"messier.fab");
    messiers->Read(tempName);        // read the messiers object data
    initUi();                        // initialisation of the User Interface
    global.XYZVision.Set(0,1,-0.4);
    return true;		     // Return TRUE (Initialization Successful)
}

// ***************************  Deinitialize  **********************************
void Deinitialize(void)		     // Any User Deinitialization Goes Here
{
	return;		       	     // We Have Nothing To Deinit Now
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
	
    // We Want A Hardware Surface
    Vflags = SDL_HWSURFACE|SDL_OPENGL;

    if(SDL_Init(SDL_INIT_VIDEO)<0)  // Init The SDL Library, The VIDEO Subsystem
    {
         printf("Unable to open SDL: %s\n", SDL_GetError() );
         exit(1);
    }

    // Make Sure That SDL_Quit Will Be Called In Case of exit()
    atexit(SDL_Quit);
    
    // If So, We Always Need The Fullscreen Video Init Flag
    if (global.Fullscreen) Vflags|=SDL_FULLSCREEN; 
    
    // Create The Window
    if(!CreateWindowGL(Screen, global.X_Resolution, global.Y_Resolution, global.bppMode, Vflags))
	{
		printf("Unable to open screen surface: %s\n", SDL_GetError() );		
		exit(1);
	}

	SDL_WM_SetCaption(APP_NAME, APP_NAME);	// Set The Window Caption
    strcpy(tempName,global.DataDir);
    strcat(tempName,"icon.bmp");
	SDL_WM_SetIcon(SDL_LoadBMP(tempName), NULL);

	if(!InitTimers(&LastCount))			// We Call The Timers Init Function
	{
		printf("Can't init the timers: %s\n", SDL_GetError() );
		exit(1);
	}

	if(!InitGL(Screen))					// CallThe OpenGL Init Function
	{
		printf("Can't init GL: %s\n", SDL_GetError() );
		exit(1);
	}

	if(!Initialize())					// Init The Application
	{
		printf("App init failed: %s\n", SDL_GetError() );
		exit(1);
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
				LastCount = TickCount;		// Save The Present Tick Probing
				Draw();						// Do The Drawings!
				SDL_GL_SwapBuffers();		// And Swap The Buffers
			}
		}
	}

	Deinitialize();
	exit(0);

	return 0;
	
}
