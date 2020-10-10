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

#include <QKeySequence>
#include <QModelIndex>
#include <QSortFilterProxyModel>

#include "StelDialog.hpp"


class Ui_shortcutsDialogForm;
class ShortcutLineEdit;

class QStandardItemModel;
class QStandardItem;


//! Custom filter class for filtering tree sub-items.
//! (The standard QSortFilterProxyModel shows child items only if the
//! parent item matches the filter.)
class ShortcutsFilterModel : public QSortFilterProxyModel
{
	Q_OBJECT
	
public:
	ShortcutsFilterModel(QObject* parent = Q_NULLPTR);
	
protected:
	virtual bool filterAcceptsRow(int source_row,
			      const QModelIndex &source_parent) const Q_DECL_OVERRIDE;
};


class ShortcutsDialog : public StelDialog
{
	Q_OBJECT

public:
	ShortcutsDialog(QObject* parent);
	~ShortcutsDialog() Q_DECL_OVERRIDE;

	//! highlight items that have collisions with current lineEdits' state according to CSS.
	//! Note: previous collisions aren't redrawn.
	void drawCollisions();

public slots:
	//! restore colors of all items it TreeWidget to defaults.
	void resetCollisions();
	virtual void retranslate() Q_DECL_OVERRIDE;
	//! initialize editors state when current item changed.
	void initEditors();
	//! checks whether one QKeySequence is prefix of another.
	bool prefixMatchKeySequence(const QKeySequence &ks1, const QKeySequence &ks2);
	//! Compile a list of items that share a prefix with this sequence.
	QList<QStandardItem*> findCollidingItems(QKeySequence ks);
	void handleCollisions(ShortcutLineEdit* currentEdit);
	//! called when editors' state changed.
	void handleChanges();
	//! called when apply button clicked.
	void applyChanges();
	//! called by double-click; if click is on editable item, switch to editors
	void switchToEditors(const QModelIndex& index);
	//! update shortcut representation in tree correspondingly to its actual contents.
	//! if no item is specified, search for it in tree, if no items found, create new item
	void updateShortcutsItem(class StelAction* action, QStandardItem* shortcutItem = Q_NULLPTR);
	void restoreDefaultShortcuts();
	void restoreAllDefaultShortcuts();
	void updateTreeData();

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent() Q_DECL_OVERRIDE;

private:
	//! checks whether given item can be changed by editors.
	static bool itemIsEditable(QStandardItem *item);

	//! Apply style changes.
	//! See http://qt-project.org/faq/answer/how_can_my_stylesheet_account_for_custom_properties
	void polish();

	QStandardItem* updateGroup(const QString& group);

	//! search for first appearance of item with requested data.
	QStandardItem* findItemByData(QVariant value, int role, int column = 0) const;

	//! pointer to mgr, for not getting it from StelApp every time.
	class StelActionMgr* actionMgr;

	//! list for storing collisions items, so we can easy restore their colors.
	QList<QStandardItem*> collisionItems;

	Ui_shortcutsDialogForm *ui;
	ShortcutsFilterModel* filterModel;
	QStandardItemModel* mainModel;
	//! Initialize or reset the main model.
	void resetModel();
	//! Set the main model's column lables.
	void setModelHeader();
};

#endif // SHORTCUTSDIALOG_HPP
