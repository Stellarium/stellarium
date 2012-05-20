/***************************************************************************
 *   Copyright (C) 2008  Matthew Gates                                     *
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
#include <cmath>

AngleSpinBox::AngleSpinBox(QWidget* parent, DisplayFormat format, PrefixType prefix)
	: QAbstractSpinBox(parent),
	  angleSpinBoxFormat(format),
	  currentPrefixType(prefix),
	  decimalPlaces(2),
	  radAngle(0.0)
{
	connect(this, SIGNAL(editingFinished()), this, SLOT(updateValue()));
	formatText();
}

const QString AngleSpinBox::positivePrefix(PrefixType prefix)
{
	switch(prefix)
	{
	case NormalPlus:
		return("+");
		break;
	case Longitude:
		return(q_("E "));
		break;
	case Latitude:
		return(q_("N "));
		break;
	case Normal:
	default:
		return("");
		break;
	}
}

const QString AngleSpinBox::negativePrefix(PrefixType prefix)
{
	switch(prefix)
	{
	case NormalPlus:
		return(QLocale().negativeSign());
		break;
	case Longitude:
		return(q_("W "));
		break;
	case Latitude:
		return(q_("S "));
		break;
	case Normal:
	default:
		return(QLocale().negativeSign());
		break;
	}
}

AngleSpinBox::~AngleSpinBox()
{
}


AngleSpinBox::AngleSpinboxSection AngleSpinBox::getCurrentSection() const
{
	int cusorPos = lineEdit()->cursorPosition();
	const QString str = lineEdit()->text();
	
	int cPosMin = str.indexOf(QRegExp("[+-"+q_("N")+q_("S")+q_("E")+q_("W")+"]"), 0);
	int cPosMax = cPosMin+1;
	
	if (cusorPos>=cPosMin && cusorPos<cPosMax) {
		return SectionPrefix;
	}
	
	cPosMin = cPosMax;
	cPosMax = str.indexOf(QRegExp(QString("[h%1]").arg(QChar(176))), 0)+1;
	if (cusorPos > cPosMin && cusorPos <= cPosMax) {
		return SectionDegreesHours;
	}
	
	cPosMin = cPosMax;
	cPosMax = str.indexOf(QRegExp("[m']"), 0)+1;
	if (cusorPos > cPosMin && cusorPos <= cPosMax) {
		return SectionMinutes;
	}
	
	cPosMin = cPosMax;
	cPosMax = str.indexOf(QRegExp("[s\"]"), 0)+1;
	if (cusorPos > cPosMin && cusorPos <= cPosMax) {
		return SectionSeconds;
	}
	
	return SectionNone;
}
	
void AngleSpinBox::stepBy (int steps)
{
	const int cusorPos = lineEdit()->cursorPosition();
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
			if (angleSpinBoxFormat==DMSLetters || angleSpinBoxFormat==DMSSymbols || angleSpinBoxFormat==DecimalDeg)
				radAngle += M_PI/180.*steps;
			else
				radAngle += M_PI/12.*steps;
			break;
		}
		case SectionMinutes:
		{
			if (angleSpinBoxFormat==DMSLetters || angleSpinBoxFormat==DMSSymbols || angleSpinBoxFormat==DecimalDeg)
				radAngle += M_PI/180.*steps/60.;
			else
				radAngle += M_PI/12.*steps/60.;
			break;
		}
		case SectionSeconds:
		case SectionNone:
		{
			if (angleSpinBoxFormat==DMSLetters || angleSpinBoxFormat==DMSSymbols || angleSpinBoxFormat==DecimalDeg)
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
	formatText();
	lineEdit()->setCursorPosition(cusorPos);
	emit(valueChanged());
}

QValidator::State AngleSpinBox::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
	QValidator::State state;
	stringToDouble(input, &state);
	return state;
}

void AngleSpinBox::clear()
{
	radAngle = 0.0;
	formatText();
	emit(valueChanged());
}

QAbstractSpinBox::StepEnabled AngleSpinBox::stepEnabled() const
{
	return (StepUpEnabled|StepDownEnabled);
}

double AngleSpinBox::valueRadians()
{
	return radAngle;
} 

double AngleSpinBox::valueDegrees()
{
	return (radAngle*360.)/(2.*M_PI);
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
	else
	{
		sign = 1;
		input = input.mid(positivePrefix(prefix).length());
	}

	QRegExp dmsRx("^\\s*(\\d+)\\s*[d\\x00b0](\\s*(\\d+(\\.\\d*)?)\\s*[m'](\\s*(\\d+(\\.\\d*)?)\\s*[s\"]\\s*)?)?$", 
                  Qt::CaseInsensitive);
	QRegExp hmsRx("^\\s*(\\d+)\\s*h(\\s*(\\d+(\\.\\d*)?)\\s*[m'](\\s*(\\d+(\\.\\d*)?)\\s*[s\"]\\s*)?)?$", 
                  Qt::CaseInsensitive);
	QRegExp decRx("^(\\d+(\\.\\d*)?)(\\s*[\\x00b0]\\s*)?$");
	QRegExp badRx("[^hdms0-9 \\x00b0'\"\\.]", Qt::CaseInsensitive);

	QValidator::State dummy;
	if (state == NULL)
	{
		state = &dummy;
	}
		
	if (dmsRx.exactMatch(input))
	{
		double degree = dmsRx.capturedTexts().at(1).toDouble();
		double minute = dmsRx.capturedTexts().at(3).toDouble();
		double second = dmsRx.capturedTexts().at(6).toDouble();
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
	else if (hmsRx.exactMatch(input))
	{
		double hour   = hmsRx.capturedTexts().at(1).toDouble();
		double minute = hmsRx.capturedTexts().at(3).toDouble();
		double second = hmsRx.capturedTexts().at(6).toDouble();
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
	else if (decRx.exactMatch(input))
	{
		double dec = decRx.capturedTexts().at(1).toDouble();
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

	if (radAngle == a)
		return;
	radAngle = a;
	formatText();
	emit(valueChanged());
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
	switch (angleSpinBoxFormat)
	{
		case DMSLetters:
		case DMSSymbols:
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

			d = (int)angle;
			m = (int)((angle - d)*60);
			s = (angle-d)*3600-60*m;

			// we may have seconds as 60 and one less minute...
			if (s > 60.0 - ::pow(10, -1 * (decimalPlaces+1)))
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
			if (s < ::pow(10, -1 * (decimalPlaces+1)))
				s= 0.0;
			else if (s < 0.0 && 0.0 - ::pow(10, -1 * (decimalPlaces+1)))
				s= 0.0;

			QString signInd = positivePrefix(currentPrefixType);
			if (!sign)
				signInd = negativePrefix(currentPrefixType);

			if (angleSpinBoxFormat == DMSLetters)
				lineEdit()->setText(QString("%1%2d %3m %4s")
                                    .arg(signInd).arg(d).arg(m).arg(s, 0, 'f', decimalPlaces, ' '));
			else
				lineEdit()->setText(QString("%1%2%3 %4' %5\"")
                                    .arg(signInd).arg(d).arg(QChar(176)).arg(m)
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
			h = (unsigned int)angle;
			m = (unsigned int)((angle-h)*60);
			s = (angle-h)*3600.-60.*m;

			// we may have seconds as 60 and one less minute...
			if (s > 60.0 - ::pow(10, -1 * (decimalPlaces+1)))
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
			if (s < ::pow(10, -1 * (decimalPlaces+1)))
				s= 0.0;
			else if (s < 0.0 && 0.0 - ::pow(10, -1 * (decimalPlaces+1)))
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

			lineEdit()->setText(QString("%1%2%3")
                                .arg(signInd)
                                .arg(fmod(angle * 180.0 / M_PI, 360.0), 0, 'f', decimalPlaces, ' ')
                                .arg(QChar(176)));
			break;
		}
		default:
		{
			qWarning() << "AngleSpinBox::formatText - WARNING - unknown format" 
                       << (int)(angleSpinBoxFormat);
			break;
		}
	}
}

