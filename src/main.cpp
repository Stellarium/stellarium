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

#if defined( MACOSX )
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

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

using namespace std;

stellariumParams global;
Hip_Star_mgr * HipVouteCeleste;       // Class to manage the Hipparcos catalog
Constellation_mgr * ConstellCeleste;  // Constellation boundary and name
Nebula_mgr * messiers;                // Class to manage the messier objects
Planet_mgr * SolarSystem;             // Class to manage the planets
s_texture * texIds[200];              // Common Textures

/*ShootingStar * TheShooting = NULL;*/

static int timeAtmosphere=0;

void init();

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
    printf("Copyright (C) 2002 Fabien Chereau chereau@free.fr\n\n");
    printf("Please send bug report & comments to stellarium@free.fr\n");
    printf("and check last version on http://stellarium.free.fr\n\n");
};

// ************************  Main display loop  ************************
void glutDisplay(void)
{  
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(global.Fov, (double)global.X_Resolution / 
		   global.Y_Resolution, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    
    // Update the position of observation and time etc...
    Update_time(*SolarSystem);
    Update_variables();

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

    glutSwapBuffers();               // Swap the 2 buffers
}

// ************************  On resize  *******************************
void glutResize(int w, int h)
{   
    if (!h) return;
    global.X_Resolution = w;
    global.Y_Resolution = h;
    clearUi();
    initUi();
    glViewport(0, 0, global.X_Resolution, global.Y_Resolution);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(global.Fov, (double) global.X_Resolution / 
		   global.Y_Resolution, 1, 10000);   // Update the ratio
    glutPostRedisplay();
    glMatrixMode(GL_MODELVIEW);
}

// *******************  Handle the flags keys  **************************
void processNormalKeys(unsigned char key, int x, int y) 
{
    HandleNormalKey(key,GUI_DOWN);
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

// ********************  Handle the special keys  **********************
void pressKey(int key, int, int) 
{   
    // Direction and zoom deplacements
    switch (key) 
    {
    case GLUT_KEY_LEFT :
	global.deltaAz = -1;
	global.FlagTraking=false; 
	break;
    case GLUT_KEY_RIGHT : 
	global.deltaAz =  1;
	global.FlagTraking=false;
	break;
    case GLUT_KEY_UP :
	if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
	{
		global.deltaFov= -1;
	}
	else
	{	global.deltaAlt =  1;
		global.FlagTraking=false;
	}
	break;
    case GLUT_KEY_DOWN :
	if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
	{
		global.deltaFov= 1;
	}
	else
	{
		global.deltaAlt = -1;
		global.FlagTraking=false;
	}
	break;
    case GLUT_KEY_PAGE_UP :
	global.deltaFov= -1;
	break;
    case GLUT_KEY_PAGE_DOWN :
	global.deltaFov=  1;
	break;
    case GLUT_KEY_F1 : 
	glutFullScreen();
	break;
    case GLUT_KEY_F2 :
	glutReshapeWindow(640, 480);
	break;
    }
}

// *******************  Stop mooving and zooming  **********************
void releaseKey(int key, int, int)
{   
    // When a deplacement key is released stop mooving
    switch (key)
    {
    case GLUT_KEY_LEFT :
    case GLUT_KEY_RIGHT : 
	global.deltaAz = 0;
	break;
    case GLUT_KEY_UP : 
    case GLUT_KEY_DOWN :
    if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
	{
		global.deltaFov = 0;
	}
	else
	{
		global.deltaAlt = 0;
	}
	break;
    case GLUT_KEY_PAGE_UP :
    case GLUT_KEY_PAGE_DOWN :
	global.deltaFov = 0;
	break;
    }
}

// ***************  Handle the mouse deplacement  *********************
void MoveMouse(int x, int y)
{
    HandleMove(x,y);
}

// *******************  Handle the mouse clic  *************************
void Mouse(int button, int state, int x, int y)
{
    HandleClic(x,y,state,button);
}


// ****************  Initialisation of glut and openGL  ****************
void init() 
{   
    glutKeyboardFunc(processNormalKeys);
    glutSpecialFunc(pressKey);
    glutSpecialUpFunc(releaseKey);
    glutDisplayFunc(glutDisplay);
    glutIdleFunc(glutDisplay);
    glutReshapeFunc(glutResize);
    glutMouseFunc(Mouse);
    glutMotionFunc(MoveMouse);
    glutPassiveMotionFunc(MoveMouse);
}

// ***************************  Main  **********************************
int main (int argc, char **argv)
{   
    setDirectories();
    
    drawIntro();                     // Print the console logo
    
    char tempName[255];
    char tempName2[255];
    char tempName3[255];
    
    printf("Loading configuration file...\n");
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"config.txt");
    strcpy(tempName2,global.ConfigDir);
    strcat(tempName2,"location.txt");

    loadConfig(tempName,tempName2);  // Load the params from config.txt
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    if (global.Fullscreen)           // FullScreen Mode
    {
	char str[20];                // Init screen size
        sprintf(str,"%dx%d:%d",global.X_Resolution,
		global.Y_Resolution,global.bppMode);
        glutGameModeString(str);     // define resolution, color depth
        if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE))   // enter full screen
        {
	    	glutEnterGameMode();
        }
        else                         // Error
        {   
	    	printf("\nWARNING : True fullscreen is unsuported.\n Try to run in Windowed mode...\n");
	    	global.Fullscreen = 0;
        	dumpConfig();
        }
    }
    if (!global.Fullscreen)          // Windowed mode
    {  
		glutCreateWindow(APP_NAME);
        glutFullScreen();            // Windowed fullscreen mode
    }

    glutIgnoreKeyRepeat(1);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glDisable(GL_DEPTH_TEST);
    // init the blending function parameters
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    init();                          // Set the callbacks
    
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
    global.XYZVision.Set(0,1,0);
    
    glutMainLoop();                  // Start drawing
    return 0;
}
