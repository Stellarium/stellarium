/***************************************************************************
 *   Copyright (C) 2008  Matthew Gates                                     *
 *   Copyright (C) 2015 Georg Zotti (min/max limits)                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.           *
 ***************************************************************************/

#include "AngleSpinBox.hpp"
#include "StelTranslator.hpp"
#include <QDebug>
#include <QString>
#include <QLineEdit>
#include <QWidget>
#include <QLocale>
#include <limits>
#include <QRegularExpression>

AngleSpinBox::AngleSpinBox(QWidget* parent, DisplayFormat format, PrefixType prefix)
	: QAbstractSpinBox(parent),
	  angleSpinBoxFormat(format),
	  currentPrefixType(prefix),
	  decimalPlaces(2),
	  radAngle(0.0),
	  minRad(-std::numeric_limits<double>::max()),
	  maxRad( std::numeric_limits<double>::max())
{
	connect(this, SIGNAL(editingFinished()), this, SLOT(updateValue()));
	formatText();
	setWrapping(false); // but should be set true for longitudinal cycling
}

const QString AngleSpinBox::positivePrefix(PrefixType prefix)
{
	switch(prefix)
	{
		case NormalPlus:
			return("+");
		case Longitude:
			return(q_("E")+" ");
		case Latitude:
			return(q_("N")+" ");
		case Normal:
		default:
			return("");
	}
}

const QString AngleSpinBox::negativePrefix(PrefixType prefix)
{
	switch(prefix)
	{
		case NormalPlus:
			return(QLocale().negativeSign());
		case Longitude:
			return(q_("W")+" ");
		case Latitude:
			return(q_("S")+" ");
		case Normal:
		default:
			return(QLocale().negativeSign());
	}
}

AngleSpinBox::~AngleSpinBox()
{
}


AngleSpinBox::AngleSpinboxSection AngleSpinBox::getCurrentSection() const
{
	int cursorPos = lineEdit()->cursorPosition();
	const QString str = lineEdit()->text();
	
	// Regexp must not have "+-" immediately behind "[" !
	static QRegularExpression cardExp("^["+q_("N")+q_("S")+q_("E")+q_("W")+"+-]");
	int cPosMin = str.indexOf(cardExp, 0);
	// without prefix (e.g. right ascension): avoid unwanted negating!
	if ((cPosMin==-1) && (cursorPos==0)) {
		return SectionDegreesHours;
	}
	int cPosMax = cPosMin+1;
	
	if (cursorPos>=cPosMin && cursorPos<cPosMax) {
		return SectionPrefix;
	}
	
	cPosMin = cPosMax;
	static const QRegularExpression dExp("[dh°]");
	cPosMax = str.indexOf(dExp, 0)+1;
	if (cursorPos >= cPosMin && cursorPos <= cPosMax) {
		return SectionDegreesHours;
	}
	
	cPosMin = cPosMax;
	static const QRegularExpression mExp("[m']");
	cPosMax = str.indexOf(mExp, 0)+1;
	if (cursorPos > cPosMin && cursorPos <= cPosMax) {
		return SectionMinutes;
	}
	
	cPosMin = cPosMax;
	static const QRegularExpression sExp("[s\"]");
	cPosMax = str.indexOf(sExp, 0)+1;
	if (cursorPos > cPosMin && cursorPos <= cPosMax) {
		return SectionSeconds;
	}
	
	return SectionNone;
}
	
