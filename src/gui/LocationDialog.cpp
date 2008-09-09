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
#include "StelLocaleMgr.hpp"

#include <QSettings>
#include <QDebug>
#include <QFrame>
#include <QSortFilterProxyModel>

LocationDialog::LocationDialog() : isEditingNew(false)
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
	
	ui->countryNameComboBox->insertItems(0, StelLocaleMgr::getAllCountryNames());
	
	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(listItemActivated(const QModelIndex&)));
	
	// Connect all the QT signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->mapLabel, SIGNAL(positionChanged(double, double)), this, SLOT(setPositionFromMap(double, double)));

	connect(ui->saveLocationPushButton, SIGNAL(clicked()), this, SLOT(saveCurrentLocation()));

	setFieldsFromLocation(StelApp::getInstance().getCore()->getNavigation()->getCurrentLocation());
	
	connectEditSignals();
	
	startTimer(200);	// Refresh the dialog every 0.5 second if the position is changed programmatically
}

// Update the widget to make sure it is synchrone if the location is changed programmatically
void LocationDialog::updateFromProgram()
{
	if (!dialog->isVisible() || isEditingNew==true)
		return;
	
	const QString& key1 = StelApp::getInstance().getCore()->getNavigation()->getCurrentLocation().toSmallString();
	const QString& key2 = locationFromFields().toSmallString();
	
	if (key1!=key2)
	{
		setFieldsFromLocation(StelApp::getInstance().getCore()->getNavigation()->getCurrentLocation());
	}
}
	
void LocationDialog::disconnectEditSignals()
{
	disconnect(ui->longitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(spinBoxChanged()));
	disconnect(ui->latitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(spinBoxChanged()));
	disconnect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(spinBoxChanged(int)));
	disconnect(ui->planetNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(comboBoxChanged(const QString&)));
	disconnect(ui->countryNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(comboBoxChanged(const QString&)));
	disconnect(ui->countryNameComboBox, SIGNAL(activated(const QString&)), this, SLOT(comboBoxChanged(const QString&)));
	disconnect(ui->cityNameLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(locationNameChanged(const QString&)));
}

void LocationDialog::connectEditSignals()
{
	connect(ui->longitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(spinBoxChanged()));
	connect(ui->latitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(spinBoxChanged()));
	connect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(spinBoxChanged(int)));
	connect(ui->planetNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(comboBoxChanged(const QString&)));
	connect(ui->countryNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(comboBoxChanged(const QString&)));
	connect(ui->countryNameComboBox, SIGNAL(activated(const QString&)), this, SLOT(comboBoxChanged(const QString&)));
	connect(ui->cityNameLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(locationNameChanged(const QString&)));
}

void LocationDialog::setFieldsFromLocation(const PlanetLocation& loc)
{
	// Deactivate edit signals
	disconnectEditSignals();
	
	ui->cityNameLineEdit->setText(loc.name);
	int idx = ui->countryNameComboBox->findText(loc.country);
	if (idx==-1)
	{
		// Use France as default
		ui->countryNameComboBox->findText("France", Qt::MatchCaseSensitive);
	}
	ui->countryNameComboBox->setCurrentIndex(idx);
	
	ui->longitudeSpinBox->setDegrees(loc.longitude);
	ui->latitudeSpinBox->setDegrees(loc.latitude);
	ui->altitudeSpinBox->setValue(loc.altitude);
	idx = ui->planetNameComboBox->findText(loc.planetName, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use Earth as default
		ui->planetNameComboBox->findText("Earth");
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
	
	// Reactivate edit signals
	connectEditSignals();
}

// Create a PlanetLocation instance from the fields
PlanetLocation LocationDialog::locationFromFields() const
{
	PlanetLocation loc;
	loc.planetName = ui->planetNameComboBox->currentText();
	loc.name = ui->cityNameLineEdit->text();
	loc.latitude = ui->latitudeSpinBox->valueDegrees();
	loc.longitude = ui->longitudeSpinBox->valueDegrees();
	loc.altitude = ui->altitudeSpinBox->value();
	loc.country = ui->countryNameComboBox->currentText();
	return loc;
}

void LocationDialog::listItemActivated(const QModelIndex& index)
{
	isEditingNew=false;
	ui->saveLocationPushButton->setEnabled(false);
	
	PlanetLocation loc = StelApp::getInstance().getPlanetLocationMgr().locationForSmallString(index.data().toString());
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc, 0.);
	
	// Make location persistent
	StelApp::getInstance().getSettings()->setValue("init_location/location",loc.toSmallString());
}

void LocationDialog::setPositionFromMap(double longitude, double latitude)
{
	reportEdit();
	PlanetLocation loc = locationFromFields();
	loc.latitude = latitude;
	loc.longitude = longitude;
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc, 0.);
}

// Called when the planet name is changed by hand
void LocationDialog::comboBoxChanged(const QString& text)
{
	reportEdit();
	PlanetLocation loc = locationFromFields();
	if (loc.planetName!=StelApp::getInstance().getCore()->getNavigation()->getCurrentLocation().planetName)
		setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc, 0.);
}

void LocationDialog::spinBoxChanged(int i)
{
	reportEdit();
	PlanetLocation loc = locationFromFields();
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc, 0.);
}

// Called when the location name is manually changed
void LocationDialog::locationNameChanged(const QString& text)
{
	reportEdit();
}

void LocationDialog::reportEdit()
{
	if (isEditingNew==false)
	{
		// The user starts editing manually a field, this creates automatically a new location
		// and allows to save it to the user locations list
		isEditingNew=true;
	}
	
	PlanetLocation loc = locationFromFields();
	if (!StelApp::getInstance().getPlanetLocationMgr().canSaveUserLocation(loc))
	{
		ui->cityNameLineEdit->setText("New Location");
		ui->cityNameLineEdit->selectAll();
		loc = locationFromFields();
	}
	ui->saveLocationPushButton->setEnabled(isEditingNew && StelApp::getInstance().getPlanetLocationMgr().canSaveUserLocation(loc));
}

// Called when the user clic on the save button
void LocationDialog::saveCurrentLocation()
{
	const PlanetLocation& loc = locationFromFields();
	StelApp::getInstance().getPlanetLocationMgr().saveUserLocation(loc);
	isEditingNew=false;
	ui->saveLocationPushButton->setEnabled(false);
	StelApp::getInstance().getCore()->getNavigation()->moveObserverTo(loc, 0.);
	
	// Make location persistent
	StelApp::getInstance().getSettings()->setValue("init_location/location",loc.toSmallString());
}
