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
#include "stel_command_interface.h"
#include "stel_ui.h"
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
StelApp::StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT) :
		maxfps(10000.f), core(NULL), fps(0), frame(0), timefr(0), timeBase(0), ui(NULL),
	 draw_mode(StelApp::DM_NORMAL)
{
	// Can't create 2 StelApp instances
	assert(!singleton);
	singleton = this;

	dotStellariumDir = CDIR;
	localeDir = LDIR;
	dataDir = DATA_ROOT+"/data";
	rootDir = DATA_ROOT + "/";

	// This is necessary so boost will allow paths which 
	// include ".", e.g. .stellarium 
        fs::path::default_name_check(fs::native);
	
	// Prepare the search path to pass to the file manager
	vector<fs::path> searchPaths;
	searchPaths.push_back(CDIR);
	searchPaths.push_back(DATA_ROOT);
        stelFileMgr = new StelFileMgr();
	stelFileMgr->setSearchPaths(searchPaths);
	textureMgr = new StelTextureMgr(rootDir + "textures/");
	localeMgr = new StelLocaleMgr();
	string fontMapFile("");
	try 
	{
		fontMapFile = stelFileMgr->findFile("data/fontmap.dat").string();
	}
	catch(exception& e)
	{
		cerr << "ERROR when locating font map file: " << e.what() << endl;
	}
	fontManager = new StelFontMgr(fontMapFile);
	string skyCulturesDir("");
	try
	{
		skyCulturesDir = stelFileMgr->findFile("data/sky_cultures", StelFileMgr::DIRECTORY).string();
	}
	catch(exception& e)
	{
		cerr << "ERROR when locating sky cultures directory: " << e.what() << endl;
	}
	skyCultureMgr = new StelSkyCultureMgr(skyCulturesDir);
	moduleMgr = new StelModuleMgr();
	
	core = new StelCore();
	ui = new StelUI(core, this);
	commander = new StelCommandInterface(core, this);
	scripts = new ScriptMgr(commander);
	time_multiplier = 1;
	distorter = 0;
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	delete ui;
	delete scripts;
	delete commander;
	delete core;
	if (distorter) delete distorter;
	delete skyCultureMgr;
	delete localeMgr;
	delete fontManager;
	delete stelObjectMgr;
	
	// Delete all the modules
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		delete *iter;
		*iter = NULL;
	}

	delete textureMgr;
	delete moduleMgr;
}

/*************************************************************************
 Get the configuration file path.
*************************************************************************/
string StelApp::getConfigFilePath(void) const
{
	return stelFileMgr->findFile("config.ini", StelFileMgr::WRITABLE).string();
}

/*************************************************************************
 Get the full path to a file.
 Deprecated in favour of using StelFileMgr::findFile
*************************************************************************/
string StelApp::getFilePath(const string& fileName) const
{
	cerr << "WARNING StelApp::getFilePath is deprecated - please use StelFileMgr::findFile instead" << endl;
	if (StelUtils::checkAbsolutePath(fileName))
		return fileName;
	
	if (StelUtils::fileExists(dotStellariumDir + fileName))
		return dotStellariumDir + fileName;
	
	if (StelUtils::fileExists(rootDir + fileName))
		return rootDir + fileName;
	
	cerr << "Can't find file " << fileName << endl;
	
	return "";
}

/*************************************************************************
 Get a vector of paths for a file.
 Deprecated in favour of using StelFileMgr::getSearchPaths
*************************************************************************/
vector<string> StelApp::getFilePathList(const string& fileName) const
{
	cerr << "WARNING StelApp::getFilePathList is deprecated - please use StelFileMgr::getSearchPaths instead" << endl;
	vector<string> result;
	
	if (StelUtils::checkAbsolutePath(fileName)) {
		result.push_back(fileName);
		return result;
	}
	
	if (StelUtils::fileExists(dotStellariumDir + fileName))
		result.push_back(dotStellariumDir + fileName);
	
	if (StelUtils::fileExists(rootDir + fileName))
		result.push_back(rootDir + fileName);
	
	if (result.size() == 0 )
		cerr << "StelApp::getFilePathList: Can't find file " << fileName << endl;
	
	return result;
}