void AngleSpinBox::stepBy (int steps)
{
	const int cursorPos = lineEdit()->cursorPosition();
	const AngleSpinBox::AngleSpinboxSection sec = getCurrentSection();
	switch (sec)
	{
		case SectionPrefix:
		{
			radAngle = -radAngle;
			break;
		}
		case SectionDegreesHours:		
		{
			if (angleSpinBoxFormat==DMSLetters || angleSpinBoxFormat==DMSSymbols || angleSpinBoxFormat==DecimalDeg
				|| angleSpinBoxFormat==DMSLettersUnsigned || angleSpinBoxFormat==DMSSymbolsUnsigned )
				radAngle += M_PI/180.*steps;
			else
				radAngle += M_PI/12.*steps;
			break;
		}
		case SectionMinutes:
		{
			if (angleSpinBoxFormat==DMSLetters || angleSpinBoxFormat==DMSSymbols || angleSpinBoxFormat==DecimalDeg
				|| angleSpinBoxFormat==DMSLettersUnsigned || angleSpinBoxFormat==DMSSymbolsUnsigned )
				radAngle += M_PI/180.*steps/60.;
			else
				radAngle += M_PI/12.*steps/60.;
			break;
		}
		case SectionSeconds:
		case SectionNone:
		{
			if (angleSpinBoxFormat==DMSLetters || angleSpinBoxFormat==DMSSymbols || angleSpinBoxFormat==DecimalDeg
				|| angleSpinBoxFormat==DMSLettersUnsigned || angleSpinBoxFormat==DMSSymbolsUnsigned )
				radAngle += M_PI/180.*steps/3600.;
			else
				radAngle += M_PI/12.*steps/3600.;
			break;
		}
		default:
		{
			return;
		}
	}
	if (wrapping())
	{
		if (radAngle > maxRad)
			radAngle=minRad+(radAngle-maxRad);
		else if (radAngle < minRad)
			radAngle=maxRad-(minRad-radAngle);
	}
	radAngle=qMin(radAngle, maxRad);
	radAngle=qMax(radAngle, minRad);
	formatText();
	lineEdit()->setCursorPosition(cursorPos);
	emit valueChanged();
	emit valueChangedDeg(valueDegrees());
	emit valueChangedRad(valueRadians());
}

QValidator::State AngleSpinBox::validate(QString& input, int& pos) const
{
	Q_UNUSED(pos)
	QValidator::State state;
	stringToDouble(input, &state);
	return state;
}

void AngleSpinBox::clear()
{
	radAngle = 0.0;
	formatText();
	emit valueChanged();
	emit valueChangedDeg(valueDegrees());
	emit valueChangedRad(valueRadians());
}

QAbstractSpinBox::StepEnabled AngleSpinBox::stepEnabled() const
{
	return (StepUpEnabled|StepDownEnabled);
}

double AngleSpinBox::valueRadians() const
{
	return radAngle;
} 

double AngleSpinBox::valueDegrees() const
{
	return radAngle*(180./M_PI);
} 

