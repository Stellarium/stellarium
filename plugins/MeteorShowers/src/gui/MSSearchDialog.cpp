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
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelUtils.hpp"
#include "StelMainView.hpp"
#include "ui_MSSearchDialog.h"

MSSearchDialog::MSSearchDialog(MeteorShowersMgr* mgr)
	: StelDialog("MeteorShowersSearch")
	, m_mgr(mgr)
	, m_ui(new Ui_MSSearchDialog)
{}

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

		if (!m_ui->listEvents->findItems("", Qt::MatchContains, 0).isEmpty())
			searchEvents();
	}
}

void MSSearchDialog::createDialogContent()
{
	m_ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << m_ui->listEvents;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(refreshRangeDates()));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	connect(m_ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(m_ui->searchButton, SIGNAL(clicked()), this, SLOT(checkDates()));

	connect(m_ui->listEvents, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(m_ui->listEvents, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		m_ui->listEvents, SLOT(repaint()));

	// TODO: Switch a QDateTimeEdit to StelDateTimeEdit widget to apply wide range of dates
	QDate min = QDate(100,1,1);
	m_ui->dateFrom->setMinimumDate(min);
	m_ui->dateTo->setMinimumDate(min);

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
		QMessageBox::warning(&StelMainView::getInstance(), "Stellarium", q_("Start date greater than end date!"));
	}
	else if (jdTo-jdFrom > 365)
	{
		QMessageBox::warning(&StelMainView::getInstance(), "Stellarium", q_("Time interval must be less than one year!"));
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
	for (const auto& r : searchResult)
	{
		MSTreeWidgetItem* treeItem = new MSTreeWidgetItem(m_ui->listEvents);
		treeItem->setText(ColumnName, r.name);
		treeItem->setText(ColumnDataType, r.type);
		treeItem->setText(ColumnPeak, QString("%1 %2").arg(r.peak.day()).arg(StelLocaleMgr::longGenitiveMonthName(r.peak.month())));
		if (r.zhrMin != r.zhrMax)
			treeItem->setText(ColumnZHR, QString("%1-%2").arg(r.zhrMin).arg(r.zhrMax));
		else
			treeItem->setText(ColumnZHR, QString::number(r.zhrMax));

		// let's store the stuff in the UserRole to allow easier sorting
		// check MSTreeWidgetItem::operator <()
		treeItem->setData(ColumnName, Qt::UserRole, r.name);
		treeItem->setData(ColumnDataType, Qt::UserRole, r.type);
		treeItem->setData(ColumnPeak, Qt::UserRole, r.peak);
		treeItem->setData(ColumnZHR, Qt::UserRole, r.zhrMax);
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
		m_mgr->setEnablePlugin(true);

	// Change date
	StelCore *core = StelApp::getInstance().getCore();
	QDate peak = modelIndex.sibling(modelIndex.row(), ColumnPeak).data(Qt::UserRole).toDate();
	core->setJD(peak.toJulianDay());

	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), ColumnName).data().toString();
	StelObjectP obj = m_mgr->getMeteorShowers()->searchByNameI18n(nameI18n);
	if (!obj)
		obj = m_mgr->getMeteorShowers()->searchByName(nameI18n);

	if (obj) // Set time near transit...
	{
		Vec3f rts = obj->getRTSTime(core);
		double JD = core->getJD();
		JD = static_cast<int>(JD) + 0.5 + rts[1]/24.f - core->getUTCOffset(JD)/24.;
		core->setJD(JD);
	}

	// Move to object
	if (GETSTELMODULE(StelObjectMgr)->findAndSelectI18n(nameI18n))
		GETSTELMODULE(StelMovementMgr)->setFlagTracking(true);
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
