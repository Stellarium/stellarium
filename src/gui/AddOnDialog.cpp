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
#include <QSqlQueryModel>
#include <QStringBuilder>

#include "AddOnDialog.hpp"
#include "ui_addonDialog.h"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

AddOnDialog::AddOnDialog(QObject* parent) : StelDialog(parent)
{
    ui = new Ui_addonDialogForm;
    m_StelAddOn = StelApp::getInstance().getStelAddOn();
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
	}
}

void AddOnDialog::styleChanged()
{
}

void AddOnDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// build and populate all tableviews
	populateTables();

	// catalog updates
	ui->txtLastUpdate->setText(m_StelAddOn.getLastUpdateString());
	connect(ui->btnUpdate, SIGNAL(clicked()), this, SLOT(updateCatalog()));

	// setting up tabs
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	ui->stackedWidget->setCurrentIndex(CATALOG);
	ui->stackListWidget->setCurrentRow(CATALOG);
	m_currentTableView = ui->catalogsTableView;

	connect(ui->btnInstall, SIGNAL(clicked()), this, SLOT(installSelectedRows()));
}

void AddOnDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
	{
		current = previous;
	}
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	switch (ui->stackedWidget->currentIndex())
	{
		case CATALOG:
			m_currentTableView = ui->catalogsTableView;
			break;
		case LANDSCAPE:
			m_currentTableView = ui->landscapeTableView;
			break;
		case LANGUAGEPACK:
			m_currentTableView = ui->languageTableView;
			break;
		case SCRIPT:
			m_currentTableView = ui->scriptsTableView;
			break;
		case STARLORE:
			m_currentTableView = ui->starloreTbleView;
			break;
		case TEXTURE:
			m_currentTableView = ui->texturesTableView;
			break;
	}


	// Destroys all checkboxes of the previous tableview
	for(int i=0; i<m_checkBoxes.size(); ++i)
	{
	    delete m_checkBoxes.take(i);
	}
	m_checkBoxes.clear();

	// Insert checkboxes to the current tableview
	int lastColumn = m_currentTableView->horizontalHeader()->count() - 1;
	for(int row=0; row<m_currentTableView->model()->rowCount(); ++row)
	{
		QCheckBox* cbox = new QCheckBox();
		m_currentTableView->setIndexWidget(m_currentTableView->model()->index(row, lastColumn), cbox);
		cbox->setStyleSheet("QCheckBox { padding-left: 8px; }");
		m_checkBoxes.insert(row, cbox);
	}
}

void AddOnDialog::setUpTableView(QTableView* tableView)
{
	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	int lastColumn = tableView->horizontalHeader()->count() - 1;
	tableView->horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);
	tableView->verticalHeader()->setVisible(false);
	tableView->setAlternatingRowColors(true);
	tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableView->setEditTriggers(false);
}

void AddOnDialog::initModel(QTableView* tableView, QString table)
{
	QSqlQueryModel* model = new QSqlQueryModel;
	QString query;

	if (table == TABLE_CATALOG)
	{
		query = "SELECT " % table % ".id, addon.id, title, type, version, installed, NULL "
			"FROM addon INNER JOIN " % table %
			" ON addon.id = " % table % ".addon";
		model->setQuery(query);
		model->setHeaderData(0, Qt::Horizontal, q_("Id"));
		model->setHeaderData(1, Qt::Horizontal, q_("AddOnId"));
		model->setHeaderData(2, Qt::Horizontal, q_("Title"));
		model->setHeaderData(3, Qt::Horizontal, q_("Type"));
		model->setHeaderData(4, Qt::Horizontal, q_("Last Version"));
		model->setHeaderData(5, Qt::Horizontal, q_("Installed Version"));
		model->setHeaderData(6, Qt::Horizontal, "");
	}
	else
	{
		query = "SELECT " % table % ".id, addon.id, title, version, installed, NULL "
			"FROM addon INNER JOIN " % table %
			" ON addon.id = " % table % ".addon";
		model->setQuery(query);
		model->setHeaderData(0, Qt::Horizontal, q_("Id"));
		model->setHeaderData(1, Qt::Horizontal, q_("AddOnId"));
		model->setHeaderData(2, Qt::Horizontal, q_("Title"));
		model->setHeaderData(3, Qt::Horizontal, q_("Last Version"));
		model->setHeaderData(4, Qt::Horizontal, q_("Installed Version"));
		model->setHeaderData(5, Qt::Horizontal, "");
	}

	tableView->setModel(model);
	tableView->setColumnHidden(0, true); // hide id
	tableView->setColumnHidden(1, true); // hide addonid
}

