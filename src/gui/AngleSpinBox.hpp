/*
 * Stellarium
 * Copyright (C) 2008 Matthew Gates
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

#ifndef _ANGLESPINBOX_HPP_
#define _ANGLESPINBOX_HPP_

#include <QAbstractSpinBox>
#include <QString>

//! @class AngleSpinBox
//! A spin box for displaying/entering angular values.
//! This class can accept angles in various formats commonly used in astronomy
//! including decimal degrees, DMS and HMS.

class AngleSpinBox : public QAbstractSpinBox
{
	Q_OBJECT

public:
	//! @enum DisplayFormat used to decide how to display the angle.
	enum DisplayFormat
	{
		DMSLetters,     //!< Degrees, minutes and seconds, e.g. 180d 4m 8s
		DMSSymbols,     //!< Degrees, minutes and seconds, e.g. 180° 4' 8"
		HMSLetters,     //!< Hours, minutes and seconds, e.g. 12h 4m 6s
		HMSSymbols,     //!< Hours, minutes and seconds, e.g. 12h 4' 6s"
		DecimalDeg      //!< Decimal degrees, e.g. 180.06888
	};

	//! @enum PrefixType deteremines how positive and negative values are indicated.
	enum PrefixType
	{
		Normal,		//!< negative values have '-' prefix
		NormalPlus,	//!< positive values have '+' prefix, negative values have '-' prefix.
		Longitude,	//!< positive values have 'E' prefix, negative values have 'W' prefix.
		Latitude,	//!< positive values have 'N' prefix, negative values have 'S' prefix.
 		Unknown
	};

	AngleSpinBox(QWidget* parent=0, DisplayFormat format=DMSSymbols, PrefixType prefix=Normal);
	~AngleSpinBox();

	// QAbstractSpinBox virtual members
	void stepBy (int steps);
	QValidator::State validate (QString& input, int& pos) const;

	//! Get the angle held in the AngleSpinBox
	//! @return the angle in radians
	double valueRadians();
	//! Get the angle held in the AngleSpinBox
	//! @return the angle in degrees
	double valueDegrees();

	//! Get the angle held in the AngleSpinBox
	//! @return the angle as formatted according to the current format and flags
	QString text();

	//! Convert a string value to a angle in radians.
	//! This function can be used to validate a string as expressing an angle. Accepted
	//! are any formats which the AngleSpinBox understands.
	//! @param input the string value to be converted / validated.
	//! @param state a pointer to a QValidator::State value which is set according to the validation.
	//! @return the value of the angle expressed in input in radians.
	double stringToDouble(QString input, QValidator::State* state, PrefixType prefix=Unknown) const;

	//! Set the number of decimal places to express float values to (e.g. seconds in DMSLetters format)
	//! @param places the number of decimal places to use.
	void setDecimals(int places) { decimalPlaces = places; }

	//! Set the number of decimal places to express float values to (e.g. seconds in DMSLetters format)
	//! @param places the number of decimal places to use.
	int decimals() { return decimalPlaces; }

	//! Set the display format
	//! @param format the new format to use
	void setDisplayFormat(DisplayFormat format) { angleSpinBoxFormat=format; emit(formatText()); }

	//! Get the current display format
	//! @return the current DisplayFormat
	DisplayFormat displayFormat() { return angleSpinBoxFormat; }

	//! Set the prefix type
	//! @param prefix the new prefix type to use
	void setPrefixType(PrefixType prefix) { currentPrefixType=prefix; emit(formatText()); }

	//! Get the current display format
	//! @return the current DisplayFormat
	PrefixType prefixType() { return currentPrefixType; }

public slots:
	void clear();

	//! Set the value of the spin box in radians.
	//! @param radians the value to set, in radians
	void setRadians(double radians);
	//! Set the value of the spin box in radians.
	//! @param radians the value to set, in radians
	void setDegrees(double degrees);
	//! Set the value of the spin box in decimal degrees.
	//! @param radians the value to set, in decimal degrees

signals:
	//! emitted when the value changes.
	void valueChanged(void);

protected:
	StepEnabled stepEnabled () const;

private slots:
	//! reformats the input according to the current value of angleSpinBoxFormat/
	//! This is called whenever an editingFinished() signal is emitted, 
	//! e.g. when RETURN is pressed.
	void formatText(void);

	//! updates radAngle (internal representation of the angle) and calls formatText
	void updateValue(void);

private:
	static const QString positivePrefix(PrefixType prefix);
	static const QString negativePrefix(PrefixType prefix);

	DisplayFormat angleSpinBoxFormat;
	PrefixType currentPrefixType;
	int decimalPlaces;
	double radAngle;

};

#endif // _ANGLESPINBOX_HPP_
