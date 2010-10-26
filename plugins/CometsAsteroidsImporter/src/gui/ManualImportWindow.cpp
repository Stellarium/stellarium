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

#include "ManualImportWindow.hpp"
#include "ui_manualImportWindow.h"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"


ManualImportWindow::ManualImportWindow()
{
	ui = new Ui_manualImportWindow();
	ssoManager = GETSTELMODULE(CAImporter);
}

ManualImportWindow::~ManualImportWindow()
{
	delete ui;
}

void ManualImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	//ui->labelEccentricity->setText(QString("Eccentricity (%1):").arg(QChar(0x1D452)));//Doesn't work: wrong symbol?
	ui->labelLongitudeOfTheAscendingNode->setText(QString("Longitude of the ascending node (%1):").arg(QChar(0x03A9)));//Capital omega
}

void ManualImportWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}
