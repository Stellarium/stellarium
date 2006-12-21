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
 
#include "StelApp.hpp"

#include "StelCore.hpp"
#include "ViewportDistorter.hpp"
#include "StelUtils.hpp"
#include "callbacks.hpp"
#include "stel_command_interface.h"
#include "stel_ui.h"
#include "StelTextureMgr.hpp"

//#include "LoadingBar.hpp"
//#include "CeguiGui.hpp"

// Initialize static variables
StelApp* StelApp::singleton = NULL;

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT) :
		frame(0), timefr(0), timeBase(0), fps(0), maxfps(10000.f),
	 draw_mode(StelApp::DM_NORMAL)
{
	// Can't create 2 StelApp instances
	assert(!singleton);
	singleton = this;

	dotStellariumDir = CDIR;
	localeDir = LDIR;
	dataDir = DATA_ROOT+"/data";
	rootDir = DATA_ROOT + "/";
	
	textureMgr = new StelTextureMgr(rootDir + "textures/");
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
	return getDotStellariumDir() + "config.ini";
}

/*************************************************************************
 Get the full path to a file.
*************************************************************************/
string StelApp::getFilePath(const string& fileName) const
{
	// Absolute path if starts by '/'
	if (fileName!="" && fileName[0]=='/')
		return fileName;
	
	if (StelUtils::fileExists(dotStellariumDir + fileName))
		return dotStellariumDir + fileName;
	
	if (StelUtils::fileExists(rootDir + fileName))
		return rootDir + fileName;
	
	cerr << "Can't find file " << fileName << endl;
	
	return "";
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
		
	return getFilePath("data/" + dataFileName);
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
	distorter = ViewportDistorter::create(type,screenW,screenH, core->getProjection());
	InitParser conf;
	conf.load(dotStellariumDir + "config.ini");
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
//	StelUtils::downloadFile("http://chereau.free.fr/", "/home/fchereau/Desktop/tmp.html");
	Translator::initSystemLanguage();

	// Load language codes
	Translator::initIso639_1LanguageCodes(getDataFilePath("iso639-1.utf8"));
	
	// Initialize video device and other sdl parameters
	InitParser conf;
	conf.load(dotStellariumDir + "config.ini");

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
			conf.load(dotStellariumDir + "config.ini");  // Read new config!
		}
		else
		{
			cout << "Attempting to use an existing older config file." << endl;
		}
	}

	// don't mess with SDL init if already initialized earlier
	screenW = conf.get_int("video:screen_w");
	screenH = conf.get_int("video:screen_h");
	initSDL(screenW, screenH, conf.get_int("video:bbp_mode"), conf.get_boolean("video:fullscreen"), getDataFilePath("icon.bmp"));

	// Initialize after creation of openGL context
	textureMgr->init();

	// Clear screen, this fixes a strange artifact at loading time in the upper corner.
	glClear(GL_COLOR_BUFFER_BIT);

	maxfps 				= conf.get_double ("video","maximum_fps",10000);
	minfps 				= conf.get_double ("video","minimum_fps",10000);

	scripts->set_allow_ui( conf.get_boolean("gui","flag_script_allow_ui",0) );

	localeMgr->init(conf);
	skyCultureMgr->init(conf);
	
	core->init(conf);
//ugly fix by johannes: call skyCultureMgr->init twice so that
// star names are loaded again
	skyCultureMgr->init(conf);

//	LoadingBar dummy(core->getProjection(), 12, "logo24bits.png", 0, 0,StelUtils::stringToWstring(VERSION), 45, 320, 121);
//	// New CEGUI widgets
//	CeguiGui* ceguiGui = new CeguiGui();
//	ceguiGui->init(conf, dummy);
//	getModuleMgr().registerModule(ceguiGui);

	// TODO: Need way to update settings from config without reinitializing whole gui
	ui->init(conf);

	ui->init_tui();  // don't reinit tui since probably called from there

	// Initialisation of the color scheme
	draw_mode = draw_mode=DM_NIGHT;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();

	if (distorter == 0)
	{
		setViewPortDistorterType(conf.get_str("video","distorter","none"));
	}

	// play startup script, if available
	if(scripts) scripts->play_startup_script();
}

void StelApp::update(int delta_time)
{
	textureMgr->update();
	
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

    // clear areas not redrawn by main viewport (i.e. fisheye square viewport)
	// (because ui can draw outside the main viewport)
    glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	set2DfullscreenProjection();
	glBegin(GL_QUADS);
    {
        glVertex2f(0,screenH);
        glVertex2f(screenW,screenH);
        glVertex2f(screenW,0);
		glVertex2f(0,0);
	}
	glEnd();
	restoreFrom2DfullscreenProjection();

	// Render all the main objects of stellarium
	double squaredDistance = core->draw(delta_time);

	// Draw the Graphical ui and the Text ui
	ui->draw();

	distorter->distort();

	return squaredDistance;
}

// Handle mouse clics
int StelApp::handleClick(int x, int y, Uint8 button, Uint8 state)
{
	distorter->distortXY(x,y);
	if (ui->handle_clic(x, y, button, state)) return 1;
	
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		if ((*iter)->handleMouseClicks(x, y, button, state)==true)
			return 1;
	}
	
	return 0;
}

// Handle mouse move
int StelApp::handleMove(int x, int y)
{
	distorter->distortXY(x,y);
	
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		if ((*iter)->handleMouseMoves(x, y)==true)
			return 1;
	}
	
	return ui->handle_move(x, y);
}


// Handle key press and release
int StelApp::handleKeys(SDLKey key, SDLMod mod, Uint16 unicode, Uint8 state)
{
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		if ((*iter)->handleKeys(key, mod, unicode, state)==true)
			return 1;
	}

	if (ui->handle_keys_tui(unicode, state)) return 1;
	if (ui->handle_keys(key, mod, unicode, state)) return 1;

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
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		(*iter)->updateI18n();
	}
}

// Update and reload sky culture informations everywhere in the program
void StelApp::updateSkyCulture()
{
	core->updateSkyCulture();
}
