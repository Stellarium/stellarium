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

#include "TimeZoneConfigurationWindow.hpp"
#include "DefineTimeZoneWindow.hpp"
#include "ui_defineTimeZone.h"
#include "StelApp.hpp"

#include <QRegExpValidator>

DefineTimeZoneWindow::DefineTimeZoneWindow()
{
	ui = new Ui_defineTimeZoneForm();
	timeZoneNameValidator = new QRegExpValidator(QRegExp("[^:\\d,+-/]{3,}"), this);
}

DefineTimeZoneWindow::~DefineTimeZoneWindow()
{
	delete ui;
	delete timeZoneNameValidator;
}

void DefineTimeZoneWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void DefineTimeZoneWindow::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonUseDefinition, SIGNAL(clicked()), this, SLOT(useDefinition()));

	connect(ui->doubleSpinBoxOffset, SIGNAL(valueChanged(double)), this, SLOT(updateDstOffset(double)));

	connect(ui->comboBoxDstStartDateMonth, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDayNumberMaximumDstStart(int)));
	connect(ui->comboBoxDstEndDateMonth, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDayNumberMaximumDstEnd(int)));

	ui->lineEditName->setValidator(timeZoneNameValidator);
	ui->lineEditNameDst->setValidator(timeZoneNameValidator);

	resetWindowState();
}

void DefineTimeZoneWindow::useDefinition()
{
	QString definition;
	QString timeZoneName = ui->lineEditName->text();
	if (timeZoneName.length() < 3)
	{
		return;
	}
	definition.append(timeZoneName);
	definition.append(TimeZoneConfigurationWindow::getTzOffsetStringFrom(ui->doubleSpinBoxOffset));

	//Daylight saving time
	if (ui->checkBoxDst->isChecked())
	{
		QString dstTimeZoneName = ui->lineEditNameDst->text();
		if (dstTimeZoneName.length() < 3 || dstTimeZoneName == timeZoneName)
		{
			return;
		}
		//The name is the minimum required for DST
		definition.append(dstTimeZoneName);

		//The offset is not necessary
		if (ui->checkBoxOffsetDst)
		{
			definition.append(TimeZoneConfigurationWindow::getTzOffsetStringFrom(ui->doubleSpinBoxOffsetDst));
		}

		if (ui->groupBoxDstStart->isChecked() && ui->groupBoxDstEnd->isChecked())
		{
			if (ui->radioButtonDstStartDate->isChecked())
			{
				const int month = ui->comboBoxDstStartDateMonth->currentIndex() + 1;
				const int day = ui->spinBoxDstStartDateDay->value();
				QDate startDate;
				if (month == 2 && day == 29)
				{
					//Leap year: day is indexed between 0 and 365
					startDate.setDate(2000, month, day);
					definition.append(QString(",%1").arg(startDate.dayOfYear() - 1));
				}
				else
				{
					startDate.setDate(2010, month, day);
					definition.append(QString(",J%1").arg(startDate.dayOfYear()));
				}
			}
			else //Day of week
			{
				//Day of the week: 0-6, 0 is Sunday
				int day  = ui->comboBoxDstStartDay->currentIndex();
				//Week ordinal number: 1-5, 5 is "last"
				int week = ui->comboBoxDstStartWeek->currentIndex() + 1;
				//Month: 1-12
				int month = ui->comboBoxDstStartMonth->currentIndex() + 1;

				definition.append(QString(",M%1.%2.%3").arg(month).arg(week).arg(day));
			}

			if (ui->checkBoxDstStartTime->isChecked())
			{
				definition.append(ui->timeEditDstStart->time().toString("'/'hh:mm:ss"));
			}

			if (ui->radioButtonDstEndDate->isChecked())
			{
				const int month = ui->comboBoxDstEndDateMonth->currentIndex() + 1;
				const int day = ui->spinBoxDstEndDateDay->value();
				QDate endDate;
				if (month == 2 && day == 29)
				{
					//Leap year: day is indexed between 0 and 365
					endDate.setDate(2000, month, day);
					definition.append(QString(",%1").arg(endDate.dayOfYear() - 1));
				}
				else
				{
					endDate.setDate(2010, month, day);
					definition.append(QString(",J%1").arg(endDate.dayOfYear()));
				}
			}
			else //Day of week
			{
				//Day of the week: 0-6, 0 is Sunday
				int day  = ui->comboBoxDstEndDay->currentIndex();
				//Week ordinal number: 1-5, 5 is "last"
				int week = ui->comboBoxDstEndWeek->currentIndex() + 1;
				//Month: 1-12
				int month = ui->comboBoxDstEndMonth->currentIndex() + 1;

				definition.append(QString(",M%1.%2.%3").arg(month).arg(week).arg(day));
			}

			if (ui->checkBoxDstEndTime->isChecked())
			{
				definition.append(ui->timeEditDstEnd->time().toString("'/'hh:mm:ss"));
			}
		}
	}

	emit timeZoneDefined(definition);
	close();
}

