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

#include "StelApp.hpp"
#include "StelShortcutMgr.hpp"

#include <QDebug>
#include <QDialog>
#include <QLineEdit>

#include "ShortcutsDialog.hpp"
#include "ui_shortcutsDialog.h"

ShortcutLineEdit::ShortcutLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	clear();
}

void ShortcutLineEdit::clear()
{
	m_keyNum = m_keys[0] = m_keys[1] = m_keys[2] = m_keys[3] = 0;
	emit contentsChanged();
	QLineEdit::clear();
}

void ShortcutLineEdit::keyPressEvent(QKeyEvent *e)
{
	int nextKey = e->key();
	if ( m_keyNum > 3 || // too long shortcut
			 nextKey == Qt::Key_Control ||
			 nextKey == Qt::Key_Shift ||
			 nextKey == Qt::Key_Meta ||
			 nextKey == Qt::Key_Alt )
		return;
	nextKey |= getModifiers(e->modifiers(), e->text());
	m_keys[m_keyNum] = nextKey;
	++m_keyNum;
	QKeySequence ks(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
	setText(ks);
	emit contentsChanged();
	e->accept();
}

void ShortcutLineEdit::focusInEvent(QFocusEvent *e)
{
	emit focusChanged(true);
	QLineEdit::focusInEvent(e);
}

void ShortcutLineEdit::focusOutEvent(QFocusEvent *e)
{
	emit focusChanged(false);
	QLineEdit::focusOutEvent(e);
}


int ShortcutLineEdit::getModifiers(Qt::KeyboardModifiers state,
																				 const QString &text)
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
	delete ui; ui = NULL;
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
	if (ui->shortcutsTreeWidget->currentItem()->isSelected()) {
		ui->primaryShortcutEdit->setEnabled(true);
		ui->altShortcutEdit->setEnabled(true);
		ui->primaryShortcutEdit->setText(
					ui->shortcutsTreeWidget->currentItem()->text(1));
		ui->altShortcutEdit->setText(
					ui->shortcutsTreeWidget->currentItem()->text(2));
		handleChanges();
	}
	else {
		ui->primaryShortcutEdit->setEnabled(false);
		ui->altShortcutEdit->setEnabled(false);
		ui->applyButton->setEnabled(false);
		ui->primaryShortcutEdit->clear();
		ui->altShortcutEdit->clear();
	}
}

void ShortcutsDialog::setActionsEnabled(bool enable)
{
	if (enable)
	{
		shortcutMgr->disableAllActions();
	}
	else
	{
		shortcutMgr->enableAllActions();
	}
}

void ShortcutsDialog::handleChanges()
{
	QString changedText;
	// updating apply button
	if (ui->primaryShortcutEdit->hasFocus())
	{
		changedText = ui->primaryShortcutEdit->text();
		if (changedText == ui->shortcutsTreeWidget->currentItem()->text(1))
		{
			// nothing to apply
			ui->applyButton->setEnabled(false);
		}
		else
		{
			ui->applyButton->setEnabled(true);
		}
	}
	if (ui->altShortcutEdit->hasFocus())
	{
		changedText = ui->altShortcutEdit->text();
		if (changedText == ui->shortcutsTreeWidget->currentItem()->text(2))
		{
			// nothing to apply
			ui->applyButton->setEnabled(false);
		}
		else
		{
			ui->applyButton->setEnabled(true);
		}
	}
	if (ui->primaryShortcutEdit->text().isEmpty())
	{
		ui->clearPrimaryButton->setEnabled(false);
	}
	else
	{
		ui->clearPrimaryButton->setEnabled(true);
	}
	if (ui->altShortcutEdit->text().isEmpty())
	{
		ui->clearAltButton->setEnabled(false);
	}
	else
	{
		ui->clearAltButton->setEnabled(true);
	}
}

void ShortcutsDialog::applyChanges()
{

}

void ShortcutsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->shortcutsTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(initEditors()));
	// apply button logic
	connect(ui->applyButton, SIGNAL(released()), this, SLOT(applyChanges()));
	// we need to disable all shortcut actions, so we can enter shortcuts without activating any actions
	connect(ui->primaryShortcutEdit, SIGNAL(focusChanged(bool)), this, SLOT(setActionsEnabled(bool)));
	connect(ui->altShortcutEdit, SIGNAL(focusChanged(bool)), this, SLOT(setActionsEnabled(bool)));
	// handling changes in editlines
	connect(ui->primaryShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));
	connect(ui->altShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));

	// Creating shortcuts tree
	QList<StelShortcutGroup*> groups = shortcutMgr->getGroupList();
	foreach (StelShortcutGroup* group, groups)
	{
		QTreeWidgetItem* groupItem = new QTreeWidgetItem(ui->shortcutsTreeWidget);
		groupItem->setFlags(Qt::ItemIsEnabled);
		groupItem->setText(0, group->getId());
		groupItem->setData(0, Qt::UserRole, group->getId());
		groupItem->setExpanded(true);
		// setup bold font for group lines
		QFont rootFont = groupItem->font(0);
		rootFont.setBold(true); rootFont.setPixelSize(14);
		groupItem->setFont(0, rootFont);
		QList<StelShortcut*> shortcuts = group->getActionList();
		foreach (StelShortcut* shortcut, shortcuts)
		{
			QTreeWidgetItem* shortcutItem = new QTreeWidgetItem(groupItem);
			shortcutItem->setText(0, shortcut->getText());
			shortcutItem->setText(1, shortcut->getPrimaryKey());
			shortcutItem->setText(2, shortcut->getAltKey());
			shortcutItem->setData(0, Qt::UserRole, QVariant(shortcut->getId()));
		}
	}
	updateText();
}

ShortcutLineEdit *ShortcutsDialog::getCurrrentEdit()
{
	if (ui->primaryShortcutEdit->hasFocus())
		return ui->primaryShortcutEdit;
	if (ui->altShortcutEdit->hasFocus())
		return ui->altShortcutEdit;
	return NULL;
}

void ShortcutsDialog::updateText()
{
}
