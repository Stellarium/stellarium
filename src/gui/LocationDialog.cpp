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
#include <QTimeZone>

LocationDialog::LocationDialog(QObject* parent)
	: StelDialog("Location", parent)
	, isEditingNew(false)
	, allModel(Q_NULLPTR)
	, pickedModel(Q_NULLPTR)
	, proxyModel(Q_NULLPTR)
{
	ui = new Ui_locationDialogForm;
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
		populateTimeZonesList();
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

	//enable resizability
	ui->mapLabel->setMinimumSize(0,0);
	ui->mapLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	ui->mapLabel->setScaledContents(false);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	// Init the SpinBox entries
	ui->longitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->longitudeSpinBox->setPrefixType(AngleSpinBox::Longitude);
	ui->longitudeSpinBox->setMinimum(-180.0, true);
	ui->longitudeSpinBox->setMaximum( 180.0, true);
	ui->longitudeSpinBox->setWrapping(true);
	ui->latitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->latitudeSpinBox->setPrefixType(AngleSpinBox::Latitude);
	ui->latitudeSpinBox->setMinimum(-90.0, true);
	ui->latitudeSpinBox->setMaximum( 90.0, true);
	ui->latitudeSpinBox->setWrapping(false);

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

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->citiesListView;
	installKineticScrolling(addscroll);
#endif

	populatePlanetList();
	populateCountryList();
	populateTimeZonesList();

	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(setPositionFromList(const QModelIndex&)));

	// Connect all the QT signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->mapLabel, SIGNAL(positionChanged(double, double)), this, SLOT(setPositionFromMap(double, double)));

	connect(ui->addLocationToListPushButton, SIGNAL(clicked()), this, SLOT(addCurrentLocationToList()));
	connect(ui->deleteLocationFromListPushButton, SIGNAL(clicked()), this, SLOT(deleteCurrentLocationFromList()));
	connect(ui->resetListPushButton, SIGNAL(clicked()), this, SLOT(resetCompleteList()));
	connect(ui->countryNameComboBox, SIGNAL(activated(const QString &)), this, SLOT(filterSitesByCountry()));

	StelCore* core = StelApp::getInstance().getCore();
	const StelLocation& currentLocation = core->getCurrentLocation();
	bool b = (currentLocation.getID() == core->getDefaultLocationID());
	QSettings* conf = StelApp::getInstance().getSettings();
	if (conf->value("init_location/location", "auto").toString() == "auto")
	{
		ui->useIpQueryCheckBox->setChecked(true);
		b = false;
	}
	updateDefaultLocationControls(b);

	customTimeZone = conf->value("localization/time_zone", "").toString();
	if (!customTimeZone.isEmpty())
		ui->useCustomTimeZoneCheckBox->setChecked(true);
	else
		ui->timeZoneNameComboBox->setEnabled(false);

	setFieldsFromLocation(currentLocation);

#ifdef ENABLE_GPS
	connect(ui->gpsPushButton, SIGNAL(clicked(bool)), this, SLOT(gpsQueryLocation()));
	connect(locMgr, SIGNAL(gpsQueryFinished(bool)), this, SLOT(gpsReturn(bool)));
#else
	ui->gpsPushButton->setEnabled(false);
	ui->gpsPushButton->hide();
#endif
	connect(ui->useIpQueryCheckBox, SIGNAL(clicked(bool)), this, SLOT(ipQueryLocation(bool)));
	connect(ui->useAsDefaultLocationCheckBox, SIGNAL(clicked(bool)), this, SLOT(setDefaultLocation(bool)));
	connect(ui->pushButtonReturnToDefault, SIGNAL(clicked()), core, SLOT(returnToDefaultLocation()));
	connect(ui->useCustomTimeZoneCheckBox, SIGNAL(clicked(bool)), this, SLOT(updateTimeZoneControls(bool)));

	ui->dstCheckBox->setChecked(core->getUseDST());
	connect(ui->dstCheckBox, SIGNAL(clicked(bool)), core, SLOT(setUseDST(bool)));

	connectEditSignals();

	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(updateFromProgram(StelLocation)));

	ui->citySearchLineEdit->setFocus();
}

