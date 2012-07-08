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

#ifndef SHORTCUTSDIALOG_HPP
#define SHORTCUTSDIALOG_HPP

#include "StelDialog.hpp"
#include "StelShortcutMgr.hpp"

#include <QLineEdit>

class Ui_shortcutsDialogForm;
class ShortcutsDialog;

class ShortcutLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	ShortcutLineEdit(QWidget* parent);

	QKeySequence getKeySequence();

public slots:
	void clear();

signals:
	void focusChanged(bool focus);
	// emits when content is changed; resetable defines whether content can be changed back or not
	void contentsChanged();

protected:
	void keyPressEvent(QKeyEvent *e);
	void focusInEvent(QFocusEvent *e);
	void focusOutEvent(QFocusEvent *e);

private:
	int getModifiers(Qt::KeyboardModifiers state, const QString &text);

	// counter and array for store keys entered
	int m_keyNum;
	int m_keys[4]; // QKeySequence allows only 4 keys in single shortcut
};

class ShortcutsDialog : public StelDialog
{
	Q_OBJECT

public:
	ShortcutsDialog();
	~ShortcutsDialog();

public slots:
	void retranslate();
	void initEditors();
	void setActionsEnabled(bool enable);
	void handleChanges();
	void applyChanges();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

private:
	//! This function concatenates the header, key codes and footer to build
	//! up the help text.
	void updateText(void);

	// pointer to mgr, for not getting it from stelapp every time
	StelShortcutMgr* shortcutMgr;

	Ui_shortcutsDialogForm *ui;
};

#endif // SHORTCUTSDIALOG_HPP
