/*
 * Stellarium
 * Copyright (C) 2015 Marcos Cardinot
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

#include "AddOnScanner.hpp"
#include "StelApp.hpp"
#include "StelMainView.hpp"
#include "ui_addonScanner.h"

AddOnScanner::AddOnScanner(QObject* parent)
	: StelDialog(parent)
{
	ui = new Ui_AddOnScanner;
}

AddOnScanner::~AddOnScanner()
{
	delete ui;
	ui = NULL;
}

void AddOnScanner::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}


void AddOnScanner::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->btInstall, SIGNAL(clicked()), this, SLOT(slotInstallCompatibles()));
	connect(ui->btRemoveAll, SIGNAL(clicked()), this, SLOT(slotRemoveAll()));
	connect(ui->btRemoveIncompatibles, SIGNAL(clicked()), this, SLOT(slotRemoveIncompatibles()));
}

void AddOnScanner::loadAddons(QList<AddOn*> addons)
{
	if (addons.isEmpty())
	{
		this->setVisible(false);
		return;
	}

	this->setVisible(true);
	ui->tableWidget->clear();
	ui->tableWidget->setRowCount(0);
	foreach (AddOn* addon, addons) {
		int lastRow = ui->tableWidget->rowCount();
		ui->tableWidget->insertRow(lastRow);
		ui->tableWidget->setItem(lastRow, 0, new QTableWidgetItem(addon->getTypeDisplayRole()));
		ui->tableWidget->setItem(lastRow, 1, new QTableWidgetItem(addon->getTitle()));
		ui->tableWidget->setItem(lastRow, 2, new QTableWidgetItem(addon->isValid()?"Compatible":"Incompatible"));
	}
	ui->tableWidget->resizeColumnsToContents();
	m_addonsList = addons;
}

void AddOnScanner::slotInstallCompatibles()
{
	foreach (AddOn* addon, m_addonsList) {
		if (addon->isValid())
		{
			StelApp::getInstance().getStelAddOnMgr().installAddOn(addon);
			m_addonsList.removeOne(addon);
		}
	}
	loadAddons(m_addonsList);
}

void AddOnScanner::slotRemoveAll()
{
	foreach (AddOn* addon, m_addonsList) {
		if (QFile(addon->getDownloadFilepath()).remove())
		{
			m_addonsList.removeOne(addon);
		}
		else
		{
			qWarning() << "Add-On Scanner - Unable to remove"
				   << addon->getDownloadFilepath();
		}
	}
	loadAddons(m_addonsList);
}

void AddOnScanner::slotRemoveIncompatibles()
{
	foreach (AddOn* addon, m_addonsList) {
		if (!addon->isValid())
		{
			if (QFile(addon->getDownloadFilepath()).remove())
			{
				m_addonsList.removeOne(addon);
			}
			else
			{
				qWarning() << "Add-On Scanner - Unable to remove"
					   << addon->getDownloadFilepath();
			}
		}
	}
	loadAddons(m_addonsList);
}
