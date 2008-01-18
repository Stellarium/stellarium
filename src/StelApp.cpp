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
#include "QtScriptMgr.hpp"

#include <QStringList>
#include <QString>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <set>
#include <string>

// Initialize static variables
StelApp* StelApp::singleton = NULL;

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(int argc, char** argv) :
	maxfps(10000.f), core(NULL), fps(0), frame(0), timefr(0), 
	timeBase(0), draw_mode(StelApp::DM_NORMAL), configFile("config.ini"), initialized(false)
{
	distorter=NULL;
	skyCultureMgr=NULL;
	localeMgr=NULL;
	fontManager=NULL;
	stelObjectMgr=NULL;
	textureMgr=NULL;
	moduleMgr=NULL;
	loadingBar=NULL;
	
	// Can't create 2 StelApp instances
	assert(!singleton);
	singleton = this;
	
	argList = new QStringList;
	for(int i=0; i<argc; i++)
		*argList << argv[i];
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	delete loadingBar; loadingBar=NULL;
	delete core; core=NULL;
	if (distorter) {delete distorter; distorter=NULL;}
	delete skyCultureMgr; skyCultureMgr=NULL;
	delete localeMgr; localeMgr=NULL;
	delete fontManager; fontManager=NULL;
	delete stelObjectMgr; stelObjectMgr=NULL;
	delete stelFileMgr; stelFileMgr=NULL;
	delete moduleMgr; moduleMgr=NULL;	// Also delete all modules
	delete textureMgr; textureMgr=NULL;
	delete argList; argList=NULL;
}

/*************************************************************************
 Return the full name of stellarium, i.e. "stellarium 0.9.0"
*************************************************************************/
QString StelApp::getApplicationName()
{
	return QString("Stellarium")+" "+PACKAGE_VERSION;
}

void StelApp::setViewPortDistorterType(const QString &type)
{
	if (type != getViewPortDistorterType()) setResizable(type == "none");
	if (distorter)
	{
		delete distorter;
		distorter = 0;
	}
	InitParser conf;
	conf.load(getConfigFilePath());
	distorter = ViewportDistorter::create(type.toStdString(),getScreenW(),getScreenH(),core->getProjection(),conf);
}

QString StelApp::getViewPortDistorterType() const
{
	if (distorter)
            return distorter->getType().c_str();
	return "none";
}


