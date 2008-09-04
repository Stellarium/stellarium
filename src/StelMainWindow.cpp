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


#include "StelMainWindow.hpp"

#include <QSettings>
#include <QResizeEvent>
#include <QIcon>
#include <QDebug>
#include <QFontDatabase>
#include <QCoreApplication>

#include <stdexcept>
#include "StelApp.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"

// Initialize static variables
StelMainWindow* StelMainWindow::singleton = NULL;

StelMainWindow::StelMainWindow(QWidget* parent) : QMainWindow(parent), initComplete(false)
{
	// Can't create 2 StelMainWindow instances
	Q_ASSERT(!singleton);
	singleton = this;
}

void StelMainWindow::init()
{
	setWindowIcon(QIcon(":/mainWindow/icon.bmp"));
	
	QString fName;
	try
	{
		fName = StelApp::getInstance().getFileMgr().findFile("data/DejaVuSans.ttf");
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading font DejaVuSans : " << e.what();
	}
	if (!fName.isEmpty())
		QFontDatabase::addApplicationFont(fName);
	
	// Init the main window. It must be done here because it is not the responsability of StelApp to do that
	QSettings* settings = StelApp::getInstance().getSettings();
	resize(settings->value("video/screen_w", 800).toInt(), settings->value("video/screen_h", 600).toInt());
	if (settings->value("video/fullscreen", true).toBool())
	{
		showFullScreen();
	}
	else
	{
		show();
	}
	
	StelMainGraphicsView::getInstance().init();
	
	initComplete = true;
}

// Alternate fullscreen mode/windowed mode if possible
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

// Get whether fullscreen is activated or not
bool StelMainWindow::getFullScreen() const
{
	return isFullScreen();
}

// Set whether fullscreen is activated or not
void StelMainWindow::setFullScreen(bool b)
{
	if (b)
		showFullScreen();
	else
		showNormal();

	StelApp::getInstance().getSettings()->setValue("video/fullscreen", b);
}

void StelMainWindow::closeEvent(QCloseEvent* event)
{
	StelMainGraphicsView::getInstance().deinitGL();
	QCoreApplication::exit();
}


void StelMainWindow::resizeEvent(QResizeEvent* event)
{
	if (initComplete)
	{
		QSettings* conf = StelApp::getInstance().getSettings();
		if (conf!=NULL)
		{
			if (!isFullScreen())
			{
				conf->setValue("video/screen_h", event->size().height());
				conf->setValue("video/screen_w", event->size().width());
			}
		}
	}
	QMainWindow::resizeEvent(event);
}
