/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Dialog.hpp"
#include "LocationDialog.hpp"
#include "StelMainGraphicsView.hpp"
#include "PlanetLocationMgr.hpp"
#include "ui_locationDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Navigator.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFileMgr.hpp"

#include <QSettings>
#include <QDebug>
#include <QFrame>
#include <QSortFilterProxyModel>

LocationDialog::LocationDialog()
{
	ui = new Ui_locationDialogForm;
}

LocationDialog::~LocationDialog()
{
	delete ui;
}

void LocationDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void LocationDialog::styleChanged()
{
	// Nothing for now
}

// Initialize the dialog widgets and connect the signals/slots
void LocationDialog::createDialogContent()
{
	// We try to directly connect to the observer slots as much as we can
	ui->setupUi(dialog);

	// Init the SpinBox entries
	ui->longitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->longitudeSpinBox->setPrefixType(AngleSpinBox::Longitude);
	ui->latitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->latitudeSpinBox->setPrefixType(AngleSpinBox::Latitude);
	
	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
	proxyModel->setSourceModel((QAbstractItemModel*)StelApp::getInstance().getPlanetLocationMgr().getModelAll());
	proxyModel->sort(0, Qt::AscendingOrder);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
							  
	ui->citiesListView->setModel(proxyModel);
	
	SolarSystem* ssystem = (SolarSystem*)GETSTELMODULE("SolarSystem");
	ui->planetNameComboBox->insertItems(0, ssystem->getAllPlanetEnglishNames());
	
	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(listItemActivated(const QModelIndex&)));
	
	// Connect all the QT signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->mapLabel, SIGNAL(positionChanged(double, double)), this, SLOT(setPositionFromMap(double, double)));

	setFieldsFromLocation(StelApp::getInstance().getCore()->getNavigation()->getCurrentLocation());
	
// 	connect(ui->longitudeSpinBox, SIGNAL(valueChanged(void)), this, SLOT(spinBoxChanged(void)));
// 	connect(ui->latitudeSpinBox, SIGNAL(valueChanged(void)), this, SLOT(spinBoxChanged(void)));
// 	connect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), observer, SLOT(setAltitude(int)));
}

void LocationDialog::setPositionFromMap(double longitude, double latitude)
{
	PlanetLocation loc;
	loc.longitude = longitude;
	loc.latitude = latitude;
	loc.planetName = ui->planetNameComboBox->currentText();
	loc.name = "New Location";
	loc.country = "";
	loc.state = "";
	
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc,0);
}

void LocationDialog::setFieldsFromLocation(const PlanetLocation& loc)
{
	ui->cityNameLineEdit->setText(loc.name);
	ui->countryNameLineEdit->setText(loc.country);
	ui->longitudeSpinBox->setDegrees(loc.longitude);
	ui->latitudeSpinBox->setDegrees(loc.latitude);
	int idx = ui->planetNameComboBox->findText(loc.planetName, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use Earth as default
		ui->planetNameComboBox->findText("Earth", Qt::MatchCaseSensitive);
	}
	ui->planetNameComboBox->setCurrentIndex(idx);
	
	// Try to set the proper planet map image
	if (loc.planetName=="Earth")
	{
		// Special case for earth, we don't want to see the clouds
		QString path;
		try
		{
			path = StelApp::getInstance().getFileMgr().findFile("data/gui/world.png");
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "ERROR - could not find planet map for " << loc.planetName << e.what();
			return;
		}
		ui->mapLabel->setPixmap(path);
	}
	else
	{
		SolarSystem* ssm = (SolarSystem*)GETSTELMODULE("SolarSystem");
		Planet* p = ssm->searchByEnglishName(loc.planetName);
		if (p)
		{
			QString path;
			try
			{
				path = StelApp::getInstance().getFileMgr().findFile("textures/"+p->getTextMapName());
			}
			catch (std::runtime_error& e)
			{
				qWarning() << "ERROR - could not find planet map for " << loc.planetName << e.what();
				return;
			}
			ui->mapLabel->setPixmap(path);
		}
	}
	// Set pointer position
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);
}

void LocationDialog::listItemActivated(const QModelIndex& index)
{
	PlanetLocation loc = StelApp::getInstance().getPlanetLocationMgr().locationForSmallString(index.data().toString());
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc, 0.);
	
	// Make location persistent
	StelApp::getInstance().getSettings()->setValue("init_location/location",loc.toSmallString());
}
