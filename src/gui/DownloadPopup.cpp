/*
 *
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

#include "DownloadPopup.hpp"
#include "ui_downloadPopup.h"
#include "StelMainGraphicsView.hpp"
#include <QDialog>

DownloadPopup::DownloadPopup()
{
	ui = new Ui_downloadPopupForm;
}

DownloadPopup::~DownloadPopup()
{
	delete ui;
}

void DownloadPopup::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void DownloadPopup::styleChanged()
{
	// Nothing for now
}

void DownloadPopup::createDialogContent()
{
	ui->setupUi(dialog);
	connect(ui->cancelButton, SIGNAL(clicked(void)), this, SLOT(cancel(void)));
	connect(ui->continueButton, SIGNAL(clicked(void)), this, SLOT(cont(void)));
}

void DownloadPopup::cancel(void)
{
	emit cancelClicked();
}

void DownloadPopup::cont(void)
{
	emit continueClicked();
}

void DownloadPopup::setText(const QString& text)
{
	ui->label->setText(text);
}