void StelApp::init()
{
	stelFileMgr = new StelFileMgr();
	// Load language codes
	try
	{
		Translator::init(stelFileMgr->findFile("data/iso639-1.utf8"));
	}
	catch (exception& e)
	{
		cerr << "ERROR while loading translations: " << e.what() << endl;
	}
	
	// Parse for first set of CLI arguments - stuff we want to process before other
	// output, such as --help and --version, and if we want to set the configFile value.
	parseCLIArgsPreConfig();
	
	// OK, print the console splash and get on with loading the program
	cout << " -------------------------------------------------------" << endl;
	cout << "[ This is "<< qPrintable(StelApp::getApplicationName()) << " - http://www.stellarium.org ]" << endl;
	cout << "[ Copyright (C) 2000-2008 Fabien Chereau et al         ]" << endl;
	cout << " -------------------------------------------------------" << endl;
	
	QStringList p=stelFileMgr->getSearchPaths();
	cout << "File search paths:" << endl;
	int n=0;
	foreach (QString i, p)
	{
		cout << " " << n << ". " << qPrintable(i) << endl;
		++n;
	}
	cout << "Config file is: " << qPrintable(configFile) << endl;
	
	if (!stelFileMgr->exists(configFile))
	{
		cerr << "config file \"" << qPrintable(configFile) << "\" does not exist - copying the default file." << endl;
		copyDefaultConfigFile();
	}
	
	textureMgr = new StelTextureMgr();
	localeMgr = new StelLocaleMgr();
	
	QString fontMapFile("");
	try 
	{
		fontMapFile = stelFileMgr->findFile(QFile::decodeName("data/fontmap.dat"));
	}
	catch(exception& e)
	{
		cerr << "ERROR when locating font map file: " << e.what() << endl;
	}
	fontManager = new StelFontMgr(QFile::encodeName(fontMapFile).data());
	
	skyCultureMgr = new StelSkyCultureMgr();
	moduleMgr = new StelModuleMgr();
	
	core = new StelCore();
	time_multiplier = 1;
	distorter = 0;
	
	// Initialize video device and other sdl parameters
	InitParser conf;
	conf.load(getConfigFilePath());

	// Main section
	string version = conf.get_str("main:version");
	
	if (version.empty())
	{
		qWarning() << "Found an invalid config file. Overwrite with default.";
		QFile::remove(getConfigFilePath());
		copyDefaultConfigFile();
		conf.load(getConfigFilePath());  // Read new config
		version = conf.get_str("main:version");
	}
	
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
			conf.load(getConfigFilePath());  // Read new config
		}
		else
		{
			cout << "Attempting to use an existing older config file." << endl;
		}
	}
	
	parseCLIArgsPostConfig(conf);

	// Create openGL context first
	QString iconPath;
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
	
	maxfps = conf.get_double ("video","maximum_fps",10000);
	minfps = conf.get_double ("video","minimum_fps",10000);

	core->initProj(conf);

	// Clear screen, this fixes a strange artifact at loading time in the upper corner.
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	swapGLBuffers();
	glClear(GL_COLOR_BUFFER_BIT);

	loadingBar = new LoadingBar(core->getProjection(), 12., "logo24bits.png",
	              getScreenW(), getScreenH(),
	              StelUtils::stringToWstring(PACKAGE_VERSION), 45, 320, 121);
	loadingBar->setClearBuffer(true);	// Prevent flickering
	
	// Stel Object Data Base manager
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init(conf);
	
	localeMgr->init(conf);
	skyCultureMgr->init(conf);
	
	// Init the solar system first
	SolarSystem* ssystem = new SolarSystem();
	ssystem->init(conf);
	getModuleMgr().registerModule(ssystem);
	
	// Load hipparcos stars & names
	StarMgr* hip_stars = new StarMgr();
	hip_stars->init(conf);
	getModuleMgr().registerModule(hip_stars);	
	
	core->init(conf);

	// Init nebulas
	NebulaMgr* nebulas = new NebulaMgr();
	nebulas->init(conf);
	getModuleMgr().registerModule(nebulas);
	
	// Init milky way
	MilkyWay* milky_way = new MilkyWay();
	milky_way->init(conf);
	getModuleMgr().registerModule(milky_way);
	
	// Telescope manager
	TelescopeMgr* telescope_mgr = new TelescopeMgr();
	telescope_mgr->init(conf);
	getModuleMgr().registerModule(telescope_mgr);
	
	// Constellations
	ConstellationMgr* asterisms = new ConstellationMgr(hip_stars);
	asterisms->init(conf);
	getModuleMgr().registerModule(asterisms);
	
	// Landscape, atmosphere & cardinal points section
	LandscapeMgr* landscape = new LandscapeMgr();
	landscape->init(conf);
	getModuleMgr().registerModule(landscape);

	GridLinesMgr* gridLines = new GridLinesMgr();
	gridLines->init(conf);
	getModuleMgr().registerModule(gridLines);
	
	// Meteors
	MeteorMgr* meteors = new MeteorMgr(10, 60);
	meteors->init(conf);
	getModuleMgr().registerModule(meteors);

