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
#include "TuiNodeActivate.hpp"
#include <QKeyEvent>


TuiNodeActivate::TuiNodeActivate(const QString& text, QObject* receiver, const char* method, TuiNode* parent, TuiNode* prev)
	: TuiNode(text, parent, prev)
{
	this->connect(this, SIGNAL(activate()), receiver, method);
}

TuiNodeResponse TuiNodeActivate::handleKey(int key)
{
	if (key==Qt::Key_Return)
	{
		emit(activate());
		TuiNodeResponse response;
		response.accepted = true;
		response.newNode = this;
		return response;
	}
	else
	{
		return navigation(key);
	}
}

QString TuiNodeActivate::getDisplayText() 
{
	return prefixText + q_(displayText) + q_(" [RETURN to activate]");
}

