/*
 * Time zone manager plug-in for Stellarium
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DefineTimeZoneWindow.hpp"
#include "ui_defineTimeZone.h"

DefineTimeZoneWindow::DefineTimeZoneWindow()
{
	ui = new Ui_defineTimeZoneForm();
}

DefineTimeZoneWindow::~DefineTimeZoneWindow()
{
	delete ui;
}

void DefineTimeZoneWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void DefineTimeZoneWindow::createDialogContent()
{
	ui->setupUi(dialog);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonUseDefinition, SIGNAL(clicked()), this, SLOT(useDefinition()));
}

void DefineTimeZoneWindow::useDefinition()
{
	emit timeZoneDefined(QString("SCT+0"));
	close();
}