//ugly fix by johannes: call skyCultureMgr->init twice so that
// star names are loaded again
	skyCultureMgr->init(conf);
	
	// Those 3 are going to disapear
	StelCommandInterface* commander = new StelCommandInterface(core, this);
	getModuleMgr().registerModule(commander);
	ScriptMgr* scripts = new ScriptMgr(commander);
	scripts->init(conf);
	getModuleMgr().registerModule(scripts);
	scripts->set_removable_media_path(conf.get_str("files","removable_media_path", "").c_str());
	ImageMgr* script_images = new ImageMgr();
	script_images->init(conf);
	getModuleMgr().registerModule(script_images);	
	
	StelUI* ui = new StelUI(core, this);
	ui->init(conf);
	getModuleMgr().registerModule(ui);
	
	// Initialisation of the color scheme
	draw_mode = draw_mode=DM_NIGHT;  // fool caching
	setVisionModeNormal();
	if (conf.get_boolean("viewing:flag_night")) setVisionModeNight();

	setViewPortDistorterType(conf.get_str("video","distorter","none").c_str());
	
	// Load dynamically all the modules found in the modules/ directories
	// which are configured to be loaded at startup
	foreach (StelModuleMgr::ExternalStelModuleDescriptor i, moduleMgr->getExternalModuleList())
	{
		if (i.loadAtStartup==false)
			continue;
		StelModule* m = moduleMgr->loadExternalPlugin(i.key);
		if (m!=NULL)
		{
			m->init(conf);
			moduleMgr->registerModule(m);
		}
	}
	
	// Generate dependency Lists for all modules
	moduleMgr->generateCallingLists();
	
	updateSkyLanguage();
	updateAppLanguage();
	
	// play startup script, if available
	scripts->play_startup_script();
	
	//QtScriptMgr scriptMgr;
	//scriptMgr.test();
	
	initialized = true;
}

void StelApp::parseCLIArgsPreConfig(void)
{	
	if (argsGetOption(argList, "-v", "--version"))
	{
		cout << qPrintable(getApplicationName()) << endl;
		exit(0);
	}

	if (argsGetOption(argList, "-h", "--help"))
	{
		// Get the basename of binary
		QString binName = argList->at(0);
		binName.remove(QRegExp("^.*[/\\\\]"));
		
		cout << "Usage:" << endl
				<< "  " 
				<< qPrintable(binName) << " [options]" << endl << endl
				<< "Options:" << endl
				<< "--version (or -v)       : Print program name and version and exit." << endl
				<< "--help (or -h)          : This cruft." << endl
				<< "--config-file (or -c)   : Use an alternative name for the config file" << endl
				<< "--full-screen (or -f)   : With argument \"yes\" or \"no\" over-rides" << endl
				<< "                          the full screen setting in the config file" << endl
				<< "--home-planet           : Specify observer planet (English name)" << endl
				<< "--altitude              : Specify observer altitude in meters" << endl
				<< "--longitude             : Specify longitude, e.g. +53d58\\'16.65\\\"" << endl
				<< "--latitude              : Specify latitude, e.g. -1d4\\'27.48\\\"" << endl 
				<< "--list-landscapes       : Print a list of value landscape IDs" << endl 
				<< "--landscape             : Start using landscape whose ID (dir name)" << endl
				<< "                          is passed as parameter to option" << endl
				<< "--sky-date              : Specify sky date in format yyyymmdd" << endl
				<< "--sky-time              : Specify sky time in format hh:mm:ss" << endl
				<< "--fov                   : Specify the field of view (degrees)" << endl
				<< "--projection-type       : Specify projection type, e.g. stereographic" << endl;
		exit(0);
	}
	
	if (argsGetOption(argList, "", "--list-landscapes"))
	{
		QSet<QString> landscapeIds = stelFileMgr->listContents("landscapes", StelFileMgr::DIRECTORY);
		for(QSet<QString>::iterator i=landscapeIds.begin(); i!=landscapeIds.end(); ++i)
		{
			try 
			{
				// finding the file will throw an exception if it is not found
				// in that case we won't output the landscape ID as it canont work
				stelFileMgr->findFile("landscapes/" + *i + "/landscape.ini");
				cout << (*i).toUtf8().data() << endl;
			}
			catch(exception& e){}
		}
		exit(0);
	}
	
	try
	{
		setConfigFile(qPrintable(argsGetOptionWithArg<QString>(argList, "-c", "--config-file", "config.ini")));
	}
	catch(exception& e)
	{
		cerr << "ERROR: while looking for --config-file option: " << e.what() << ". Using \"config.ini\"" << endl;
		setConfigFile("config.ini");		
	}
}

