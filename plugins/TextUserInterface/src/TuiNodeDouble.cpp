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
#include "TuiNodeDouble.hpp"
#include <QKeyEvent>

TuiNodeDouble::TuiNodeDouble(const QString& text, QObject* receiver, const char* method, double defValue,
                             double min, double max, double inc, TuiNode* parent, TuiNode* prev)
	: TuiNodeEditable(text, parent, prev), value(defValue), minimum(min), maximum(max), increment(inc), typing(false), typedDecimal(false)
{
	this->connect(this, SIGNAL(setValue(double)), receiver, method);
	stringValue.setNum(value,'g',-1);
}

TuiNodeResponse TuiNodeDouble::handleEditingKey(int key)
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
		stringValue.setNum(value,'g',-1);
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
		stringValue.setNum(value,'g',-1);
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
		stringValue.setNum(value,'g',-1);
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Period)
	{
		if (!typing)
		{
			value = 0;
			stringValue.clear();
			typedDecimal = true;
		}
		else
		{
			// first check if there already a decimal point
			if (!stringValue.contains('.'))
			{
				stringValue.setNum(value,'g',-1).append('.');
				typedDecimal = true;
			}
		}
		response.accepted = true;
	}
	else if (key>=Qt::Key_0 && key<=Qt::Key_9)
	{
		if (!typing)
			typing = true;
		
		stringValue.append(QString::number(key - Qt::Key_0));
		
		bool ok;
		double d = stringValue.toDouble(&ok);
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
		if (stringValue.length() == 1)
		{
			value = 0;
			stringValue.clear();
		}
		else
		{
			stringValue.chop(1);
			bool ok;
			double d = stringValue.toDouble(&ok);
			if (ok && d>=minimum && d<=maximum)
			{
				value = d;
			}
		}
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Minus)
	{
		typing = true;
		int i = value *= -1; //BM: Is this deliberate?
		if (i>=minimum && i<=maximum)
			value = i;
		stringValue.setNum(value,'g',-1);;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	return response;
}

QString TuiNodeDouble::getDisplayText() 
{
	if (!editing)
	{
		return prefixText + q_(displayText) + QString("  %1").arg(value);
	}
	else
	{
		return prefixText + q_(displayText) + QString(" >%1<").arg(stringValue);
	}
}