void LocationDialog::handleDialogSizeChanged(QSizeF size)
{
	StelDialog::handleDialogSizeChanged(size);
	StelLocation loc = locationFromFields();
	resizePixmap();
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);
}

void LocationDialog::reloadLocations()
{
	allModel->setStringList(StelApp::getInstance().getLocationMgr().getAllMap().keys());
}

// Update the widget to make sure it is synchrone if the location is changed programmatically
void LocationDialog::updateFromProgram(const StelLocation& currentLocation)
{
	if (!dialog->isVisible())
		return;

	StelCore* stelCore = StelApp::getInstance().getCore();

	isEditingNew = false;

	// Check that the use as default check box needs to be updated
	// Move to setFieldsFromLocation()? --BM?
	const bool b = currentLocation.getID() == stelCore->getDefaultLocationID();
	QSettings* conf = StelApp::getInstance().getSettings();
	if (conf->value("init_location/location", "auto").toString() != ("auto"))
	{
		updateDefaultLocationControls(b);
		ui->pushButtonReturnToDefault->setEnabled(!b);
	}

	const QString& key1 = currentLocation.getID();
	const QString& key2 = locationFromFields().getID();	
	if (key1!=key2)
	{
		setFieldsFromLocation(currentLocation);
	}
}

void LocationDialog::disconnectEditSignals()
{
	disconnect(ui->longitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setPositionFromCoords()));
	disconnect(ui->latitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setPositionFromCoords()));
	disconnect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPositionFromCoords(int)));
	disconnect(ui->planetNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(moveToAnotherPlanet(const QString&)));
	disconnect(ui->countryNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(reportEdit()));
	disconnect(ui->timeZoneNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(saveTimeZone()));
	disconnect(ui->cityNameLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(reportEdit()));
}

void LocationDialog::connectEditSignals()
{
	connect(ui->longitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setPositionFromCoords()));
	connect(ui->latitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setPositionFromCoords()));
	connect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPositionFromCoords(int)));
	connect(ui->planetNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(moveToAnotherPlanet(const QString&)));
	connect(ui->countryNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(reportEdit()));
	connect(ui->timeZoneNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(saveTimeZone()));
	connect(ui->cityNameLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(reportEdit()));
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

	QString tz = loc.ianaTimeZone;
	if (loc.planetName=="Earth" && tz.isEmpty())
		tz = "system_default";
	if (!customTimeZone.isEmpty())
		tz = customTimeZone;
	idx = ui->timeZoneNameComboBox->findData(tz, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		if (loc.planetName=="Earth")
			tz = "system_default";
		else
			tz = "LMST";
		// Use LMST/system_default as default
		idx = ui->timeZoneNameComboBox->findData(tz, Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->timeZoneNameComboBox->setCurrentIndex(idx);
	StelApp::getInstance().getCore()->setCurrentTimeZone(tz);

	setMapForLocation(loc);

	// Set pointer position
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);

	ui->deleteLocationFromListPushButton->setEnabled(StelApp::getInstance().getLocationMgr().canDeleteUserLocation(loc.getID()));

	SolarSystem* ssm = GETSTELMODULE(SolarSystem);
	PlanetP p = ssm->searchByEnglishName(loc.planetName);
	StelModule* ls = StelApp::getInstance().getModule("LandscapeMgr");
	if (ls->property("flagAtmosphereAutoEnable").toBool())
	{
		if (loc.planetName != StelApp::getInstance().getCore()->getCurrentLocation().planetName)
		{
			QSettings* conf = StelApp::getInstance().getSettings();
			ls->setProperty("atmosphereDisplayed", p->hasAtmosphere() & conf->value("landscape/flag_atmosphere", true).toBool());
			ls->setProperty("fogDisplayed", p->hasAtmosphere() & conf->value("landscape/flag_fog", true).toBool());
		}
	}

	// Reactivate edit signals
	connectEditSignals();
}

