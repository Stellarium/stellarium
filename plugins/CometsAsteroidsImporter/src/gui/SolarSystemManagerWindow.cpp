/*
 * Comet and asteroids importer plug-in for Stellarium
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "CAImporter.hpp"

#include "SolarSystemManagerWindow.hpp"
#include "ui_solarSystemManagerWindow.h"

#include "ImportWindow.hpp"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"

SolarSystemManagerWindow::SolarSystemManagerWindow()
{
	ui = new Ui_solarSystemManagerWindow();
	importWindow = NULL;

	ssoManager = GETSTELMODULE(CAImporter);
}

SolarSystemManagerWindow::~SolarSystemManagerWindow()
{
	delete ui;

	if (importWindow)
		delete importWindow;
}

void SolarSystemManagerWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(removeObject()));
	connect(ui->pushButtonImportMPC, SIGNAL(clicked()), this, SLOT(newImportMPC()));

	connect(ssoManager, SIGNAL(solarSystemChanged()), this, SLOT(populateSolarSystemList()));

	populateSolarSystemList();
}

void SolarSystemManagerWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);

	if (importWindow)
		importWindow->languageChanged();
}

void SolarSystemManagerWindow::newImportMPC()
{
	if (importWindow == NULL)
	{
		importWindow = new ImportWindow();
		connect(importWindow, SIGNAL(visibleChanged(bool)), this, SLOT(resetImportMPC(bool)));
	}

	importWindow->setVisible(true);
}

void SolarSystemManagerWindow::resetImportMPC(bool show)
{
	if (show)
		return;

	if (importWindow)
	{
		//TODO:Move this out of here!
		//Reload the list, in case there are new objects
		populateSolarSystemList();

		delete importWindow;
		importWindow = NULL;
	}
}

void SolarSystemManagerWindow::populateSolarSystemList()
{
	//TODO: Display the names instead of identifiers
	QStringList ssoIds = ssoManager->readAllCurrentSsoIds();
	ui->listWidgetObjects->clear();
	ui->listWidgetObjects->addItems(ssoIds);
}

void SolarSystemManagerWindow::removeObject()
{
	if(ui->listWidgetObjects->currentItem())
	{
		QString ssoId = ui->listWidgetObjects->currentItem()->text();
		//qDebug() << ssoId;
		//TODO: Ask for confirmation first?
		ssoManager->removeSsoWithId(ssoId);
	}
}
