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
 
#include <cstdlib>
#include "StelApp.hpp"

#include "StelCore.hpp"
#include "ViewportDistorter.hpp"
#include "StelUtils.hpp"
#include "stel_command_interface.h"
#include "stel_ui.h"
#include "script_mgr.h"
#include "StelTextureMgr.hpp"
#include "LoadingBar.hpp"
#include "StelObjectMgr.hpp"
#include "image_mgr.h"
#include "TelescopeMgr.hpp"
#include "ConstellationMgr.hpp"
#include "NebulaMgr.hpp"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "MeteorMgr.hpp"
#include "StarMgr.hpp"
#include "SolarSystem.hpp"
#include "InitParser.hpp"
#include "Projector.hpp"

#include "StelModuleMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "MovementMgr.hpp"
#include "StelFileMgr.hpp"

// Initialize static variables
StelApp* StelApp::singleton = NULL;

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(int argc, char** argv) :
	maxfps(10000.f), core(NULL), fps(0), frame(0), timefr(0), 
	timeBase(0), draw_mode(StelApp::DM_NORMAL), initialized(false)
{
	distorter=0;
	skyCultureMgr=0;
	localeMgr=0;
	fontManager=0;
	stelObjectMgr=0;
	textureMgr=0;
	moduleMgr=0;

	// Can't create 2 StelApp instances
	assert(!singleton);
	singleton = this;
	
	// C++-ize argv for use with StelUtils::argsHaveOption... functions
	for(int i=0; i<argc; i++) { argList.push_back(argv[i]); }
		
	stelFileMgr = new StelFileMgr();
	
	// Check if the user directory exists, and create it if necessary
	stelFileMgr->checkUserDir();
	
	// Check some command-line options
	try
	{
		setConfigFile(StelUtils::argsHaveOptionWithArg<string>(argList, "-c", "--config-file", "config.ini"));
	}
	catch(exception& e)
	{
		cerr << "ERROR: while looking for --config-file option: " << e.what() << ". Using \"config.ini\"" << endl;
		setConfigFile("config.ini");		
	}
	cout << "config file is: " << configFile << endl;
	
	if (!stelFileMgr->exists(configFile))
	{
		cerr << "config file \"" << configFile << "\" does not exist - copying the default file." << endl;
		copyDefaultConfigFile();
	}
        
	textureMgr = new StelTextureMgr();
	localeMgr = new StelLocaleMgr();
	
	string fontMapFile("");
	try 
	{
		fontMapFile = stelFileMgr->findFile("data/fontmap.dat");
	}
	catch(exception& e)
	{
		cerr << "ERROR when locating font map file: " << e.what() << endl;
	}
	fontManager = new StelFontMgr(fontMapFile);
	
	skyCultureMgr = new StelSkyCultureMgr();
	moduleMgr = new StelModuleMgr();
	
	core = new StelCore();
	time_multiplier = 1;
	distorter = 0;
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	delete core; core=NULL;
	if (distorter) {delete distorter; distorter=NULL;}
	delete skyCultureMgr; skyCultureMgr=NULL;
	delete localeMgr; localeMgr=NULL;
	delete fontManager; fontManager=NULL;
	delete stelObjectMgr; stelObjectMgr=NULL;
	delete stelFileMgr; stelFileMgr=NULL;
	
	// Delete all the modules
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		delete *iter;
		*iter = NULL;
	}

	delete textureMgr; textureMgr=NULL;
	delete moduleMgr; moduleMgr=NULL;
}

/*************************************************************************
 Return the full name of stellarium, i.e. "stellarium 0.9.0"
*************************************************************************/
std::string StelApp::getApplicationName()
{
	return string("Stellarium")+" "+PACKAGE_VERSION;
}

void StelApp::setViewPortDistorterType(const string &type)
{
	if (type != getViewPortDistorterType()) setResizable(type == "none");
	if (distorter)
	{
		delete distorter;
		distorter = 0;
	}
	InitParser conf;
	conf.load(getConfigFilePath());
	distorter = ViewportDistorter::create(type,getScreenW(),getScreenH(),
	                                      core->getProjection(),conf);
}

string StelApp::getViewPortDistorterType() const
{
	if (distorter) return distorter->getType();
	return "none";
}


