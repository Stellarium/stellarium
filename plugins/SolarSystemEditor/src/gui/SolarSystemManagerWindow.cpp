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

#include "SolarSystemEditor.hpp"

#include "SolarSystemManagerWindow.hpp"
#include "ui_solarSystemManagerWindow.h"

#include "MpcImportWindow.hpp"
#include "ManualImportWindow.hpp"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"

#include <QFileDialog>

SolarSystemManagerWindow::SolarSystemManagerWindow()
{
	ui = new Ui_solarSystemManagerWindow();
	mpcImportWindow = new MpcImportWindow();
	manualImportWindow = NULL;

	ssoManager = GETSTELMODULE(SolarSystemEditor);
}

SolarSystemManagerWindow::~SolarSystemManagerWindow()
{
	delete ui;

	if (mpcImportWindow)
		delete mpcImportWindow;
	if (manualImportWindow)
		delete manualImportWindow;
}

void SolarSystemManagerWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonCopyFile, SIGNAL(clicked()), this, SLOT(copyConfiguration()));
	connect(ui->pushButtonReplaceFile, SIGNAL(clicked()), this, SLOT(replaceConfiguration()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(removeObject()));
	connect(ui->pushButtonImportMPC, SIGNAL(clicked()), this, SLOT(newImportMPC()));
	//connect(ui->pushButtonManual, SIGNAL(clicked()), this, SLOT(newImportManual()));

	connect(ssoManager, SIGNAL(solarSystemChanged()), this, SLOT(populateSolarSystemList()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), ssoManager, SLOT(resetSolarSystemToDefault()));

	updateTexts();

	Q_ASSERT(mpcImportWindow);
	//Rebuild the list if any planets have been imported
	connect(mpcImportWindow, SIGNAL(objectsImported()), this, SLOT(populateSolarSystemList()));

	ui->lineEditUserFilePath->setText(ssoManager->getCustomSolarSystemFilePath());
	populateSolarSystemList();
}

void SolarSystemManagerWindow::updateTexts()
{
	//Solar System tab
	// TRANSLATORS: Appears as the text of hyperlinks linking to websites. :)
	QString linkText(q_("website"));
	QString linkCode = QString("<a href=\"http://www.minorplanetcenter.net/\">%1</a>").arg(linkText);
	       
	// TRANSLATORS: IAU = International Astronomical Union
	QString mpcText(q_("You can import comet and asteroid data formatted in the export formats of the IAU's Minor Planet Center (%1). You can import files with lists of objects, download such lists from the Internet or search the online Minor Planet and Comet Ephemeris Service (MPES)."));
	ui->labelMPC->setText(QString(mpcText).arg(linkCode));
	
	//About tab
	ui->labelTitle->setText(q_("Solar System Editor plug-in"));
	ui->labelVersion->setText(QString(q_("Version %1")).arg(SOLARSYSTEMEDITOR_VERSION));
}

void SolarSystemManagerWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateSolarSystemList();
		updateTexts();
	}
}

void SolarSystemManagerWindow::newImportMPC()
{
	Q_ASSERT(mpcImportWindow);

	mpcImportWindow->setVisible(true);
}

void SolarSystemManagerWindow::newImportManual()
{
	if (manualImportWindow == NULL)
	{
		manualImportWindow = new ManualImportWindow();
		connect(manualImportWindow, SIGNAL(visibleChanged(bool)), this, SLOT(resetImportManual(bool)));
	}

	manualImportWindow->setVisible(true);
}

void SolarSystemManagerWindow::resetImportManual(bool show)
{
	//If the window is being displayed, do nothing
	if (show)
		return;

	if (manualImportWindow)
	{
		//TODO:Move this out of here!
		//Reload the list, in case there are new objects
		populateSolarSystemList();

		delete manualImportWindow;
		manualImportWindow = NULL;

		//This window is in the background, bring it to the foreground
		dialog->setVisible(true);
	}
}

void SolarSystemManagerWindow::populateSolarSystemList()
{
	unlocalizedNames.clear();
	foreach (const PlanetP & object, GETSTELMODULE(SolarSystem)->getAllPlanets())
	{
		unlocalizedNames.insert(object->getNameI18n(), object->getEnglishName());
	}

	ui->listWidgetObjects->clear();
	ui->listWidgetObjects->addItems(unlocalizedNames.keys());
	//No explicit sorting is necessary: sortingEnabled is set in the .ui
}

void SolarSystemManagerWindow::removeObject()
{
	if(ui->listWidgetObjects->currentItem())
	{
		QString ssoI18nName = ui->listWidgetObjects->currentItem()->text();
		QString ssoEnglishName = unlocalizedNames.value(ssoI18nName);
		//qDebug() << ssoId;
		//TODO: Ask for confirmation first?
		ssoManager->removeSsoWithName(ssoEnglishName);
	}
}

void SolarSystemManagerWindow::copyConfiguration()
{
	QString filePath = QFileDialog::getSaveFileName(0, "Save the Solar System configuration file as...", StelFileMgr::getScreenshotDir());
	ssoManager->copySolarSystemConfigurationFileTo(filePath);
}

void SolarSystemManagerWindow::replaceConfiguration()
{
	QString filePath = QFileDialog::getOpenFileName(0, "Select a file to replace the Solar System configuration file", StelFileMgr::getScreenshotDir(), QString("Configration files (*.ini)"));
	ssoManager->replaceSolarSystemConfigurationFileWith(filePath);
}