/*************************************************************************
 Get the full path to a data file. This method will try to find the file 
 in all valid data directories until it finds it.
 Deprecated in favour of using StelFileMgr::findFile
*************************************************************************/
string StelApp::getDataFilePath(const string& dataFileName) const
{
	cerr << "WARNING StelApp::getDataFilePath is deprecated - please use StelFileMgr::findFile instead" << endl;
	if (StelUtils::checkAbsolutePath(dataFileName))
		return dataFileName;

	if (StelUtils::fileExists(dataDir + "/" + dataFileName))
		return dataDir + "/" + dataFileName;
		
	return getFilePath("data/" + dataFileName);
}

/*************************************************************************
 Get the full path to a texture file. This method will try to find the 
 file in all valid data 
 Deprecated in favour of using StelFileMgr::findFile
*************************************************************************/
string StelApp::getTextureFilePath(const string& textureFileName) const
{
	cerr << "WARNING StelApp::getTextureFilePathis deprecated - please use StelFileMgr::findFile instead" << endl;
	return rootDir + "textures/" + textureFileName;
}

bool restart_ui = false;

void StelApp::setViewPortDistorterType(const string &type)
{
	if (type != getViewPortDistorterType()) setResizable(type == "none");
	if (distorter)
	{
		delete distorter;
		distorter = 0;
	}
	InitParser conf;
	conf.load(dotStellariumDir + "config.ini");
	distorter = ViewportDistorter::create(type,getScreenW(),getScreenH(),
	                                      core->getProjection(),conf);
}

string StelApp::getViewPortDistorterType(void) const
{
	if (distorter) return distorter->getType();
	return "none";
}


void StelApp::init(void)
{
	Translator::initSystemLanguage();

	// Load language codes
	try
	{
		Translator::initIso639_1LanguageCodes(stelFileMgr->findFile("data/iso639-1.utf8").string());
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
		confPath = stelFileMgr->findFile("config.ini").string();
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
			printf("The current config file is from a version too old for parameters to be imported (%s).\nIt will be replaced by the default config file.\n", version.empty() ? "<0.6.0" : version.c_str());
			system( (string("cp -f ") + dataDir + "default_config.ini " + getConfigFilePath()).c_str() );
			conf.load(dotStellariumDir + "config.ini");  // Read new config!
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
		iconPath = stelFileMgr->findFile("data/icon.bmp").string();
	}
	catch(exception& e)
	{
		cerr << "ERROR when trying to locate icon file: " << e.what() << endl;
	}
	initOpenGL(conf.get_int("video:screen_w"), conf.get_int("video:screen_h"), conf.get_int("video:bbp_mode"), conf.get_boolean("video:fullscreen"), iconPath);

	// Initialize AFTER creation of openGL context
	textureMgr->init();

	// Clear screen, this fixes a strange artifact at loading time in the upper corner.
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	swapGLBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	
	maxfps = conf.get_double ("video","maximum_fps",10000);
	minfps = conf.get_double ("video","minimum_fps",10000);

	scripts->set_allow_ui( conf.get_boolean("gui","flag_script_allow_ui",0) );

	core->initProj(conf);

	LoadingBar lb(core->getProjection(), 12., "logo24bits.png",
	              getScreenW(), getScreenH(),
	              StelUtils::stringToWstring(PACKAGE_VERSION), 45, 320, 121);

	// Stel Object Data Base manager
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init(conf, lb);

	localeMgr->init(conf);
	skyCultureMgr->init(conf);
	
	ImageMgr* script_images = new ImageMgr();
	script_images->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(script_images);
	
	// Init the solar system first
	SolarSystem* ssystem = new SolarSystem();
	ssystem->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(ssystem);
	
	// Load hipparcos stars & names
	StarMgr* hip_stars = new StarMgr();
	hip_stars->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(hip_stars);	
	
	core->init(conf, lb);

	// Init nebulas
	NebulaMgr* nebulas = new NebulaMgr();
	nebulas->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(nebulas);
	
	// Init milky way
	MilkyWay* milky_way = new MilkyWay();
	milky_way->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(milky_way);
	
	// Telescope manager
	TelescopeMgr* telescope_mgr = new TelescopeMgr();
	telescope_mgr->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(telescope_mgr);
	
	// Constellations
	ConstellationMgr* asterisms = new ConstellationMgr(hip_stars);
	asterisms->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(asterisms);
	
	// Landscape, atmosphere & cardinal points section
	LandscapeMgr* landscape = new LandscapeMgr();
	landscape->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(landscape);

	GridLinesMgr* gridLines = new GridLinesMgr();
	gridLines->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(gridLines);
	
	// Meteors
	MeteorMgr* meteors = new MeteorMgr(10, 60);
	meteors->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(meteors);

//ugly fix by johannes: call skyCultureMgr->init twice so that
// star names are loaded again
	skyCultureMgr->init(conf);

	// TODO: Need way to update settings from config without reinitializing whole gui
	ui->init(conf);
	ui->init_tui();  // don't reinit tui since probably called from there

	// Initialisation of the color scheme
	draw_mode = draw_mode=DM_NIGHT;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();

	setViewPortDistorterType(conf.get_str("video","distorter","none"));

	// Load dynamic external modules
	int i=1;
	while (true)
	{
		ostringstream oss;
		oss << "external_modules:module" << i; 
		if (conf.find_entry(oss.str()))
		{
			StelModule* m = moduleMgr->loadExternalModule(conf.get_str(oss.str()));
			if (m!=NULL)
			{
				m->init(conf, lb);
				moduleMgr->registerModule(m);
			}
		}
		else
			break;
		i++;
	}
	
	// Generate dependency Lists for all modules
	moduleMgr->generateCallingLists();
	
	scripts->set_removable_media_path(conf.get_str("files:removable_media_path"));
	
	// play startup script, if available
	scripts->play_startup_script();
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

	core->update(delta_time);

	if(!scripts->is_paused()) ((ImageMgr*)moduleMgr->getModule("image_mgr"))->update(delta_time);
	
	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("update");
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		(*i)->update((double)delta_time/1000);
	}
	
	stelObjectMgr->update((double)delta_time/1000);
}

