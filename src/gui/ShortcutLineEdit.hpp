/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
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

//! Specialised GUI control for entering keyboard shortcut combinations.
//! Allows Emacs-style key sequences (for example "Ctrl+E, Ctrl+2",
//! see the documentation of QKeySequence for details.)
class ShortcutLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	ShortcutLineEdit(QWidget* parent);

	QKeySequence getKeySequence();
	bool isEmpty() const;

public slots:
	//! Clear contents and stored keys.
	void clear();
	//! Remove the last key from the key sequence.
	void backspace();
	void setContents(QKeySequence ks);

signals:
	//! Needed for enabling/disabling buttons in ShortcutsDialog.
	void focusChanged(bool focus);
	void contentsChanged();

protected:
	void keyPressEvent(QKeyEvent *e);
	void focusInEvent(QFocusEvent *e);
	void focusOutEvent(QFocusEvent *e);

private:
	//! transform modifiers to int.
	static int getModifiers(Qt::KeyboardModifiers state, const QString &text);

	//! Length of the stored key sequence.
	int m_keyNum;
	int m_keys[4]; // QKeySequence allows only 4 keys in single shortcut
};

#endif // SHORTCUTLINEEDIT_H