void StelApp::init()
{
	Translator::initSystemLanguage();

	// Load language codes
	try
	{
		Translator::initIso639_1LanguageCodes(stelFileMgr->findFile("data/iso639-1.utf8"));
	}
	catch(exception& e)
	{
		cerr << "ERROR while loading translations: " << e.what() << endl;
	}
	
	// Initialize video device and other sdl parameters
	InitParser conf;
	string confPath;
	try
	{
		confPath = getConfigFilePath();
	}
	catch(exception& e)
	{
		cerr << "ERROR finding config file: " << e.what() << endl;
	}
	conf.load(confPath);

	// Main section
	string version = conf.get_str("main:version");
	if (version!=string(PACKAGE_VERSION))
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
			cout << "The current config file is from a version too old for parameters to be imported (" 
					<< (version.empty() ? "<0.6.0" : version.c_str())
					<< ")." << endl 
					<< "It will be replaced by the default config file." << endl;

            copyDefaultConfigFile();
            conf.load(configFile);  // Read new config
		}
		else
		{
			cout << "Attempting to use an existing older config file." << endl;
		}
	}

	// Create openGL context first
	string iconPath;
	try
	{
		iconPath = stelFileMgr->findFile("data/icon.bmp");
	}
	catch(exception& e)
	{
		cerr << "ERROR when trying to locate icon file: " << e.what() << endl;
	}
	initOpenGL(conf.get_int("video:screen_w"), conf.get_int("video:screen_h"), conf.get_int("video:bbp_mode"), conf.get_boolean("video:fullscreen"), iconPath);

	// Initialize AFTER creation of openGL context
	textureMgr->init(conf);

	// Clear screen, this fixes a strange artifact at loading time in the upper corner.
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	swapGLBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	
	maxfps = conf.get_double ("video","maximum_fps",10000);
	minfps = conf.get_double ("video","minimum_fps",10000);

	core->initProj(conf);

	LoadingBar lb(core->getProjection(), 12., "logo24bits.png",
	              getScreenW(), getScreenH(),
	              StelUtils::stringToWstring(PACKAGE_VERSION), 45, 320, 121);

	// Stel Object Data Base manager
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init(conf, lb);
	
	localeMgr->init(conf);
	skyCultureMgr->init(conf);
	StelCommandInterface* commander = new StelCommandInterface(core, this);
	getModuleMgr().registerModule(commander);
	
	ScriptMgr* scripts = new ScriptMgr(commander);
	scripts->init(conf, lb);
	getModuleMgr().registerModule(scripts);
	
	ImageMgr* script_images = new ImageMgr();
	script_images->init(conf, lb);
	getModuleMgr().registerModule(script_images);
	// Init the solar system first
	SolarSystem* ssystem = new SolarSystem();
	ssystem->init(conf, lb);
	getModuleMgr().registerModule(ssystem);
	
	// Load hipparcos stars & names
	StarMgr* hip_stars = new StarMgr();
	hip_stars->init(conf, lb);
	getModuleMgr().registerModule(hip_stars);	
	
	core->init(conf, lb);

	// Init nebulas
	NebulaMgr* nebulas = new NebulaMgr();
	nebulas->init(conf, lb);
	getModuleMgr().registerModule(nebulas);
	
	// Init milky way
	MilkyWay* milky_way = new MilkyWay();
	milky_way->init(conf, lb);
	getModuleMgr().registerModule(milky_way);
	
	// Telescope manager
	TelescopeMgr* telescope_mgr = new TelescopeMgr();
	telescope_mgr->init(conf, lb);
	getModuleMgr().registerModule(telescope_mgr);
	
	// Constellations
	ConstellationMgr* asterisms = new ConstellationMgr(hip_stars);
	asterisms->init(conf, lb);
	getModuleMgr().registerModule(asterisms);
	
	// Landscape, atmosphere & cardinal points section
	LandscapeMgr* landscape = new LandscapeMgr();
	landscape->init(conf, lb);
	getModuleMgr().registerModule(landscape);

	GridLinesMgr* gridLines = new GridLinesMgr();
	gridLines->init(conf, lb);
	getModuleMgr().registerModule(gridLines);
	
	// Meteors
	MeteorMgr* meteors = new MeteorMgr(10, 60);
	meteors->init(conf, lb);
	getModuleMgr().registerModule(meteors);

