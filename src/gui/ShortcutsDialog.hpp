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

#include <QLineEdit>

//! Specialised GUI control for entering keyboard shortcut combinations.
//! Allows Emacs-style key sequences (for example "Ctrl+E, Ctrl+2",
//! see the documentation of QKeySequence for details.)
class ShortcutLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	ShortcutLineEdit(QWidget* parent);

	QKeySequence getKeySequence();
	bool isEmpty() const
	{
		return (m_keyNum <= 0);
	}

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

class Ui_shortcutsDialogForm;
class StelShortcut;
class StelShortcutGroup;
class StelShortcutMgr;
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

class ShortcutsDialog : public StelDialog
{
	Q_OBJECT

public:
	ShortcutsDialog();
	~ShortcutsDialog();

	//! higlight items that have collisions with current lineEdits' state according to css.
	//! Note: previous collisions aren't redrawn.
	void drawCollisions();

public slots:
	//! restore colors of all items it TreeWidget to defaults.
	void resetCollisions();
	void retranslate();
	//! ititialize editors state when current item changed.
	void initEditors();
	//! checks whether one QKeySequence is prefix of another.
	bool prefixMatchKeySequence(QKeySequence ks1, QKeySequence ks2);
	QList<QTreeWidgetItem*> findCollidingItems(QKeySequence ks);
	void handleCollisions(ShortcutLineEdit* currentEdit);
	//! called when editors' state changed.
	void handleChanges();
	//! called when apply button clicked.
	void applyChanges() const;
	//! called by doubleclick; if click is on editable item, switch to editors
	void switchToEditors(QTreeWidgetItem* item, int column);
	//! update shortcut representation in tree correspondingly to its actual contents
	//! if no shortcutTreeItem specified, search for it in tree, if no items found, create new item
	void updateShortcutsItem(StelShortcut* shortcut, QTreeWidgetItem* shortcutsTreeItem = NULL);
	void restoreDefaultShortcuts();
	void updateTreeData();

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent();

private:
	//! checks whether given item can be changed by editors.
	static bool itemIsEditable(QTreeWidgetItem *item);
	//! Concatenate the header, key codes and footer to build
	//! up the help text.
	//! @todo FIXME: This does nothing? 
	void updateText();

	//! Apply style changes.
	//! See http://qt-project.org/faq/answer/how_can_my_stylesheet_account_for_custom_properties
	void polish();

	QTreeWidgetItem* updateGroup(StelShortcutGroup* group);

	//! search for first appearence of item with requested data.
	QTreeWidgetItem* findItemByData(QVariant value, int role, int column = 0);

	//! pointer to mgr, for not getting it from stelapp every time.
	StelShortcutMgr* shortcutMgr;

	//! list for storing collisions items, so we can easy restore their colors.
	QList<QTreeWidgetItem*> collisionItems;

	Ui_shortcutsDialogForm *ui;
};

#endif // SHORTCUTSDIALOG_HPP
