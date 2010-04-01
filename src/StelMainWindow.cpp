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
#include <stdexcept>
#include "StelApp.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QSettings>
#include <QResizeEvent>
#include <QIcon>
#include <QDebug>
#include <QCoreApplication>
#include <QApplication>
#include <QHBoxLayout>
#include <QGLWidget>

// Initialize static variables
StelMainWindow* StelMainWindow::singleton = NULL;

StelMainWindow::StelMainWindow() : QMainWindow(NULL)
{
	// Can't create 2 StelMainWindow instances
	Q_ASSERT(!singleton);
	singleton = this;

	setAttribute(Qt::WA_NoSystemBackground);

	setWindowIcon(QIcon(":/mainWindow/icon.bmp"));
	initTitleI18n();

	mainGraphicsView = new StelMainGraphicsView(this);
	setCentralWidget(mainGraphicsView);
}

// Update the translated title
void StelMainWindow::initTitleI18n()
{
	QString appNameI18n = q_("Stellarium %1").arg(StelUtils::getApplicationVersion());
	setWindowTitle(appNameI18n);
}

void StelMainWindow::init(QSettings* conf)
{
	resize(conf->value("video/screen_w", 800).toInt(), conf->value("video/screen_h", 600).toInt());
	if (conf->value("video/fullscreen", true).toBool())
	{
		setFullScreen(true);
	}
	else
	{
		setFullScreen(false);
	}
	show();
	// Process the event to make the window visible and create the openGL context.
	QCoreApplication::processEvents();
	mainGraphicsView->init(conf);
}

void StelMainWindow::deinit()
{
	StelMainGraphicsView::getInstance().deinitGL();
}

// Alternate fullscreen mode/windowed mode if possible
void StelMainWindow::toggleFullScreen()
{
	// Toggle full screen
	if (getFullScreen())
	{
		setFullScreen(false);
	}
	else
	{
		setFullScreen(true);
	}
}

// Get whether fullscreen is activated or not
bool StelMainWindow::getFullScreen() const
{
	return windowState().testFlag(Qt::WindowFullScreen);
}

// Set whether fullscreen is activated or not.
// Don't use the showFullScreen() and showNormal() methods since they have an unexpected behaviour.
void StelMainWindow::setFullScreen(bool b)
{
	if (b)
		setWindowState(windowState() | Qt::WindowFullScreen);
	else
		setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void StelMainWindow::closeEvent(QCloseEvent* event)
{
	event->ignore();
	QCoreApplication::exit();
}

