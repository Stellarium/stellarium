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
#include "ConfigurationDialog.hpp"
#include "StelMainWindow.hpp"
#include "ui_configurationDialog.h"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"

#include <QDebug>
#include <QFrame>
#include <QFile>

#include "StelAppGraphicsItem.hpp"
#include <QDialog>
#include <QGraphicsProxyWidget>

ConfigurationDialog::ConfigurationDialog() : dialog(NULL)
{
	ui = new Ui_configurationDialogForm;
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
}

void ConfigurationDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ConfigurationDialog::close()
{
	emit closed();
}

void ConfigurationDialog::setVisible(bool v)
{
	if (v) 
	{
		dialog = new QDialog(&StelMainWindow::getInstance());
		ui->setupUi(dialog);
		connect(ui->closeView, SIGNAL(clicked()), this, SLOT(close()));
		
		// Fill the language list widget from the available list
		QComboBox* c = ui->programLanguageComboBox;
		c->clear();
		c->addItems(Translator::globalTranslator.getAvailableLanguagesNamesNative(StelApp::getInstance().getFileMgr().getLocaleDir()));
		c->setCurrentIndex(c->findText(StelApp::getInstance().getLocaleMgr().getAppLanguage(), Qt::MatchExactly));
		connect(c, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(languageChanged(const QString&)));
		
		StelAppGraphicsItem* item = &StelAppGraphicsItem::getInstance();
		QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(item, Qt::Tool);
		proxy->setWidget(dialog);
		proxy->setWindowFrameMargins(0,0,0,0);
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
		
		dialog->move(200, 100);
		dialog->show();
		dialog->raise();
	}
	else
	{
		dialog->deleteLater();
		dialog = NULL;
	}
}

void ConfigurationDialog::languageChanged(const QString& langName)
{
	StelApp::getInstance().getLocaleMgr().setAppLanguage(Translator::nativeNameToIso639_1Code(langName));
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(Translator::nativeNameToIso639_1Code(langName));
}
