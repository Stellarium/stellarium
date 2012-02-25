/*
 * Time zone configuration plug-in for Stellarium
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

#include "TimeZoneConfiguration.hpp"
#include "TimeZoneConfigurationWindow.hpp"
#include "DefineTimeZoneWindow.hpp"
#include "ui_timeZoneConfigurationWindow.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"

TimeZoneConfigurationWindow::TimeZoneConfigurationWindow()
{
	ui = new Ui_timeZoneConfigurationWindowForm();
	timeZoneConfiguration = GETSTELMODULE(TimeZoneConfiguration);
	defineTimeZoneWindow = NULL;
}

TimeZoneConfigurationWindow::~TimeZoneConfigurationWindow()
{
	delete ui;
	if (defineTimeZoneWindow)
		delete defineTimeZoneWindow;
}

void TimeZoneConfigurationWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateAboutText();
	}
}

void TimeZoneConfigurationWindow::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveTimeZoneSettings()));
	connect(ui->pushButtonEditTimeZone, SIGNAL(clicked()), this, SLOT(openDefineTimeZoneWindow()));

	connect(ui->radioButtonTimeDefault, SIGNAL(toggled(bool)), this, SLOT(setTimeFormat(bool)));
	connect(ui->radioButtonTime12Hour, SIGNAL(toggled(bool)), this, SLOT(setTimeFormat(bool)));
	connect(ui->radioButtonTime24Hour, SIGNAL(toggled(bool)), this, SLOT(setTimeFormat(bool)));

	connect(ui->radioButtonDateDefault, SIGNAL(toggled(bool)), this, SLOT(setDateFormat(bool)));
	connect(ui->radioButtonDateDMY, SIGNAL(toggled(bool)), this, SLOT(setDateFormat(bool)));
	connect(ui->radioButtonDateMDY, SIGNAL(toggled(bool)), this, SLOT(setDateFormat(bool)));
	connect(ui->radioButtonDateYMD, SIGNAL(toggled(bool)), this, SLOT(setDateFormat(bool)));

	updateDisplayFormatSwitches();

	QString currentTimeZoneString = timeZoneConfiguration->readTimeZone();
	//ui->lineEditCurrent->setText(currentTimeZoneString);
	ui->lineEditUserDefined->setText(currentTimeZoneString);
	ui->frameUserDefined->setEnabled(false);

	if (currentTimeZoneString.startsWith("SCT"))
	{
		if (currentTimeZoneString == "SCT+0")
		{
			ui->radioButtonUtc->setChecked(true);
		}
		else
		{
			ui->radioButtonOffset->setChecked(true);
			double offset = readTzOffsetString(currentTimeZoneString.mid(3));
			ui->doubleSpinBoxOffset->setValue(offset);
		}
	}
	else if (currentTimeZoneString == "system_default")
	{
		ui->radioButtonLocalSettings->setChecked(true);
	}
	else
	{
		ui->radioButtonUserDefined->setChecked(true);
	}

	updateAboutText();
}

void TimeZoneConfigurationWindow::updateAboutText()
{
	ui->labelTitle->setText(q_("Time Zone plug-in"));
	QString version = QString(q_("Version %1")).arg(TIME_ZONE_CONFIGURATION_VERSION);
	ui->labelVersion->setText(version);
}

void TimeZoneConfigurationWindow::saveTimeZoneSettings()
{
	QString timeZoneString;
	if (ui->radioButtonUtc->isChecked())
	{
		timeZoneString = "SCT+0";//"Stellarium Custom Time" :)
	}
	else if (ui->radioButtonOffset->isChecked())
	{
		timeZoneString = QString("SCT").append(getTzOffsetStringFrom(ui->doubleSpinBoxOffset));
	}
	else if (ui->radioButtonUserDefined->isChecked())
	{
		timeZoneString = ui->lineEditUserDefined->text();
	}
	else
	{
		timeZoneString = "system_default";
	}

	timeZoneConfiguration->setTimeZone(timeZoneString);
}

void TimeZoneConfigurationWindow::openDefineTimeZoneWindow()
{
	dialog->setEnabled(false);

	if (defineTimeZoneWindow == NULL)
	{
		defineTimeZoneWindow = new DefineTimeZoneWindow();
		connect(defineTimeZoneWindow, SIGNAL(timeZoneDefined(QString)), this, SLOT(timeZoneDefined(QString)));
		connect(defineTimeZoneWindow, SIGNAL(visibleChanged(bool)), this, SLOT(closeDefineTimeZoneWindow(bool)));
	}

	defineTimeZoneWindow->setVisible(true);
	defineTimeZoneWindow->setTimeZone(ui->lineEditUserDefined->text());
}

void TimeZoneConfigurationWindow::closeDefineTimeZoneWindow(bool show)
{
	if (show)
		return;

	disconnect(defineTimeZoneWindow, SIGNAL(timeZoneDefined(QString)), this, SLOT(timeZoneDefined(QString)));
	disconnect(defineTimeZoneWindow, SIGNAL(visibleChanged(bool)), this, SLOT(closeDefineTimeZoneWindow(bool)));
	delete defineTimeZoneWindow;
	defineTimeZoneWindow = NULL;

	dialog->setEnabled(true);
}

void TimeZoneConfigurationWindow::timeZoneDefined(QString timeZoneDefinition)
{
	ui->lineEditUserDefined->setText(timeZoneDefinition);
}

double TimeZoneConfigurationWindow::readTzOffsetString(const QString& string)
{
	const QChar signChar = string.at(0);
	//Offset is POSIX style: UTF - local time = offset,
	//so invert the sign:
	const int sign = (signChar == '-') ? 1 : -1;
	int hours, minutes, seconds;
	readTzTimeString((signChar.isDigit()) ? string : string.mid(1),
	                 hours, minutes, seconds);
	const double offset = sign * (hours + floor(((minutes * 60 + seconds)/3600.0) * 100) / 100.0);//Round to the second digit
	return offset;
}

void TimeZoneConfigurationWindow::readTzTimeString(const QString& string,
                                                   int& hours,
                                                   int& minutes,
                                                   int& seconds)
{
	hours = 0;
	minutes = 0;
	seconds = 0;

	QRegExp tzTimeFormat("(\\d{1,2})(?:\\:(\\d{1,2})(?:\\:(\\d{1,2}))?)?");
	if (!tzTimeFormat.exactMatch(string))
		return;

	int count = tzTimeFormat.captureCount();
	switch (count)
	{
		case 3:
			seconds = tzTimeFormat.cap(3).toInt();
			//Fallthrough!
		case 2:
			minutes = tzTimeFormat.cap(2).toInt();
			//Fallthrough!
		case 1:
		default:
			hours = tzTimeFormat.cap(1).toInt();
	}
}

QString TimeZoneConfigurationWindow::getTzOffsetStringFrom(QDoubleSpinBox * spinBox)
{
	Q_ASSERT(spinBox);

	int offset = spinBox->value() * 3600;
	//Offset is POSIX style: UTC - local time = offset,
	//so invert the sign:
	QChar offsetSign = (offset > 0) ? '-' : '+';
	offset = abs(offset);
	int offsetSeconds = offset % 60; offset /= 60.;
	int offsetMinutes = offset % 60; offset /= 60.;
	int offsetHours = offset;

	return QString("%1%2:%3:%4").arg(offsetSign).arg(offsetHours, 2, 10, QChar('0')).arg(offsetMinutes, 2, 10, QChar('0')).arg(offsetSeconds, 2, 10, QChar('0'));
}

void TimeZoneConfigurationWindow::updateDisplayFormatSwitches()
{
	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();

	QString timeFormat = localeManager.getTimeFormatStr();
	if (timeFormat == "12h")
		ui->radioButtonTime12Hour->setChecked(true);
	else if (timeFormat == "24h")
		ui->radioButtonTime24Hour->setChecked(true);
	else
		ui->radioButtonTimeDefault->setChecked(true);

	QString dateFormat = localeManager.getDateFormatStr();
	if (dateFormat == "yyyymmdd")
		ui->radioButtonDateYMD->setChecked(true);
	else if (dateFormat == "ddmmyyyy")
		ui->radioButtonDateDMY->setChecked(true);
	else if (dateFormat == "mmddyyyy")
		ui->radioButtonDateMDY->setChecked(true);
	else
		ui->radioButtonDateDefault->setChecked(true);
}

void TimeZoneConfigurationWindow::setTimeFormat(bool)
{
	//TODO: This will break if the settings' format is changed.
	//It's a pity StelLocaleMgr::sTimeFormatToString() is private...
	QString selectedFormat;
	if (ui->radioButtonTime12Hour->isChecked())
		selectedFormat = "12h";
	else if (ui->radioButtonTime24Hour->isChecked())
		selectedFormat = "24h";
	else
		selectedFormat = "system_default";

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();
	if (selectedFormat == localeManager.getTimeFormatStr())
		return;
	localeManager.setTimeFormatStr(selectedFormat);
	timeZoneConfiguration->saveDisplayFormats();
}

void TimeZoneConfigurationWindow::setDateFormat(bool)
{
	//TODO: This will break if the settings' format is changed.
	//It's a pity StelLocaleMgr::sDateFormatToString() is private...
	QString selectedFormat;
	if (ui->radioButtonDateYMD->isChecked())
		selectedFormat = "yyyymmdd";
	else if (ui->radioButtonDateDMY->isChecked())
		selectedFormat = "ddmmyyyy";
	else if (ui->radioButtonDateMDY->isChecked())
		selectedFormat = "mmddyyyy";
	else
		selectedFormat = "system_default";

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();
	if (selectedFormat == localeManager.getDateFormatStr())
		return;
	localeManager.setDateFormatStr(selectedFormat);
	timeZoneConfiguration->saveDisplayFormats();
}
