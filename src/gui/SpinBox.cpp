/***************************************************************************
 *   Copyright (C) 2008 by guillaume,,,   *
 *   guillaume@hellokitty   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "SpinBox.hpp"
#include <QDebug>
#include <QLineEdit>

SpinBox::SpinBox(QWidget *parent):
	QSpinBox(parent)
{
}


QRegExp SpinBox::regExp() const
{		
	QString ex;
	QString d;
	foreach(d, delimiters)
	{
		ex.append(QString("(\\-?\\d+)%1").arg(d));
	}
	return QRegExp(ex);
}

QRegExp SpinBox::regExpIntermediate() const
{		
	QString ex;
	QString d;
	foreach(d, delimiters)
	{
		ex.append(QString("(\\-?\\d*)%1").arg(d));
	}
	return QRegExp(ex);
}

SpinBox::~SpinBox()
{
}

int SpinBox::valueFromText(const QString &text) const
{
	QRegExp regExp = this->regExp();
	if (regExp.exactMatch(text)) {
		QVector<int> values;
		for (int i = 0; i < regExp.numCaptures(); ++i)
		{
			values.append(regExp.cap(i+1).toInt());
		}
		return valueFromValues(values);
	} else {
		return 0;
	}
}



QString SpinBox::textFromValue(int value) const
{
	QString ret;
	QVector<int> values = valuesFromValue(value);

	for(int i = 0; i < values.size(); ++i)
	{
		ret = ret.append("%1%2").arg(values[i]).arg(delimiters[i]);
	}
	return ret;
}


QValidator::State SpinBox::validate(QString & input, int & pos) const
{
	static QRegExp reg = regExp();
	if (reg.exactMatch(input))
	{
		return QValidator::Acceptable;
	}
	else
	{
		static QRegExp reg = regExpIntermediate();
		if (reg.exactMatch(input)) {
			return QValidator::Intermediate;
		}
		else
		{
			return QValidator::Invalid;
		}
	}
}

void SpinBox::stepBy(int steps)
{
	int inc = 1;
	int i;
	for(i = 1; i < ranges.size(); ++i)
	{
		inc *= ranges[i];
	}
	setValue(value() + steps * inc);
}

QVector<int> SpinBox::valuesFromValue(int value) const
{
	int size = mins.size();
	QVector<int> ret(mins); // just so that the size is correct
	for(int i = size-1; i >= 0; --i)
	{
		ret[i] = value % (ranges[i]) + mins[i];
		value -= value % (ranges[i]);
		value /= ranges[i];
	}
	return ret;
}

int SpinBox::valueFromValues(const QVector<int>& values) const
{
	int size = mins.size();
	int ret = 0;
	int coeff = 1;
	for(int i = size-1; i >= 0; --i)
	{
		ret += (values[i] - mins[i]) * coeff;
		coeff *= ranges[i];
	}
	return ret;
}


LongitudeSpinBox::LongitudeSpinBox(QWidget *parent):
		SpinBox(parent)
{
	mins << -180 << 0 << 0;
	ranges << 360 << 60 << 60;
	delimiters << QChar(176) << "'" << "\"";
	setRange(0, 60 * 60 * 360 - 1);
	setValue(0.);
}

void LongitudeSpinBox::setValue(double value)
{
	SpinBox::setValue((int)((value + 180) * 60 * 60));
}

float LongitudeSpinBox::getValue()
{
	return (float)value() / (60 * 60) - 180;
}

LatitudeSpinBox::LatitudeSpinBox(QWidget *parent):
		SpinBox(parent)
{
	mins << -90 << 0 << 0;
	ranges << 180 << 60 << 60;
	delimiters << QChar(176) << "'" << "\"";
	setRange(0, 60 * 60 * 180 - 1);
	setValue(0.);
}

void LatitudeSpinBox::setValue(double value)
{
	SpinBox::setValue((int)((value + 90) * 60 * 60));
}

float LatitudeSpinBox::getValue()
{
	return (float)value() / (60 * 60) - 90;
}
