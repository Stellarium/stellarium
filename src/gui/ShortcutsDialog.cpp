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

#include <QDebug>
#include <QDialog>

#include "StelApp.hpp"
#include "StelShortcutMgr.hpp"
#include "StelTranslator.hpp"
#include "StelShortcutGroup.hpp"

#include "ShortcutsDialog.hpp"
#include "ui_shortcutsDialog.h"

ShortcutLineEdit::ShortcutLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	// call clear for setting up private fields
	clear();
}

QKeySequence ShortcutLineEdit::getKeySequence()
{
	return QKeySequence(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
}

void ShortcutLineEdit::clear()
{
	m_keyNum = m_keys[0] = m_keys[1] = m_keys[2] = m_keys[3] = 0;
	QLineEdit::clear();
	emit contentsChanged();
}

void ShortcutLineEdit::backspace()
{
	if (m_keyNum <= 0)
	{
		qWarning() << "Clear button works when it shouldn't: lineEdit is empty ";
		return;
	}
	--m_keyNum;
	m_keys[m_keyNum] = 0;
	// update text
	setContents(getKeySequence());
}

void ShortcutLineEdit::setContents(QKeySequence ks)
{
	// need for avoiding infinite loop of same signal-slot emitting/calling
	if (ks.toString() == text())
		return;
	// clear before setting up
	clear();
	// set up m_keys from given key sequence
	m_keyNum = ks.count();
	for (int i = 0; i < m_keyNum; ++i)
	{
		m_keys[i] = ks[i];
	}
	setText(ks.toString());
	emit contentsChanged();
}

void ShortcutLineEdit::keyPressEvent(QKeyEvent *e)
{
	int nextKey = e->key();
	if ( m_keyNum > 3 || // too long shortcut
			 nextKey == Qt::Key_Control || // dont count modifier keys
			 nextKey == Qt::Key_Shift ||
			 nextKey == Qt::Key_Meta ||
			 nextKey == Qt::Key_Alt )
		return;
	// applying current modifiers to key
	nextKey |= getModifiers(e->modifiers(), e->text());
	m_keys[m_keyNum] = nextKey;
	++m_keyNum;
	// set displaying information
	QKeySequence ks(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
	setText(ks);
	emit contentsChanged();
	// not call QLineEdit's event because we already changed contents
	e->accept();
}

void ShortcutLineEdit::focusInEvent(QFocusEvent *e)
{
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

ShortcutsDialog::ShortcutsDialog() :
	ui(new Ui_shortcutsDialogForm)
{
	shortcutMgr = StelApp::getInstance().getStelShortcutManager();
}

ShortcutsDialog::~ShortcutsDialog()
{
	collisionItems.clear();
	delete ui; ui = NULL;
}

void ShortcutsDialog::paintCollisions(QList<QTreeWidgetItem *> items)
{
	collisionItems.append(items);
	foreach(QTreeWidgetItem* item, items)
	{
		// change colors of all columns for better visibility
		item->setForeground(0, Qt::red);
		item->setForeground(1, Qt::red);
		item->setForeground(2, Qt::red);
	}
}

void ShortcutsDialog::resetCollisions()
{
	foreach(QTreeWidgetItem* item, collisionItems)
	{
		QColor defaultColor = ui->shortcutsTreeWidget->palette().color(QPalette::Foreground);
		item->setForeground(0, defaultColor);
		item->setForeground(1, defaultColor);
		item->setForeground(2, defaultColor);
	}
	collisionItems.clear();
}

void ShortcutsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateText();
	}
}

void ShortcutsDialog::initEditors()
{
	if (itemIsEditable(ui->shortcutsTreeWidget->currentItem())) {
		// current item is shortcut, not group (group items aren't selectable)
		ui->primaryShortcutEdit->setEnabled(true);
		ui->altShortcutEdit->setEnabled(true);
		// fill editors with item's shortcuts
		ui->primaryShortcutEdit->setContents(
					ui->shortcutsTreeWidget->currentItem()->
					data(1, Qt::DisplayRole).value<QKeySequence>());
		ui->altShortcutEdit->setContents(
					ui->shortcutsTreeWidget->currentItem()->
					data(2, Qt::DisplayRole).value<QKeySequence>());
		handleChanges();
	}
	else {
		// item is group, not shortcut
		ui->primaryShortcutEdit->setEnabled(false);
		ui->altShortcutEdit->setEnabled(false);
		ui->applyButton->setEnabled(false);
		ui->primaryShortcutEdit->clear();
		ui->altShortcutEdit->clear();
	}
}

void ShortcutsDialog::handleCollisions()
{
	QString primText = ui->primaryShortcutEdit->text();
	QString altText = ui->altShortcutEdit->text();
	// clear previous collisions
	resetCollisions();
	QList<QTreeWidgetItem*> collisionList;
	bool collisionInPrimeEdit = false;
	bool collisionInAltEdit = false;
	if (!primText.isEmpty())
	{
		// check in primary shortcuts
		collisionList.append(ui->shortcutsTreeWidget->findItems(primText, Qt::MatchFixedString | Qt::MatchRecursive, 1));
		// check in alternative shortcuts
		collisionList.append(ui->shortcutsTreeWidget->findItems(primText, Qt::MatchFixedString | Qt::MatchRecursive, 2));
		// remove current item
		collisionList.removeOne(ui->shortcutsTreeWidget->currentItem());
	}
	if (!collisionList.isEmpty())
	{
		collisionInPrimeEdit = true;
		paintCollisions(collisionList);
	}
	// clear for proper handling for alternative edit
	collisionList.clear();
	if (!altText.isEmpty())
	{
		// check in primary shortcuts
		collisionList.append(ui->shortcutsTreeWidget->findItems(altText, Qt::MatchFixedString | Qt::MatchRecursive, 1));
		// check in alternative shortcuts
		collisionList.append(ui->shortcutsTreeWidget->findItems(altText, Qt::MatchFixedString | Qt::MatchRecursive, 2));
		// remove current item
		collisionList.removeOne(ui->shortcutsTreeWidget->currentItem());
	}
	if (!collisionList.isEmpty())
	{
		collisionInAltEdit = true;
		paintCollisions(collisionList);
	}
	if (collisionInPrimeEdit || collisionInAltEdit)
	{
		ui->applyButton->setEnabled(false);
		// scrolling to first collision item
		ui->shortcutsTreeWidget->scrollToItem(collisionItems.first());
	}
	else
	{
		// scrolling back to current item
		ui->shortcutsTreeWidget->scrollToItem(ui->shortcutsTreeWidget->currentItem());
	}
	ui->primaryShortcutEdit->setProperty("collision", collisionInPrimeEdit);
	ui->altShortcutEdit->setProperty("collision", collisionInAltEdit);
}

void ShortcutsDialog::handleChanges()
{
	// updating clear buttons
	if (ui->primaryShortcutEdit->isEmpty())
	{
		ui->primaryBackspaceButton->setEnabled(false);
	}
	else
	{
		ui->primaryBackspaceButton->setEnabled(true);
	}
	if (ui->altShortcutEdit->isEmpty())
	{
		ui->altBackspaceButton->setEnabled(false);
	}
	else
	{
		ui->altBackspaceButton->setEnabled(true);
	}
	// updating apply button
	QString primText = ui->primaryShortcutEdit->text();
	QString altText = ui->altShortcutEdit->text();
	if (ui->shortcutsTreeWidget->currentItem() == NULL ||
			(primText == ui->shortcutsTreeWidget->currentItem()->text(1) &&
			 altText == ui->shortcutsTreeWidget->currentItem()->text(2)))
	{
		// nothing to apply
		ui->applyButton->setEnabled(false);
	}
	else
	{
		ui->applyButton->setEnabled(true);
	}
	handleCollisions();
	// apply style changes, see http://qt-project.org/faq/answer/how_can_my_stylesheet_account_for_custom_properties
	ui->primaryShortcutEdit->style()->unpolish(ui->primaryShortcutEdit);
	ui->primaryShortcutEdit->style()->polish(ui->primaryShortcutEdit);
	ui->altShortcutEdit->style()->unpolish(ui->altShortcutEdit);
	ui->altShortcutEdit->style()->polish(ui->altShortcutEdit);
}

void ShortcutsDialog::applyChanges() const
{
	// get ids stored in tree
	QString actionId = ui->shortcutsTreeWidget->currentItem()->data(0, Qt::UserRole).toString();
	QString groupId = ui->shortcutsTreeWidget->currentItem()->parent()->
			data(0, Qt::UserRole).toString();
	// changing keys in shortcuts
	shortcutMgr->changeActionPrimaryKey(actionId, groupId, ui->primaryShortcutEdit->getKeySequence());
	shortcutMgr->changeActionAltKey(actionId, groupId, ui->altShortcutEdit->getKeySequence());
	// no need to change displaying information, as it changed in mgr, and will be updated in connected slot

	// save shortcuts to file
	shortcutMgr->saveShortcuts();

	// nothing to apply until edits' content changes
	ui->applyButton->setEnabled(false);
}

void ShortcutsDialog::switchToEditors(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);
	if (itemIsEditable(item))
	{
		ui->primaryShortcutEdit->setFocus();
	}
}

void ShortcutsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->shortcutsTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(initEditors()));
	connect(ui->shortcutsTreeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(switchToEditors(QTreeWidgetItem*, int)));
	// apply button logic
	connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(applyChanges()));
	// restore defaults button logic
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaultShortcuts()));
	// we need to disable all shortcut actions, so we can enter shortcuts without activating any actions
	connect(ui->primaryShortcutEdit, SIGNAL(focusChanged(bool)), shortcutMgr, SLOT(setAllActionsEnabled(bool)));
	connect(ui->altShortcutEdit, SIGNAL(focusChanged(bool)), shortcutMgr, SLOT(setAllActionsEnabled(bool)));
	// handling changes in editors
	connect(ui->primaryShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));
	connect(ui->altShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));
	// handle outer shortcuts changes
	connect(shortcutMgr, SIGNAL(shortcutChanged(StelShortcut*)), this, SLOT(updateShortcutsItem(StelShortcut*)));

	updateTreeData();
}

void ShortcutsDialog::updateText()
{
}

QTreeWidgetItem *ShortcutsDialog::updateGroup(StelShortcutGroup *group)
{
	QTreeWidgetItem* groupItem = findItemByData(QVariant(group->getId()), Qt::UserRole);
	if (!groupItem)
	{
		// create new
		groupItem = new QTreeWidgetItem(ui->shortcutsTreeWidget);
	}
	// group items aren't selectable, so reset default flag
	groupItem->setFlags(Qt::ItemIsEnabled);
	// setup displayed text
	QString text(q_(group->getText().isEmpty() ? group->getId() : group->getText()));
	groupItem->setText(0, text);
	// store id
	groupItem->setData(0, Qt::UserRole, group->getId());
	// expand only enabled group
	groupItem->setExpanded(group->isEnabled());
	// setup bold font for group lines
	QFont rootFont = groupItem->font(0);
	rootFont.setBold(true); rootFont.setPixelSize(14);
	groupItem->setFont(0, rootFont);

	return groupItem;
}

