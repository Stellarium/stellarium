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
#include "StelTranslator.hpp"

#include <cmath>
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

void DefineTimeZoneWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		int startDateMonthIndex = ui->comboBoxDstStartDateMonth->currentIndex();
		int startDayIndex = ui->comboBoxDstStartDay->currentIndex();
		int startMonthIndex = ui->comboBoxDstStartMonth->currentIndex();
		int startWeekIndex = ui->comboBoxDstStartWeek->currentIndex();
		int endDateMonthIndex = ui->comboBoxDstEndDateMonth->currentIndex();
		int endDayIndex = ui->comboBoxDstEndDay->currentIndex();
		int endMonthIndex = ui->comboBoxDstEndMonth->currentIndex();
		int endWeekIndex = ui->comboBoxDstEndWeek->currentIndex();
		populateDateLists();
		ui->comboBoxDstStartDateMonth->setCurrentIndex(startDateMonthIndex);
		ui->comboBoxDstStartDay->setCurrentIndex(startDayIndex);
		ui->comboBoxDstStartMonth->setCurrentIndex(startMonthIndex);
		ui->comboBoxDstStartWeek->setCurrentIndex(startWeekIndex);
		ui->comboBoxDstEndDateMonth->setCurrentIndex(endDateMonthIndex);
		ui->comboBoxDstEndDay->setCurrentIndex(endDayIndex);
		ui->comboBoxDstEndMonth->setCurrentIndex(endMonthIndex);
		ui->comboBoxDstEndWeek->setCurrentIndex(endWeekIndex);
	}
}

void DefineTimeZoneWindow::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(retranslate()));

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->pushButtonUseDefinition, SIGNAL(clicked()),
	        this, SLOT(useDefinition()));

	connect(ui->doubleSpinBoxOffset, SIGNAL(valueChanged(double)),
	        this, SLOT(updateDstOffset(double)));

	connect(ui->comboBoxDstStartDateMonth, SIGNAL(currentIndexChanged(int)),
	        this, SLOT(updateDayNumberMaximumDstStart(int)));
	connect(ui->comboBoxDstEndDateMonth, SIGNAL(currentIndexChanged(int)),
	        this, SLOT(updateDayNumberMaximumDstEnd(int)));

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
		if (ui->checkBoxOffsetDst->isChecked())
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
	monthList.append(q_("January"));
	monthList.append(q_("February"));
	monthList.append(q_("March"));
	monthList.append(q_("April"));
	monthList.append(q_("May"));
	monthList.append(q_("June"));
	monthList.append(q_("July"));
	monthList.append(q_("August"));
	monthList.append(q_("September"));
	monthList.append(q_("October"));
	monthList.append(q_("November"));
	monthList.append(q_("December"));

	ui->comboBoxDstStartMonth->clear();
	ui->comboBoxDstStartMonth->addItems(monthList);
	ui->comboBoxDstEndMonth->clear();
	ui->comboBoxDstEndMonth->addItems(monthList);
	ui->comboBoxDstStartDateMonth->clear();
	ui->comboBoxDstStartDateMonth->addItems(monthList);
	ui->comboBoxDstEndDateMonth->clear();
	ui->comboBoxDstEndDateMonth->addItems(monthList);

	QStringList weekList;
	weekList.append(q_("First week"));
	weekList.append(q_("Second week"));
	weekList.append(q_("Third week"));
	weekList.append(q_("Fourth week"));
	weekList.append(q_("Last week"));

	ui->comboBoxDstStartWeek->clear();
	ui->comboBoxDstStartWeek->addItems(weekList);
	ui->comboBoxDstEndWeek->clear();
	ui->comboBoxDstEndWeek->addItems(weekList);

	//Starts from Sunday deliberately
	QStringList dayList;
	dayList.append(q_("Sunday"));
	dayList.append(q_("Monday"));
	dayList.append(q_("Tuesday"));
	dayList.append(q_("Wednesday"));
	dayList.append(q_("Thursday"));
	dayList.append(q_("Friday"));
	dayList.append(q_("Saturday"));

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

