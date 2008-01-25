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

#include <iostream>
#include "StelMainWindow.hpp"
#include "StelGLWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "InitParser.hpp"
#include "fixx11h.h"

using namespace std;

#include <QtGui/QImage>
#include <QtOpenGL>
#include <QStringList>
#include <QCoreApplication>
#include <QRegExp>
#include <QDate>
#include <QTime>
#include <QDateTime>

// Initialize static variables
StelMainWindow* StelMainWindow::singleton = NULL;
		 
StelMainWindow::StelMainWindow(int argc, char** argv)
{
	setWindowTitle(StelApp::getApplicationName());
	setObjectName("stellariumMainWin");
	setMinimumSize(400,400);
	
	// Can't create 2 StelMainWindow instances
	assert(!singleton);
	singleton = this;
	
	// Create the main instance of stellarium
	StelApp* stelApp = new StelApp(argc, argv, this);
	
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
	
	// Show the window during loading for the loading bar
	show();
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	openGLWin->initializeGL();
	
	stelApp->init();
	openGLWin->init();
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


bool StelMainWindow::getFullScreen() const
{
	return isFullScreen();
}
