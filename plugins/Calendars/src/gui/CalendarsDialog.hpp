/*
 * Calendars plug-in for Stellarium
 *
 * Copyright (C) 2020 Georg Zotti
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

#ifndef CALENDARSDIALOG_HPP
#define CALENDARSDIALOG_HPP

#include "StelDialog.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "Calendars.hpp"

#include <QString>

class Ui_calendarsDialog;

//! Main window of the Calendars plug-in.
//! @ingroup calendars
class CalendarsDialog : public StelDialog
{
	Q_OBJECT

public:
	CalendarsDialog();
	~CalendarsDialog();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_calendarsDialog* ui;
	Calendars* cal;

	void setAboutHtml();

private slots:
	// populate the respective GUI elements when calendars change. It is possible to even retrieve sender address (?) to possibly call particular other functions.
	void populateGregorianParts(QVector<int> parts);
	void populateJulianParts(QVector<int> parts);
	void populateISOParts(QVector<int> parts);
	void populateMayaLongCountParts(QVector<int> parts);
	void populateMayaHaabParts(QVector<int> parts);
	void populateMayaTzolkinParts(QVector<int> parts);
	void populateAztecXihuitlParts(QVector<int> parts);
	void populateAztecTonalpohualliParts(QVector<int> parts);

	// handle changes in the editable fields. One method per calendar.
	void julianChanged();
	void gregorianChanged();
	void isoChanged();
	void mayaLongCountChanged();
	void mayaHaabChanged();
	void mayaTzolkinChanged();
	void aztecXihuitlChanged();
	void aztecTonalpohualliChanged();

	void resetCalendarsSettings();
};

#endif /* CALENDARSDIALOG_HPP */