//ugly fix by johannes: call skyCultureMgr->init twice so that
// star names are loaded again
	skyCultureMgr->init(conf);
	
	StelUI* ui = new StelUI(core, this);
	ui->init(conf, lb);
	getModuleMgr().registerModule(ui);
	
	// Initialisation of the color scheme
	draw_mode = draw_mode=DM_NIGHT;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();

	setViewPortDistorterType(conf.get_str("video","distorter","none"));

	// Load dynamically all the modules found in the modules/ directories
	// which are configured to be loaded at startup
	std::vector<StelModuleMgr::ExternalStelModuleDescriptor> mDesc = moduleMgr->getExternalModuleList();
	for (std::vector<StelModuleMgr::ExternalStelModuleDescriptor>::const_iterator i=mDesc.begin();i!=mDesc.end();++i)
	{
		if (i->loadAtStartup==false)
			continue;
		StelModule* m = moduleMgr->loadExternalPlugin(i->key);
		if (m!=NULL)
		{
			m->init(conf, lb);
			moduleMgr->registerModule(m);
		}
	}
	
	// Generate dependency Lists for all modules
	moduleMgr->generateCallingLists();
	
	scripts->set_removable_media_path(conf.get_str("files","removable_media_path", ""));
	
	// play startup script, if available
	scripts->play_startup_script();
	
	initialized = true;
}

void StelApp::update(int delta_time)
{
     if (!initialized)
        return;
        
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

	core->update(delta_time);
	
	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("update");
	//cerr << "-------" << endl;
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		//cerr << (*i)->getModuleID() << endl;
		(*i)->update((double)delta_time/1000);

	}
	
	stelObjectMgr->update((double)delta_time/1000);
}

//! Main drawing function called at each frame
double StelApp::draw(int delta_time)
{
     if (!initialized)
        return 0.;
        
    // clear areas not redrawn by main viewport (i.e. fisheye square viewport)
	// (because ui can draw outside the main viewport)
	glClear(GL_COLOR_BUFFER_BIT);

	distorter->prepare();

	core->preDraw();

	// Render all the main objects of stellarium
	double squaredDistance = 0.;
	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("draw");
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		double d = (*i)->draw(core->getProjection(), core->getNavigation(), core->getToneReproducer());
		if (d>squaredDistance)
			squaredDistance = d;
	}

	stelObjectMgr->draw(core->getProjection(), core->getNavigation(), core->getToneReproducer());

	core->postDraw();
	distorter->distort();

	// Draw the Graphical ui
	StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
	ui->drawGui();

	return squaredDistance;
}

/*************************************************************************
 Call this when the size of the GL window has changed
*************************************************************************/
void StelApp::glWindowHasBeenResized(int w, int h)
{
	// no resizing allowed in distortion mode
	if (distorter && distorter->getType() == "none") {
		if (core && core->getProjection())
			core->getProjection()->windowHasBeenResized(getScreenW(),
			                                            getScreenH());
		// Send the event to every StelModule
		for (StelModuleMgr::Iterator iter=moduleMgr->begin();
		     iter!=moduleMgr->end();++iter)
		{
			(*iter)->glWindowHasBeenResized(w, h);
		}
	}
}

// Handle mouse clics
int StelApp::handleClick(int x, int y, Uint8 button, Uint8 state, StelMod mod)
{
	const int ui_x = x;
	const int ui_y = y;
	y = getScreenH() - 1 - y;
	distorter->distortXY(x,y);

	StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
	if (ui->handleClick(ui_x, ui_y, button, state, mod)) return 1;

	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("handleMouseClicks");
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		if ((*i)->handleMouseClicks(x, y, button, state, mod)==true)
			return 1;
	}
	
	// Manage the event for the main window
	{
		StelCommandInterface* commander = (StelCommandInterface*)getModuleMgr().getModule("command_interface");
		
		// Deselect the selected object
		if (button==Stel_BUTTON_RIGHT && state==Stel_MOUSEBUTTONUP)
		{
			commander->execute_command("select");
			return 1;
		}
		MovementMgr* mvmgr = (MovementMgr*)getModuleMgr().getModule("movements");
		if (button==Stel_BUTTON_LEFT && state==Stel_MOUSEBUTTONUP && !mvmgr->getHasDragged())
		{
#ifdef MACOSX
			// CTRL + left clic = right clic for 1 button mouse
			if (mod & StelMod_CTRL)
			{
				commander->execute_command("select");
				return 1;
			}

			// Try to select object at that position
			getStelObjectMgr().findAndSelect(core, x, y, mod & StelMod_META);
#else
			getStelObjectMgr().findAndSelect(core, x, y, mod & StelMod_CTRL);
#endif
			// If an object was selected update informations
			if (getStelObjectMgr().getWasSelected())
			{
				((MovementMgr*)moduleMgr->getModule("movements"))->setFlagTracking(false);
				StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
				ui->updateInfoSelectString();
			}
		}
	}
	
	return 0;
}

