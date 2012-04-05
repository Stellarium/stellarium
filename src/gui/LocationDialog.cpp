/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
 * Copyright (C) 2011 Alexander Wolf
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

#include "Dialog.hpp"
#include "LocationDialog.hpp"
#include "StelLocationMgr.hpp"
#include "ui_locationDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include <QSettings>
#include <QDebug>
#include <QFrame>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QStringListModel>

LocationDialog::LocationDialog() : isEditingNew(false)
{
	ui = new Ui_locationDialogForm;
	lastVisionMode = StelApp::getInstance().getVisionModeNight();
}

LocationDialog::~LocationDialog()
{
	delete ui;
}

void LocationDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populatePlanetList();
		populateCountryList();
	}
}

void LocationDialog::styleChanged()
{
	// Make the map red if needed
	if (dialog)
		setMapForLocation(StelApp::getInstance().getCore()->getCurrentLocation());
}

// Initialize the dialog widgets and connect the signals/slots
void LocationDialog::createDialogContent()
{
	// We try to directly connect to the observer slots as much as we can
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	// Init the SpinBox entries
	ui->longitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->longitudeSpinBox->setPrefixType(AngleSpinBox::Longitude);
	ui->latitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->latitudeSpinBox->setPrefixType(AngleSpinBox::Latitude);

	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
	proxyModel->setSourceModel((QAbstractItemModel*)StelApp::getInstance().getLocationMgr().getModelAll());
	proxyModel->sort(0, Qt::AscendingOrder);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->citiesListView->setModel(proxyModel);

	populatePlanetList();
	populateCountryList();

	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(listItemActivated(const QModelIndex&)));

	// Connect all the QT signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->mapLabel, SIGNAL(positionChanged(double, double)), this, SLOT(setPositionFromMap(double, double)));

	connect(ui->addLocationToListPushButton, SIGNAL(clicked()), this, SLOT(addCurrentLocationToList()));
	connect(ui->deleteLocationFromListPushButton, SIGNAL(clicked()), this, SLOT(deleteCurrentLocationFromList()));

	setFieldsFromLocation(StelApp::getInstance().getCore()->getCurrentLocation());

	const bool b = StelApp::getInstance().getCore()->getCurrentLocation().getID()
			==StelApp::getInstance().getCore()->getDefaultLocationID();
	ui->useAsDefaultLocationCheckBox->setChecked(b);
	ui->useAsDefaultLocationCheckBox->setEnabled(!b);
	connect(ui->useAsDefaultLocationCheckBox, SIGNAL(clicked()), this, SLOT(useAsDefaultClicked()));

	connectEditSignals();

	QTimer* refreshTimer = new QTimer(this);
	connect(refreshTimer, SIGNAL(timeout()), this, SLOT(updateFromProgram()));
	refreshTimer->start(200);

	ui->citySearchLineEdit->setFocus();
}

