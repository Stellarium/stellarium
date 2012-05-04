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
#include "TuiNodeColor.hpp"
#include <QKeyEvent>

TuiNodeColor::TuiNodeColor(const QString& text, QObject* receiver, const char* method, Vec3f defValue, TuiNode* parent, TuiNode* prev)
	: TuiNodeEditable(text, parent, prev), value(defValue), editingPart(0)
{
	this->connect(this, SIGNAL(setValue(Vec3f)), receiver, method);
}

TuiNodeResponse TuiNodeColor::handleEditingKey(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;
	if (key==Qt::Key_Left)
	{
		if (editingPart==0)
		{
			editing = false;
		}
		else
		{
			editingPart--;
		}
		response.accepted = true;
		if (!editing) 
		{
			emit(setValue(value));
		}
		return response;
	}
	if (key==Qt::Key_Return)
	{
		editing = false;
		editingPart = 0;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	if (key==Qt::Key_Right)
	{
		if (editingPart<3)
		{
			editingPart++;
		}
		response.accepted = true;
		return response;
	}
	if (key==Qt::Key_Up)
	{
		incPart(editingPart, true);
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	if (key==Qt::Key_Down)
	{
		incPart(editingPart, false);
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	return response;
}

QString TuiNodeColor::getDisplayText() 
{
	QString red   = QString("%1").arg(value[0], 2, 'f', 2);
	QString green = QString("%1").arg(value[1], 2, 'f', 2);
	QString blue  = QString("%1").arg(value[2], 2, 'f', 2);
	const char* formatString;
	if (!editing)
	{
		formatString = ":  RGB %1,%2,%3";
	}
	else
	{
		switch (editingPart)
		{
		case 0:
			formatString = ":  RGB >%1<,%2,%3";
			break;
		case 1:
			formatString = ":  RGB %1,>%2<,%3";
			break;
		case 2:
			formatString = ":  RGB %1,%2,>%3<";
			break;
		default:
			return QString(q_("error, unknown color part \"%1\"")).arg(editingPart);
		}			
	}
	QString stringValue = QString(formatString).arg(red, green, blue);
	return prefixText + q_(displayText) + stringValue;
}

void TuiNodeColor::incPart(int part, bool add)
{
	float diff = 0.01;

	if (add)
	{
		if (value[part]+diff > 1.0)
			value[part] = 1.0;
		else
			value[part]+=diff;
	}
	else
	{
		if (value[part]-diff < 0.)
			value[part] = 0.;
		else
			value[part]-=diff;

	}
}

