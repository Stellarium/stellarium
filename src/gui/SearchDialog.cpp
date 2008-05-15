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
#include "StelMainWindow.hpp"
#include "ui_searchDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Observer.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "MovementMgr.hpp"

#include <QDebug>
#include <QFrame>

#include "StelMainGraphicsView.hpp"
#include <QDialog>
#include <QGraphicsProxyWidget>

SearchDialog::SearchDialog() : dialog(0)
{
	ui = new Ui_searchDialogForm;
}

SearchDialog::~SearchDialog()
{
	delete ui;
	if (dialog)
		delete dialog;
}

void SearchDialog::close()
{
	emit closed();
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

void SearchDialog::setVisible(bool v)
{
	if (v) 
	{
		dialog = new QDialog(&StelMainGraphicsView::getInstance());
		ui->setupUi(dialog);

		connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
		connect(ui->lineEditSearchSkyObject, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
		connect(ui->pushButtonGotoSearchSkyObject, SIGNAL(clicked()), this, SLOT(gotoObject()));
		onTextChanged(ui->lineEditSearchSkyObject->text());
		connect(ui->lineEditSearchSkyObject, SIGNAL(returnPressed()), this, SLOT(gotoObject()));
		
		QGraphicsProxyWidget* proxy = StelMainGraphicsView::getInstance().scene()->addWidget(dialog, Qt::Dialog);
		proxy->setWindowFrameMargins(0,0,0,0);
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
		proxy->setZValue(100);
		dialog->move(200, 100);
	}
	else
	{
		dialog->deleteLater();
		dialog = 0;
	}
}

void SearchDialog::onTextChanged(const QString& text)
{
	QStringList matches = StelApp::getInstance().getStelObjectMgr().listMatchingObjectsI18n(text, 5);
	ui->labelCompletions->setText(matches.join(","));
}

void SearchDialog::gotoObject()
{
	QString name = ui->lineEditSearchSkyObject->text();
	
	MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	
	if (StelApp::getInstance().getStelObjectMgr().findAndSelectI18n(name))
	{
		const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
		if (!newSelected.empty())
		{
			ui->lineEditSearchSkyObject->clear();
			mvmgr->moveTo(newSelected[0]->getEarthEquatorialPos(
				StelApp::getInstance().getCore()->getNavigation()),mvmgr->getAutoMoveDuration()
			);
			mvmgr->setFlagTracking(true);
			close();
		}
	}
	else
	{
		ui->labelCompletions->setText(QString("%1 is unknown!").arg(name));
	}
}

