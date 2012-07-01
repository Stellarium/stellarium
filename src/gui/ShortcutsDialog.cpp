/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
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

#include "StelApp.hpp"

#include "ShortcutsDialog.hpp"
#include "ui_shortcutsDialog.h"


ShortcutsDialog::ShortcutsDialog() :
		ui(new Ui_shortcutsDialogForm)
{
}

ShortcutsDialog::~ShortcutsDialog()
{
		delete ui; ui = NULL;
}

void ShortcutsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateText();
	}
}

void ShortcutsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
//	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	updateText();
}

void ShortcutsDialog::updateText()
{
}
