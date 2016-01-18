/*
 * Stellarium
 * Copyright (C) 2014-2016 Marcos Cardinot
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

#include <QDateTime>
#include <QFileDialog>
#include <QStringBuilder>

#include "AddOnDialog.hpp"
#include "AddOnTableModel.hpp"
#include "AddOnWidget.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "ui_addonDialog.h"

AddOnDialog::AddOnDialog(QObject* parent)
	: StelDialog(parent)
	, m_pSettingsDialog(new AddOnSettingsDialog(this))
{
	ui = new Ui_addonDialogForm;
}

AddOnDialog::~AddOnDialog()
{
	delete ui;
	ui = NULL;
}

void AddOnDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateTabBarListWidgetWidth();
	}
}

void AddOnDialog::styleChanged()
{
}

void AddOnDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(&StelApp::getInstance().getStelAddOnMgr(), SIGNAL(updateTableViews()),
		this, SLOT(populateTables()));

	// build and populate all tableviews
	populateTables();

	// catalog updates
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	connect(ui->btnUpdate, SIGNAL(clicked()), this, SLOT(updateCatalog()));

	// setting up tabs
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

	// button to install/uninstall/update
	connect(ui->button, SIGNAL(clicked()), this, SLOT(slotCheckedRows()));
	ui->button->setEnabled(false);

	connect(ui->availableTableView, SIGNAL(addonSelected(AddOn*)), this, SLOT(slotAddonSelected(AddOn*)));
	connect(ui->installedTableView, SIGNAL(addonSelected(AddOn*)), this, SLOT(slotAddonSelected(AddOn*)));
	connect(ui->updatesTableView, SIGNAL(addonSelected(AddOn*)), this, SLOT(slotAddonSelected(AddOn*)));

	connect(ui->availableTableView, SIGNAL(addonChecked()), this, SLOT(slotUpdateButton()));
	connect(ui->installedTableView, SIGNAL(addonChecked()), this, SLOT(slotUpdateButton()));
	connect(ui->updatesTableView, SIGNAL(addonChecked()), this, SLOT(slotUpdateButton()));

	// button Install from File
	connect(ui->btnInstallFromFile, SIGNAL(clicked()), this, SLOT(installFromFile()));

	// display the AddOnMgr message
	connect(&StelApp::getInstance().getStelAddOnMgr(), SIGNAL(addOnMgrMsg(StelAddOnMgr::AddOnMgrMsg)),
		this, SLOT(slotUpdateMsg(StelAddOnMgr::AddOnMgrMsg)));

	// fix dialog width
	updateTabBarListWidgetWidth();
}

void AddOnDialog::slotAddonSelected(AddOn *addon)
{
	if (!addon)
	{
		ui->browser->clear();
		return;
	}

	QString html = "<html><head></head><body>";
	html += "<h2>" + addon->getTitle() + "</h2>";
	html += addon->getDescription();
	ui->browser->setHtml(html);
}

void AddOnDialog::slotUpdateButton()
{
	int amount = 0;
	QString tabName = ui->stackedWidget->currentWidget()->objectName();
	if (tabName == ui->updates->objectName())
	{
		amount = ui->updatesTableView->getCheckedAddons().size();
		ui->button->setText(QString("%1 (%2)").arg(q_("Update")).arg(amount));
	}
	else if (tabName == ui->installed->objectName())
	{
		amount = ui->installedTableView->getCheckedAddons().size();
		ui->button->setText(QString("%1 (%2)").arg(q_("Uninstall")).arg(amount));
	}
	else if (tabName == ui->available->objectName())
	{
		amount = ui->availableTableView->getCheckedAddons().size();
		ui->button->setText(QString("%1 (%2)").arg(q_("Install")).arg(amount));
	}
	ui->button->setEnabled(amount > 0);
}

void AddOnDialog::slotUpdateMsg(const StelAddOnMgr::AddOnMgrMsg msg)
{
	QString txt, toolTip;
	switch (msg) {
		case StelAddOnMgr::RestartRequired:
			txt = q_("Stellarium restart requiried!");
			toolTip = q_("You must restart the Stellarium to make some changes take effect.");
			break;
		case StelAddOnMgr::UnableToWriteFiles:
			txt = q_("Unable to write files!");
			toolTip = q_("The user directory is not writable. Please check the permissions on that directory.");
			break;
	}
	ui->msg->setText(txt);
	ui->msg->setToolTip(toolTip);
}

void AddOnDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);

	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();

	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
		return;

	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		width += ui->stackListWidget->sizeHintForRow(row);
		width += ui->stackListWidget->iconSize().width();
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void AddOnDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	current = current ? current : previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	ui->updatesTableView->clearSelection();
	ui->installedTableView->clearSelection();
	ui->availableTableView->clearSelection();
	slotUpdateButton();
}

void AddOnDialog::populateTables()
{
	ui->availableTableView->setModel(new AddOnTableModel(StelApp::getInstance().getStelAddOnMgr().getAddonsAvailable()));
	ui->installedTableView->setModel(new AddOnTableModel(StelApp::getInstance().getStelAddOnMgr().getAddonsInstalled()));
	ui->updatesTableView->setModel(new AddOnTableModel(StelApp::getInstance().getStelAddOnMgr().getAddonsToUpdate()));
	updateTabBarListWidgetWidth();
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QNetworkRequest req;
	req.setUrl(QUrl(StelApp::getInstance().getStelAddOnMgr().getUrlForUpdates()));
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, false);
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelApp::getInstance().getStelAddOnMgr().getUserAgent());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void AddOnDialog::downloadFinished()
{
	ui->btnUpdate->setEnabled(true);
	QByteArray result(m_pUpdateCatalogReply->readAll());
	if (m_pUpdateCatalogReply->error() == QNetworkReply::NoError && !result.isEmpty())
	{
		QFile jsonFile(StelApp::getInstance().getStelAddOnMgr().getAddonJsonPath());
		jsonFile.remove(); // overwrite
		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(result);
			jsonFile.close();

			StelApp::getInstance().getStelAddOnMgr().reloadCatalogues();
			qint64 currentTime = QDateTime::currentMSecsSinceEpoch() / 1000;
			StelApp::getInstance().getStelAddOnMgr().setLastUpdate(currentTime);
			ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
			populateTables();
		}
		else
		{
			qWarning() << "AddOnDialog : unable to update the database! cannot write json file";
			ui->txtLastUpdate->setText(q_("Database update failed!"));
		}
	}
	else
	{
		qWarning() << "AddOnDialog : unable to update the database!" << m_pUpdateCatalogReply->errorString();
		ui->txtLastUpdate->setText(q_("Database update failed!"));
	}

	m_pUpdateCatalogReply->deleteLater();
	m_pUpdateCatalogReply = NULL;
}

void AddOnDialog::installFromFile()
{
	QString filePath = QFileDialog::getOpenFileName(NULL, q_("Select Add-On"), QDir::homePath(), "*.zip");
	StelApp::getInstance().getStelAddOnMgr().installAddOnFromFile(filePath);
}

void AddOnDialog::slotCheckedRows()
{
	AddOnTableView* tableview;
	QString tabName = ui->stackedWidget->currentWidget()->objectName();
	if (tabName == ui->updates->objectName())
	{
		tableview = ui->updatesTableView;
		StelApp::getInstance().getStelAddOnMgr().updateAddons(tableview->getCheckedAddons());
	}
	else if (tabName == ui->installed->objectName())
	{
		tableview = ui->installedTableView;
		StelApp::getInstance().getStelAddOnMgr().removeAddons(tableview->getCheckedAddons());
	}
	else if (tabName == ui->available->objectName())
	{
		tableview = ui->availableTableView;
		StelApp::getInstance().getStelAddOnMgr().installAddons(tableview->getCheckedAddons());
	}
	tableview->clearSelection();
}
