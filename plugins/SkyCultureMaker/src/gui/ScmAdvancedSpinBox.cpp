/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
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

#include "ScmAdvancedSpinBox.hpp"
#include <qapplication.h>

ScmAdvancedSpinBox::ScmAdvancedSpinBox(QWidget *parent)
	: QSpinBox(parent)
	, displayCustomStringForValue(false)
	, useCustomMinimum(false)
	, useCustomMaximum(false)
{
}

void ScmAdvancedSpinBox::ScmAdvancedSpinBox::fixup(QString &input) const
{
	auto isValid = false;
	auto value = input.toInt(&isValid, displayIntegerBase());

	if (isValid)
	{
		value = qBound(minimum(), value, maximum());
		input = QString::number(value, displayIntegerBase());
	}
	else
	{
		QSpinBox::fixup(input);
	}
}

QValidator::State ScmAdvancedSpinBox::validate(QString &text, int &pos) const
{
	bool isValid = false;
	int value = text.toInt(&isValid, displayIntegerBase());

	if (isValid)
	{
		if (value >= minimum() && value <= maximum())
		{
			return QValidator::Acceptable;
		}
		return QValidator::Intermediate;
	}
	else
	{
		return QSpinBox::validate(text, pos);
	}
}

void ScmAdvancedSpinBox::setDisplayCustomStringForValue(bool display)
{
	displayCustomStringForValue = display;
}

void ScmAdvancedSpinBox::addCustomStringForValue(int value, QString text)
{
	customStringForValue.insert(value, text);
}

void ScmAdvancedSpinBox::removeCustomStringForValue(int value)
{
	customStringForValue.remove(value);
}

void ScmAdvancedSpinBox::setCustomStringForMin(QString text)
{
	customStringForValue.insert(minimum(), text);
}

void ScmAdvancedSpinBox::setCustomStringForMax(QString text)
{
	customStringForValue.insert(maximum(), text);
}

void ScmAdvancedSpinBox::setUseCustomMinimum(bool useCustomMin)
{
	useCustomMinimum = useCustomMin;
}

void ScmAdvancedSpinBox::setUseCustomMaximum(bool useCustomMax)
{
	useCustomMaximum = useCustomMax;
}

void ScmAdvancedSpinBox::setCustomMinimum(int min)
{
	customMinimum = min;
}

void ScmAdvancedSpinBox::setCustomMaximum(int max)
{
	customMaximum = max;
}

void ScmAdvancedSpinBox::setMinimum(int min)
{
	if (useCustomMinimum)
	{
		if (min < customMinimum)
		{
			return;
		}
	}
	QSpinBox::setMinimum(min);
}

void ScmAdvancedSpinBox::setMaximum(int max)
{
	if (useCustomMaximum)
	{
		if (max > customMaximum)
		{
			return;
		}
	}
	QSpinBox::setMaximum(max);
}

QString ScmAdvancedSpinBox::textFromValue(int value) const
{
	if (displayCustomStringForValue)
	{
		if (customStringForValue.contains(value))
		{
			return customStringForValue.value(value);
		}
	}

	return QString::number(value);
}
