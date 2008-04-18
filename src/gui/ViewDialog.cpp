/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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
#include "ViewDialog.hpp"
#include "StelMainWindow.hpp"
#include "ui_viewDialog.h"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "Mapping.hpp"
#include "Projector.hpp"
#include "LandscapeMgr.hpp"
#include "StelModuleMgr.hpp"

#include <QDebug>
#include <QFrame>
#include <QFile>

ViewDialog::ViewDialog() : dialog(NULL)
{
	ui = new Ui_viewDialogForm;
}

ViewDialog::~ViewDialog()
{
	delete ui;
}

void ViewDialog::close()
{
	emit closed();
}

void ViewDialog::setVisible(bool v)
{
	if (v) 
	{
		dialog = new DialogFrame(&StelMainWindow::getInstance());
		ui->setupUi(dialog);
		dialog->raise();
		dialog->move(200, 100);	// TODO: put in the center of the screen
		dialog->setVisible(true);
		connect(ui->closeView, SIGNAL(clicked()), this, SLOT(close()));
//		connect(ui->longitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(spinBoxChanged(void)));
		
		// Fill the culture list widget from the available list
		QListWidget* l = ui->culturesListWidget;
		l->clear();
		l->addItems(StelApp::getInstance().getSkyCultureMgr().getSkyCultureListI18());
		updateSkyCultureText();
		l->setCurrentItem(l->findItems(StelApp::getInstance().getSkyCultureMgr().getSkyCultureNameI18(), Qt::MatchExactly).at(0));
		connect(l, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyCultureChanged(const QString&)));
		
		// Fill the projection list
		l = ui->projectionListWidget;
		l->clear();
		const QMap<QString, const Mapping*>& mappings = StelApp::getInstance().getCore()->getProjection()->getAllMappings();
		QMapIterator<QString, const Mapping*> i(mappings);
		while (i.hasNext())
		{
			i.next();
			l->addItem(q_(i.value()->getNameI18()));
		}
		l->setCurrentItem(l->findItems(StelApp::getInstance().getCore()->getProjection()->getCurrentMapping().getNameI18(), Qt::MatchExactly).at(0));
		ui->projectionTextBrowser->setHtml(StelApp::getInstance().getCore()->getProjection()->getCurrentMapping().getHtmlSummary());
		connect(l, SIGNAL(currentTextChanged(const QString&)), this, SLOT(projectionChanged(const QString&)));
		
		// Fill the landscape list
		l = ui->landscapesListWidget;
		l->clear();
		LandscapeMgr* lmgr = (LandscapeMgr*)GETSTELMODULE("LandscapeMgr");
		l->addItems(lmgr->getAllLandscapeNames());
		l->setCurrentItem(l->findItems(lmgr->getCurrentLandscapeName(), Qt::MatchExactly).at(0));
		ui->landscapeTextBrowser->setHtml(lmgr->getCurrentLandscapeHtmlDescription());
		connect(l, SIGNAL(currentTextChanged(const QString&)), this, SLOT(landscapeChanged(const QString&)));
	}
	else
	{
		dialog->deleteLater();
		dialog = NULL;
	}
}

void ViewDialog::skyCultureChanged(const QString& cultureName)
{
	StelApp::getInstance().getSkyCultureMgr().setSkyCulture(cultureName);
	updateSkyCultureText();
}

void ViewDialog::updateSkyCultureText()
{	
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	QString descPath;
	try
	{
		descPath = fileMan.findFile("skycultures/" + StelApp::getInstance().getSkyCultureMgr().getSkyCultureDir() + "/description."+StelApp::getInstance().getLocaleMgr().getAppLanguage()+".utf8");
	}
	catch (exception& e)
	{
		try
		{
			descPath = fileMan.findFile("skycultures/" + StelApp::getInstance().getSkyCultureMgr().getSkyCultureDir() + "/description.en.utf8");
		}
		catch (exception& e)
		{
			qWarning() << "WARNING: can't find description for skyculture" << StelApp::getInstance().getSkyCultureMgr().getSkyCultureDir();
		}
	}
	if (descPath.isEmpty())
	{
		ui->skyCultureTextBrowser->setHtml("No description");
	}
	else
	{
		QFile f(descPath);
		f.open(QIODevice::ReadOnly);
		ui->skyCultureTextBrowser->setHtml(QString::fromUtf8(f.readAll()));
	}
}

void ViewDialog::projectionChanged(const QString& projectionName)
{
	const QMap<QString, const Mapping*>& mappings = StelApp::getInstance().getCore()->getProjection()->getAllMappings();
	QMapIterator<QString, const Mapping*> i(mappings);
	while (i.hasNext())
	{
		i.next();
		if (q_(i.value()->getNameI18()) == projectionName)
			break;
	}

	StelApp::getInstance().getCore()->getProjection()->setCurrentMapping(i.value()->getId());
	ui->projectionTextBrowser->setHtml(StelApp::getInstance().getCore()->getProjection()->getCurrentMapping().getHtmlSummary());
}

void ViewDialog::landscapeChanged(const QString& landscapeName)
{
	LandscapeMgr* lmgr = (LandscapeMgr*)GETSTELMODULE("LandscapeMgr");
	lmgr->setLandscapeByName(landscapeName);
	ui->landscapeTextBrowser->setHtml(lmgr->getCurrentLandscapeHtmlDescription());
}