void StelApp::parseCLIArgsPostConfig(InitParser& conf)
{
	// Over-ride config file options with command line options
	// We should catch exceptions from argsGetOptionWithArg...
	int fullScreen, altitude;
	float fov;
	QString landscapeId, homePlanet, longitude, latitude, skyDate, skyTime, projectionType;
	
	try
	{
		fullScreen = argsGetYesNoOption(argList, "-f", "--full-screen", -1);
		landscapeId = argsGetOptionWithArg<QString>(argList, "", "--landscape", "");
		homePlanet = argsGetOptionWithArg<QString>(argList, "", "--home-planet", "");
		altitude = argsGetOptionWithArg<int>(argList, "", "--altitude", -1);
		longitude = argsGetOptionWithArg<QString>(argList, "", "--longitude", "");
		latitude = argsGetOptionWithArg<QString>(argList, "", "--latitude", "");
		skyDate = argsGetOptionWithArg<QString>(argList, "", "--sky-date", "");
		skyTime = argsGetOptionWithArg<QString>(argList, "", "--sky-time", "");
		fov = argsGetOptionWithArg<float>(argList, "", "--fov", -1.0);
		projectionType = argsGetOptionWithArg<QString>(argList, "", "--projection-type", "");

	}
	catch (exception& e)
	{
		cerr << "ERROR while checking command line options: " << e.what() << endl;
		exit(0);
	}

	// Will be -1 if option is not found, in which case we don't change anything.
	if (fullScreen == 1) conf.set_boolean("video:fullscreen", true);
	else if (fullScreen == 0) conf.set_boolean("video:fullscreen", false);
	
	if (landscapeId != "") conf.set_str("init_location:landscape_name", landscapeId);
	
	if (homePlanet != "") conf.set_str("init_location:home_planet", homePlanet);
	
	if (altitude != -1) conf.set_int("init_location:altitude", altitude);
	
	QRegExp longLatRx("[\\-+]?\\d+d\\d+\\'\\d+(\\.\\d+)?\"");
	if (longitude != "")
	{
		if (longLatRx.exactMatch(longitude))
			conf.set_str("init_location:longitude", longitude);
		else
			cerr << "WARNING: --longitude argument has unrecognised format" << endl;
	}
	
	if (latitude != "")
	{
		if (longLatRx.exactMatch(latitude))
			conf.set_str("init_location:latitude", latitude);
		else
			cerr << "WARNING: --latitude argument has unrecognised format" << endl;
	}
	
	if (skyDate != "" || skyTime != "")
	{
		// Get the Julian date for the start of the current day
		// and the extra necessary for the time of day as separate
		// components.  Then if the --sky-date and/or --sky-time flags
		// are set we over-ride the component, and finally add them to 
		// get the full julian date and set that.
		
		// First, lets determine the Julian day number and the part for the time of day
		QDateTime now = QDateTime::currentDateTime();
		double skyDatePart = now.date().toJulianDay();
		double skyTimePart = StelUtils::qTimeToJDFraction(now.time());
		
		// Over-ride the sktDatePart if the user specified the date using --sky-date
		if (skyDate != "")
		{
			// validate the argument format, we will tolerate yyyy-mm-dd by removing all -'s
			QRegExp dateRx("\\d{8}");
			if (dateRx.exactMatch(skyDate.remove("-")))
				skyDatePart = QDate::fromString(skyDate, "yyyyMMdd").toJulianDay();
			else
				cerr << "WARNING: --sky-date argument has unrecognised format  (I want yyyymmdd)" << endl;
		}
		
		if (skyTime != "")
		{
			QRegExp timeRx("\\d{1,2}:\\d{2}:\\d{2}");
			if (timeRx.exactMatch(skyTime))
				skyTimePart = StelUtils::qTimeToJDFraction(QTime::fromString(skyTime, "hh:mm:ss"));
			else
				cerr << "WARNING: --sky-time argument has unrecognised format (I want hh:mm:ss)" << endl;
		}

		conf.set_str("navigation:startup_time_mode", "preset");
		conf.set_double ("navigation:preset_sky_time", skyDatePart + skyTimePart);
	}

	if (fov > 0.0) conf.set_double("navigation:init_fov", fov);
	
	if (projectionType != "") conf.set_str("projection:type", projectionType);
}