double AngleSpinBox::stringToDouble(QString input, QValidator::State* state, PrefixType prefix) const
{
	if (prefix==Unknown)
	{
		prefix=currentPrefixType;
	}
	int sign=1;
	if (input.startsWith(negativePrefix(prefix), Qt::CaseInsensitive)) 
	{
		sign = -1;
		input = input.mid(negativePrefix(prefix).length());
	}
	else if (input.startsWith(positivePrefix(prefix), Qt::CaseInsensitive)) 
	{
		sign = 1;
		input = input.mid(positivePrefix(prefix).length());
	}
	else if (input.startsWith("-", Qt::CaseInsensitive))
	{
		sign = -1;
		input = input.mid(1);
	}
	else if (input.startsWith("+", Qt::CaseInsensitive))
	{
		sign = 1;
		input = input.mid(1);
	}

	static const QRegularExpression dmsRx("^\\s*(\\d+)\\s*[d°](\\s*(\\d+(\\.\\d*)?)\\s*[m'](\\s*(\\d+(\\.\\d*)?)\\s*[s\"]\\s*)?)?$",
		  QRegularExpression::CaseInsensitiveOption);
	static const QRegularExpression hmsRx("^\\s*(\\d+)\\s*h(\\s*(\\d+(\\.\\d*)?)\\s*[m'](\\s*(\\d+(\\.\\d*)?)\\s*[s\"]\\s*)?)?$",
		  QRegularExpression::CaseInsensitiveOption);
	static const QRegularExpression decRx(u8"^(\\d+(\\.\\d*)?)(\\s*\u00b0\\s*)?$");
	static const QRegularExpression badRx("[^hdms0-9 °'\"\\.]", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch dmsMatch=dmsRx.match(input);
	QRegularExpressionMatch hmsMatch=hmsRx.match(input);
	QRegularExpressionMatch decMatch=decRx.match(input);

	QValidator::State dummy;
	if (state == Q_NULLPTR)
	{
		state = &dummy;
	}

	if (dmsMatch.hasMatch())
	{
		double degree = dmsMatch.captured(1).toDouble();
		double minute = dmsMatch.captured(3).toDouble();
		double second = dmsMatch.captured(6).toDouble();
		if (degree > 360.0 || degree < -360.0)
		{
			*state = QValidator::Invalid;
			return 0.0;
		}

		if (minute > 60.0 || minute < 0.0) 
		{
			*state = QValidator::Invalid;
			return 0.0;
		}

		if (second > 60.0 || second < 0.0)
		{
			*state = QValidator::Invalid;
			return 0.0;
		}

		*state = QValidator::Acceptable;
		return (sign * (degree + (minute/60.0) + (second/3600.0))) * M_PI / 180.0;
	}
	else if (hmsMatch.hasMatch())
	{
		double hour   = hmsMatch.captured(1).toDouble();
		double minute = hmsMatch.captured(3).toDouble();
		double second = hmsMatch.captured(6).toDouble();
		if (hour >= 24.0 || hour < 0.0)
		{
			*state = QValidator::Invalid;
			return 0.0;
		}

		if (minute > 60.0 || minute < 0.0) 
		{
			*state = QValidator::Invalid;
			return 0.0;
		}

		if (second > 60.0 || second < 0.0)
		{
			*state = QValidator::Invalid;
			return 0.0;
		}

		*state = QValidator::Acceptable;
		return sign * (((360.0*hour/24.0) + (minute*15/60.0) + (second*15/3600.0)) * M_PI / 180.0);
	}
	else if (decMatch.hasMatch())
	{
		double dec = decMatch.captured(1).toDouble();
		if (dec < 0.0 || dec > 360.0)
		{
			*state = QValidator::Invalid;
			return 0.0;
		}
		else
		{
			*state = QValidator::Acceptable;
			return sign * (dec * M_PI / 180.0);
		}
	}
	else if (input.contains(badRx))
	{
		*state = QValidator::Invalid; 
		return 0.0;
	}
	*state = QValidator::Intermediate;
	return 0.0;
}

void AngleSpinBox::updateValue(void)
{
	QValidator::State state;
	double a = stringToDouble(lineEdit()->text(), &state);
	if (state != QValidator::Acceptable)
		return;

	if (qFuzzyCompare(radAngle, a))
		return;
	radAngle = a;

	if (wrapping())
	{
		if (radAngle > maxRad)
			radAngle=minRad+(radAngle-maxRad);
		else if (radAngle < minRad)
			radAngle=maxRad-(minRad-radAngle);
	}
	radAngle=qMin(radAngle, maxRad);
	radAngle=qMax(radAngle, minRad);

	formatText();
	emit valueChanged();
	emit valueChangedDeg(valueDegrees());
	emit valueChangedRad(valueRadians());
}

void AngleSpinBox::setRadians(double radians)
{
	radAngle = radians;
	formatText();
}

void AngleSpinBox::setDegrees(double degrees)
{
	radAngle = degrees * M_PI/180.;
	formatText();
}

void AngleSpinBox::formatText(void)
{
	const int cursorPos=lineEdit()->cursorPosition();
	switch (angleSpinBoxFormat)
	{
		case DMSLetters:
		case DMSSymbols:
		case DMSLettersUnsigned:
		case DMSSymbolsUnsigned:
		{
			double angle = radAngle;
		 	int d, m;
			double s;
			bool sign=true;
			if (angle<0)
			{
				angle *= -1;
				sign = false;
			}
			angle = fmod(angle,2.0*M_PI);
			angle *= 180./M_PI;

			if ( (!sign) && ( (angleSpinBoxFormat==DMSLettersUnsigned) || (angleSpinBoxFormat==DMSSymbolsUnsigned))) {
				angle = 360.0-angle;
				sign=true;
			}

			d = static_cast<int>(angle);
			m = static_cast<int>((angle - d)*60);
			s = (angle-d)*3600-60*m;

			// we may have seconds as 60 and one less minute...
			if (s > 60.0 - ::pow(10.0, -1 * (decimalPlaces+1)))
			{
				m+=1;
				s-=60.0;
			}

			// may have to carry to the degrees...
			if (m >= 60)
			{
				d= (d+1) % 360;
				m-=60;
			}

			// fix when we have tiny tiny tiny values.
			if (abs(s) < ::pow(10.0, -1 * (decimalPlaces+1)))
				s= 0.0;

			QString signInd = positivePrefix(currentPrefixType);
			if (!sign)
				signInd = negativePrefix(currentPrefixType);

			if ((angleSpinBoxFormat == DMSLetters) || (angleSpinBoxFormat == DMSLettersUnsigned))
				lineEdit()->setText(QString("%1%2d %3m %4s")
                                    .arg(signInd).arg(d).arg(m).arg(s, 0, 'f', decimalPlaces, ' '));
			else
				lineEdit()->setText(QString("%1%2° %3' %4\"")
				    .arg(signInd).arg(d).arg(m)
                                    .arg(s, 0, 'f', decimalPlaces, ' '));
			break;
		}
		case HMSLetters:
		case HMSSymbols:
		{
			unsigned int h, m;
			double s;
			double angle = radAngle;
			angle = fmod(angle,2.0*M_PI);
			if (angle < 0.0) angle += 2.0*M_PI; // range: [0..2.0*M_PI)
			angle *= 12./M_PI;
			h = static_cast<unsigned int>(angle);
			m = static_cast<unsigned int>((angle-h)*60);
			s = (angle-h)*3600.-60.*m;

			// we may have seconds as 60 and one less minute...
			if (s > 60.0 - ::pow(10.0, -1 * (decimalPlaces+1)))
			{
				m+=1;
				s-=60.0;
			}

			// may have to carry to the degrees...
			if (m >= 60)
			{
				h = (h+1) % 24;
				m-=60;
			}

			// fix when we have tiny tiny tiny values.
			if (abs(s) < ::pow(10.0, -1 * (decimalPlaces+1)))
				s= 0.0;

			if (angleSpinBoxFormat == HMSLetters)
				lineEdit()->setText(QString("%1h %2m %3s")
                                    .arg(h).arg(m).arg(s, 0, 'f', decimalPlaces, ' '));
			else
				lineEdit()->setText(QString("%1h %2' %3\"")
                                    .arg(h).arg(m).arg(s, 0, 'f', decimalPlaces, ' '));
			break;
		}
		case DecimalDeg:
		{			
			double angle = radAngle;
			QString signInd = positivePrefix(currentPrefixType);

			if (radAngle<0)
			{
				angle *= -1;				
				signInd = negativePrefix(currentPrefixType);
			}

			lineEdit()->setText(QString("%1%2°")
                                .arg(signInd)
				.arg(fmod(angle * 180.0 / M_PI, 360.0), 0, 'f', decimalPlaces, ' '));
			break;
		}
		default:
		{
			qWarning() << "AngleSpinBox::formatText - WARNING - unknown format" 
		       << static_cast<int>(angleSpinBoxFormat);
			break;
		}
	}
	lineEdit()->setCursorPosition(cursorPos);
}

QSize AngleSpinBox::minimumSizeHint() const
{
	QString refText;
	const auto signPlaceholder = minRad >= 0 ? positivePrefix(currentPrefixType) : negativePrefix(currentPrefixType);
	switch (angleSpinBoxFormat)
	{
		case DMSLetters:
		case DMSSymbols:
		case DMSLettersUnsigned:
		case DMSSymbolsUnsigned:
		{
			if (angleSpinBoxFormat == DMSLetters || angleSpinBoxFormat == DMSLettersUnsigned)
				refText = QString("%1%2d %3m %4s").arg(signPlaceholder).arg(359).arg(59)
				                                  .arg(0, 0, 'f', decimalPlaces, ' ');
			else
				refText = QString("%1%2° %3' %4\"").arg(signPlaceholder).arg(359).arg(59)
				                                   .arg(0, 0, 'f', decimalPlaces, ' ');
			break;
		}
		case HMSLetters:
		case HMSSymbols:
		{
			if (angleSpinBoxFormat == HMSLetters)
				refText = QString("%1h %2m %3s").arg(23).arg(59).arg(0, 0, 'f', decimalPlaces, ' ');
			else
				refText = QString("%1h %2' %3\"").arg(23).arg(59).arg(0, 0, 'f', decimalPlaces, ' ');
			break;
		}
		case DecimalDeg:
		{
			refText = QString("%1%2°").arg(signPlaceholder).arg(360, 0, 'f', decimalPlaces, ' ');
			break;
		}
		default:
		{
			qWarning() << "AngleSpinBox::updateSizeHint - WARNING - unknown format" << static_cast<int>(angleSpinBoxFormat);
			break;
		}
	}
	const QFontMetrics fm(font());
	const auto minWidth = fm.size(Qt::TextSingleLine, refText).width();
	auto size = QAbstractSpinBox::minimumSizeHint();
	size.rwidth() += minWidth;
	return size;
}