// Update the widget to make sure it is synchrone if the location is changed programmatically
void LocationDialog::updateFromProgram()
{
	if (!dialog->isVisible())
		return;

	// Check that the use as default check box needs to be updated
	const bool b = StelApp::getInstance().getCore()->getCurrentLocation().getID() == StelApp::getInstance().getCore()->getDefaultLocationID();
	if (b!=ui->useAsDefaultLocationCheckBox->isChecked())
	{
		ui->useAsDefaultLocationCheckBox->setChecked(b);
		ui->useAsDefaultLocationCheckBox->setEnabled(!b);
	}

	// removing this check and return... we might have the location changed
	// by a script or plugin, and as such we should update the map whenever the
	// location window is visible.
	if (isEditingNew==true)
		return;

	const QString& key1 = StelApp::getInstance().getCore()->getCurrentLocation().getID();
	const QString& key2 = locationFromFields().getID();
	if (key1!=key2)
	{
		setFieldsFromLocation(StelApp::getInstance().getCore()->getCurrentLocation());
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

void LocationDialog::setFieldsFromLocation(const StelLocation& loc)
{	
	// Deactivate edit signals
	disconnectEditSignals();

	ui->cityNameLineEdit->setText(loc.name);
	int idx = ui->countryNameComboBox->findData(loc.country, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use France as default
		ui->countryNameComboBox->findData(QVariant("France"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->countryNameComboBox->setCurrentIndex(idx);

	ui->longitudeSpinBox->setDegrees(loc.longitude);
	ui->latitudeSpinBox->setDegrees(loc.latitude);
	ui->altitudeSpinBox->setValue(loc.altitude);

	idx = ui->planetNameComboBox->findData(loc.planetName, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use Earth as default
		idx = ui->planetNameComboBox->findData(QVariant("Earth"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->planetNameComboBox->setCurrentIndex(idx);
	setMapForLocation(loc);

	// Set pointer position
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);

	ui->deleteLocationFromListPushButton->setEnabled(StelApp::getInstance().getLocationMgr().canDeleteUserLocation(loc.getID()));

	// Reactivate edit signals
	connectEditSignals();
}

// Update the map for the given location.
void LocationDialog::setMapForLocation(const StelLocation& loc)
{
	// Avoids usless processing
	if (lastPlanet==loc.planetName && lastVisionMode==StelApp::getInstance().getVisionModeNight())
		return;

	QPixmap pixmap;
	// Try to set the proper planet map image
	if (loc.planetName=="Earth")
	{
		// Special case for earth, we don't want to see the clouds
		pixmap = QPixmap(":/graphicGui/world.png");
	}
	else
	{
		SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		PlanetP p = ssm->searchByEnglishName(loc.planetName);
		QString path;
		if (p)
		{
			try
			{
				path = StelFileMgr::findFile("textures/"+p->getTextMapName());
			}
			catch (std::runtime_error& e)
			{
				qWarning() << "ERROR - could not find planet map for " << loc.planetName << e.what();
				return;
			}
			pixmap = QPixmap(path);
		}
	}

	if (StelApp::getInstance().getVisionModeNight())
	{
		ui->mapLabel->setPixmap(StelButton::makeRed(pixmap));
	}
	else
	{
		ui->mapLabel->setPixmap(pixmap);
	}

	// For caching
	lastPlanet = loc.planetName;
	lastVisionMode = StelApp::getInstance().getVisionModeNight();
}

void LocationDialog::populatePlanetList()
{
	Q_ASSERT(ui);
	Q_ASSERT(ui->planetNameComboBox);

	QComboBox* planets = ui->planetNameComboBox;
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	QStringList planetNames(ssystem->getAllPlanetEnglishNames());

	//Save the current selection to be restored later
	planets->blockSignals(true);
	int index = planets->currentIndex();
	QVariant selectedPlanetId = planets->itemData(index);
	planets->clear();
	//For each planet, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	foreach(const QString& name, planetNames)
	{
		planets->addItem(q_(name), name);
	}
	//Restore the selection
	index = planets->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	planets->setCurrentIndex(index);
	planets->model()->sort(0);
	planets->blockSignals(false);
}

void LocationDialog::populateCountryList()
{
	Q_ASSERT(ui);
	Q_ASSERT(ui->countryNameComboBox);
	
	QComboBox* countries = ui->countryNameComboBox;
	QStringList countryNames(StelLocaleMgr::getAllCountryNames());

	//Save the current selection to be restored later
	countries->blockSignals(true);
	int index = countries->currentIndex();
	QVariant selectedCountryId = countries->itemData(index);
	countries->clear();
	//For each country, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	foreach(const QString& name, countryNames)
	{
		countries->addItem(q_(name), name);
	}
	//Restore the selection
	index = countries->findData(selectedCountryId, Qt::UserRole, Qt::MatchCaseSensitive);
	countries->setCurrentIndex(index);
	countries->model()->sort(0);
	countries->blockSignals(false);

}

// Create a StelLocation instance from the fields
StelLocation LocationDialog::locationFromFields() const
{
	StelLocation loc;
	int index = ui->planetNameComboBox->currentIndex();
	if (index < 0)
		loc.planetName = QString();//As returned by QComboBox::currentText()
	else
		loc.planetName = ui->planetNameComboBox->itemData(index).toString();
	loc.name = ui->cityNameLineEdit->text();
	loc.latitude = ui->latitudeSpinBox->valueDegrees();
	loc.longitude = ui->longitudeSpinBox->valueDegrees();
	loc.altitude = ui->altitudeSpinBox->value();
	index = ui->countryNameComboBox->currentIndex();
	if (index < 0)
		loc.country = QString();//As returned by QComboBox::currentText()
	else
		loc.country = ui->countryNameComboBox->itemData(index).toString();
	
	return loc;
}

void LocationDialog::listItemActivated(const QModelIndex& index)
{
	isEditingNew=false;
	ui->addLocationToListPushButton->setEnabled(false);

	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForSmallString(index.data().toString());

	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);

	const bool b = loc.getID()==StelApp::getInstance().getCore()->getDefaultLocationID();
	ui->useAsDefaultLocationCheckBox->setChecked(b);
	ui->useAsDefaultLocationCheckBox->setEnabled(!b);
}

void LocationDialog::setPositionFromMap(double longitude, double latitude)
{
	reportEdit();
	StelLocation loc = locationFromFields();
	loc.latitude = latitude;
	loc.longitude = longitude;
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
}

// Called when the planet name is changed by hand
void LocationDialog::comboBoxChanged(const QString&)
{
	reportEdit();
	StelLocation loc = locationFromFields();
	if (loc.planetName!=StelApp::getInstance().getCore()->getCurrentLocation().planetName)
		setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
}

void LocationDialog::spinBoxChanged(int )
{
	reportEdit();
	StelLocation loc = locationFromFields();
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
	//Update the position of the map pointer
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);
}

// Called when the location name is manually changed
void LocationDialog::locationNameChanged(const QString&)
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

	StelLocation loc = locationFromFields();
	if (!StelApp::getInstance().getLocationMgr().canSaveUserLocation(loc))
	{
		if (ui->cityNameLineEdit->hasFocus())
		{
			// The user is editing the location name: don't change it!
			ui->addLocationToListPushButton->setEnabled(false);
			ui->deleteLocationFromListPushButton->setEnabled(false);
			return;
		}
		else
		{
			ui->cityNameLineEdit->setText(q_("New Location"));
			ui->cityNameLineEdit->selectAll();
			loc = locationFromFields();
		}
	}
	ui->addLocationToListPushButton->setEnabled(isEditingNew && StelApp::getInstance().getLocationMgr().canSaveUserLocation(loc));
	ui->deleteLocationFromListPushButton->setEnabled(StelApp::getInstance().getLocationMgr().canDeleteUserLocation(loc.getID()));
}

// Called when the user clic on the save button
void LocationDialog::addCurrentLocationToList()
{
	const StelLocation& loc = locationFromFields();
	ui->citySearchLineEdit->clear();
	StelApp::getInstance().getLocationMgr().saveUserLocation(loc);
	isEditingNew=false;
	ui->addLocationToListPushButton->setEnabled(false);

	const QAbstractItemModel* model = ui->citiesListView->model();
	const QString id = loc.getID();
	for (int i=0;i<model->rowCount();++i)
	{
		if (model->index(i,0).data()==id)
		{
			ui->citiesListView->scrollTo(model->index(i,0));
			ui->citiesListView->selectionModel()->select(model->index(i,0), QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
			listItemActivated(model->index(i,0));
			disconnectEditSignals();
			ui->citySearchLineEdit->setFocus();
			connectEditSignals();
			break;
		}
	}
}

// Called when the user wants to use the current location as default
void LocationDialog::useAsDefaultClicked()
{
	StelCore* core = StelApp::getInstance().getCore();
	QString newDefaultLocationId = core->getCurrentLocation().getID();
	core->setDefaultLocationID(newDefaultLocationId);

	QString currentLocationId = core->getCurrentLocation().getID();
	const bool show = (currentLocationId == core->getDefaultLocationID());
	disconnectEditSignals();
	ui->useAsDefaultLocationCheckBox->setChecked(show);
	ui->useAsDefaultLocationCheckBox->setEnabled(!show);
	//The focus need to be switched to another control, otherwise
	//ui->latitudeSpinBox receives it and emits a valueChanged() signal when
	//the window is closed.
	ui->citySearchLineEdit->setFocus();
	connectEditSignals();
}

// Called when the user clic on the delete button
void LocationDialog::deleteCurrentLocationFromList()
{
	const StelLocation& loc = locationFromFields();
	StelApp::getInstance().getLocationMgr().deleteUserLocation(loc.getID());
}
