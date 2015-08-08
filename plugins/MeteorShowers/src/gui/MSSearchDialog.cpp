/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#include <QMessageBox>

#include "MSSearchDialog.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"
#include "ui_MSSearchDialog.h"

MSSearchDialog::MSSearchDialog(MeteorShowersMgr* mgr)
	: m_mgr(mgr)
	, m_ui(new Ui_MSSearchDialog)
{
}

MSSearchDialog::~MSSearchDialog()
{
	delete m_ui;
}

void MSSearchDialog::retranslate()
{
	if (dialog)
	{
		m_ui->retranslateUi(dialog);
		setHeaderNames();

		//Retranslate name and datatype strings
		QTreeWidgetItemIterator it(m_ui->listEvents);
		while (*it)
		{
			//Name
			(*it)->setText(ColumnName, q_((*it)->text(ColumnName)));
			//Data type
			(*it)->setText(ColumnDataType, q_((*it)->text(ColumnDataType)));
			++it;
		}
	}
}

void MSSearchDialog::createDialogContent()
{
	m_ui->setupUi(dialog);

#ifdef Q_OS_WIN
	// Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << m_ui->listEvents;
	installKineticScrolling(addscroll);
#endif

	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(refreshRangeDates()));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	connect(m_ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(m_ui->searchButton, SIGNAL(clicked()), this, SLOT(checkDates()));

	connect(m_ui->listEvents, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(m_ui->listEvents, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		m_ui->listEvents, SLOT(repaint()));

	refreshRangeDates();
	initListEvents();
}

void MSSearchDialog::initListEvents()
{
	m_ui->listEvents->clear();
	m_ui->listEvents->setColumnCount(ColumnCount);
	setHeaderNames();
	m_ui->listEvents->header()->setSectionsMovable(false);
	m_ui->listEvents->header()->setStretchLastSection(true);
}

void MSSearchDialog::checkDates()
{
	double jdFrom = m_ui->dateFrom->date().toJulianDay();
	double jdTo = m_ui->dateTo->date().toJulianDay();

	if (jdFrom > jdTo)
	{
		QMessageBox::warning(0, "Stellarium", q_("Start date greater than end date!"));
	}
	else if (jdTo-jdFrom > 365)
	{
		QMessageBox::warning(0, "Stellarium", q_("Time interval must be less than one year!"));
	}
	else
	{
		searchEvents();
	}
}

void MSSearchDialog::searchEvents()
{
	QList<MeteorShowers::SearchResult> searchResult;
	searchResult = m_mgr->getMeteorShowers()->searchEvents(m_ui->dateFrom->date(), m_ui->dateTo->date());

	//Fill list of events
	initListEvents();
	foreach (const MeteorShowers::SearchResult& r, searchResult)
	{
		TreeWidgetItem *treeItem = new TreeWidgetItem(m_ui->listEvents);
		treeItem->setText(ColumnName, r.name);
		treeItem->setText(ColumnZHR, r.zhr);
		treeItem->setText(ColumnDataType, r.type);
		treeItem->setText(ColumnPeak, r.peak.toString("dd/MMM/yyyy"));
	}

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    m_ui->listEvents->resizeColumnToContents(i);
	}

	// sort-by-date
	m_ui->listEvents->sortItems(ColumnPeak, Qt::AscendingOrder);
}

void MSSearchDialog::selectEvent(const QModelIndex &modelIndex)
{
	// plugin is disabled ? enable it automatically
	if (!m_mgr->getEnablePlugin())
	{
		m_mgr->setEnablePlugin(true);
	}

	// Change date
	QString peak = modelIndex.sibling(modelIndex.row(), ColumnPeak).data().toString();
	StelApp::getInstance().getCore()->setJD(QDate::fromString(peak, "dd/MMM/yyyy").toJulianDay());
	m_mgr->repaint();

	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), ColumnName).data().toString();
	StelObjectP obj = m_mgr->getMeteorShowers()->searchByNameI18n(nameI18n);
	if (!obj)
	{
		obj = m_mgr->getMeteorShowers()->searchByName(nameI18n);
	}

	//Move to object
	if (obj)
	{
		StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
		mvmgr->moveToObject(obj, mvmgr->getAutoMoveDuration());
		mvmgr->setFlagTracking(true);
	}
}

void MSSearchDialog::refreshRangeDates()
{
	int year = QDate::fromJulianDay(StelApp::getInstance().getCore()->getJD()).year();
	m_ui->dateFrom->setDate(QDate(year, 1, 1));
	m_ui->dateTo->setDate(QDate(year, 12, 31));
}

void MSSearchDialog::setHeaderNames()
{
	QStringList headerStrings;
	headerStrings << q_("Name");
	headerStrings << q_("ZHR");
	headerStrings << q_("Data Type");
	headerStrings << q_("Peak");
	m_ui->listEvents->setHeaderLabels(headerStrings);

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    m_ui->listEvents->resizeColumnToContents(i);
	}
}