// Handle mouse move
int StelApp::handleMove(int x, int y, StelMod mod)
{
	const int ui_x = x;
	const int ui_y = y;
	y = getScreenH() - 1 - y;
	distorter->distortXY(x,y);
	
	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("handleMouseMoves");
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		if ((*i)->handleMouseMoves(x, y, mod)==true)
			return 1;
	}
	StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
	return ui->handleMouseMoves(ui_x,ui_y, mod);
}

// Handle key press and release
int StelApp::handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state)
{
	StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
	// Standard keys should not be able to be hijacked by modules - Rob
	// (this could be debated)
	if (ui->handle_keys_tui(key, state)) return 1;

	if (ui->handle_keysGUI(key, mod, unicode, state)) return 1;

	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("handleKeys");
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		if ((*i)->handleKeys(key, mod, unicode, state)==true)
			return 1;
	}

	// Non widget key handling
	return ui->handle_keys(key, mod, unicode, state);
}


//! Set the drawing mode in 2D for drawing in the full screen
void StelApp::set2DfullscreenProjection() const
{
	glViewport(0,0,getScreenW(),getScreenH());
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
	glPushMatrix();						// store previous matrix
	glLoadIdentity();
	glOrtho(0,getScreenW(),0,getScreenH(),-1,1);	// set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode
	glPushMatrix();
	glLoadIdentity();
	glScalef(1, -1, 1);					// invert the y axis, down is positive
	glTranslatef(0, -getScreenH(), 0);		// move the origin from the bottom left corner to the upper left corner
}

void StelApp::setConfigFile(const string& configName)
{
	try
	{
		configFile = stelFileMgr->findFile(configName, StelFileMgr::FLAGS(StelFileMgr::WRITABLE|StelFileMgr::FILE));
		return;
	}
	catch(exception& e)
	{
		//cerr << "DEBUG StelApp::setConfigFile could not locate writable config file " << configName << endl;
	}
	
	try
	{
		configFile = stelFileMgr->findFile(configName, StelFileMgr::FILE);	
		return;
	}
	catch(exception& e)
	{
		//cerr << "DEBUG StelApp::setConfigFile could not find read only config file " << configName << endl;
	}		
	
	try
	{
		configFile = stelFileMgr->findFile(configName, StelFileMgr::NEW);
		//cerr << "DEBUG StelApp::setConfigFile found NEW file path: " << configFile << endl;
		return;
	}
	catch(exception& e)
	{
		cerr << "ERROR StelApp::setConfigFile could not find or create configuration file " << configName << endl;
		exit(1);
	}
}

void StelApp::copyDefaultConfigFile()
{
	string defaultConfigFilePath;
	try
	{
		defaultConfigFilePath = stelFileMgr->findFile("data/default_config.ini");
	}
	catch(exception& e)
	{
		cerr << "ERROR (copyDefaultConfigFile): failed to locate data/default_config.ini.  Please check your installation." << endl;
		exit(1);
	}
	
	StelFileMgr::copy(defaultConfigFilePath, configFile);
	if (!stelFileMgr->exists(configFile))
	{
		cerr << "ERROR (copyDefaultConfigFile): failed to copy file " << defaultConfigFilePath << " to " << configFile << ". You could try to copy it by hand." << endl;
		exit(1);
	}
}

//! Restore previous projection mode
void StelApp::restoreFrom2DfullscreenProjection() const
{
	glMatrixMode(GL_PROJECTION);		// Restore previous matrix
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

// Set the colorscheme for all the modules
void StelApp::setColorScheme(const std::string& fileName, const std::string& section)
{
	InitParser conf;
	conf.load(fileName);
	
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		(*iter)->setColorScheme(conf, section);
	}
}

//! Set flag for activating night vision mode
void StelApp::setVisionModeNight()
{
	if (!getVisionModeNight())
	{
		setColorScheme(getConfigFilePath(), "night_color");
	}
	draw_mode=DM_NIGHT;
}

//! Set flag for activating chart vision mode
// ["color" section name used for easier backward compatibility for older configs - Rob]
void StelApp::setVisionModeNormal()
{
	if (!getVisionModeNormal())
	{
		setColorScheme(getConfigFilePath(), "color");
	}
	draw_mode=DM_NORMAL;
}

// Update translations and font everywhere in the program
void StelApp::updateAppLanguage()
{
	// update translations and font in tui
	StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
	if (ui)
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
	LoadingBar lb(core->getProjection(), 12., "logo24bits.png",
     core->getProjection()->getViewportWidth(), core->getProjection()->getViewportHeight(),
      StelUtils::stringToWstring(PACKAGE_VERSION), 45, 320, 121);
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		(*iter)->updateSkyCulture(lb);
	}
}
