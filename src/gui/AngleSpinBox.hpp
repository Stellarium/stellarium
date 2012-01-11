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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
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
	//! @enum DisplayFormat
	//! Used to decide how to display the angle.
	enum DisplayFormat
	{
		DMSLetters,     //!< Degrees, minutes and seconds, e.g. 180d 4m 8s
		DMSSymbols,     //!< Degrees, minutes and seconds, e.g. 180° 4' 8"
		HMSLetters,     //!< Hours, minutes and seconds, e.g. 12h 4m 6s
		HMSSymbols,     //!< Hours, minutes and seconds, e.g. 12h 4' 6s"
		DecimalDeg      //!< Decimal degrees, e.g. 180.06888
	};

	//! @enum PrefixType
	//! Determines how positive and negative values are indicated.
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
	virtual void stepBy(int steps);
	virtual QValidator::State validate(QString& input, int& pos) const;

	//! Get the angle held in the AngleSpinBox
	//! @return the angle in radians
	double valueRadians();
	//! Get the angle held in the AngleSpinBox
	//! @return the angle in degrees
	double valueDegrees();

	//! Set the number of decimal places to express float values to (e.g. seconds in DMSLetters format).
	//! @param places the number of decimal places to use.
	void setDecimals(int places) { decimalPlaces = places; }

	//! Get the number of decimal places to express float values to (e.g. seconds in DMSLetters format).
	//! @return the number of decimal places used.
	int decimals() { return decimalPlaces; }

	//! Set the display format.
	//! @param format the new format to use.
	void setDisplayFormat(DisplayFormat format) { angleSpinBoxFormat=format; formatText(); }

	//! Get the current display format.
	//! @return the current DisplayFormat.
	DisplayFormat displayFormat() { return angleSpinBoxFormat; }

	//! Set the prefix type.
	//! @param prefix the new prefix type to use.
	void setPrefixType(PrefixType prefix) { currentPrefixType=prefix; formatText(); }

	//! Get the current display format.
	//! @return the current DisplayFormat.
	PrefixType prefixType() { return currentPrefixType; }
	
public slots:
	//! Set the value to default 0 angle.
	virtual void clear();
	
	//! Set the value of the spin box in radians.
	//! @param radians the value to set, in radians.
	void setRadians(double radians);
	
	//! Set the value of the spin box in decimal degrees.
	//! @param degrees the value to set, in decimal degrees.
	void setDegrees(double degrees);

signals:
	//! Emitted when the value changes.
	void valueChanged();

protected:
	virtual StepEnabled stepEnabled() const;

private slots:
	//! Updates radAngle (internal representation of the angle) and calls formatText
	void updateValue(void);

private:
	//! Convert a string value to an angle in radians.
	//! This function can be used to validate a string as expressing an angle. Accepted
	//! are any formats which the AngleSpinBox understands.
	//! @param input the string value to be converted / validated.
	//! @param state a pointer to a QValidator::State value which is set according to the validation.
	//! @param prefix the kind of prefix to use for conversion.
	//! @return the value of the angle expressed in input in radians.
	double stringToDouble(QString input, QValidator::State* state, PrefixType prefix=Unknown) const;
	
	//! @enum AngleSpinboxSection
	enum AngleSpinboxSection
	{
		SectionPrefix,			//! Section of the S/W or E/W or +/-
  		SectionDegreesHours,	//! Section of the degree or hours
  		SectionMinutes,			//! Section of the minutes (of degree or of hours)
  		SectionSeconds,			//! Section of the seconds (of degree or of hours)
  		SectionNone				//! No matching section, e.g. between 2 sections
	};
	
	//! Get the current section in which the line edit cursor is.
	AngleSpinboxSection getCurrentSection() const;
	
	//! Reformats the input according to the current value of angleSpinBoxFormat/
	//! This is called whenever an editingFinished() signal is emitted, 
	//! e.g. when RETURN is pressed.
	void formatText(void);
		
	static const QString positivePrefix(PrefixType prefix);
	static const QString negativePrefix(PrefixType prefix);

	DisplayFormat angleSpinBoxFormat;
	PrefixType currentPrefixType;
	int decimalPlaces;
	double radAngle;

};

#endif // _ANGLESPINBOX_HPP_