// Update the map for the given location.
void LocationDialog::setMapForLocation(const StelLocation& loc)
{
	// Avoids useless processing
	if (lastPlanet==loc.planetName)
		return;

	//QPixmap pixmap;
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
		if (p)
		{
			QString path = StelFileMgr::findFile("textures/"+p->getTextMapName());
			if (path.isEmpty())
			{
				qWarning() << "ERROR - could not find planet map for " << loc.planetName;
				return;
			}
			pixmap = QPixmap(path);
		}
	}
	//StelCore * core = StelApp::getInstance().getCore();
	//pixmap.setDevicePixelRatio(core->getCurrentStelProjectorParams().devicePixelsPerPixel);
	ui->mapLabel->setPixmap(pixmap);
	resizePixmap();
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);
	// For caching
	lastPlanet = loc.planetName;
}

void LocationDialog::resizePixmap()
{
	int w = ui->mapLabel->width();
	int h = ui->mapLabel->height();
	ui->mapLabel->setPixmap(pixmap.scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void LocationDialog::populatePlanetList()
{
	Q_ASSERT(ui->planetNameComboBox);
	QComboBox* planets = ui->planetNameComboBox;
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	QList<PlanetP> ss = ssystem->getAllPlanets();

	//Save the current selection to be restored later
	planets->blockSignals(true);
	int index = planets->currentIndex();
	QVariant selectedPlanetId = planets->itemData(index);
	planets->clear();
	//For each planet, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	foreach(const PlanetP& p, ss)
	{
		planets->addItem(p->getNameI18n(), p->getEnglishName());
	}
	//Restore the selection
	index = planets->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	planets->setCurrentIndex(index);
	planets->model()->sort(0);
	planets->blockSignals(false);
}

void LocationDialog::populateCountryList()
{
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
	countries->addItem(QChar(0x2014), "");
	//Restore the selection
	index = countries->findData(selectedCountryId, Qt::UserRole, Qt::MatchCaseSensitive);
	countries->setCurrentIndex(index);
	countries->model()->sort(0);
	countries->blockSignals(false);

}

void LocationDialog::populateTimeZonesList()
{
	Q_ASSERT(ui->timeZoneNameComboBox);

	QComboBox* timeZones = ui->timeZoneNameComboBox;
	// Return a list of all the known time zone names (from Qt)
	QStringList tzNames;
	QList<QByteArray> tzList = QTimeZone::availableTimeZoneIds(); // System dependent set of IANA timezone names.
	QList<QByteArray>::iterator i;
	for (i = tzList.begin(); i!= tzList.end(); ++i)
	{
		tzNames.append(*i);
		// Activate this to get a list of known TZ names...
		//qDebug() << "Qt/IANA TZ entry: " << *i;
	}

	tzNames.sort();

	//Save the current selection to be restored later
	timeZones->blockSignals(true);
	int index = timeZones->currentIndex();
	QVariant selectedTzId = timeZones->itemData(index);
	timeZones->clear();
	//For each time zone, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a loop.
	foreach(const QString& name, tzNames)
	{
		timeZones->addItem(name, name);
	}
	timeZones->addItem(q_("Local Mean Solar Time"), "LMST");
	timeZones->addItem(q_("Local True Solar Time"), "LTST");
	timeZones->addItem(q_("System default"), "system_default");
	//Restore the selection
	index = timeZones->findData(selectedTzId, Qt::UserRole, Qt::MatchCaseSensitive);
	// TODO: Handle notfound!?
	//Q_ASSERT(index!=-1);
	timeZones->setCurrentIndex(index);
	timeZones->blockSignals(false);

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
	loc.name = ui->cityNameLineEdit->text().trimmed(); // avoid locations with leading whitespace
	loc.latitude = qBound(-90.0, ui->latitudeSpinBox->valueDegrees(), 90.0);
	loc.longitude = ui->longitudeSpinBox->valueDegrees();
	loc.altitude = ui->altitudeSpinBox->value();
	index = ui->countryNameComboBox->currentIndex();
	if (index < 0)
		loc.country = QString();//As returned by QComboBox::currentText()
	else
		loc.country = ui->countryNameComboBox->itemData(index).toString();

	index = ui->timeZoneNameComboBox->currentIndex();
	if (index < 0)
		loc.ianaTimeZone = QString(); //As returned by QComboBox::currentText()
	else
	{
		QString tz=ui->timeZoneNameComboBox->itemData(index).toString();
		loc.ianaTimeZone = tz;
	}

	return loc;
}

void LocationDialog::setPositionFromList(const QModelIndex& index)
{
	isEditingNew=false;
	ui->addLocationToListPushButton->setEnabled(false);
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForString(index.data().toString());
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
	// This calls indirectly updateFromProgram()
}

void LocationDialog::setPositionFromMap(double longitude, double latitude)
{
	reportEdit();
	StelLocation loc = locationFromFields();
	loc.latitude = latitude;
	loc.longitude = longitude;
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
	if (customTimeZone.isEmpty())
		ui->timeZoneNameComboBox->setCurrentIndex(ui->timeZoneNameComboBox->findData("LMST", Qt::UserRole, Qt::MatchCaseSensitive));
	// GZ: Filter location list for nearby sites. I assume Earth locations are better known. With only few locations on other planets in the list, 30 degrees seem OK.
	LocationMap results = StelApp::getInstance().getLocationMgr().pickLocationsNearby(loc.planetName, longitude, latitude, loc.planetName=="Earth" ? 3.0f: 30.0f);
	pickedModel->setStringList(results.keys());
	proxyModel->setSourceModel(pickedModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	ui->citySearchLineEdit->clear();	
}

// Called when the planet name is changed by hand
void LocationDialog::moveToAnotherPlanet(const QString&)
{
	reportEdit();
	StelLocation loc = locationFromFields();
	StelCore* stelCore = StelApp::getInstance().getCore();
	StelModule* ls = StelApp::getInstance().getModule("LandscapeMgr");
	if (loc.planetName != stelCore->getCurrentLocation().planetName)
	{
		setFieldsFromLocation(loc);
		if (ls->property("flagLandscapeAutoSelection").toBool())
		{
			// If we have a landscape for selected planet then set it, otherwise use default landscape
			// Details: https://bugs.launchpad.net/stellarium/+bug/1173254
			if (ls->property("allLandscapeNames").toStringList().indexOf(loc.planetName)>0)
				ls->setProperty("currentLandscapeName", loc.planetName);
			else
				ls->setProperty("currentLandscapeID", ls->property("defaultLandscapeID"));
		}

		// GZ populate site list with sites only from that planet, or full list for Earth (faster than removing the ~50 non-Earth positions...).
		StelLocationMgr &locMgr=StelApp::getInstance().getLocationMgr();
		if (loc.planetName == "Earth")
		{
			proxyModel->setSourceModel(allModel);
		}
		else
		{
			LocationMap results = locMgr.pickLocationsNearby(loc.planetName, 0.0f, 0.0f, 180.0f);
			pickedModel->setStringList(results.keys());
			proxyModel->setSourceModel(pickedModel);
			ui->countryNameComboBox->setCurrentIndex(ui->countryNameComboBox->findData("", Qt::UserRole, Qt::MatchCaseSensitive));
			if (customTimeZone.isEmpty())
				ui->timeZoneNameComboBox->setCurrentIndex(ui->timeZoneNameComboBox->findData("LMST", Qt::UserRole, Qt::MatchCaseSensitive));

		}
		proxyModel->sort(0, Qt::AscendingOrder);
		ui->citySearchLineEdit->clear();
		ui->citySearchLineEdit->setFocus();
	}
	// Planet transition time also set to null to prevent uglyness when
	// "use landscape location" is enabled for that planet's landscape. --BM
	// NOTE: I think it also makes sense in the other cases. --BM
	// FIXME: Avoid the unnecessary change of the location anyway. --BM
	stelCore->moveObserverTo(loc, 0., 0.);
}

void LocationDialog::setPositionFromCoords(int i)
{
	Q_UNUSED(i)
	reportEdit();
	StelLocation loc = locationFromFields();
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
	//Update the position of the map pointer
	ui->mapLabel->setCursorPos(loc.longitude, loc.latitude);
}

void LocationDialog::saveTimeZone()
{
	QString tz = ui->timeZoneNameComboBox->itemData(ui->timeZoneNameComboBox->currentIndex()).toString();
	StelCore* core = StelApp::getInstance().getCore();
	core->setCurrentTimeZone(tz);
	if (ui->useCustomTimeZoneCheckBox->isChecked())
	{
		StelApp::getInstance().getSettings()->setValue("localization/time_zone", tz);
		core->setUseCustomTimeZone(true);
	}
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
	StelLocationMgr& locationMgr = StelApp::getInstance().getLocationMgr();
	bool canSave = locationMgr.canSaveUserLocation(loc);
	if (!canSave)
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
			ui->cityNameLineEdit->setText("");
			ui->cityNameLineEdit->selectAll();
			loc = locationFromFields();
		}
	}
	ui->addLocationToListPushButton->setEnabled(isEditingNew && canSave);
	ui->deleteLocationFromListPushButton->setEnabled(locationMgr.canDeleteUserLocation(loc.getID()));
	ui->timeZoneNameComboBox->setEnabled(isEditingNew && canSave);
}

// Called when the user clicks on the save button
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
			//FIXME: scroll caused artifcats in the GUI for long lists. WTF????
			//ui->citiesListView->scrollTo(model->index(i,0));
			ui->citiesListView->selectionModel()->select(model->index(i,0), QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
			setPositionFromList(model->index(i,0));
			disconnectEditSignals();
			ui->citySearchLineEdit->setFocus();
			connectEditSignals();
			break;
		}
	}
}