bool DefineTimeZoneWindow::readDstEndpoint(const QString& string,
                                           DstEndpoint endpoint)
{
	QGroupBox* groupBox = 0;
	QRadioButton* radioButtonDay = 0;
	QRadioButton* radioButtonDate = 0;
	QCheckBox* checkBoxTime = 0;
	QComboBox* comboBoxWeek = 0;
	QComboBox* comboBoxMonth = 0;
	QComboBox* comboBoxDay = 0;
	QTimeEdit* timeEdit = 0;

	if (endpoint == DstStart)
	{
		groupBox = ui->groupBoxDstStart;
		radioButtonDay = ui->radioButtonDstStartDay;
		radioButtonDate = ui->radioButtonDstStartDate;
		checkBoxTime = ui->checkBoxDstStartTime;
		comboBoxWeek = ui->comboBoxDstStartWeek;
		comboBoxMonth = ui->comboBoxDstStartMonth;
		comboBoxDay = ui->comboBoxDstStartDay;
		timeEdit = ui->timeEditDstStart;
	}
	else
	{
		groupBox = ui->groupBoxDstEnd;
		radioButtonDay = ui->radioButtonDstEndDay;
		radioButtonDate = ui->radioButtonDstEndDate;
		checkBoxTime = ui->checkBoxDstEndTime;
		comboBoxWeek = ui->comboBoxDstEndWeek;
		comboBoxMonth = ui->comboBoxDstEndMonth;
		comboBoxDay = ui->comboBoxDstEndDay;
		timeEdit = ui->timeEditDstEnd;
	}

	QRegExp endPointFormat("(J\\d{1,3}|\\d{1,3}|M\\d{1,2}\\.\\d\\.\\d)(?:\\/(\\d{1,2}(?:\\:(?:\\d{1,2})(?:\\:(?:\\d{1,2}))?)?))?");
	if (!endPointFormat.exactMatch(string))
		return false;

	if (endPointFormat.numCaptures() == 2)
	{
		QString timeString = endPointFormat.cap(2).trimmed();
		if (!timeString.isEmpty())
		{
			int hours, minutes, seconds;
			TimeZoneConfigurationWindow::readTzTimeString(timeString,
			                                              hours,
			                                              minutes,
			                                              seconds);
			QTime time(hours, minutes, seconds);
			if (time.isValid())
			{
				checkBoxTime->setChecked(true);
				timeEdit->setTime(time);
			}
			else
				return false;
		}
	}

	QString date = endPointFormat.cap(1);
	if (date.startsWith('J'))
	{
		int julianDay = date.mid(1).toInt();
		if (julianDay < 1 || julianDay > 365)
			return false;

		radioButtonDate->setChecked(true);
		setEndPointDate(julianDay, false, endpoint);
	}
	else if (date.startsWith('M'))
	{
		QStringList list = date.mid(1).split('.');
		if (list.count() != 3)
			return false;

		int month = list[0].toInt();
		if (month < 1 || month > 12)
			return false;
		int week = list[1].toInt();
		if (week < 1 || week > 5)
			return false;
		int day = list[2].toInt();
		if (day < 0 || day > 6)
			return false;

		radioButtonDay->setChecked(true);
		comboBoxMonth->setCurrentIndex(month - 1);
		comboBoxWeek->setCurrentIndex(week - 1);
		comboBoxDay->setCurrentIndex(day);
	}
	else
	{
		int julianDay = date.toInt();
		if (julianDay < 0 || julianDay > 365)
			return false;

		radioButtonDate->setChecked(true);
		setEndPointDate(julianDay+1, true, endpoint);
	}

	groupBox->setChecked(true);
	return true;
}

void DefineTimeZoneWindow::setEndPointDate(int ordinalDate,
                                           bool leapYear,
                                           DstEndpoint endpoint)
{
	QSpinBox* spinBoxDateDay = 0;
	QComboBox* comboBoxDateMonth = 0;
	if (endpoint == DstStart)
	{
		spinBoxDateDay = ui->spinBoxDstStartDateDay;
		comboBoxDateMonth = ui->comboBoxDstStartDateMonth;
	}
	else
	{
		spinBoxDateDay = ui->spinBoxDstEndDateDay;
		comboBoxDateMonth = ui->comboBoxDstEndDateMonth;
	}

	int monthLength[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (leapYear)
		monthLength[1] = 29;
	for (int i = 0; i < 12; i++)
	{
		if (ordinalDate <= monthLength[i])
		{
			spinBoxDateDay->setValue(ordinalDate);
			comboBoxDateMonth->setCurrentIndex(i);
			break;
		}
		else
		{
			ordinalDate -= monthLength[i];
		}
	}
}

void DefineTimeZoneWindow::setTimeZone(QString tzString)
{
	if (dialog == NULL) //As initialized in StelDialog
		return;

	if (tzString.isEmpty())
		return;

	//TZ variable format:
	// - name
	// - signed offset
	// The following is optional:
	// - DST name
	// - DST signed offset
	// - DST beginning
	// - DST end
	QRegExp tzFormat("^([^:\\d,+-/]{3,})([+-]?[\\d:]+)(?:([^:\\d,+-/]{3,})((?:[+-]?[\\d:]+)?),([JM\\d\\./:]+),([JM\\d\\./:]+))?$");
	if (!tzFormat.exactMatch(tzString))
		return;

	double offset = TimeZoneConfigurationWindow::readTzOffsetString(tzFormat.cap(2));
	if (fabs(offset) >= 24)
		return;

	QString name = tzFormat.cap(1).trimmed();
	if (name.isEmpty())
		return;

	ui->lineEditName->setText(name);
	ui->doubleSpinBoxOffset->setValue(offset);

	//TODO: Clean this up. The offset and the end point are not obligatory?
	int count = tzFormat.captureCount();
	if (count > 2)
	{
		QString dstName = tzFormat.cap(3).trimmed();
		if (dstName.isEmpty())
			return;
		QString dstOffsetString = tzFormat.cap(4).trimmed();
		if (!dstOffsetString.isEmpty())
		{
			double dstOffset = TimeZoneConfigurationWindow::readTzOffsetString(dstOffsetString);
			if (fabs(dstOffset) >= 24)
				return;

			ui->checkBoxOffsetDst->setChecked(true);
			ui->doubleSpinBoxOffsetDst->setValue(dstOffset);
		}
		else
		{
			ui->checkBoxOffsetDst->setChecked(false);
			ui->doubleSpinBoxOffsetDst->setValue(offset + 1);
		}

		QString dstStartString = tzFormat.cap(5).trimmed();
		if (!readDstEndpoint(dstStartString, DstStart))
			return;
		QString dstEndString = tzFormat.cap(6).trimmed();
		if (!readDstEndpoint(dstEndString, DstEnd))
			return;

		ui->checkBoxDst->setChecked(true);
		ui->lineEditNameDst->setText(dstName);
	}


}
