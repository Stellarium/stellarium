/*
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef BALINESEPAWUKONGCALENDAR_HPP
#define BALINESEPAWUKONGCALENDAR_HPP

#include <QString>
#include "Calendar.hpp"

//! Balinese Pawukon calendar, a cycle count of 10 simultaneous cycles of different lengths. Only cycles 5, 6, 7 are enough to determine a date.
//! The cycles are not counted into longer periods, therefore no absolute "epoch" dating is possible.
//!
class BalinesePawukonCalendar : public Calendar
{
	Q_OBJECT

public:
	BalinesePawukonCalendar(double jd);

	~BalinesePawukonCalendar() override {}

public slots:
	//! Translate e.g. stringlists of part names
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	//! This triggers the partsChanged() signal
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted in canonical order, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
	//! This triggers the jdChanged() signal
	//! Note that this must not change the time of day! You must retrieve the time from the current JD before recomputing a new JD.
	//! This actually sets bali-on-or-before()
	void setDate(QVector<int> parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date. This concatenates all strings from getDateStrings() with a space in between, plus a linebreak after element number 5.
	QString getFormattedDateString() const override;

	static QVector<int> baliPawukonFromFixed(const int rd);
	static int baliDayFromFixed(const int rd);
	static int baliTriwaraFromFixed(const int rd);
	static int baliSadwaraFromFixed(const int rd);
	static int baliSaptawaraFromFixed(const int rd);
	static int baliPancawaraFromFixed(const int rd);
	static int baliWeekFromFixed(const int rd);
	static int baliDasawaraFromFixed(const int rd);
	static int baliDwiwaraFromFixed(const int rd);
	static int baliLuangFromFixed(const int rd);
	static int baliSangawaraFromFixed(const int rd);
	static int baliAsatawaraFromFixed(const int rd);
	static int baliCaturwaraFromFixed(const int rd);

	static int baliOnOrBefore(const QVector<int>baliDate, const int rd);
public:
	static const int baliEpoch;

protected:
	static QMap<int, QString>ekawaraNames;
	static QMap<int, QString>dwiwaraNames;
	static QMap<int, QString>triwaraNames;
	static QMap<int, QString>caturwaraNames;
	static QMap<int, QString>pancawaraNames;
	static QMap<int, QString>sadwaraNames;
	static QMap<int, QString>saptawaraNames;
	static QMap<int, QString>asatawaraNames;
	static QMap<int, QString>sangawaraNames;
	static QMap<int, QString>dasawaraNames;
	static QMap<int, QString>weekNames;
};

#endif