// Called when the user wants to use the current location as default
void LocationDialog::setDefaultLocation(bool state)
{
	if (state)
	{
		StelCore* core = StelApp::getInstance().getCore();
		QString currentLocationId = core->getCurrentLocation().getID();
		core->setDefaultLocationID(currentLocationId);

		// Why this code even exists? After the previous code, this should always
		// be true, except if setting the default location somehow fails. --BM
		bool isDefault = (currentLocationId == core->getDefaultLocationID());
		disconnectEditSignals();
		updateDefaultLocationControls(isDefault);
		ui->pushButtonReturnToDefault->setEnabled(!isDefault);
		ui->useIpQueryCheckBox->setChecked(!state);
		ui->citySearchLineEdit->setFocus();
		connectEditSignals();
	}
}

// Called when the user clicks on the delete button
void LocationDialog::deleteCurrentLocationFromList()
{
	const StelLocation& loc = locationFromFields();
	StelApp::getInstance().getLocationMgr().deleteUserLocation(loc.getID());
}

void LocationDialog::updateDefaultLocationControls(bool currentIsDefault)
{
	ui->useAsDefaultLocationCheckBox->setChecked(currentIsDefault);
	ui->useAsDefaultLocationCheckBox->setEnabled(!currentIsDefault);
}

void LocationDialog::updateTimeZoneControls(bool useCustomTimeZone)
{
	if (useCustomTimeZone)
	{
		ui->useCustomTimeZoneCheckBox->setChecked(true);
		saveTimeZone();
	}
	else
	{
		StelCore* core = StelApp::getInstance().getCore();
		StelLocation loc = core->getCurrentLocation();
		QString tz = loc.ianaTimeZone;
		if (loc.planetName=="Earth" && tz.isEmpty())
			tz = "system_default";
		int idx = ui->timeZoneNameComboBox->findData(tz, Qt::UserRole, Qt::MatchCaseSensitive);
		if (idx==-1)
		{
			QString defTZ = "LMST";
			if (loc.planetName=="Earth")
				defTZ = "system_default";
			// Use LMST/system_default as default
			idx = ui->timeZoneNameComboBox->findData(defTZ, Qt::UserRole, Qt::MatchCaseSensitive);
		}
		ui->timeZoneNameComboBox->setCurrentIndex(idx);
		StelApp::getInstance().getSettings()->remove("localization/time_zone");
		core->setUseCustomTimeZone(false);
	}

	ui->timeZoneNameComboBox->setEnabled(useCustomTimeZone);	
}

