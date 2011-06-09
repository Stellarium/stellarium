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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"

#include "ui_dateTimeDialogGui.h"

#include <QDebug>
#include <QFrame>
#include <QLineEdit>

DateTimeDialog::DateTimeDialog() :
  year(0),
  month(0),
  day(0),
  hour(0),
  minute(0),
  second(0)
{
	ui = new Ui_dateTimeDialogForm;
}

DateTimeDialog::~DateTimeDialog()
{
	delete ui;
	ui=NULL;
}

void DateTimeDialog::createDialogContent()
{
	ui->setupUi(dialog);
	double jd = StelApp::getInstance().getCore()->getJDay();
	setDateTime(jd + (StelApp::getInstance().getLocaleMgr().getGMTShift(jd)/24.0)); // UTC -> local tz

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->spinner_year, SIGNAL(valueChanged(int)), this, SLOT(yearChanged(int)));
	connect(ui->spinner_month, SIGNAL(valueChanged(int)), this, SLOT(monthChanged(int)));
	connect(ui->spinner_day, SIGNAL(valueChanged(int)), this, SLOT(dayChanged(int)));
	connect(ui->spinner_hour, SIGNAL(valueChanged(int)), this, SLOT(hourChanged(int)));
	connect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	connect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));

	connect(this, SIGNAL(dateTimeChanged(double)), StelApp::getInstance().getCore(), SLOT(setJDay(double)));
}

//! take in values, adjust for calendrical correctness if needed, and push to
//! the widgets and signals
bool DateTimeDialog::valid(int y, int m, int d, int h, int min, int s)
{
	int dy, dm, dd, dh, dmin, ds;

	if ( ! StelUtils::changeDateTimeForRollover(y, m, d, h, min, s, &dy, &dm, &dd, &dh, &dmin, &ds) )
	{
		dy = y;
		dm = m;
		dd = d;
		dh = h;
		dmin = min;
		ds = s;
	}

	year = dy;
	month = dm;
	day = dd;
	hour = dh;
	minute = dmin;
	second = ds;
	pushToWidgets();
	emit dateTimeChanged(newJd());
	return true;
}

void DateTimeDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void DateTimeDialog::styleChanged()
{
	// Nothing for now
}

void DateTimeDialog::close()
{
	ui->dateTimeBox->setFocus();
	StelDialog::close();
}

/************************************************************************
 year slider or dial changed
************************************************************************/

void DateTimeDialog::yearChanged(int newyear)
{
  if ( year != newyear ) {
	valid( newyear, month, day, hour, minute, second );
  }
}
void DateTimeDialog::monthChanged(int newmonth)
{
  if ( month != newmonth ) {
	valid( year, newmonth, day, hour, minute, second );
  }
}
void DateTimeDialog::dayChanged(int newday)
{
  if ( day != newday ) {
	valid( year, month, newday, hour, minute, second );
  }
}
void DateTimeDialog::hourChanged(int newhour)
{
  if ( hour != newhour ) {
	valid( year, month, day, newhour, minute, second );
  }
}
void DateTimeDialog::minuteChanged(int newminute)
{
  if ( minute != newminute ) {
	valid( year, month, day, hour, newminute, second );
  }
}
void DateTimeDialog::secondChanged(int newsecond)
{
  if ( second != newsecond ) {
	valid( year, month, day, hour, minute, newsecond );
  }
}

double DateTimeDialog::newJd()
{
  double jd;
  StelUtils::getJDFromDate(&jd,year, month, day, hour, minute, second);
  jd -= (StelApp::getInstance().getLocaleMgr().getGMTShift(jd)/24.0); // local tz -> UTC
  return jd;
}

void DateTimeDialog::pushToWidgets()
{
  ui->spinner_year->setValue(year);
  ui->spinner_month->setValue(month);
  ui->spinner_day->setValue(day);
  ui->spinner_hour->setValue(hour);
  if (!ui->spinner_minute->hasFocus() || (ui->spinner_minute->value() == -1) || (ui->spinner_minute->value() == 60))
    ui->spinner_minute->setValue(minute);
  if (!ui->spinner_second->hasFocus() || (ui->spinner_second->value() == -1) || (ui->spinner_second->value() == 60))
    ui->spinner_second->setValue(second);
}

/************************************************************************
Send newJd to spinner_*
 ************************************************************************/
void DateTimeDialog::setDateTime(double newJd)
{
	newJd += (StelApp::getInstance().getLocaleMgr().getGMTShift(newJd)/24.0); // UTC -> local tz
	StelUtils::getDateFromJulianDay(newJd, &year, &month, &day);
	StelUtils::getTimeFromJulianDay(newJd, &hour, &minute, &second);
	pushToWidgets();
}

