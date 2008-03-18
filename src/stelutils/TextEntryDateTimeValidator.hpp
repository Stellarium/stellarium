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

#ifndef _TEXTENTRYDATETIMEBALIDATOR_HPP_
#define _TEXTENTRYDATETIMEBALIDATOR_HPP_

#include <QValidator>
#include <QRegExp>
#include <vector>

using namespace std;

// QRegExpValidator was not enough to both 1) allow only valid
// datetimes at the end, but also 2) allow intermediate editing.
// Approach here: test first for final valid state (return accordingly
// if final valid), then the several intermediate states (return
// accordingly if intermediate valid), then return invalid.

class TextEntryDateTimeValidator : public QValidator
{
public:
	TextEntryDateTimeValidator(QObject *parent);

	//! Return QValidator status of the input string based on whether it
	//! matches the RegExps defined on this class.
	QValidator::State validate(QString & input, int & pos) const;

	//! use RegExp for final to parse a QString to six ints, use in the event QDateTime cannot handle the date.
	static vector<int> get_ints_from_ISO8601_string(const QString &);
private:
	QRegExp final, badEpoch, editingYear, editingMonth, editingDay, editingHour, editingMinute, editingSecond;
};

#endif
