/*
 * ArchaeoLines plug-in for Stellarium
 *
 * Copyright (C) 2021 Georg Zotti
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ArchaeoLines.hpp"
#include "ArchaeoLinesDialogLocations.hpp"
#include "ui_archaeoLinesDialogLocations.h"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelLocationMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

ArchaeoLinesDialogLocations::ArchaeoLinesDialogLocations()
	: StelDialog("ArchaeoLinesLocations")
	, al(nullptr)
	, modalContext(0)
	, allModel(new QStringListModel(this))
	, pickedModel(new QStringListModel(this))
	, proxyModel(nullptr)
{
	ui = new Ui_archaeoLinesDialogLocations();
}

ArchaeoLinesDialogLocations::~ArchaeoLinesDialogLocations()
{
	delete ui;          ui=nullptr;
}

void ArchaeoLinesDialogLocations::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ArchaeoLinesDialogLocations::createDialogContent()
{
	al = GETSTELMODULE(ArchaeoLines);
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->citiesListView;
	StelGui* gui= static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, &StelGui::flagUseKineticScrollingChanged, this, &ArchaeoLinesDialogLocations::enableKineticScrolling);

	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &ArchaeoLinesDialogLocations::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &ArchaeoLinesDialogLocations::handleMovedTo);

	//initialize list model
	StelLocationMgr *locMgr=&(StelApp::getInstance().getLocationMgr());
	connect(locMgr, &StelLocationMgr::locationListChanged, this, &ArchaeoLinesDialogLocations::reloadLocations);
	reloadLocations();
	proxyModel = new QSortFilterProxyModel(ui->citiesListView);
	proxyModel->setSourceModel(allModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->citiesListView->setModel(proxyModel);

	connect(ui->citySearchLineEdit, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterWildcard);
	connect(ui->citiesListView,     &QListView::clicked,     this,       &ArchaeoLinesDialogLocations::setLocationFromList);
}

void ArchaeoLinesDialogLocations::setLocationFromList(const QModelIndex& index)
{
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForString(index.data().toString());
	switch (modalContext)
	{
		case 1:
			al->setGeographicLocation1Latitude(static_cast<double>(loc.getLatitude()));
			al->setGeographicLocation1Longitude(static_cast<double>(loc.getLongitude()));
			al->setGeographicLocation1Name(loc.name);
			break;
		case 2:
			al->setGeographicLocation2Latitude(static_cast<double>(loc.getLatitude()));
			al->setGeographicLocation2Longitude(static_cast<double>(loc.getLongitude()));
			al->setGeographicLocation2Name(loc.name);
			break;
		default:
			// do nothing
			break;
	}
}

void ArchaeoLinesDialogLocations::reloadLocations()
{
	allModel->setStringList(StelApp::getInstance().getLocationMgr().getAllMap().keys());
}

void ArchaeoLinesDialogLocations::setModalContext(int context)
{
	modalContext=context;
}
