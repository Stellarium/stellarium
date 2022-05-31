/*
 * Stellarium Satellites Plug-in: satellites communications editor
 * Copyright (C) 2022 Alexander Wolf
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

#include "SatellitesCommDialog.hpp"
#include "ui_satellitesCommDialog.h"

#include "StelApp.hpp"
#include "StelTranslator.hpp"

SatellitesCommDialog::SatellitesCommDialog()
	: StelDialog("SatellitesComms")
{
	ui = new Ui_satellitesCommDialog;
}

SatellitesCommDialog::~SatellitesCommDialog()
{
	delete ui;
}

void SatellitesCommDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void SatellitesCommDialog::setVisible(bool visible)
{
	StelDialog::setVisible(visible);
}

void SatellitesCommDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
}
