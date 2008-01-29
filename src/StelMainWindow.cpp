/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include <config.h>

#include "GLee.h"
#include "fixx11h.h"

#include "StelMainWindow.hpp"
#include "StelGLWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include <QIcon>
#include <QSettings>
#include <QCoreApplication>

#include "ui_mainGui.h"
#include "StelModuleMgr.hpp"

// Initialize static variables
StelMainWindow* StelMainWindow::singleton = NULL;
		 
StelMainWindow::StelMainWindow()
{
	// Can't create 2 StelMainWindow instances
	assert(!singleton);
	singleton = this;
}

void StelMainWindow::init(int argc, char** argv)
{
	setWindowTitle(StelApp::getApplicationName());
	setObjectName("stellariumMainWin");
	
	// Create the main instance of stellarium
	StelApp* stelApp = new StelApp(argc, argv, this);
	
	show();
	
	Ui_Form testUi;
	testUi.setupUi(this);
	
	// Init the main window. It must be done here because it is not the responsability of StelApp to do that
	QString iconPath;
	try
	{
		iconPath = stelApp->getFileMgr().findFile("data/icon.bmp");
	}
	catch (exception& e)
	{
		qWarning() << "ERROR when trying to locate icon file: " << e.what();
	}
	setWindowIcon(QIcon(iconPath));
	
	QSettings* settings = stelApp->getSettings();
	resize(settings->value("video/screen_w", 800).toInt(), settings->value("video/screen_h", 600).toInt());
	if (settings->value("video/fullscreen", true).toBool())
	{
		showFullScreen();
	}
	
	// Create the OpenGL widget in which the main modules will be drawn
	StelGLWidget* openGLWin = new StelGLWidget(this);
	setCentralWidget(openGLWin);
	openGLWin->initializeGL();
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	// Show the window during loading for the loading bar
	openGLWin->initializeGL();
	stelApp->init();
	openGLWin->init();
	openGLWin->setFocus();
	
	///////////////////////////////////////////////////////////////////////////
	// Set up the new GUI
	// The actions need to be added to the main form to be effective
	foreach (QObject* obj, this->children())
	{
		QAction* a = qobject_cast<QAction *>(obj);
		if (a)
			this->addAction(a);
	}
	
	// Connect all the GUI actions signals with the Core of Stellarium
	QObject* module = GETSTELMODULE("ConstellationMgr");
	QObject::connect(testUi.actionShow_Constellation_Lines, SIGNAL(toggled(bool)), module, SLOT(setFlagLines(bool)));
	QObject::connect(testUi.actionShow_Constellation_Art, SIGNAL(toggled(bool)), module, SLOT(setFlagArt(bool)));
	QObject::connect(testUi.actionShow_Constellation_Labels, SIGNAL(toggled(bool)), module, SLOT(setFlagNames(bool)));
	
	module = GETSTELMODULE("GridLinesMgr");
	QObject::connect(testUi.actionShow_Equatorial_Grid, SIGNAL(toggled(bool)), module, SLOT(setFlagEquatorGrid(bool)));
	QObject::connect(testUi.actionShow_Azimutal_Grid, SIGNAL(toggled(bool)), module, SLOT(setFlagAzimutalGrid(bool)));
	
	module = GETSTELMODULE("LandscapeMgr");
	QObject::connect(testUi.actionShow_Ground, SIGNAL(toggled(bool)), module, SLOT(setFlagLandscape(bool)));
	QObject::connect(testUi.actionShow_Cardinal_points, SIGNAL(toggled(bool)), module, SLOT(setFlagCardinalsPoints(bool)));
	QObject::connect(testUi.actionShow_Atmosphere, SIGNAL(toggled(bool)), module, SLOT(setFlagAtmosphere(bool)));
	
	module = GETSTELMODULE("NebulaMgr");
	QObject::connect(testUi.actionShow_Nebulas, SIGNAL(toggled(bool)), module, SLOT(setFlagHints(bool)));
	
	module = (QObject*)StelApp::getInstance().getCore()->getNavigation();
	QObject::connect(testUi.actionIncrease_Time_Speed, SIGNAL(triggered()), module, SLOT(increaseTimeSpeed()));
	QObject::connect(testUi.actionDecrease_Time_Speed, SIGNAL(triggered()), module, SLOT(decreaseTimeSpeed()));
	QObject::connect(testUi.actionSet_Real_Time_Speed, SIGNAL(triggered()), module, SLOT(setRealTimeSpeed()));
	QObject::connect(testUi.actionReturn_To_Current_Time, SIGNAL(triggered()), module, SLOT(setTimeNow()));
	QObject::connect(testUi.actionSwitch_Equatorial_Mount, SIGNAL(toggled(bool)), module, SLOT(setEquatorialMount(bool)));
			
	module = &StelApp::getInstance();
	QObject::connect(testUi.actionShow_Night_Mode, SIGNAL(toggled(bool)), module, SLOT(setVisionModeNight(bool)));
	
	module = GETSTELMODULE("MovementMgr");
	QObject::connect(testUi.actionGoto_Selected_Object, SIGNAL(triggered()), module, SLOT(setFlagTracking()));
	
	testUi.actionSet_Full_Screen->setChecked(isFullScreen());
	QObject::connect(testUi.actionSet_Full_Screen, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));
	
}

StelMainWindow::~StelMainWindow()
{
}

void StelMainWindow::saveScreenShot(const QString& filePrefix, const QString& saveDir) const
{
	QString shotDir;
	QImage im = findChild<StelGLWidget*>("stellariumOpenStelGLWidget")->grabFrameBuffer();

	if (saveDir == "")
	{
		try
		{
			shotDir = StelApp::getInstance().getFileMgr().getScreenshotDir();
			if (!StelApp::getInstance().getFileMgr().isWritable(shotDir))
			{
				cerr << "ERROR StelAppSdl::saveScreenShot: screenshot directory is not writable: " << qPrintable(shotDir) << endl;
				return;
			}
		}
		catch(exception& e)
		{
			cerr << "ERROR StelAppSdl::saveScreenShot: could not determine screenshot directory: " << e.what() << endl;
			return;
		}
	}
	else
	{
		shotDir = saveDir;
	}

	QString shotPath;
	for(int j=0; j<1000; ++j)
	{
		shotPath = shotDir+"/"+filePrefix + QString("%1").arg(j, 3, 10, QLatin1Char('0')) + ".bmp";
		if (!StelApp::getInstance().getFileMgr().exists(shotPath))
			break;
	}
	// TODO - if no more filenames available, don't just overwrite the last one
	// we should at least warn the user, perhaps prompt her, "do you want to overwrite?"
	
	cout << "Saving screenshot in file: " << qPrintable(shotPath) << endl;
	im.save(shotPath);
}


/*************************************************************************
 Alternate fullscreen mode/windowed mode if possible
*************************************************************************/
void StelMainWindow::toggleFullScreen()
{
	// Toggle full screen
	if (!isFullScreen())
	{
		showFullScreen();
	}
	else
	{
		showNormal();
	}
}

/*************************************************************************
 Get whether fullscreen is activated or not
*************************************************************************/
bool StelMainWindow::getFullScreen() const
{
	return isFullScreen();
}

/*************************************************************************
 Set whether fullscreen is activated or not
 *************************************************************************/
void StelMainWindow::setFullScreen(bool b)
{
	if (b)
		showFullScreen();
	else
		showNormal();
}
