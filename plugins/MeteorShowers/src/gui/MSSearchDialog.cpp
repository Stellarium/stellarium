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

#include <QColorDialog>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QGraphicsColorizeEffect>
#include <QMessageBox>
#include <QTimer>
#include <QUrl>

#include "MSSearchDialog.hpp"
#include "MeteorShowers.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "ui_MSSearchDialog.h"

MSSearchDialog::MSSearchDialog(MeteorShowersMgr* mgr)
	: m_mgr(mgr)
	, m_ui(new Ui_MSSearchDialog)
	, m_treeWidget(NULL)
{
}

MSSearchDialog::~MSSearchDialog()
{
	delete m_ui;
	delete m_treeWidget;
}

void MSSearchDialog::retranslate()
{
	if (dialog)
	{
		m_ui->retranslateUi(dialog);
		setHeaderNames();

		//Retranslate name and datatype strings
		QTreeWidgetItemIterator it(m_treeWidget);
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

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(m_ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(m_ui->searchButton, SIGNAL(clicked()), this, SLOT(checkDates()));
	refreshRangeDates(StelApp::getInstance().getCore());

	m_treeWidget = m_ui->listEvents;
	connect(m_treeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));
	connect(m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(repaintTreeWidget()));
	initListEvents();
}

void MSSearchDialog::repaintTreeWidget()
{
	// Enable force repaint listEvents to avoiding artifacts
	// Seems bug in Qt5. Details: https://bugs.launchpad.net/stellarium/+bug/1350669
	m_treeWidget->repaint();
}

void MSSearchDialog::initListEvents()
{
	m_treeWidget->clear();
	m_treeWidget->setColumnCount(ColumnCount);
	setHeaderNames();
	m_treeWidget->header()->setSectionsMovable(false);
	m_treeWidget->header()->setStretchLastSection(true);
}

void MSSearchDialog::setHeaderNames()
{
	QStringList headerStrings;
	headerStrings << q_("Name");
	headerStrings << q_("ZHR");
	headerStrings << q_("Data Type");
	headerStrings << q_("Peak");
	m_treeWidget->setHeaderLabels(headerStrings);
}

void MSSearchDialog::checkDates()
{
	double jdFrom = StelUtils::qDateTimeToJd((QDateTime) m_ui->dateFrom->date());
	double jdTo = StelUtils::qDateTimeToJd((QDateTime) m_ui->dateTo->date());

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
		TreeWidgetItem *treeItem = new TreeWidgetItem(m_treeWidget);
		treeItem->setText(ColumnName, r.name);
		treeItem->setText(ColumnZHR, r.zhr);
		treeItem->setText(ColumnDataType, r.type);
		treeItem->setText(ColumnPeak, r.peak.toString("dd/MMM/yyyy"));
	}
}

void MSSearchDialog::selectEvent(const QModelIndex &modelIndex)
{
	Q_UNUSED(modelIndex);
	StelCore *core = StelApp::getInstance().getCore();

	// if user select an event but the m_mgr is disabled,
	// 'MeteorShowers' is enabled automatically
	if (!m_mgr->getEnablePlugin())
	{
		//m_mgr->setEnableMeteorShowers(true);
	}

	//Change date
	QString dateString = m_treeWidget->currentItem()->text(ColumnPeak);
	QDateTime qDateTime = QDateTime::fromString(dateString, "dd/MMM/yyyy");
	core->setJDay(StelUtils::qDateTimeToJd(qDateTime));

	//Select object
	QString namel18n = m_treeWidget->currentItem()->text(ColumnName);
	StelObjectMgr* objectMgr = GETSTELMODULE(StelObjectMgr);
	if (objectMgr->findAndSelectI18n(namel18n) || objectMgr->findAndSelect(namel18n))
	{
		//Move to object
		StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
		mvmgr->moveToObject(objectMgr->getSelectedObject()[0], mvmgr->getAutoMoveDuration());
		mvmgr->setFlagTracking(true);
	}
}

void MSSearchDialog::refreshRangeDates(StelCore* core)
{
	double JD = core->getJDay();
	QDateTime skyDate = StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);
	int year = skyDate.toString("yyyy").toInt();
	m_ui->dateFrom->setDate(QDate(year, 1, 1)); // first date in the range - first day of the year
	m_ui->dateTo->setDate(QDate(year, 12, 31)); // second date in the range - last day of the year
}