// called when the user clicks on the IP Query button
void LocationDialog::ipQueryLocation(bool state)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	if (state)
	{
		conf->setValue("init_location/location", "auto");
		disconnectEditSignals();
		resetCompleteList(); // in case we are on Moon/Mars, we must get list back to show all (earth) locations...
		StelLocationMgr &locMgr=StelApp::getInstance().getLocationMgr();
		locMgr.locationFromIP(); // This just triggers asynchronous lookup.
		ui->useAsDefaultLocationCheckBox->setChecked(!state);
		ui->pushButtonReturnToDefault->setEnabled(!state);
		ui->useCustomTimeZoneCheckBox->setChecked(!state);
		updateTimeZoneControls(!state);
		connectEditSignals();
		ui->citySearchLineEdit->setFocus();
	}
	else
		conf->setValue("init_location/location", StelApp::getInstance().getCore()->getCurrentLocation().getID());
}

#ifdef ENABLE_GPS
// called when the user clicks on the GPS Query button. Use gpsd or Qt's NMEA reader.
void LocationDialog::gpsQueryLocation()
{
	disconnectEditSignals();
	ui->gpsPushButton->setText(q_("GPS..."));

	//only use a single call from a service class here
	StelApp::getInstance().getLocationMgr().locationFromGPS();
}

