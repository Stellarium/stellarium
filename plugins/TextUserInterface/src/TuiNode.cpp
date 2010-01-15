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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "TuiNode.hpp"
#include <QKeyEvent>

TuiNode::TuiNode(const QString& text, TuiNode* parent, TuiNode* prev)
	: parentNode(parent), childNode(NULL), prevNode(prev), nextNode(NULL), displayText(text)
{
}

TuiNodeResponse TuiNode::handleKey(int key)
{
	return navigation(key);
}

TuiNodeResponse TuiNode::navigation(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;

	if (key==Qt::Key_Left)
	{
		if (parentNode != NULL)
		{
			parentNode->setChildNode(this);
			response.newNode = this->parentNode;
		}
		response.accepted = true;
	}
	else if (key==Qt::Key_Right)
	{
		if (childNode != NULL)
		{
			response.newNode = childNode;
		}
		response.accepted = true;
	}
	else if (key==Qt::Key_Up)
	{
		if (prevNode != NULL)
		{
			response.newNode = prevNode;
		}
		response.accepted = true;
	}
	else if (key==Qt::Key_Down)
	{
		if (nextNode != NULL)
		{
			response.newNode = nextNode;
		}
		response.accepted = true;
	}
	return response;
}

QString TuiNode::getDisplayText()
{
	return displayText;
}


