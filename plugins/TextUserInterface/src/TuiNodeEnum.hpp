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
 
#ifndef TUINODEENUM_HPP
#define TUINODEENUM_HPP

#include "TuiNodeEditable.hpp"
#include <QObject>
#include <QString>
#include <QStringList>


//! @class TuiNodeEnum
//! Allows navigation but also selection from a list of string values.
class TuiNodeEnum : public TuiNodeEditable
{
	Q_OBJECT

public:
	//! Create a TuiNodeEnum node.
	//! @param text the text to be displayed for this node
	//! @param receiver a QObject which will receive a signal when the value is changed
	//! @param method the method in the receiver which will be called when the value is changed.  Note that this should be passed using the SLOT() macro.
	//! @param items a list of string values which the item may take.
	//! @param defValue the string value which is used as the initially selected value.  Note if this is not in the items list, the first item in the items list will be used instead.
	//! @param parent the node for the parent menu item
	//! @param prev the previous node in the current menu (typically shares the same parent)
	TuiNodeEnum(const QString& text, QObject* receiver, const char* method, QStringList items, 
		    QString defValue, TuiNode* parent=Q_NULLPTR, TuiNode* prev=Q_NULLPTR);
	virtual TuiNodeResponse handleEditingKey(int key);
	virtual QString getDisplayText() const;

signals:
	void setValue(QString s);

private:
	int currentIdx;
	QStringList stringList;
	QString defValue;
};

#endif /*TUINODEENUM_HPP*/

