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
 
#ifndef _TUINODEEDITABLE_HPP_
#define _TUINODEEDITABLE_HPP_ 1

#include "TuiNode.hpp"
#include <QObject>


//! @class TuiNodeEditable
//! pure virtual from which editables for different data types are derived.
class TuiNodeEditable : public TuiNode
{
	Q_OBJECT
public:
	TuiNodeEditable(const QString& text, TuiNode* parent=NULL, TuiNode* prev=NULL);
	virtual TuiNodeResponse handleKey(int key);
	virtual TuiNodeResponse handleEditingKey(int key) = 0;

protected:
	bool editing;
};

#endif /*_TUINODEEDITABLE_HPP_*/

