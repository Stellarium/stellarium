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
#include "addons/AddOnTableModel.hpp"

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

	// storing all tableViews in a list (easier to iterate)
	// it must be ordered according to the stackwidget
	m_tableViews.append(ui->catalogsTableView);
	m_tableViews.append(ui->landscapeTableView);
	m_tableViews.append(ui->languageTableView);
	m_tableViews.append(ui->scriptsTableView);
	m_tableViews.append(ui->starloreTableView);
	m_tableViews.append(ui->texturesTableView);

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
	// cleaning checkboxes
	Tab prev = (Tab) ui->stackedWidget->currentIndex();
	foreach (QAbstractButton* cbox, m_checkBoxes.value(prev)->buttons()) {
		cbox->setChecked(false);
	}

	if (!current)
	{
		current = previous;
	}
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}

void AddOnDialog::populateTables()
{
	m_iSelectedAddOns.clear();

	// destroying all checkboxes
	int i = 0;
	while (!m_checkBoxes.isEmpty())
	{
		delete m_checkBoxes.take((Tab)i);
		i++;
	}

	int tab = 0;
	foreach (QTableView* view, m_tableViews)
	{
		setUpTableView(view, m_tabToTableName.value((Tab)tab));
		insertCheckBoxes(view, (Tab)tab);
		tab++;
	}
}

void AddOnDialog::insertCheckBoxes(QTableView* tableview, Tab tab)
{
	QButtonGroup* checkboxes = new QButtonGroup(this);
	checkboxes->setExclusive(false);
	connect(checkboxes, SIGNAL(buttonToggled(int, bool)),
		this, SLOT(slotRowSelected(int, bool)));

	int lastColumn = tableview->horizontalHeader()->count() - 1;
	for(int row=0; row < tableview->model()->rowCount(); ++row)
	{
		QCheckBox* cbox = new QCheckBox();
		cbox->setStyleSheet("QCheckBox { padding-left: 8px; }");
		tableview->setIndexWidget(tableview->model()->index(row, lastColumn), cbox);
		checkboxes->addButton(cbox, row);
	}
	m_checkBoxes.insert(tab, checkboxes);
}

void AddOnDialog::setUpTableView(QTableView* tableView, QString tableName)
{
	AddOnTableModel* model = new AddOnTableModel(tableName);
	tableView->setModel(model);

	// hide internal columns
	tableView->setColumnHidden(model->findColumn(COLUMN_ID), true);
	tableView->setColumnHidden(model->findColumn(COLUMN_ADDONID), true);
	tableView->setColumnHidden(model->findColumn(COLUMN_FIRST_STEL), true);
	tableView->setColumnHidden(model->findColumn(COLUMN_LAST_STEL), true);

	// hide imcompatible add-ons
	for (int row = 0; row < model->rowCount(); row++)
	{
		tableView->setRowHidden(row,
					!isCompatible(model->index(row, 2).data().toString(),
						      model->index(row, 3).data().toString()));
	}

	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	int lastColumn = tableView->horizontalHeader()->count() - 1; // checkbox
	tableView->horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);
	tableView->verticalHeader()->setVisible(false);
	tableView->setAlternatingRowColors(false);
	tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableView->setEditTriggers(false);
}

bool AddOnDialog::isCompatible(QString first, QString last)
{
	QStringList c = StelUtils::getApplicationVersion().split(".");
	QStringList f = first.split(".");
	QStringList l = last.split(".");

	if (c.size() < 3 || f.size() < 3 || l.size() < 3) {
		return false; // invalid version
	}

	int currentVersion = QString(c.at(0) % "00" % c.at(1) % "0" % c.at(2)).toInt();
	int firstVersion = QString(f.at(0) % "00" % f.at(1) % "0" % f.at(2)).toInt();
	int lastVersion = QString(l.at(0) % "00" % l.at(1) % "0" % l.at(2)).toInt();

	if (currentVersion < firstVersion || currentVersion > lastVersion)
	{
		return false; // out of bounds
	}

	return true;
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QUrl url(StelApp::getInstance().getStelAddOnMgr().getUrlForUpdates());
	url.setQuery(QString("time=%1").arg(StelApp::getInstance().getStelAddOnMgr().getLastUpdate()));
	QNetworkRequest req(url);
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
	if (m_pUpdateCatalogReply->error() != QNetworkReply::NoError)
	{
		m_pUpdateCatalogReply->deleteLater();
		m_pUpdateCatalogReply = NULL;
		return;
	}

	QByteArray data = m_pUpdateCatalogReply->readAll();
	QString result(data);

	if (!result.isEmpty())
	{
		if(!StelApp::getInstance().getStelAddOnMgr().updateCatalog(result))
		{
			ui->btnUpdate->setEnabled(true);
			ui->txtLastUpdate->setText(q_("Database update failed!"));
			return;
		}
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch()/1000;
	ui->btnUpdate->setEnabled(true);
	StelApp::getInstance().getStelAddOnMgr().setLastUpdate(currentTime);
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	populateTables();
}

void AddOnDialog::slotRowSelected(int row, bool checked)
{
	AddOnTableModel* model = (AddOnTableModel*) m_tableViews.at(ui->stackedWidget->currentIndex())->model();
	int addOnId = model->findIndex(row, COLUMN_ADDONID).data().toInt();
	bool installed = "Yes" == model->findIndex(row, COLUMN_INSTALLED).data().toString();
	if (checked)
	{
		m_iSelectedAddOns.append(qMakePair(addOnId, installed));
	}
	else
	{
		m_iSelectedAddOns.removeOne(qMakePair(addOnId, installed));
	}

	// it disables the buttons when no line is selected.
	bool allOff = m_iSelectedAddOns.size() <= 0;
	ui->btnInstall->setEnabled(!allOff);
	ui->btnRemove->setEnabled(!allOff);
}

void AddOnDialog::installSelectedRows()
{
	for (int i = 0; i < m_iSelectedAddOns.size(); i++)
	{
		QPair<int,int> p = m_iSelectedAddOns.at(i);
		if (!p.second) // not installed
		{
			StelApp::getInstance().getStelAddOnMgr().installAddOn(p.first);
		}
	}
}

void AddOnDialog::removeSelectedRows()
{
	for (int i = 0; i < m_iSelectedAddOns.size(); i++)
	{
		QPair<int,int> p = m_iSelectedAddOns.at(i);
		if (p.second) // installed
		{
			StelApp::getInstance().getStelAddOnMgr().removeAddOn(p.first);
		}
	}
}
