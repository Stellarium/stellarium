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
#include "TuiNodeBool.hpp"
#include <QKeyEvent>


TuiNodeBool::TuiNodeBool(const QString& text, QObject* receiver, const char* method, bool defValue, TuiNode* parent, TuiNode* prev)
	: TuiNodeEditable(text, parent, prev), state(defValue)
{
	this->connect(this, SIGNAL(setValue(bool)), receiver, method);
}

TuiNodeResponse TuiNodeBool::handleEditingKey(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;
	if (key==Qt::Key_Left || key==Qt::Key_Return)
	{
		editing = false;
		response.accepted = true;
		emit(setValue(state));
		return response;
	}
	else if (key==Qt::Key_Up || key==Qt::Key_Down || key==Qt::Key_Return)
	{
		state = !state;
		response.accepted = true;
		emit(setValue(state));
		return response;
	}
	else if (key==Qt::Key_1 || key==Qt::Key_Y || key==Qt::Key_T)
	{
		state = true;
		emit(setValue(state));
		response.accepted = true;
		emit(setValue(state));
		return response;
	}
	else if (key==Qt::Key_0 || key==Qt::Key_N || key==Qt::Key_F)
	{
		state = false;
		emit(setValue(state));
		response.accepted = true;
		emit(setValue(state));
		return response;
	}
	return response;
}

QString TuiNodeBool::getDisplayText() 
{
	// TODO: The label/value separation needs to be reworked. This way of using
	// the colon is not i18n-friendly. --BM
	QString stringOn = q_("On");
	QString stringOff = q_("Off");
	QString value;
	if (!editing)
	{
		if (state)
			value = QString(":  %1").arg(stringOn);
		else
			value = QString(":  %1").arg(stringOff);
	}
	else
	{
		if (state)
			value = QString(": >%1<").arg(stringOn);
		else
			value = QString(": >%1<").arg(stringOff);
	}
	return prefixText + q_(displayText) + value;
}

