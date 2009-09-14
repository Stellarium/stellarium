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

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QTranslator>
#include "StelMainGraphicsView.hpp"
#include "StelMainWindow.hpp"
#include "StelTranslator.hpp"
#include <QDebug>
#include <QGLFormat>
#include <QPlastiqueStyle>
#ifdef MACOSX
#include "StelMacosxDirs.hpp"
#endif

//! @class GettextStelTranslator
//! Provides i18n support through gettext.
class GettextStelTranslator : public QTranslator
{
public:
	virtual bool isEmpty() const { return false; }

	virtual QString translate(const char* context, const char* sourceText, const char* comment=0) const
	{
		return q_(sourceText);
	}
};


// Main stellarium procedure
int main(int argc, char **argv)
{
	//QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
	QCoreApplication::setApplicationName("stellarium");
	QCoreApplication::setApplicationVersion(StelApp::getApplicationVersion());
	QCoreApplication::setOrganizationDomain("stellarium.org");
	QCoreApplication::setOrganizationName("stellarium");

	//QApplication::setDesktopSettingsAware(false);
	QApplication::setStyle(new QPlastiqueStyle());

	// Handle command line options for alternative QT graphics system types
	// DEFAULT_GRAPHICS_SYSTEM is defined per playform in the main CMakeLists.txt file
	int argc2;
	char** argv2;
	char cmd1[] = "-graphicssystem";
	char cmd2[] = DEFAULT_GRAPHICS_SYSTEM;

	// Having said that, we should also allow users to specify the mode if they like
	// which also gives them the option to use "opengl".  Good job these three strings
	// are all the same length, else we'd be mallocing all over the place...
	for (int i=1; i<argc; i++)
	{
		QString a(argv[i]);
		if (a == "--graphics-system" && i+1 < argc)
		{
			a += "=" + QString(argv[i+1]);
		}

		if (a == "--graphics-system=native")
		{
			strcpy(cmd2, "native");
		}
		else if (a == "--graphics-system=raster")
		{
			strcpy(cmd2, "raster");
		}
		else if (a == "--graphics-system=opengl")
		{
			strcpy(cmd2, "opengl");
		}
	}

	argv2 = (char**)malloc(sizeof(char*)*(argc+2)); 
	memcpy(argv2, argv, argc*sizeof(char*));
	argv2[argc]=cmd1;
	argv2[argc+1]=cmd2;
	argc2 = argc+2;

	QApplication app(argc2, argv2);

#ifdef MACOSX
	StelMacosxDirs::addApplicationPluginDirectory();
#endif
	GettextStelTranslator trans;
	app.installTranslator(&trans);
	if (!QGLFormat::hasOpenGL())
	{
		QMessageBox::warning(0, "Stellarium", q_("This system does not support OpenGL."));
	}

	StelMainWindow* mainWin = new StelMainWindow(NULL);
	StelMainGraphicsView* view = new StelMainGraphicsView(NULL, argc, argv);
	mainWin->setCentralWidget(view);
	mainWin->init();
	app.exec();
	view->deinitGL();
	delete view;
	delete mainWin;
	free(argv2);
	return 0;
}

