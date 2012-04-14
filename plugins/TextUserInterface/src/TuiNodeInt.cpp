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
#include "TuiNodeInt.hpp"
#include <QKeyEvent>


TuiNodeInt::TuiNodeInt(const QString& text, QObject* receiver, const char* method, int defValue,
                       int min, int max, int inc, TuiNode* parent, TuiNode* prev)
	: TuiNodeEditable(text, parent, prev), value(defValue), minimum(min), maximum(max), increment(inc), typing(false)
{
	this->connect(this, SIGNAL(setValue(int)), receiver, method);
}

TuiNodeResponse TuiNodeInt::handleEditingKey(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;
	if (key==Qt::Key_Left || key==Qt::Key_Return)
	{
		editing = false;
		typing = false;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Up)
	{
		typing = false;
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
		value-=increment;
		if (value < minimum)
			value = minimum;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	else if (key>=Qt::Key_0 && key<=Qt::Key_9)
	{
		QString s;
		if (!typing)
		{
			typing = true;
			s = QString("%1").arg(key - Qt::Key_0);
		}
		else
		{
			s = QString("%1%2").arg(value).arg(key - Qt::Key_0);
		}

		if (s.toInt()>=minimum && s.toInt()<=maximum)
		{
			value = s.toInt();
		}
		response.accepted = true;
		response.newNode = this;
		emit(setValue(value));
		return response;
	}
	else if (key==Qt::Key_Backspace)
	{
		typing = true;
		QString s = QString("%1").arg(value);
		if (s.length()==1)
		{
			value = 0;
		}
		else
		{
			s = s.left(s.length()-1);
			bool ok;
			int i = s.toInt(&ok);
			if (ok)
			{
				if (i>=minimum && i<=maximum)
				{
					value = i;
				}
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

QString TuiNodeInt::getDisplayText() 
{
	if (!editing)
	{
		return prefixText + q_(displayText) + QString("  %1").arg(value);
	}
	else
	{
		return prefixText + q_(displayText) + QString(" >%1<").arg(value);
	}
}

