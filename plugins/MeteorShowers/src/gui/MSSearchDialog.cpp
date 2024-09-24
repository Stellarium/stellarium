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

	connect(m_ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(m_ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(m_ui->searchButton, SIGNAL(clicked()), this, SLOT(searchEvents()));

	connect(m_ui->listEvents, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(m_ui->listEvents, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		m_ui->listEvents, SLOT(repaint()));

	int year = QDate::fromJulianDay(StelApp::getInstance().getCore()->getJD()).year();
	m_ui->fromYearSpinBox->setValue(year);
	m_ui->fromYearSpinBox->setToolTip(QString("%1 %2..%3").arg(q_("Valid range years:"), QString::number(m_ui->fromYearSpinBox->minimum()), QString::number(m_ui->fromYearSpinBox->maximum())));

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

void MSSearchDialog::searchEvents()
{
	QList<MeteorShowers::SearchResult> searchResult;
	searchResult = m_mgr->getMeteorShowers()->searchEvents(m_ui->fromYearSpinBox->value());

	//Fill list of events
	initListEvents();
	for (const auto& r : std::as_const(searchResult))
	{
		MSTreeWidgetItem* treeItem = new MSTreeWidgetItem(m_ui->listEvents);
		treeItem->setText(ColumnCode, r.code);
		treeItem->setText(ColumnName, r.name);
		treeItem->setText(ColumnDataType, r.type);
		// peak JD from solar longitude
		double peakJD = MeteorShower::JDfromSolarLongitude(r.peak, r.peakyear);
		StelCore* core = StelApp::getInstance().getCore();
		const double utcShift = core->getUTCOffset(core->getJD()) / 24.;
		int Year, Month, Day;
		StelUtils::getDateFromJulianDay(peakJD+utcShift, &Year, &Month, &Day);
		treeItem->setText(ColumnPeak, QString("%1 %2").arg(Day).arg(StelLocaleMgr::longGenitiveMonthName(Month)));
		if (r.zhrMin != r.zhrMax)
			treeItem->setText(ColumnZHR, QString("%1-%2").arg(r.zhrMin).arg(r.zhrMax));
		else
			treeItem->setText(ColumnZHR, QString::number(r.zhrMax));
		treeItem->setText(ColumnClass, r.activityClass.first>0 ? r.activityClass.second : QChar(0x2014));

		// let's store the stuff in the UserRole to allow easier sorting
		// check MSTreeWidgetItem::operator <()
		treeItem->setData(ColumnCode, Qt::UserRole, r.code);
		treeItem->setData(ColumnName, Qt::UserRole, r.name);
		treeItem->setData(ColumnDataType, Qt::UserRole, r.type);
		treeItem->setData(ColumnPeak, Qt::UserRole, peakJD);
		treeItem->setData(ColumnZHR, Qt::UserRole, r.zhrMax);
		treeItem->setData(ColumnClass, Qt::UserRole, r.activityClass.first);
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
	double JD = modelIndex.sibling(modelIndex.row(), ColumnPeak).data(Qt::UserRole).toDouble();
	core->setJD(JD - core->getUTCOffset(JD)/24.);

	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), ColumnName).data().toString();
	StelObjectP obj = m_mgr->getMeteorShowers()->searchByNameI18n(nameI18n);
	if (!obj)
		obj = m_mgr->getMeteorShowers()->searchByName(nameI18n);

	if (obj) // Set time near transit...
	{
		Vec4d rts = obj->getRTSTime(core);
		core->setJD(rts[1]);
	}

	// Move to object
	if (GETSTELMODULE(StelObjectMgr)->findAndSelectI18n(nameI18n))
		GETSTELMODULE(StelMovementMgr)->setFlagTracking(true);
}

void MSSearchDialog::refreshRangeDates()
{
	int year = QDate::fromJulianDay(StelApp::getInstance().getCore()->getJD()).year();
	m_ui->fromYearSpinBox->setValue(year);
}

void MSSearchDialog::setHeaderNames()
{
	QStringList headerStrings;
	headerStrings << q_("Code");
	headerStrings << q_("Name");
	headerStrings << qc_("ZHR","column name");
	headerStrings << q_("Data Type");
	headerStrings << q_("Peak");
	headerStrings << q_("Class");
	m_ui->listEvents->setHeaderLabels(headerStrings);

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    m_ui->listEvents->resizeColumnToContents(i);
	}
}
