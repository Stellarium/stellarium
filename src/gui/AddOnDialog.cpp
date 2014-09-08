/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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
#include "AddOnWidget.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "ui_addonDialog.h"
#include "addons/AddOnTableModel.hpp"
#include "addons/AddOnTableProxyModel.hpp"

AddOnDialog::AddOnDialog(QObject* parent) : StelDialog(parent)
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
	connect(&StelApp::getInstance().getStelAddOnMgr(), SIGNAL(updateTableViews()),
		this, SLOT(populateTables()));

	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// naming tables according to the category
	ui->catalogsTableView->setObjectName(CATEGORY_CATALOG);
	ui->landscapeTableView->setObjectName(CATEGORY_LANDSCAPE);
	ui->languageTableView->setObjectName(CATEGORY_LANGUAGE_PACK);
	ui->scriptsTableView->setObjectName(CATEGORY_SCRIPT);
	ui->starloreTableView->setObjectName(CATEGORY_SKY_CULTURE);
	ui->texturesTableView->setObjectName(CATEGORY_TEXTURE);

	// hashing all tableViews
	m_tableViews.insert(CATALOG, ui->catalogsTableView);
	m_tableViews.insert(LANDSCAPE, ui->landscapeTableView);
	m_tableViews.insert(LANGUAGEPACK, ui->languageTableView);
	m_tableViews.insert(SCRIPT, ui->scriptsTableView);
	m_tableViews.insert(STARLORE, ui->starloreTableView);
	m_tableViews.insert(TEXTURE, ui->texturesTableView);

	// mapping enum_tab to table names
	m_tabToTableName.insert(CATALOG, TABLE_CATALOG);
	m_tabToTableName.insert(LANDSCAPE, TABLE_LANDSCAPE);
	m_tabToTableName.insert(LANGUAGEPACK, TABLE_LANGUAGE_PACK);
	m_tabToTableName.insert(SCRIPT, TABLE_SCRIPT);
	m_tabToTableName.insert(STARLORE, TABLE_SKY_CULTURE);
	m_tabToTableName.insert(TEXTURE, TABLE_TEXTURE);

	// build and populate all tableviews
	populateTables();

	// catalog updates
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	connect(ui->btnUpdate, SIGNAL(clicked()), this, SLOT(updateCatalog()));

	// setting up tabs
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	ui->stackedWidget->setCurrentIndex(CATALOG);
	ui->stackListWidget->setCurrentRow(CATALOG);

	// buttons: Install and Remove
	connect(ui->btnInstall, SIGNAL(clicked()), this, SLOT(installSelectedRows()));
	connect(ui->btnRemove, SIGNAL(clicked()), this, SLOT(removeSelectedRows()));
	ui->btnInstall->setEnabled(false);
	ui->btnRemove->setEnabled(false);
	for (int itab=0; itab<COUNT; itab++) {
		Tab tab = (Tab)itab;
		AddOnTableView* view = m_tableViews.value(tab);
		connect(view, SIGNAL(somethingToInstall(bool)), ui->btnInstall, SLOT(setEnabled(bool)));
		connect(view, SIGNAL(somethingToRemove(bool)), ui->btnRemove, SLOT(setEnabled(bool)));
	}

	// button Install from File
	connect(ui->btnInstallFromFile, SIGNAL(clicked()), this, SLOT(installFromFile()));

	// fix dialog width
	updateTabBarListWidgetWidth();
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
	AddOnTableView* lastView = m_tableViews.value((Tab)ui->stackedWidget->currentIndex());
	if (lastView)
	{
		lastView->clearSelection();
		lastView->setAllChecked(false);
	}

	current = current ? current : previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}

void AddOnDialog::populateTables()
{
	for (int itab=0; itab<COUNT; itab++) {
		Tab tab = (Tab)itab;
		AddOnTableView* view = m_tableViews.value(tab);
		view->setModel(new AddOnTableProxyModel(m_tabToTableName.value(tab), this));
	}
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QUrl url(StelApp::getInstance().getStelAddOnMgr().getUrlForUpdates());
	url.setQuery(QString("time=%1").arg(StelApp::getInstance().getStelAddOnMgr().getLastUpdate()));

	QNetworkRequest req(url);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pUpdateCatalogReply->setReadBufferSize(1024*1024*2);

	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void AddOnDialog::downloadFinished()
{
	ui->btnUpdate->setEnabled(true);
	if (m_pUpdateCatalogReply->error() != QNetworkReply::NoError)
	{
		qWarning() << "AddOnDialog : unable to update the database!" << m_pUpdateCatalogReply->errorString();
		ui->txtLastUpdate->setText(q_("Database update failed!"));

		m_pUpdateCatalogReply->deleteLater();
		m_pUpdateCatalogReply = NULL;
		return;
	}

	QString result(m_pUpdateCatalogReply->readAll());
	m_pUpdateCatalogReply->deleteLater();
	m_pUpdateCatalogReply = NULL;

	if (!result.isEmpty())
	{
		if(!StelApp::getInstance().getStelAddOnMgr().updateCatalog(result))
		{
			ui->txtLastUpdate->setText(q_("Database update failed!"));
			return;
		}
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch()/1000;
	StelApp::getInstance().getStelAddOnMgr().setLastUpdate(currentTime);
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	populateTables();
}

void AddOnDialog::installFromFile()
{
	QString filePath = QFileDialog::getOpenFileName(NULL, q_("Select the add-on"), QDir::homePath());
	if (QFile(filePath).exists())
	{
		StelApp::getInstance().getStelAddOnMgr().installFromFile(filePath);
	}
}

void AddOnDialog::installSelectedRows()
{
	AddOnTableView* view = m_tableViews.value((Tab)ui->stackedWidget->currentIndex());
	QHashIterator<int, QStringList> i(view->getSelectedAddonsToInstall());
	while (i.hasNext()) {
		i.next();
		StelApp::getInstance().getStelAddOnMgr().installAddOn(i.key(), i.value());
	}
}

void AddOnDialog::removeSelectedRows()
{
	AddOnTableView* view = m_tableViews.value((Tab)ui->stackedWidget->currentIndex());
	QHashIterator<int, QStringList> i(view->getSelectedAddonsToRemove());
	while (i.hasNext()) {
		i.next();
		StelApp::getInstance().getStelAddOnMgr().removeAddOn(i.key(), i.value());
	}
}
