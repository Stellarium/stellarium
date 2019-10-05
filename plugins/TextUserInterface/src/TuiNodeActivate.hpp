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
 
#ifndef TUINODEACTIVATE_HPP
#define TUINODEACTIVATE_HPP

#include <TuiNode.hpp>
#include <QObject>

//! @class TuiNodeActivate
//! Allows navigation but also sends a signal to a specified object when
//! the return key is pressed. 
class TuiNodeActivate : public TuiNode
{
	Q_OBJECT

public:
	//! Create a TuiNodeActivate node.
	//! @param text the text to be displayed for this node
	//! @param receiver a QObject which will receive the activation signal
	//! @param method the method in the receiver which will be called when
	//! the node is activated.  Note that this should be passed using the 
	//! SLOT() macro.
	//! @param parent the node for the parent menu item
	//! @param prev the previous node in the current menu (typically 
	//! shares the same parent)
	TuiNodeActivate(const QString& text, QObject* receiver, const char* method, TuiNode* parent=Q_NULLPTR, TuiNode* prev=Q_NULLPTR);
	virtual TuiNodeResponse handleKey(int key);
	virtual QString getDisplayText() const;

signals:
	void activate();
};

#endif /* TUINODEACTIVATE_HPP */