void StelApp::update(int delta_time)
{
     if (!initialized)
        return;
	
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ACTION_UPDATE))
	{
		i->update((double)delta_time/1000);
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ACTION_DRAW))
	{
		double d = i->draw(core);
		if (d>squaredDistance)
			squaredDistance = d;
	}

	stelObjectMgr->draw(core);

	core->postDraw();
	distorter->distort();

	// Draw the Graphical ui
	StelUI* ui = (StelUI*)getModuleMgr().getModule("StelUI");
	ui->drawGui();

	// Test code
// 	ConvexPolygon poly = core->getProjection()->getViewportConvexPolygon(-100, -100);
// 	double area = poly.getArea() * 3282.8063500117441;
// 	glColor3f(1,1,1);
// 	core->getProjection()->drawPolygon(poly);
	// qWarning() << area;
	
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
		foreach (StelModule* iter, moduleMgr->getAllModules())
		{
			iter->glWindowHasBeenResized(w, h);
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ACTION_HANDLEMOUSECLICKS))
	{
		if (i->handleMouseClicks(x, y, button, state, mod)==true)
			return 1;
	}
	
	// Manage the event for the main window
	{
		StelCommandInterface* commander = (StelCommandInterface*)getModuleMgr().getModule("StelCommandInterface");
		
		// Deselect the selected object
		if (button==Stel_BUTTON_RIGHT && state==Stel_MOUSEBUTTONUP)
		{
			commander->execute_command("select");
			return 1;
		}
		MovementMgr* mvmgr = (MovementMgr*)getModuleMgr().getModule("MovementMgr");
		if (button==Stel_BUTTON_LEFT && state==Stel_MOUSEBUTTONUP && !mvmgr->getHasDragged())
		{
#ifdef MACOSX
			// CTRL + left clic = right clic for 1 button mouse
			if (mod & COMPATIBLE_StelMod_CTRL)
			{
				commander->execute_command("select");
				return 1;
			}

			// Try to select object at that position
			getStelObjectMgr().findAndSelect(core, x, y, (mod & StelMod_META) ? StelModule::ADD_TO_SELECTION : StelModule::REPLACE_SELECTION);
#else
			getStelObjectMgr().findAndSelect(core, x, y, (mod & COMPATIBLE_StelMod_CTRL) ? StelModule::ADD_TO_SELECTION : StelModule::REPLACE_SELECTION);
#endif
			// If an object was selected update informations
			if (getStelObjectMgr().getWasSelected())
			{
				((MovementMgr*)moduleMgr->getModule("MovementMgr"))->setFlagTracking(false);
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ACTION_HANDLEMOUSEMOVES))
	{
		if (i->handleMouseMoves(x, y, mod)==true)
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ACTION_HANDLEKEYS))
	{
		if (i->handleKeys(key, mod, unicode, state)==true)
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

void StelApp::setConfigFile(const QString& configName)
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
		cerr << "ERROR StelApp::setConfigFile could not find or create configuration file " << configName.toUtf8().data() << endl;
		exit(1);
	}
}

