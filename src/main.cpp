/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include <clocale>
#include <config.h>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include "StelMainWindow.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include <QDebug>
#include <QIcon>
#include <QSettings>
#include <QDesktopWidget>

// Main stellarium procedure
int main(int argc, char **argv)
{
	// Used for getting system date formatting
	setlocale(LC_TIME, "");
	
	QApplication app(argc, argv);
	if (!QGLFormat::hasOpenGL())
	{
		QMessageBox::information(0, "Stellarium", "This system does not support OpenGL.");
	}
	
	StelMainWindow mainWin;
	StelApp* stelApp = new StelApp(argc, argv, &mainWin);
	
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
	mainWin.setWindowIcon(QIcon(iconPath));
	
	QSettings* settings = stelApp->getSettings();
	mainWin.resize(settings->value("video/screen_w", 800).toInt(), settings->value("video/screen_h", 600).toInt());
	if (settings->value("video/fullscreen", true).toBool())
	{
		mainWin.showFullScreen();
		//QDesktopWidget* desktop = QApplication::desktop();
		//mainWin.resize(desktop->screenGeometry(&mainWin).width(),desktop->screenGeometry(&mainWin).height());
	}
	
	stelApp->init();
	app.exec();
	
	// At this point it is important to still have a valid GL context because the textures are deallocated
	// The GL Context is still valid because openGLWin is still in the current scope
	delete stelApp;
	return 0;
}
