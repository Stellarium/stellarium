/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
 * Copyright (C) 2012 Bogdan Marinov
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

#include <QDebug>
#include <QKeyEvent>

#include "ShortcutLineEdit.hpp"

ShortcutLineEdit::ShortcutLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	// call clear for setting up private fields
	clear();
}

QKeySequence ShortcutLineEdit::getKeySequence()
{
	return QKeySequence(keys.value(0, 0), keys.value(1, 0),
	                    keys.value(2, 0), keys.value(3, 0));
}

bool ShortcutLineEdit::isEmpty() const
{
	return (keys.isEmpty());
}

void ShortcutLineEdit::clear()
{
	keys.clear();
	QLineEdit::clear();
	emit contentsChanged();
}

void ShortcutLineEdit::backspace()
{
	if (keys.isEmpty())
		return;
	keys.removeLast();
	setContents(getKeySequence());
}

void ShortcutLineEdit::setContents(QKeySequence ks)
{
	// Avoiding infinite loop of same signal-slot emitting/calling
	if (ks.toString(QKeySequence::NativeText) == text())
		return;
	
	// Set the keys from the given key sequence
	clear();
	for (uint i = 0; i < ks.count(); ++i)
	{
		keys.append(ks[i]);
	}
	
	// Show Ctrl button as Cmd on Mac
	setText(ks.toString(QKeySequence::NativeText));
	emit contentsChanged();
}

void ShortcutLineEdit::keyPressEvent(QKeyEvent *e)
{
	int nextKey = e->key();
	if ( keys.count() > 3 || // too long shortcut
	     nextKey == Qt::Key_Control || // dont count modifier keys
	     nextKey == Qt::Key_Shift ||
	     nextKey == Qt::Key_Meta ||
	     nextKey == Qt::Key_Alt )
		return;
	
	// applying current modifiers to key
	nextKey |= getModifiers(e->modifiers(), e->text());
	
	// If there is selected text, replace *all* instead of appending. 
	if (hasSelectedText())
	{
		keys.clear();
	}
	keys.append(nextKey);
	
	// set displaying information
	setText(getKeySequence().toString(QKeySequence::NativeText));
	emit contentsChanged();
	
	// not call QLineEdit's event because we already changed contents
	e->accept();
}

void ShortcutLineEdit::focusInEvent(QFocusEvent *e)
{
	// Select the contents so that they are replaced on the next edit
	selectAll();
	emit focusChanged(false);
	QLineEdit::focusInEvent(e);
}

void ShortcutLineEdit::focusOutEvent(QFocusEvent *e)
{
	emit focusChanged(true);
	QLineEdit::focusOutEvent(e);
}


int ShortcutLineEdit::getModifiers(Qt::KeyboardModifiers state, const QString &text)
{
	int result = 0;
	// The shift modifier only counts when it is not used to type a symbol
	// that is only reachable using the shift key anyway
	if ((state & Qt::ShiftModifier) && (text.size() == 0
	                                    || !text.at(0).isPrint()
	                                    || text.at(0).isLetterOrNumber()
	                                    || text.at(0).isSpace()))
		result |= Qt::SHIFT;
	if (state & Qt::ControlModifier)
		result |= Qt::CTRL;
	// META key is the same as WIN key on non-MACs
	if (state & Qt::MetaModifier)
		result |= Qt::META;
	if (state & Qt::AltModifier)
		result |= Qt::ALT;
	return result;
}