void DefineTimeZoneWindow::updateDstOffset(double normalOffset)
{
	if (ui->checkBoxOffsetDst->isChecked())
		return;

	//By default, the DST offset is +1 hour the normal offset
	ui->doubleSpinBoxOffsetDst->setValue(normalOffset + 1.0);
}

void DefineTimeZoneWindow::resetWindowState()
{
	populateDateLists();

	//Default section
	ui->lineEditName->clear();
	ui->lineEditNameDst->clear();

	ui->doubleSpinBoxOffset->setValue(0.0);
	//(indirectly sets doubleSpinBoxOffsetDst)

	ui->checkBoxDst->setChecked(false);
	ui->frameDst->setVisible(false);

	ui->checkBoxOffsetDst->setChecked(false);
	ui->doubleSpinBoxOffsetDst->setEnabled(false);

	ui->groupBoxDstStart->setChecked(false);
	//(indirectly sets the other one)

	ui->radioButtonDstStartDay->setChecked(true);
	ui->radioButtonDstEndDay->setChecked(true);

	ui->spinBoxDstStartDateDay->setValue(1);
	ui->spinBoxDstEndDateDay->setValue(1);
	ui->comboBoxDstStartDateMonth->setCurrentIndex(0);
	ui->comboBoxDstEndDateMonth->setCurrentIndex(0);

	ui->checkBoxDstStartTime->setChecked(false);
	ui->timeEditDstStart->setEnabled(false);
	ui->timeEditDstStart->setTime(QTime(2, 0, 0, 0));
	ui->checkBoxDstEndTime->setChecked(false);
	ui->timeEditDstEnd->setEnabled(false);
	ui->timeEditDstEnd->setTime(QTime(2, 0, 0, 0));
}

void DefineTimeZoneWindow::populateDateLists()
{
	QStringList monthList;
	monthList.append("January");
	monthList.append("February");
	monthList.append("March");
	monthList.append("April");
	monthList.append("May");
	monthList.append("June");
	monthList.append("July");
	monthList.append("August");
	monthList.append("September");
	monthList.append("October");
	monthList.append("November");
	monthList.append("December");

	ui->comboBoxDstStartMonth->clear();
	ui->comboBoxDstStartMonth->addItems(monthList);
	ui->comboBoxDstEndMonth->clear();
	ui->comboBoxDstEndMonth->addItems(monthList);
	ui->comboBoxDstStartDateMonth->clear();
	ui->comboBoxDstStartDateMonth->addItems(monthList);
	ui->comboBoxDstEndDateMonth->clear();
	ui->comboBoxDstEndDateMonth->addItems(monthList);

	//TODO: For the translators: refers to any day of the week, if not possible, translate as "First week"
	QStringList weekList;
	weekList.append("First");
	weekList.append("Second");
	weekList.append("Third");
	weekList.append("Fourth");
	weekList.append("Last");

	ui->comboBoxDstStartWeek->clear();
	ui->comboBoxDstStartWeek->addItems(weekList);
	ui->comboBoxDstEndWeek->clear();
	ui->comboBoxDstEndWeek->addItems(weekList);

	//Starts from Sunday deliberately
	QStringList dayList;
	dayList.append("Sunday");
	dayList.append("Monday");
	dayList.append("Tuesday");
	dayList.append("Wednesday");
	dayList.append("Thursday");
	dayList.append("Friday");
	dayList.append("Saturday");

	ui->comboBoxDstStartDay->clear();
	ui->comboBoxDstStartDay->addItems(dayList);
	ui->comboBoxDstEndDay->clear();
	ui->comboBoxDstEndDay->addItems(dayList);
}
void DefineTimeZoneWindow::updateDayNumberMaximum(int monthIndex, QSpinBox *spinBoxDay)
{
	int maximum = 31;
	switch (monthIndex)
	{
	case 0: //January
	case 2: //March
	case 4: //May
	case 6: //July
	case 7: //August
	case 9: //October
	case 11: //December
		maximum = 31;
		break;
	case 3: //April
	case 5: //June
	case 8: //September
	case 10: //November
		maximum = 30;
		break;
	case 1: //February
		maximum = 29;
		break;
	default:
		;//
	}

	if (spinBoxDay->value() > maximum)
		spinBoxDay->setValue(maximum);
	spinBoxDay->setRange(1, maximum);
}

void DefineTimeZoneWindow::updateDayNumberMaximumDstStart(int monthIndex)
{
	updateDayNumberMaximum(monthIndex, ui->spinBoxDstStartDateDay);
}

void DefineTimeZoneWindow::updateDayNumberMaximumDstEnd(int monthIndex)
{
	updateDayNumberMaximum(monthIndex, ui->spinBoxDstEndDateDay);
}