//! Main drawinf function called at each frame
double StelApp::draw(int delta_time)
{
    // clear areas not redrawn by main viewport (i.e. fisheye square viewport)
	// (because ui can draw outside the main viewport)
	glClear(GL_COLOR_BUFFER_BIT);

	distorter->prepare();

	core->preDraw(delta_time);

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

	// Draw the Text ui into the predistorted image
	ui->drawTui();

	distorter->distort();

	// Draw the Graphical ui
	ui->drawGui();

	return squaredDistance;
}

/*************************************************************************
 Call this when the size of the GL window has changed
*************************************************************************/
void StelApp::glWindowHasBeenResized(int w, int h)
{
	if (core && core->getProjection())
		core->getProjection()->windowHasBeenResized(getScreenW(),getScreenH());
	if (ui)
		ui->resize();
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		(*iter)->glWindowHasBeenResized(w, h);
	}
}

// Handle mouse clics
int StelApp::handleClick(int x, int y, Uint8 button, Uint8 state, StelMod mod)
{
	const int ui_x = x;
	const int ui_y = y;
	y = getScreenH() - 1 - y;
	distorter->distortXY(x,y);

	if (ui->handle_clic(ui_x, ui_y, button, state, mod)) return 1;

	// Send the event to every StelModule
	std::vector<StelModule*> modList = moduleMgr->getCallOrders("handleMouseClicks");
	for (std::vector<StelModule*>::iterator i=modList.begin();i!=modList.end();++i)
	{
		if ((*i)->handleMouseClicks(x, y, button, state, mod)==true)
			return 1;
	}
	
	// Manage the event for the main window
	{
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
	
	return ui->handle_move(ui_x,ui_y, mod);
}

// Handle key press and release
int StelApp::handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state)
{

	// Standard keys should not be able to be hijacked by modules - Rob
	// (this could be debated)
	if (ui->handle_keys_tui(unicode, state)) return 1;

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
void StelApp::set2DfullscreenProjection(void) const
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

//! Restore previous projection mode
void StelApp::restoreFrom2DfullscreenProjection(void) const
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
	
	ui->setColorScheme(conf, section);
}

//! Set flag for activating night vision mode
void StelApp::setVisionModeNight(void)
{
	if (!getVisionModeNight())
	{
		setColorScheme(getConfigFilePath(), "night_color");
	}
	draw_mode=DM_NIGHT;
}

//! Set flag for activating chart vision mode
// ["color" section name used for easier backward compatibility for older configs - Rob]
void StelApp::setVisionModeNormal(void)
{
	if (!getVisionModeNormal())
	{
		setColorScheme(getConfigFilePath(), "color");
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
	LoadingBar lb(core->getProjection(), 12., "logo24bits.png",
     core->getProjection()->getViewportWidth(), core->getProjection()->getViewportHeight(),
      StelUtils::stringToWstring(PACKAGE_VERSION), 45, 320, 121);
	// Send the event to every StelModule
	for (StelModuleMgr::Iterator iter=moduleMgr->begin();iter!=moduleMgr->end();++iter)
	{
		(*iter)->updateSkyCulture(lb);
	}
}
