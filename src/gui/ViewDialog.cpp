/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#include "Dialog.hpp"
#include "ViewDialog.hpp"
#include "StelMainWindow.hpp"
#include "ui_viewDialog.h"
#include "StelApp.hpp"
#include "StelCore.hpp"

#include <QDebug>
#include <QFrame>

ViewDialog::ViewDialog() : dialog(0)
{
	ui = new Ui_viewDialogForm;
}

ViewDialog::~ViewDialog()
{
	delete ui;
}

void ViewDialog::close()
{
	emit closed();
}

void ViewDialog::setVisible(bool v)
{
	if (v) 
	{
		dialog = new DialogFrame(&StelMainWindow::getInstance());
		ui->setupUi(dialog);
		dialog->raise();
		dialog->move(200, 100);	// TODO: put in the center of the screen
		dialog->setVisible(true);
		connect(ui->closeView, SIGNAL(clicked()), this, SLOT(close()));
//		connect(ui->longitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(spinBoxChanged(void)));
	}
	else
	{
		dialog->deleteLater();
		dialog = 0;
	}
}