void AddOnDialog::populateTables()
{
	// CATALOGS
	initModel(ui->catalogsTableView, TABLE_CATALOG);
	setUpTableView(ui->catalogsTableView);

	// LANDSCAPES
	initModel(ui->landscapeTableView,TABLE_LANDSCAPE);
	setUpTableView(ui->landscapeTableView);

	// LANGUAGE PACK
	initModel(ui->languageTableView, TABLE_LANGUAGE_PACK);
	setUpTableView(ui->languageTableView);

	// SCRIPTS
	initModel(ui->scriptsTableView, TABLE_SCRIPT);
	setUpTableView(ui->scriptsTableView);

	// STARLORE
	initModel(ui->starloreTbleView, TABLE_STARLORE);
	setUpTableView(ui->starloreTbleView);

	// TEXTURES
	initModel(ui->texturesTableView, TABLE_TEXTURE);
	setUpTableView(ui->texturesTableView);
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QNetworkRequest req(QUrl("http://cardinot.sourceforge.net/getUpdates.php?time="
				 % QString::number(m_StelAddOn.getLastUpdate())));
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pUpdateCatalogReply->setReadBufferSize(1024*1024*2);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(m_pUpdateCatalogReply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void AddOnDialog::downloadError(QNetworkReply::NetworkError)
{
	Q_ASSERT(m_pUpdateCatalogReply);
	qWarning() << "Error updating database catalog!" << m_pUpdateCatalogReply->errorString();
	ui->btnUpdate->setEnabled(true);
	ui->txtLastUpdate->setText(q_("Database update failed!"));
}

void AddOnDialog::downloadFinished()
{
	Q_ASSERT(m_pUpdateCatalogReply);
	if (m_pUpdateCatalogReply->error()!=QNetworkReply::NoError)
	{
		m_pUpdateCatalogReply->deleteLater();
		m_pUpdateCatalogReply = NULL;
		return;
	}

	QByteArray data=m_pUpdateCatalogReply->readAll();
	QString result(data);

	if (!result.isEmpty())
	{
		m_StelAddOn.updateDatabase(result);
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch()/1000;
	ui->btnUpdate->setEnabled(true);
	m_StelAddOn.setLastUpdate(currentTime);
	ui->txtLastUpdate->setText(m_StelAddOn.getLastUpdateString());
	populateTables();
}

void AddOnDialog::installSelectedRows() {
	Q_ASSERT(m_checkBoxes.count() == m_currentTableView->model()->rowCount());

	int currentTab = ui->stackedWidget->currentIndex();
	for (int i=0;i<m_checkBoxes.count(); ++i)
	{
		if (!m_checkBoxes.value(i)->checkState())
		{
			continue;
		}

		int id = m_currentTableView->model()->index(i, 0).data().toInt();
		int addonId = m_currentTableView->model()->index(i, 1).data().toInt();
		switch (currentTab)
		{
			case CATALOG:
			{
				QString type = m_currentTableView->model()->index(i, 3).data().toString();
				// TODO: install
				break;
			}
			case LANDSCAPE:
				m_StelAddOn.installLandscape(id, addonId);
				break;
			case LANGUAGEPACK:
				// TODO: install
				break;
			case SCRIPT:
				// TODO: install
				break;
			case STARLORE:
				// TODO: install
				break;
			case TEXTURE:
				// TODO: install
				break;
		}
	}
}
