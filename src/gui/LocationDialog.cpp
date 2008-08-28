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
#include "Observer.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFileMgr.hpp"

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
	
	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(listItemActivated(const QModelIndex&)));
	
	// Connect all the QT signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
// 	connect(ui->graphicsView, SIGNAL(positionSelected(double, double, int, QString)), this, SLOT(selectPosition(double, double, int, QString)));
// 	connect(ui->graphicsView, SIGNAL(positionHighlighted(double, double, int, QString)), this, SLOT(highlightPosition(double, double, int, QString)));
// 	connect(ui->longitudeSpinBox, SIGNAL(valueChanged(void)), this, SLOT(spinBoxChanged(void)));
// 	connect(ui->latitudeSpinBox, SIGNAL(valueChanged(void)), this, SLOT(spinBoxChanged(void)));
// 	connect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), observer, SLOT(setAltitude(int)));
	
	// Init the position value
// 	Observer* observer = StelApp::getInstance().getCore()->getObservatory();
// 	selectPosition(observer->getLongitude(), observer->getLatitude(), observer->getAltitude());
//	spinBoxChanged();
}

// void LocationDialog::selectPosition(double longitude, double latitude, int altitude, QString city)
// {
// 	// Set all the entries
// 	ui->longitudeSpinBox->setDegrees(longitude);
// 	ui->latitudeSpinBox->setDegrees(latitude);
// 	ui->altitudeSpinBox->setValue(altitude);
// 	
// 	StelApp::getInstance().getCore()->getObservatory()->setLongitude(longitude);
// 	StelApp::getInstance().getCore()->getObservatory()->setLatitude(latitude);
// 	StelApp::getInstance().getCore()->getObservatory()->setAltitude(altitude);
// }

// void LocationDialog::spinBoxChanged()
// {
// // 	double longitude = ui->longitudeSpinBox->valueDegrees();
// // 	double latitude = ui->latitudeSpinBox->valueDegrees();
// // 	int altitude = ui->altitudeSpinBox->value();
// // 	ui->graphicsView->select(longitude, latitude, altitude);
// }

void LocationDialog::listItemActivated(const QModelIndex& index)
{
	PlanetLocation loc = StelApp::getInstance().getPlanetLocationMgr().locationForSmallString(index.data().toString());
	ui->cityNameLineEdit->setText(loc.name);
	ui->countryNameLineEdit->setText(loc.country);
	ui->longitudeSpinBox->setDegrees(loc.longitude);
	ui->latitudeSpinBox->setDegrees(loc.latitude);
	ui->planetNameLineEdit->setText(loc.planetName);
	
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
	
	StelApp::getInstance().getCore()->getObservatory()->setPlanetLocation(loc);
}
