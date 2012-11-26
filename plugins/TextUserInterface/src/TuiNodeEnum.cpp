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
#include "TuiNodeEnum.hpp"
#include <QKeyEvent>

TuiNodeEnum::TuiNodeEnum(const QString& text, QObject* receiver, const char* method, QStringList items,
                         QString defValue, TuiNode* parent, TuiNode* prev)
    : TuiNodeEditable(text, parent, prev), stringList(items), defValue(defValue)
{
	this->connect(this, SIGNAL(setValue(QString)), receiver, method);

	if (stringList.contains(defValue))
		currentIdx = stringList.indexOf(defValue);
	else
		currentIdx = 0;
}	

TuiNodeResponse TuiNodeEnum::handleEditingKey(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;
	if (key==Qt::Key_Left)
	{
		editing = false;
		response.accepted = true;
		return response;
	}
	else if (key==Qt::Key_Up)
	{
		if (currentIdx > 0)
			currentIdx--;
		response.accepted = true;
		return response;
	}
	else if (key==Qt::Key_Down)
	{
		if (currentIdx+1 < stringList.size())
			currentIdx++;
		response.accepted = true;
		return response;
	}
	else if (key==Qt::Key_Return)
	{
		emit(setValue(stringList.at(currentIdx)));
		response.accepted = true;
		response.newNode = this;
	}
	return response;
}

QString TuiNodeEnum::getDisplayText() 
{
    if (!stringList.isEmpty())
    {
        QString value = q_(stringList.at(currentIdx));
        if (!editing)
        {
            return prefixText + q_(displayText) + QString(":  %1").arg(value);
        }
        else
        {
            return prefixText + q_(displayText) + QString(": >%1<").arg(value);
        }
    }
    else
    {
        return prefixText + q_(displayText) + QString(":  %1").arg(defValue);
    }
}
