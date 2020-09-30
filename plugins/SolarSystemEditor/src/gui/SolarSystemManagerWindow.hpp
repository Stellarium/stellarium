/*
 * Solar System editor plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef SOLARSYSTEMMANAGERWINDOW_HPP
#define SOLARSYSTEMMANAGERWINDOW_HPP

#include <QObject>
#include "StelDialog.hpp"

#include <QHash>
#include <QString>

class SolarSystemEditor;

class Ui_solarSystemManagerWindow;
class MpcImportWindow;
class ManualImportWindow;

/*! \brief Main window for handling Solar System objects.
  \author Bogdan Marinov
*/
class SolarSystemManagerWindow : public StelDialog
{
	Q_OBJECT
public:
	SolarSystemManagerWindow();
	virtual ~SolarSystemManagerWindow();

public slots:
	void retranslate();

protected:
	virtual void createDialogContent();
	Ui_solarSystemManagerWindow * ui;

private slots:
	//! \todo Find a way to suggest a default file name (select directory instead of file?)

	// export the user's ssystem_minor file
	void copyConfiguration();

	// import a ssystem_minor file
	void replaceConfiguration();

	// append new data, and update existing data
	void addConfiguration();

	void populateSolarSystemList();
	void removeObjects();

	void newImportMPC();

	void newImportManual();
	void resetImportManual(bool);

	void resetSSOdefaults();

private:
	MpcImportWindow* mpcImportWindow;
	ManualImportWindow * manualImportWindow;

	SolarSystemEditor * ssEditor;

	QHash<QString,QString> unlocalizedNames;
	
	void setAboutHtml(void);
	void updateTexts();
};

#endif // SOLARSYSTEMMANAGERWINDOW_HPP