void LocationDialog::gpsReturn(bool success)
{
	if (success)
	{
		StelCore* core = StelApp::getInstance().getCore();

		ui->gpsPushButton->setText(q_("GPS:SUCCESS"));
		ui->useAsDefaultLocationCheckBox->setChecked(false);
		ui->pushButtonReturnToDefault->setEnabled(true);
		ui->useCustomTimeZoneCheckBox->setChecked(true);
		resetCompleteList(); // in case we come back from Moon/Mars, we must get list back to show all (earth) locations...
		updateTimeZoneControls(true);
		StelLocation loc=core->getCurrentLocation();
		setFieldsFromLocation(loc);
	}
	else
	{
		ui->gpsPushButton->setText(q_("GPS:FAILED"));
	}
	connectEditSignals();
	ui->citySearchLineEdit->setFocus();

	// Use QTimer to reset the labels after 2 seconds.
	QTimer::singleShot(2000, this, SLOT(resetGPSbuttonLabel()));
}

void LocationDialog::resetGPSbuttonLabel()
{
	ui->gpsPushButton->setText(q_("Get location from GPS"));
}
#endif

// called when user clicks "reset list"
void LocationDialog::resetCompleteList()
{
	//reset search before setting model, prevents unnecessary search in full list
	ui->citySearchLineEdit->clear();
	ui->citySearchLineEdit->setFocus();
	proxyModel->setSourceModel(allModel);
	proxyModel->sort(0, Qt::AscendingOrder);
}

// called when user clicks in the country combobox and selects a country. The locations in the list are updated to select only sites in that country.
void LocationDialog::filterSitesByCountry()
{
	QString country=ui->countryNameComboBox->currentData().toString();
	StelLocationMgr &locMgr=StelApp::getInstance().getLocationMgr();

	LocationMap results = locMgr.pickLocationsInCountry(country);
	pickedModel->setStringList(results.keys());
	proxyModel->setSourceModel(pickedModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	ui->citySearchLineEdit->clear();
	ui->citySearchLineEdit->setFocus();
}
