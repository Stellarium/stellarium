/*
 * Stellarium
 * Copyright (C) 2008 Nigel Kerr
 * Copyright (C) 2012 Timothy Reaves
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

#include "Dialog.hpp"
#include "DateTimeDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelUtils.hpp"
#include "StelGui.hpp"

#include "ui_dateTimeDialogGui.h"

#include <QDebug>
#include <QFrame>
#include <QLineEdit>

DateTimeDialog::DateTimeDialog(QObject* parent) :
	StelDialog("DateTime", parent),
	year(0),
	month(0),
	day(0),
	hour(0),
	minute(0),
	second(0),
	jd(0),
	oldyear(0),
	oldmonth(0),
	oldday(0),
	enableFocus(false)
{
	ui = new Ui_dateTimeDialogForm;
	updateTimer=new QTimer(this); // parenting will auto-delete timer on destruction!
	connect (updateTimer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
	updateTimer->start(20); // must be short enough to allow fast scroll-through.
	core = StelApp::getInstance().getCore();
}

DateTimeDialog::~DateTimeDialog()
{
	delete ui;
	ui=Q_NULLPTR;
}

void DateTimeDialog::createDialogContent()
{
	ui->setupUi(dialog);
	setDateTime(core->getJD());

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectSpinnerEvents();

	connect(ui->spinner_second, &ExternalStepSpinBox::stepsRequested, this, [this](int steps){secondChanged(second+steps);});
	connect(ui->spinner_minute, &ExternalStepSpinBox::stepsRequested, this, [this](int steps){minuteChanged(minute+steps);});
	connect(ui->spinner_hour  , &ExternalStepSpinBox::stepsRequested, this, [this](int steps){  hourChanged(hour  +steps);});
	connect(ui->spinner_day   , &ExternalStepSpinBox::stepsRequested, this, [this](int steps){   dayChanged(day   +steps);});
	connect(ui->spinner_month , &ExternalStepSpinBox::stepsRequested, this, [this](int steps){ monthChanged(month +steps);});

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		connect(gui, SIGNAL(flagEnableFocusOnDaySpinnerChanged(bool)), this, SLOT(setFlagEnableFocus(bool)));
		setFlagEnableFocus(gui->getFlagEnableFocusOnDaySpinner());
	}
}

void DateTimeDialog::setFlagEnableFocus(bool b)
{
	if (enableFocus!=b)
	{
		enableFocus=b;
		if (enableFocus)
			ui->spinner_day->setFocus();
		else
			ui->dateTimeTab->setFocus();
	}
}

void DateTimeDialog::connectSpinnerEvents() const
{
	connect(ui->spinner_year, SIGNAL(valueChanged(int)), this, SLOT(yearChanged(int)));
	connect(ui->spinner_month, SIGNAL(valueChanged(int)), this, SLOT(monthChanged(int)));
	connect(ui->spinner_day, SIGNAL(valueChanged(int)), this, SLOT(dayChanged(int)));
	connect(ui->spinner_hour, SIGNAL(valueChanged(int)), this, SLOT(hourChanged(int)));
	connect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	connect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));
	connect(ui->spinner_jd, SIGNAL(valueChanged(double)), this, SLOT(jdChanged(double)));
	connect(ui->spinner_mjd, SIGNAL(valueChanged(double)), this, SLOT(mjdChanged(double)));
}

void DateTimeDialog::disconnectSpinnerEvents()const
{
	disconnect(ui->spinner_year, SIGNAL(valueChanged(int)), this, SLOT(yearChanged(int)));
	disconnect(ui->spinner_month, SIGNAL(valueChanged(int)), this, SLOT(monthChanged(int)));
	disconnect(ui->spinner_day, SIGNAL(valueChanged(int)), this, SLOT(dayChanged(int)));
	disconnect(ui->spinner_hour, SIGNAL(valueChanged(int)), this, SLOT(hourChanged(int)));
	disconnect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	disconnect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));
	disconnect(ui->spinner_jd, SIGNAL(valueChanged(double)), this, SLOT(jdChanged(double)));
	disconnect(ui->spinner_mjd, SIGNAL(valueChanged(double)), this, SLOT(mjdChanged(double)));
}


//! take in values, adjust for calendrical correctness if needed, and push to
//! the widgets and signals
bool DateTimeDialog::makeValidAndApply(int y, int m, int d, int h, int min, int s)
{
	if (!StelUtils::changeDateTimeForRollover(y, m, d, h, min, s, &year, &month, &day, &hour, &minute, &second)) {
		year =  y;
		month = m;
		day = d;
		hour =  h;
		minute = min;
		second = s;
	}

	pushToWidgets();
	core->setJD(newJd());
	return true;
}

bool DateTimeDialog::applyJD(double jday)
{
	pushToWidgets();
	core->setJD(jday);
	return true;
}

void DateTimeDialog::retranslate()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void DateTimeDialog::close()
{
	ui->dateTimeTab->setFocus();
	StelDialog::close();
}

/************************************************************************
 year slider or dial changed
************************************************************************/

