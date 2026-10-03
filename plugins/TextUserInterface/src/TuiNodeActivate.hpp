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
	//! @param method the method that will be called when the node is activated.
	//! @param parent the node for the parent menu item
	//! @param prev the previous node in the current menu (typically 
	//! shares the same parent)
	template<typename PointerToMethod, typename Receiver>
	TuiNodeActivate(const QString& text, Receiver* receiver, PointerToMethod method,
	                TuiNode* parent=nullptr, TuiNode* prev=nullptr)
		: TuiNode(text, parent, prev)
	{
		this->connect(this, &TuiNodeActivate::activate, receiver, method);
	}

	TuiNodeResponse handleKey(int key) override;
	QString getDisplayText() const override;

signals:
	void activate();
};

#endif /* TUINODEACTIVATE_HPP */
