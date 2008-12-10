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
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelNavigator.hpp"
#include "StelUtils.hpp"

#include <QTextEdit>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QFrame>

#include "StelMainGraphicsView.hpp"
#include "SimbadSearcher.hpp"

// Start of members for class CompletionLabel
CompletionLabel::CompletionLabel(QWidget* parent) : QLabel(parent), selectedIdx(0)
{
}

CompletionLabel::~CompletionLabel()
{
}

void CompletionLabel::setValues(const QStringList& v)
{
	values=v;
	updateText();
}

void CompletionLabel::appendValues(const QStringList& v)
{
	values+=v;
	updateText();
}

void CompletionLabel::clearValues()
{
	values.clear();
	selectedIdx=0;
	updateText();
}

QString CompletionLabel::getSelected()
{
	if (values.isEmpty())
		return QString();
	return values.at(selectedIdx);
}

void CompletionLabel::selectNext()
{
	++selectedIdx;
	if (selectedIdx>=values.size())
		selectedIdx=0;
	updateText();
}

void CompletionLabel::selectPrevious()
{
	--selectedIdx;
	if (selectedIdx<0)
		selectedIdx = values.size()-1;
	updateText();
}

void CompletionLabel::selectFirst()
{
	selectedIdx=0;
	updateText();
}

void CompletionLabel::updateText()
{
	QString newText;
	
	// Regenerate the list with the selected item in bold
	for (int i=0;i<values.size();++i)
	{
		if (i==selectedIdx)
			newText+="<b>"+values[i]+"</b>";
		else
			newText+=values[i];
		if (i!=values.size()-1)
			newText += ", ";
	}
	setText(newText);
}

// Start of members for class SearchDialog
SearchDialog::SearchDialog() : simbadReply(NULL)
{
	ui = new Ui_searchDialogForm;
	simbadSearcher = new SimbadSearcher(this);
}

SearchDialog::~SearchDialog()
{
	delete ui;
	if (simbadReply)
	{
		simbadReply->deleteLater();
		simbadReply = NULL;
	}
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

void SearchDialog::styleChanged()
{
	// Nothing for now
}

// Initialize the dialog widgets and connect the signals/slots
void SearchDialog::createDialogContent()
{
	ui->setupUi(dialog);
	setSimpleStyle(true);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
	connect(ui->pushButtonGotoSearchSkyObject, SIGNAL(clicked()), this, SLOT(gotoObject()));
	onTextChanged(ui->lineEditSearchSkyObject->text());
	connect(ui->lineEditSearchSkyObject, SIGNAL(returnPressed()), this, SLOT(gotoObject()));
	dialog->installEventFilter(this);
	ui->RAAngleSpinBox->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->DEAngleSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->DEAngleSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	
	// This simply doesn't work. Probably a Qt bug
	ui->RAAngleSpinBox->setFocusPolicy(Qt::NoFocus);
	ui->DEAngleSpinBox->setFocusPolicy(Qt::NoFocus);
	
	connect(ui->RAAngleSpinBox, SIGNAL(valueChanged()), this, SLOT(manualPositionChanged()));
	connect(ui->DEAngleSpinBox, SIGNAL(valueChanged()), this, SLOT(manualPositionChanged()));
}

void SearchDialog::setVisible(bool v)
{	
	StelDialog::setVisible(v);
		
	// Set the focus directly on the line edit
	if (ui->lineEditSearchSkyObject->isVisible())
		ui->lineEditSearchSkyObject->setFocus();
}

void SearchDialog::setSimpleStyle(bool b)
{
	ui->RAAngleSpinBox->setVisible(!b);
	ui->DEAngleSpinBox->setVisible(!b);
	ui->simbadStatusLabel->setVisible(!b);
	ui->raDecLabel->setVisible(!b);
}

void SearchDialog::manualPositionChanged()
{
	ui->completionLabel->clearValues();
	StelMovementMgr* mvmgr = (StelMovementMgr*)GETSTELMODULE("StelMovementMgr");
	Vec3d pos;
	StelUtils::spheToRect(ui->RAAngleSpinBox->valueRadians(), ui->DEAngleSpinBox->valueRadians(), pos);
	mvmgr->moveTo(pos, 0.05);
}

void SearchDialog::onTextChanged(const QString& text)
{
	if (text.isEmpty())
	{
		ui->completionLabel->setText("");
		ui->completionLabel->selectFirst();
		ui->simbadStatusLabel->setText("");
	}
	else
	{
		if (simbadReply)
		{
			disconnect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
			delete simbadReply;
		}
		simbadResults.clear();
		simbadReply = simbadSearcher->lookup(text, 3);
		onSimbadStatusChanged();
		connect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		QStringList matches = StelApp::getInstance().getStelObjectMgr().listMatchingObjectsI18n(text, 5);
		ui->completionLabel->setValues(matches);
		ui->completionLabel->selectFirst();
	}
}

// Called when the current simbad query status changes
void SearchDialog::onSimbadStatusChanged()
{
	Q_ASSERT(simbadReply);
	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadLookupErrorOccured)
		ui->simbadStatusLabel->setText(QString("Simbad Lookup Error: ")+simbadReply->getErrorString());
	else
		ui->simbadStatusLabel->setText(QString("Simbad Lookup: ")+simbadReply->getCurrentStatusString());

	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadLookupFinished)
	{
		simbadResults = simbadReply->getResults();
		ui->completionLabel->appendValues(simbadResults.keys());
	}
	
	if (simbadReply->getCurrentStatus()!=SimbadLookupReply::SimbadLookupQuerying)
	{
		disconnect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		delete simbadReply;
		simbadReply=NULL;
	}
}
	
void SearchDialog::gotoObject()
{
	QString name = ui->completionLabel->getSelected();
	
	if (name.isEmpty())
		return;
	
	StelMovementMgr* mvmgr = (StelMovementMgr*)GETSTELMODULE("StelMovementMgr");
	if (simbadResults.contains(name))
	{
		close();
		Vec3d pos = simbadResults[name];
		StelApp::getInstance().getStelObjectMgr().unSelect();
		pos = StelApp::getInstance().getCore()->getNavigator()->j2000ToEquinoxEqu(pos);
		mvmgr->moveTo(pos, mvmgr->getAutoMoveDuration());
		ui->lineEditSearchSkyObject->clear();
		ui->completionLabel->clearValues();
	}
	else if (StelApp::getInstance().getStelObjectMgr().findAndSelectI18n(name))
	{
		const QList<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
		if (!newSelected.empty())
		{
			close();
			ui->lineEditSearchSkyObject->clear();
			ui->completionLabel->clearValues();
			mvmgr->moveTo(newSelected[0]->getEquinoxEquatorialPos(StelApp::getInstance().getCore()->getNavigator()),mvmgr->getAutoMoveDuration());
			mvmgr->setFlagTracking(true);
		}
	}
	simbadResults.clear();
}

bool SearchDialog::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyRelease) 
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Down) 
		{
			ui->completionLabel->selectNext();
			event->accept();
			return true;
		} 
		if (keyEvent->key() == Qt::Key_Up) 
		{
			ui->completionLabel->selectPrevious();
			event->accept();
			return true;
		} 
	}

	return false;
}


