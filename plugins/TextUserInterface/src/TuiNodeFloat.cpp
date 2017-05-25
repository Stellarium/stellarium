/*
 * Copyright (C) 2009 Matthew Gates
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

#include "StelTranslator.hpp"
#include "TuiNodeFloat.hpp"
#include <QKeyEvent>

TuiNodeFloat::TuiNodeFloat(const QString& text, QObject* receiver, const char* method, float defValue,
                             float min, float max, float inc, TuiNode* parent, TuiNode* prev)
	: TuiNodeEditable(text, parent, prev), value(defValue), minimum(min), maximum(max), increment(inc), typing(false), typedDecimal(false)
{
	this->connect(this, SIGNAL(setValue(float)), receiver, method);
}

TuiNodeResponse TuiNodeFloat::handleEditingKey(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;
	if (key==Qt::Key_Left || key==Qt::Key_Return)
	{
		typing = false;
		typedDecimal = false;
		editing = false;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Up)
	{
		typing = false;
		typedDecimal = false;
		value+=increment;
		if (value > maximum)
			value = maximum;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Down)
	{
		typing = false;
		typedDecimal = false;
		value-=increment;
		if (value < minimum)
			value = minimum;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Period)
	{
		QString s;
		if (!typing)
		{
			value = 0;
			typedDecimal = true;
		}
		else
		{
			// first check if there already a decimal point
			s = QString("%1").arg(value);
			if (!s.contains("."))
			{
				s = QString("%1%2").arg(value).arg(key - Qt::Key_0);
				typedDecimal = true;
			}
		}
		response.accepted = true;
		response.newNode = this;
		return response;
	}
	else if (key>=Qt::Key_0 && key<=Qt::Key_9)
	{
		QString s;
		if (!typing)
		{
			typing = true;
			if (typedDecimal)
				s = QString("0.%1").arg(key - Qt::Key_0);
			else
				s = QString("%1").arg(key - Qt::Key_0);
		}
		else
		{
			if (typedDecimal)
				s = QString("%1.%2").arg(value).arg(key - Qt::Key_0);
			else
				s = QString("%1%2").arg(value).arg(key - Qt::Key_0);
		}
		bool ok;
		float d = s.toFloat(&ok);
		if (ok && d>=minimum && d<=maximum)
		{
			value = d;
			typedDecimal = false;
		}
		response.accepted = true;
		response.newNode = this;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Backspace)
	{
		typing = true;
		typedDecimal = false;
		QString s = QString("%1").arg(value);
		if (s.length()==1)
		{
			value = 0;
		}
		else
		{
			s = s.left(s.length()-1);
			bool ok;
			float d = s.toFloat(&ok);
			if (ok && d>=minimum && d<=maximum)
			{
				value = d;
			}
		}
		response.accepted = true;
		response.newNode = this;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Minus)
	{
		typing = true;
		int i = value *= -1;
		if (i>=minimum && i<=maximum)
			value = i;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	return response;
}

QString TuiNodeFloat::getDisplayText() const
{
	if (!editing)
	{
		return prefixText + q_(displayText) + QString("  %1").arg(value);
	}
	else
	{
		if (typedDecimal)
			return prefixText + q_(displayText) + QString(" >%1.<").arg(value);
		else
			return prefixText + q_(displayText) + QString(" >%1<").arg(value);
	}
}

