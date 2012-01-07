/*
 * Stellarium Satellites Plug-in: satellites import feature
 * Copyright (C) 2012 Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "SatellitesImportDialog.hpp"
#include "ui_importSatellitesWindow.h"

SatellitesImportDialog::SatellitesImportDialog()
{
	ui = new Ui_satellitesImportDialog;
}

SatellitesImportDialog::~SatellitesImportDialog()
{
	delete ui;
}

void SatellitesImportDialog::languageChanged()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void SatellitesImportDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	connect(ui->closeStelWindow, SIGNAL(clicked()),
	        this, SLOT(close()));
	
	connect(ui->pushButtonGetData, SIGNAL(clicked()),
	        this, SLOT(getData()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()),
	        this, SLOT(acceptNewSatellites()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()),
	        this, SLOT(discardNewSatellites()));
	
	reset();
}

void SatellitesImportDialog::getData()
{
	//
}

void SatellitesImportDialog::acceptNewSatellites()
{
	close();
}

void SatellitesImportDialog::discardNewSatellites()
{
	close();
}

void SatellitesImportDialog::reset()
{
	//
}
