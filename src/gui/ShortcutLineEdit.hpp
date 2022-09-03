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
#ifndef SHORTCUTLINEEDIT_H
#define SHORTCUTLINEEDIT_H

#include <QLineEdit>
#include <QKeySequence>

//! Specialized GUI control for entering keyboard shortcut combinations.
//! Allows Emacs-style key sequences (for example "Ctrl+E, Ctrl+2")
//! no longer than four combinations. See the documentation of 
//! QKeySequence for details.
//! 
//! When ShortcutLineEdit receives the focus, its whole contents get
//! selected. On a key press, if any of the contents are selected,
//! the new key replaces @em all the previous contents. Otherwise, it's
//! appended to the sequence.
class ShortcutLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	ShortcutLineEdit(QWidget* parent = Q_NULLPTR);

	QKeySequence getKeySequence() const;
	bool isEmpty() const;

public slots:
	//! Clear contents and stored keys.
	void clear();
	//! Remove the last key from the key sequence.
	void backspace();
	//! Emits contentsChanged() if the new sequence is not the same.
	void setContents(QKeySequence ks);

signals:
	//! Needed for enabling/disabling buttons in ShortcutsDialog.
	//! @param[out] focus @b false if the widget has the focus,
	//! true otherwise.
	void focusChanged(bool focus);
	void contentsChanged();

protected:
	virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
	virtual void focusInEvent(QFocusEvent *e) Q_DECL_OVERRIDE;
	virtual void focusOutEvent(QFocusEvent *e) Q_DECL_OVERRIDE;

private:
	//! transform modifiers to int.
	static int getModifiers(Qt::KeyboardModifiers state, const QString &text);

	//! Codes of the keys in the manipulated key sequence.
	QList<int> keys;
};

#endif // SHORTCUTLINEEDIT_H
