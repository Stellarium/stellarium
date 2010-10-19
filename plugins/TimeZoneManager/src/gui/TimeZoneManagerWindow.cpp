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

#include "TimeZoneManager.hpp"
#include "TimeZoneManagerWindow.hpp"
#include "DefineTimeZoneWindow.hpp"
#include "ui_timeZoneManagerWindow.h"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"

TimeZoneManagerWindow::TimeZoneManagerWindow()
{
	ui = new Ui_timeZoneManagerWindowForm();
	timeZoneManager = GETSTELMODULE(TimeZoneManager);
	defineTimeZoneWindow = NULL;
}

TimeZoneManagerWindow::~TimeZoneManagerWindow()
{
	delete ui;
	if (defineTimeZoneWindow)
		delete defineTimeZoneWindow;
}

void TimeZoneManagerWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void TimeZoneManagerWindow::createDialogContent()
{
	ui->setupUi(dialog);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveSettings()));
	connect(ui->pushButtonEditTimeZone, SIGNAL(clicked()), this, SLOT(openDefineTimeZoneWindow()));

	QString currentTimeZoneString = timeZoneManager->readTimeZone();
	ui->lineEditCurrent->setText(currentTimeZoneString);
	ui->lineEditUserDefined->setText(currentTimeZoneString);
	ui->frameUserDefined->setEnabled(false);

	QRegExp tzTimeZoneDescription("^SCT([+-])(\\d\\d):(\\d\\d):(\\d\\d)$");
	if (tzTimeZoneDescription.indexIn(currentTimeZoneString) == 0)
	{
		ui->radioButtonOffset->setChecked(true);

		//Offset is POSIX style: UTF - local time = offset,
		//so invert the sign:
		const int sign = (tzTimeZoneDescription.cap(1).at(0).toAscii() == '-') ? 1 : -1;
		const int hours = tzTimeZoneDescription.cap(2).toInt();
		const int minutes = tzTimeZoneDescription.cap(3).toInt();
		const int seconds = tzTimeZoneDescription.cap(4).toInt();
		const double offset = sign * (hours + floor(((minutes * 60 + seconds)/3600.0) * 100) / 100.0);//Round to the second digit
		ui->doubleSpinBoxOffset->setValue(offset);
	}
	else if (currentTimeZoneString == "SCT+0")
	{
		ui->radioButtonUtc->setChecked(true);
	}
	else if (currentTimeZoneString == "system_default")
	{
		ui->radioButtonLocalSettings->setChecked(true);
	}
	else
	{
		ui->radioButtonUserDefined->setChecked(true);
	}
}

void TimeZoneManagerWindow::saveSettings()
{
	QString timeZoneString;
	if (ui->radioButtonUtc->isChecked())
	{
		timeZoneString = "SCT+0";//"Stellarium Custom Time" :)
	}
	else if (ui->radioButtonOffset->isChecked())
	{
		int offset = ui->doubleSpinBoxOffset->value() * 3600;
		//Offset is POSIX style: UTF - local time = offset,
		//so invert the sign:
		QChar offsetSign = (offset > 0) ? '-' : '+';
		offset = abs(offset);
		int offsetSeconds = offset % 60; offset /= 60.;
		int offsetMinutes = offset % 60; offset /= 60.;
		int offsetHours = offset;
		timeZoneString = QString("SCT%1%2:%3:%4").arg(offsetSign).arg(offsetHours, 2, 10, QChar('0')).arg(offsetMinutes, 2, 10, QChar('0')).arg(offsetSeconds, 2, 10, QChar('0'));
	}
	else
	{
		timeZoneString = "system_default";
	}

	timeZoneManager->setTimeZone(timeZoneString);
}

void TimeZoneManagerWindow::openDefineTimeZoneWindow()
{
	dialog->setEnabled(false);

	if (defineTimeZoneWindow == NULL)
	{
		defineTimeZoneWindow = new DefineTimeZoneWindow();
		connect(defineTimeZoneWindow, SIGNAL(timeZoneDefined(QString)), this, SLOT(timeZoneDefined(QString)));
		connect(defineTimeZoneWindow, SIGNAL(visibleChanged(bool)), this, SLOT(closeDefineTimeZoneWindow(bool)));
	}

	defineTimeZoneWindow->setVisible(true);
}

void TimeZoneManagerWindow::closeDefineTimeZoneWindow(bool show)
{
	if (show)
		return;

	disconnect(defineTimeZoneWindow, SIGNAL(timeZoneDefined(QString)), this, SLOT(timeZoneDefined(QString)));
	disconnect(defineTimeZoneWindow, SIGNAL(visibleChanged(bool)), this, SLOT(closeDefineTimeZoneWindow(bool)));
	delete defineTimeZoneWindow;
	defineTimeZoneWindow = NULL;

	dialog->setEnabled(true);
}

void TimeZoneManagerWindow::timeZoneDefined(QString timeZoneDefinition)
{
	ui->lineEditUserDefined->setText(timeZoneDefinition);
}