void DateTimeDialog::yearChanged(int newyear)
{
	if ( year != newyear )
	{
		makeValidAndApply( newyear, month, day, hour, minute, second );
	}
}

void DateTimeDialog::monthChanged(int newmonth)
{
	if ( month != newmonth )
	{
		makeValidAndApply( year, newmonth, day, hour, minute, second );
	}
}

void DateTimeDialog::dayChanged(int newday)
{
	int delta = newday - day;
	applyJD(jd + delta);
}

void DateTimeDialog::hourChanged(int newhour)
{
	int delta = newhour - hour;
	applyJD(jd + delta/24.);
}

void DateTimeDialog::minuteChanged(int newminute)
{
	int delta = newminute - minute;
	applyJD(jd + delta/1440.);
}

void DateTimeDialog::secondChanged(int newsecond)
{
	int delta = newsecond - second;
	applyJD(jd + delta/86400.);
}

void DateTimeDialog::jdChanged(double njd)
{
	applyJD(njd);
}

void DateTimeDialog::mjdChanged(double nmjd)
{
	double delta = nmjd - getMjd();
	applyJD(jd + delta);
}

double DateTimeDialog::newJd()
{
	double cjd;
	StelUtils::getJDFromDate(&cjd, year, month, day, hour, minute, static_cast<float>(second));
	cjd -= (core->getUTCOffset(cjd)/24.0); // local tz -> UTC

	return cjd;
}


void DateTimeDialog::pushToWidgets()
{
	disconnectSpinnerEvents();

	// Don't touch spinboxes that don't change. Otherwise it interferes with
	// typing (e.g. when starting a number with leading zero), and sometimes
	// even with stepping (e.g. the user clicks an arrow, but the number
	// remains the same, although the time did change).
	if(ui->spinner_year->value() != year)
		ui->spinner_year->setValue(year);
	if(ui->spinner_month->value() != month)
		ui->spinner_month->setValue(month);
	if(ui->spinner_day->value() != day)
		ui->spinner_day->setValue(day);
	if(ui->spinner_hour->value() != hour)
		ui->spinner_hour->setValue(hour);
	if(ui->spinner_minute->value() != minute)
		ui->spinner_minute->setValue(minute);
	if(ui->spinner_second->value() != second)
		ui->spinner_second->setValue(second);

	ui->spinner_jd->setValue(jd);
	ui->spinner_mjd->setValue(getMjd());

	if (jd<2299161) // 1582-10-15
	{
		ui->dateTimeTab->setToolTip(q_("Date and Time in Julian calendar"));
		ui->dateTimeTabWidget->setTabToolTip(0, q_("Date and Time in Julian calendar"));
	}
	else
	{
		ui->dateTimeTab->setToolTip(q_("Date and Time in Gregorian calendar"));
		ui->dateTimeTabWidget->setTabToolTip(0, q_("Date and Time in Gregorian calendar"));
	}

	connectSpinnerEvents();
}

/************************************************************************
Prepare date elements from newJd and send to spinner_*
 ************************************************************************/
void DateTimeDialog::setDateTime(double newJd)
{
	if (this->visible())
	{
		// JD and MJD should be at the UTC scale on the window!
		double newJdC = newJd + core->getUTCOffset(newJd)/24.0; // UTC -> local tz
		StelUtils::getDateTimeFromJulianDay(newJdC, &year, &month, &day, &hour, &minute, &second);
		jd = newJd;

		pushToWidgets();

		if (oldyear != year || oldmonth != month || oldday != day) 
			emit core->dateChanged();
		if (oldyear != year) 
			emit core->dateChangedByYear(year);
		if (oldmonth != month) 
			emit core->dateChangedForMonth();

		oldyear = year;
		oldmonth = month;
		oldday = day;
	}
}

// handle timer-triggered update
void DateTimeDialog::onTimerTimeout(void)
{
	this->setDateTime(core->getJD());
}
