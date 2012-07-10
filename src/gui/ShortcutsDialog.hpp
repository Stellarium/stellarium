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
	// clear contents; also clear stored keys
	void clear();
	void setContents(QKeySequence ks);

signals:
	// need for enable/disable buttons in dialog
	void focusChanged(bool focus);
	void contentsChanged();

protected:
	void keyPressEvent(QKeyEvent *e);
	void focusInEvent(QFocusEvent *e);
	void focusOutEvent(QFocusEvent *e);

private:
	// transform modifiers to int
	int getModifiers(Qt::KeyboardModifiers state, const QString &text);	

	// counter and array for store keys entered
	int m_keyNum;
	int m_keys[4]; // QKeySequence allows only 4 keys in single shortcut
};

QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

class ShortcutsDialog : public StelDialog
{
	Q_OBJECT

public:
	ShortcutsDialog();
	~ShortcutsDialog();

	// higlight items that have collisions with current lineEdits' state according to css
	void paintCollisions(QList<QTreeWidgetItem*> items);

public slots:
	// restore colors of all items it TreeWidget to defaults
	void resetCollisions();
	void retranslate();
	// ititialize editors state when current item changed
	void initEditors();
	void handleCollisions();
	// called when editors' state changed
	void handleChanges();
	// called when apply button clicked
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

	// list for storing collisions items, so we can easy restore their colors
	QList<QTreeWidgetItem*> collisionItems;

	Ui_shortcutsDialogForm *ui;
};

#endif // SHORTCUTSDIALOG_HPP
