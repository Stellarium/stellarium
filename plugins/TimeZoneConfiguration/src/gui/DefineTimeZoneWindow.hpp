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

#ifndef _DEFINE_TIME_ZONE_HPP_
#define _DEFINE_TIME_ZONE_HPP_

#include "StelDialog.hpp"

class Ui_defineTimeZoneForm;
class QRegExpValidator;
class QSpinBox;

class DefineTimeZoneWindow : public StelDialog
{
	Q_OBJECT

public:
	DefineTimeZoneWindow();
	~DefineTimeZoneWindow();

	//! Fills the fields of the form with the time zone described in the string.
	//! Call after createDialogContent() or setVisible() to avoid a crash.
	void setTimeZone(QString timeZoneString);

public slots:
	void retranslate();

signals:
	void timeZoneDefined(QString timeZoneString);

protected:
	void createDialogContent();

private:
	Ui_defineTimeZoneForm * ui;

	QRegExpValidator * timeZoneNameValidator;

	void resetWindowState();
	void populateDateLists();

	void updateDayNumberMaximum(int monthIndex, QSpinBox * spinBoxDay);

	enum DstEndpoint {DstStart, DstEnd};
	//! \returns false if #string is not a valid TZ DST end point description.
	bool readDstEndpoint(const QString& string, DstEndpoint endpoint);
	void setEndPointDate(int ordinalDate, bool leapYear, DstEndpoint endpoint);

private slots:
	void useDefinition();
	void updateDstOffset(double normalOffset);
	void updateDayNumberMaximumDstStart(int monthIndex);
	void updateDayNumberMaximumDstEnd(int monthIndex);
};


#endif //_DEFINE_TIME_ZONE_HPP_
