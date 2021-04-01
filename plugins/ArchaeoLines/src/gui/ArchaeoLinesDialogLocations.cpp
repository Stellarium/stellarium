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
#include "StelLocaleMgr.hpp"
#include "StelLocationMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
//#include "StelMainView.hpp"
//#include "StelOpenGL.hpp"

ArchaeoLinesDialogLocations::ArchaeoLinesDialogLocations()
	: StelDialog("ArchaeoLinesLocations")
	, al(Q_NULLPTR)
	, modalContext(0)
{
	ui = new Ui_archaeoLinesDialogLocations();
}

ArchaeoLinesDialogLocations::~ArchaeoLinesDialogLocations()
{
	delete ui;          ui=Q_NULLPTR;
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
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	//initialize list model
	allModel = new QStringListModel(this);
	pickedModel = new QStringListModel(this);
	StelLocationMgr *locMgr=&(StelApp::getInstance().getLocationMgr());
	connect(locMgr, SIGNAL(locationListChanged()), this, SLOT(reloadLocations()));
	reloadLocations();
	proxyModel = new QSortFilterProxyModel(ui->citiesListView);
	proxyModel->setSourceModel(allModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->citiesListView->setModel(proxyModel);

	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(setLocationFromList(const QModelIndex&)));
}

void ArchaeoLinesDialogLocations::setLocationFromList(const QModelIndex& index)
{
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForString(index.data().toString());
	switch (modalContext)
	{
		case 1:
			al->setGeographicLocation1Latitude(static_cast<double>(loc.latitude));
			al->setGeographicLocation1Longitude(static_cast<double>(loc.longitude));
			al->setGeographicLocation1Label(loc.name);
			break;
		case 2:
			al->setGeographicLocation2Latitude(static_cast<double>(loc.latitude));
			al->setGeographicLocation2Longitude(static_cast<double>(loc.longitude));
			al->setGeographicLocation2Label(loc.name);
			break;
		default:
			// do nothing
			break;
	}
}

// Connected to the button
void ArchaeoLinesDialogLocations::setLocationFromList()
{
	QModelIndex index=ui->citiesListView->currentIndex();
	setLocationFromList(index);
}

void ArchaeoLinesDialogLocations::reloadLocations()
{
	allModel->setStringList(StelApp::getInstance().getLocationMgr().getAllMap().keys());
}

void ArchaeoLinesDialogLocations::setModalContext(int context)
{
	modalContext=context;
}