QTreeWidgetItem *ShortcutsDialog::findItemByData(QVariant value, int role, int column)
{
	QTreeWidgetItemIterator it(ui->shortcutsTreeWidget);
	while (*it)
	{
		if ((*it)->data(column, role) == value)
		{
			return (*it);
		}
		++it;
	}
	return NULL;
}

void ShortcutsDialog::updateShortcutsItem(StelShortcut *shortcut, QTreeWidgetItem *shortcutTreeItem)
{
	if (shortcutTreeItem == NULL)
	{
		// search for item
		shortcutTreeItem = findItemByData(QVariant(shortcut->getId()), Qt::UserRole, 0);
	}
	// we didn't find item, create and add new
	if (shortcutTreeItem == NULL)
	{
		// firstly search for group
		QTreeWidgetItem* groupItem = findItemByData(QVariant(shortcut->getGroup()->getId()), Qt::UserRole, 0);
		if (groupItem == NULL)
		{
			// create and add new group to treeWidget
			groupItem = updateGroup(shortcut->getGroup());
		}
		// create shortcut item
		shortcutTreeItem = new QTreeWidgetItem(groupItem);
		// store shortcut id, so we can find it when shortcut changed
		shortcutTreeItem->setData(0, Qt::UserRole, QVariant(shortcut->getId()));
	}
	// setup properties of item
	shortcutTreeItem->setText(0, q_(shortcut->getText()));
	shortcutTreeItem->setData(1, Qt::DisplayRole, shortcut->getPrimaryKey());
	shortcutTreeItem->setData(2, Qt::DisplayRole, shortcut->getAltKey());
}

void ShortcutsDialog::restoreDefaultShortcuts()
{
	ui->shortcutsTreeWidget->clear();
	shortcutMgr->restoreDefaultShortcuts();
	updateTreeData();
	initEditors();
}

void ShortcutsDialog::updateTreeData()
{
	// Create shortcuts tree
	QList<StelShortcutGroup*> groups = shortcutMgr->getGroupList();
	foreach (StelShortcutGroup* group, groups)
	{
		updateGroup(group);
		// display group's shortcuts
		QList<StelShortcut*> shortcuts = group->getActionList();
		foreach (StelShortcut* shortcut, shortcuts)
		{
			updateShortcutsItem(shortcut);
		}
	}
	updateText();
}

bool ShortcutsDialog::itemIsEditable(QTreeWidgetItem *item)
{
	if (item == NULL) return false;
	// non-editable items(not group items) have no Qt::ItemIsSelectable flag
	return (Qt::ItemIsSelectable & item->flags());
}
