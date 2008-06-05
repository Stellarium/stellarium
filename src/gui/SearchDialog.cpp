/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Dialog.hpp"
#include "SearchDialog.hpp"
#include "ui_searchDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Observer.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "MovementMgr.hpp"

#include <QTextEdit>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QFrame>

#include "StelMainGraphicsView.hpp"

// Start of members for class CompletionLabel
CompletionLabel::CompletionLabel(QWidget* parent)
	: QLabel(parent), 
	  selectedIdx(0)
{
}

CompletionLabel::~CompletionLabel()
{
}

QString CompletionLabel::getSelected(void)
{
	QString item = text().split(",").at(selectedIdx);
	item.replace(QRegExp(" *</?b> *"), "");
	return item;
}

void CompletionLabel::selectNext(void)
{
	selectedIdx++;
	if (selectedIdx > text().count(",")) selectedIdx = 0; // wrap if necessary
	updateSelected();
}

void CompletionLabel::selectPrevious(void)
{
	selectedIdx--;
	if (selectedIdx < 0) selectedIdx = text().count(","); // wrap if necessary
	updateSelected();
}

void CompletionLabel::selectFirst(void)
{
	selectedIdx=0;
	updateSelected();
}

void CompletionLabel::updateSelected(void)
{
	QStringList items = text().split(",");
	QString newText("");
	
	// and pre-selected items, stripping bold tags if found
	for(int i=0; i<selectedIdx; i++)
	{
		QString item(items.at(i));
		item.replace(QRegExp("</?b>"), "");
		newText += item.trimmed() + ", ";
	}

	// add selected item in bold
	if (items.at(selectedIdx) != "") newText += "<b>" + items.at(selectedIdx).trimmed() + "</b>, ";

	// and post-selected items, stripping bold tags if found
	for(int i=selectedIdx+1; i<items.size(); i++)
	{
		QString item(items.at(i));
		item.replace(QRegExp("</?b>"), "");
		newText += item.trimmed() + ", ";
	}

	// remove final comma
	newText.replace(QRegExp(", *$"), "");

	setText(newText);
}

// Start of members for class SearchDialog
SearchDialog::SearchDialog()
{
	ui = new Ui_searchDialogForm;
}

SearchDialog::~SearchDialog()
{
	delete ui;
}

void SearchDialog::languageChanged()
{
	if (dialog)
	{
		QString text(ui->lineEditSearchSkyObject->text());
		ui->retranslateUi(dialog);
		ui->lineEditSearchSkyObject->setText(text);
	}
}

// Initialize the dialog widgets and connect the signals/slots
void SearchDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
	connect(ui->pushButtonGotoSearchSkyObject, SIGNAL(clicked()), this, SLOT(gotoObject()));
	onTextChanged(ui->lineEditSearchSkyObject->text());
	connect(ui->lineEditSearchSkyObject, SIGNAL(returnPressed()), this, SLOT(gotoObject()));
	ui->lineEditSearchSkyObject->installEventFilter(this);
}

void SearchDialog::setVisible(bool v)
{	
	StelDialog::setVisible(v);
		
	// Set the focus directly on the line edit
	if (ui->lineEditSearchSkyObject->isVisible())
		ui->lineEditSearchSkyObject->setFocus();
}

void SearchDialog::onTextChanged(const QString& text)
{
	if (text=="")
	{
		ui->completionLabel->setText("");
		ui->completionLabel->selectFirst();
	}
	else
	{
		QStringList matches = StelApp::getInstance().getStelObjectMgr().listMatchingObjectsI18n(text, 5);
		ui->completionLabel->setText(matches.join(", "));
		ui->completionLabel->selectFirst();
	}
}

void SearchDialog::gotoObject()
{
	QString name = ui->completionLabel->getSelected();
	
	if (name=="") return;
	else if (StelApp::getInstance().getStelObjectMgr().findAndSelectI18n(name))
	{
		MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
		const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
		if (!newSelected.empty())
		{
			ui->lineEditSearchSkyObject->clear();
			ui->completionLabel->setText("");
			ui->completionLabel->selectFirst();
			mvmgr->moveTo(newSelected[0]->getEarthEquatorialPos(
				StelApp::getInstance().getCore()->getNavigation()),mvmgr->getAutoMoveDuration()
			);
			mvmgr->setFlagTracking(true);
			// close();
		}
	}
}

bool SearchDialog::eventFilter(QObject *object, QEvent *event)
{
	if (object == ui->lineEditSearchSkyObject && event->type() == QEvent::KeyRelease) 
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Down) 
		{
			ui->completionLabel->selectNext();
			return true;
		} 
		if (keyEvent->key() == Qt::Key_Up) 
		{
			ui->completionLabel->selectPrevious();
			return true;
		} 
		else
			return false;
	}

	return false;

}

