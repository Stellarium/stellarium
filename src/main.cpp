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
#include "StelMainWindow.hpp"
#include "Translator.hpp"
#include <QGLFormat>
#include <QDebug>

// The GettextTranslator class provides i18n support through gettext.
class GettextTranslator : public QTranslator
{
public:
	virtual bool isEmpty() const { return false; }

	virtual QString translate(const char* context,
				  const char* sourceText,
				  const char* comment=0) const
	{
		return q_(sourceText);
	}
};

// Main stellarium procedure
int main(int argc, char **argv)
{	
	QApplication app(argc, argv);
	GettextTranslator trans;
	app.installTranslator(&trans);
	if (!QGLFormat::hasOpenGL())
	{
		QMessageBox::information(0, "Stellarium", q_("This system does not support OpenGL."));
	}
	StelMainWindow mainWin;
	mainWin.init(argc, argv);
	app.exec();
	return 0;
}