void StelApp::copyDefaultConfigFile()
{
	QString defaultConfigFilePath;
	try
	{
		defaultConfigFilePath = stelFileMgr->findFile("data/default_config.ini");
	}
	catch(exception& e)
	{
		cerr << "ERROR (copyDefaultConfigFile): failed to locate data/default_config.ini.  Please check your installation." << endl;
		exit(1);
	}
	
	QFile::copy(defaultConfigFilePath, configFile);
	if (!stelFileMgr->exists(configFile))
	{
		cerr << "ERROR (copyDefaultConfigFile): failed to copy file " << defaultConfigFilePath.toUtf8().data() << " to " << configFile.toUtf8().data() << ". You could try to copy it by hand." << endl;
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
void StelApp::setColorScheme(const QString& fileName, const QString& section)
{
	InitParser conf;
	conf.load(fileName);
	
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->setColorScheme(conf, section);
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
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->updateI18n();
	}
}

// Update and reload sky culture informations everywhere in the program
void StelApp::updateSkyCulture()
{
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->updateSkyCulture();
	}
}

bool StelApp::argsGetOption(QStringList* args, QString shortOpt, QString longOpt)
{
	bool result=false;

        // Don't see anything after a -- as an option
	int lastOptIdx = args->indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args->size();

	for(int i=0; i<lastOptIdx; i++)
	{
		if ((shortOpt!="" && args->at(i) == shortOpt) || args->at(i) == longOpt)
		{
			result = true;
			i=args->size();
		}
	}

	return result;
}

template<class T>
T StelApp::argsGetOptionWithArg(QStringList* args, QString shortOpt, QString longOpt, T defaultValue)
{
	// Don't see anything after a -- as an option
	int lastOptIdx = args->indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args->size();

	for(int i=0; i<lastOptIdx; i++)
	{
		bool match(false);
		QString argStr("");

		// form -n=arg
		if ((shortOpt!="" && args->at(i).left(shortOpt.length()+1)==shortOpt+"="))
		{
			match=true;
			argStr=args->at(i).right(args->at(i).length() - shortOpt.length() - 1);
		}
		// form --number=arg
		else if (args->at(i).left(longOpt.length()+1)==longOpt+"=")
		{
			match=true;
			argStr=args->at(i).right(args->at(i).length() - longOpt.length() - 1);
		}
		// forms -n arg and --number arg
		else if ((shortOpt!="" && args->at(i)==shortOpt) || args->at(i)==longOpt)
		{
			if (i+1>=lastOptIdx)
			{
				throw(runtime_error(qPrintable("optarg_missing ("+longOpt+")")));
			}
			else
			{
				match=true;
				argStr=args->at(i+1);
				i++;  // skip option argument in next iteration 
			}
		}

		if (match)
		{
			T retVal;
			QTextStream converter(qPrintable(argStr));
			converter >> retVal;
			if (converter.status() != QTextStream::Ok)
				throw(runtime_error(qPrintable("optarg_type ("+longOpt+")")));
			else
				return retVal;
		}
	}

	return defaultValue;
}

int StelApp::argsGetYesNoOption(QStringList* args, QString shortOpt, QString longOpt, int defaultValue)
{
	QString strArg = argsGetOptionWithArg<QString>(args, shortOpt, longOpt, "");
	if (strArg == "")
	{
		return defaultValue;
	}
	if (strArg.compare("yes", Qt::CaseInsensitive)==0
		   || strArg.compare("y", Qt::CaseInsensitive)==0
		   || strArg.compare("true", Qt::CaseInsensitive)==0
		   || strArg.compare("t", Qt::CaseInsensitive)==0
		   || strArg.compare("on", Qt::CaseInsensitive)==0
		   || strArg=="1")
	{
		return 1;
	}
	else if (strArg.compare("no", Qt::CaseInsensitive)==0
			|| strArg.compare("n", Qt::CaseInsensitive)==0
			|| strArg.compare("false", Qt::CaseInsensitive)==0
			|| strArg.compare("f", Qt::CaseInsensitive)==0
			|| strArg.compare("off", Qt::CaseInsensitive)==0
			|| strArg=="0")
	{
		return 0;
	}
	else
	{
		throw(runtime_error("optarg_type"));
	}
}

