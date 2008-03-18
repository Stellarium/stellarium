/*
 * Stellarium
 * Copyright (C) 2008 Nigel Kerr
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
#include "DateTimeDialog.hpp"
#include "StelMainWindow.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "Navigator.hpp"
#include "ui_dateTimeDialogGui.h"
#include "TextEntryDateTimeValidator.hpp"

#include <QDebug>
#include <QFrame>

DateTimeDialog::DateTimeDialog() : dialog(0)
{
	ui = new Ui_dateTimeDialogForm;
}

void DateTimeDialog::close()
{
	emit closed();
}

void DateTimeDialog::setVisible(bool v)
{
	if (v)
	{
		dialog = new DialogFrame(&StelMainWindow::getInstance());
		ui->setupUi(dialog);
		dialog->raise();
		dialog->move(200, 200);	// TODO: put in the center of the screen
		dialog->setVisible(true);
		connect(ui->closeDateTime, SIGNAL(clicked()), this, SLOT(close()));

		QValidator * val = new TextEntryDateTimeValidator(this);
		ui->time8601->setValidator(val);
		setDateTime(StelApp::getInstance().getCore()->getNavigation()->getJDay());
		connect(ui->time8601, SIGNAL(textEdited(const QString &)), this, SLOT(dateTimeEdited(const QString &)));
		connect(this, SIGNAL(dateTimeChanged(double)), StelApp::getInstance().getCore()->getNavigation(), SLOT(setJDay(double)));
	}
	else
	{
		disconnect(ui->time8601);
		disconnect(this, 0, StelApp::getInstance().getCore()->getNavigation(), 0);
		dialog->deleteLater();
		dialog = 0;
	}
}

/************************************************************************
Send along new JD value from time8601's QString to to connected slots.
 ************************************************************************/
void DateTimeDialog::dateTimeEdited(const QString &v)
{
	qDebug() << "dateTimeEdited " << v ;
	double jd = StelApp::getInstance().getLocaleMgr().get_jd_from_ISO8601_time_local(v);
	emit dateTimeChanged(jd);
}

/************************************************************************
Send newJd to time8601 as a ISO-8601-formatted QString.
 ************************************************************************/
void DateTimeDialog::setDateTime(double newJd)
{
	ui->time8601->setText(StelApp::getInstance().getLocaleMgr().get_ISO8601_time_local(newJd).replace("T"," "));
}

