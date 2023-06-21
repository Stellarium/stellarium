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
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"
#include "LocationDialog.hpp"
#include "StelLocationMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "ui_locationDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelSkyCultureMgr.hpp"

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
	, allModel(nullptr)
	, pickedModel(nullptr)
	, proxyModel(nullptr)
#ifdef ENABLE_GPS
	, gpsCount(0)
#endif
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
		populateRegionList(StelApp::getInstance().getCore()->getCurrentLocation().planetName);
		populateTimeZonesList();
		populateTooltips();
	}
}

void LocationDialog::styleChanged(const QString &style)
{
	StelDialog::styleChanged(style);
	// Make the map red if needed
	if (dialog)
		setMapForLocation(StelApp::getInstance().getCore()->getCurrentLocation());
}

// Initialize the dialog widgets and connect the signals/slots
void LocationDialog::createDialogContent()
{
	// We try to directly connect to the observer slots as much as we can
	ui->setupUi(dialog);

	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(app, SIGNAL(flagShowDecimalDegreesChanged(bool)), this, SLOT(setDisplayFormatForSpins(bool)));
	connect(&app->getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(populatePlanetList(QString)));
	// Init the SpinBox entries
	ui->longitudeSpinBox->setPrefixType(AngleSpinBox::Longitude);
	ui->longitudeSpinBox->setMinimum(-180.0, true);
	ui->longitudeSpinBox->setMaximum( 180.0, true);
	ui->longitudeSpinBox->setWrapping(true);
	ui->latitudeSpinBox->setPrefixType(AngleSpinBox::Latitude);
	ui->latitudeSpinBox->setMinimum(-90.0, true);
	ui->latitudeSpinBox->setMaximum( 90.0, true);
	ui->latitudeSpinBox->setWrapping(false);
	setDisplayFormatForSpins(app->getFlagShowDecimalDegrees());

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

	// Kinetic scrolling
	kineticScrollingList << ui->citiesListView;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	populatePlanetList();	
	populateTimeZonesList();

	connect(ui->citySearchLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->citiesListView, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(setLocationFromList(const QModelIndex&)));

	// Connect all the QT signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->mapWidget, SIGNAL(positionChanged(double, double, const QColor&)), this, SLOT(setLocationFromMap(double, double, const QColor&)), Qt::UniqueConnection);

	connect(ui->addLocationToListPushButton, SIGNAL(clicked()), this, SLOT(addCurrentLocationToList()));
	connect(ui->deleteLocationFromListPushButton, SIGNAL(clicked()), this, SLOT(deleteCurrentLocationFromList()));
	connect(ui->resetListPushButton, SIGNAL(clicked()), this, SLOT(resetLocationList()));
#if (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
	connect(ui->regionNameComboBox, SIGNAL(textActivated(const QString &)), this, SLOT(filterSitesByRegion()));
#else
	connect(ui->regionNameComboBox, SIGNAL(activated(const QString &)), this, SLOT(filterSitesByRegion()));
#endif

	StelCore* core = StelApp::getInstance().getCore();
	const StelLocation& currentLocation = core->getCurrentLocation();
	populateRegionList(currentLocation.planetName);
	bool b = (currentLocation.getID() == core->getDefaultLocationID());
	QSettings* conf = StelApp::getInstance().getSettings();
	if (conf->value("init_location/location", "auto").toString() == "auto")
	{
		ui->useIpQueryCheckBox->setChecked(true);
		b = false;
	}
	updateDefaultLocationControls(b);

	setFieldsFromLocation(currentLocation);
	if (currentLocation.ianaTimeZone != core->getCurrentTimeZone())
	{
		setTimezone(core->getCurrentTimeZone()); // also sets string customTimeZone and GUI checkbox
	}
	else
	{
		//ui->timeZoneNameComboBox->setEnabled(false);
		// TODO Maybe also:
		// StelApp::getInstance().getSettings()->remove("localization/time_zone");
	}

#ifdef ENABLE_GPS
#ifdef Q_OS_WIN
	ui->gpsToolButton->setText(q_("Get location from GPS or system service"));
#else
	ui->gpsToolButton->setText(q_("Get location from GPS"));
#endif
	connect(ui->gpsToolButton, SIGNAL(toggled(bool)), this, SLOT(gpsEnableQueryLocation(bool)));
	ui->gpsToolButton->setStyleSheet(QString("QToolButton{ background: gray; }")); // ? Missing default style?
	connect(locMgr, SIGNAL(gpsQueryFinished(bool)), this, SLOT(gpsReturn(bool)));
#else
	ui->gpsToolButton->setEnabled(false);
	ui->gpsToolButton->hide();
#endif
	connect(ui->useIpQueryCheckBox, SIGNAL(clicked(bool)), this, SLOT(ipQueryLocation(bool)));
	connect(ui->useAsDefaultLocationCheckBox, SIGNAL(clicked(bool)), this, SLOT(setDefaultLocation(bool)));
	connect(ui->pushButtonReturnToDefault, SIGNAL(clicked()), this, SLOT(resetLocationList()));
	connectBoolProperty(ui->dstCheckBox, "StelCore.flagUseDST");
	connectBoolProperty(ui->useCustomTimeZoneCheckBox, "StelCore.flagUseCTZ");
	connect(ui->useCustomTimeZoneCheckBox, SIGNAL(toggled(bool)), ui->timeZoneNameComboBox, SLOT(setEnabled(bool)));
	ui->timeZoneNameComboBox->setEnabled(core->getUseCustomTimeZone());
	connect(ui->useCustomTimeZoneCheckBox, SIGNAL(clicked(bool)), this, SLOT(updateTimeZoneControls(bool)));
	connect(core, SIGNAL(currentTimeZoneChanged(QString)), this, SLOT(setTimezone(QString)));
	connectBoolProperty(ui->autoEnableEnvironmentCheckBox, "LandscapeMgr.flagEnvironmentAutoEnabling");
	connectBoolProperty(ui->autoChangeLandscapesCheckBox,  "LandscapeMgr.flagLandscapeAutoSelection");

	connectEditSignals();

	populateTooltips();

	// In case this dialog is called up before going to an "observer", we need to update a few UI elements:
	updateFromProgram(core->getCurrentLocation());
	// This connection only updates the dialog with new coordinates, not the landscape
	connect(core, &StelCore::targetLocationChanged, this, [=](const StelLocation &loc, const QString &id){ updateFromProgram(loc); Q_UNUSED(id); }); // Fill with actually selected location!

	connect(ui->pushButtonReturnToDefault, &QPushButton::clicked, this, [=]{
		static StelMovementMgr *mMgr=GETSTELMODULE(StelMovementMgr);
		static LandscapeMgr *lMgr=GETSTELMODULE(LandscapeMgr);
		mMgr->setFlagTracking(false);      // break orientation lock
		this->setLocationUIvisible(true);  // restore UI
		core->returnToDefaultLocation();
		mMgr->resetInitViewPos();
		lMgr->setCurrentLandscapeID(lMgr->getDefaultLandscapeID(), 1.);
	});

	ui->citySearchLineEdit->setFocus();
}

void LocationDialog::setDisplayFormatForSpins(bool flagDecimalDegrees)
{
	int places = 2;
	AngleSpinBox::DisplayFormat format = AngleSpinBox::DMSSymbols;
	if (flagDecimalDegrees)
	{
		places = 6;
		format = AngleSpinBox::DecimalDeg;
	}
	ui->longitudeSpinBox->setDecimals(places);
	ui->longitudeSpinBox->setDisplayFormat(format);
	ui->latitudeSpinBox->setDecimals(places);
	ui->latitudeSpinBox->setDisplayFormat(format);
}

void LocationDialog::reloadLocations()
{
	allModel->setStringList(StelApp::getInstance().getLocationMgr().getAllMap().keys());
}

void LocationDialog::populateTooltips()
{
	ui->resetListPushButton->setToolTip(q_("Reset location list to show all known locations"));
#ifdef Q_OS_WIN
	ui->gpsToolButton->setToolTip(QString("<p>%1</p>").arg(q_("Toggle fetching location from GPS or system location service. (Does not change time zone!) When satisfied, toggle off to let other programs access the GPS device.")));
#else
	ui->gpsToolButton->setToolTip(QString("<p>%1</p>").arg(q_("Toggle fetching GPS location. (Does not change time zone!) When satisfied, toggle off to let other programs access the GPS device.")));
#endif
}

// Update the widget to make sure it is synchrone if the location is changed programmatically
void LocationDialog::updateFromProgram(const StelLocation& newLocation)
{
	if (!dialog)
		return;

	if (newLocation.name.contains("->")) // avoid extra updates
		return;

	populateRegionList(newLocation.planetName);
	StelCore* stelCore = StelApp::getInstance().getCore();

	isEditingNew = false;

	// Check that the use as default check box needs to be updated
	// Move to setFieldsFromLocation()? --BM?
	const bool b = newLocation.getID() == stelCore->getDefaultLocationID();
	QSettings* conf = StelApp::getInstance().getSettings();
	if (conf->value("init_location/location", "auto").toString() != ("auto"))
	{
		updateDefaultLocationControls(b);
		ui->pushButtonReturnToDefault->setEnabled(!b);
	}

	if (newLocation.role!='o')
		GETSTELMODULE(StelMovementMgr)->setFlagTracking(false);
	setFieldsFromLocation(newLocation);
	setLocationUIvisible(newLocation.role!='o'); // hide various detail settings when changing to an "observer"
}

void LocationDialog::disconnectEditSignals()
{
	disconnect(ui->longitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setLocationFromCoords()));
	disconnect(ui->latitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setLocationFromCoords()));
	disconnect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setLocationFromCoords(int)));
	disconnect(ui->planetNameComboBox, SIGNAL(currentIndexChanged(const int)), this, SLOT(moveToAnotherPlanet()));
	disconnect(ui->regionNameComboBox, SIGNAL(currentIndexChanged(const int)), this, SLOT(reportEdit()));
	disconnect(ui->timeZoneNameComboBox, SIGNAL(currentIndexChanged(const int)), this, SLOT(saveTimeZone()));
	disconnect(ui->cityNameLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(reportEdit()));
}

void LocationDialog::connectEditSignals()
{
	connect(ui->longitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setLocationFromCoords()));
	connect(ui->latitudeSpinBox, SIGNAL(valueChanged()), this, SLOT(setLocationFromCoords()));
	connect(ui->altitudeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setLocationFromCoords(int)));
	connect(ui->planetNameComboBox, SIGNAL(currentIndexChanged(const int)), this, SLOT(moveToAnotherPlanet()));
	connect(ui->regionNameComboBox, SIGNAL(currentIndexChanged(const int)), this, SLOT(reportEdit()));
	connect(ui->timeZoneNameComboBox, SIGNAL(currentIndexChanged(const int)), this, SLOT(saveTimeZone()));
	connect(ui->cityNameLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(reportEdit()));
}

void LocationDialog::setLocationUIvisible(bool visible)
{
	ui->frame_coordinates->setVisible(visible);
	ui->regionNameComboBox->setVisible(visible);
	ui->labelRegion->setVisible(visible);
	ui->cityNameLineEdit->setVisible(visible);
	ui->labelName->setVisible(visible);
	ui->frame_citylist->setVisible(visible);
	ui->mapWidget->setMarkerVisible(visible);
	if(visible)
	{
		connect(ui->mapWidget, SIGNAL(positionChanged(double, double, const QColor&)), this, SLOT(setLocationFromMap(double, double, const QColor&)), Qt::UniqueConnection);
	}
	else
	{
		disconnect(ui->mapWidget, SIGNAL(positionChanged(double, double, const QColor&)), this, SLOT(setLocationFromMap(double, double, const QColor&)));
	}
	ui->addLocationToListPushButton->setVisible(visible);
	ui->deleteLocationFromListPushButton->setVisible(visible);
}

void LocationDialog::setFieldsFromLocation(const StelLocation& loc)
{
	StelCore *core = StelApp::getInstance().getCore();

	// Deactivate edit signals
	disconnectEditSignals();

	// When user has activated a custom timezone, we should not change this!
	if (core->getUseCustomTimeZone())
		customTimeZone=core->getCurrentTimeZone();

	ui->cityNameLineEdit->setText(loc.name);
	ui->longitudeSpinBox->setDegrees(loc.getLongitude(true));
	ui->latitudeSpinBox->setDegrees(loc.getLatitude(true));
	ui->altitudeSpinBox->setValue(loc.altitude);

	int idx = ui->planetNameComboBox->findData(loc.planetName, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx<0)
	{
		// Use Earth as default
		idx = ui->planetNameComboBox->findData(QVariant("Earth"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->planetNameComboBox->setCurrentIndex(idx);

	idx = ui->regionNameComboBox->findData(loc.region, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx<0)
	{
		if (ui->planetNameComboBox->currentData(Qt::UserRole).toString()=="Earth")
		{
			// Use Western Europe as default on Earth
			idx = ui->regionNameComboBox->findData(QVariant("Western Europe"), Qt::UserRole, Qt::MatchCaseSensitive);
		}
		else
			idx = ui->regionNameComboBox->findData(QVariant(""), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->regionNameComboBox->setCurrentIndex(idx);

	QString tz = loc.ianaTimeZone;
	if (loc.planetName=="Earth" && tz.isEmpty())
	{
		qDebug() << "setFieldsFromLocation(): Empty loc.ianaTimeZone!";
		tz = "system_default";
	}
	if (loc.planetName!="Earth") // Check for non-terrestial location...
		tz = "LMST";

	if (core->getUseCustomTimeZone())
		tz=customTimeZone;

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
	core->setCurrentTimeZone(tz);

	setMapForLocation(loc);

	ui->deleteLocationFromListPushButton->setEnabled(StelApp::getInstance().getLocationMgr().canDeleteUserLocation(loc.getID()));

	SolarSystem* ssm = GETSTELMODULE(SolarSystem);
	PlanetP p = ssm->searchByEnglishName(loc.planetName);
	StelModule* ls = StelApp::getInstance().getModule("LandscapeMgr");
	if (ls->property("flagEnvironmentAutoEnable").toBool())
	{
		if (loc.planetName != core->getCurrentLocation().planetName)
		{
			QSettings* conf = StelApp::getInstance().getSettings();
			ls->setProperty("atmosphereDisplayed", p->hasAtmosphere() && conf->value("landscape/flag_atmosphere", true).toBool());
			ls->setProperty("fogDisplayed", p->hasAtmosphere() && conf->value("landscape/flag_fog", true).toBool());
		}
	}

	// Reactivate edit signals
	connectEditSignals();
}

// Update the map for the given location.
void LocationDialog::setMapForLocation(const StelLocation& loc)
{
	// Avoids useless processing
	if (lastPlanet!=loc.planetName)
	{
		QPixmap pixmap;
		// Try to set the proper planet map image
		if (loc.planetName=="Earth")
		{
			// Special case for earth, we don't want to see the clouds
			pixmap = QPixmap(":/graphicGui/miscWorldMap.jpg");
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
		ui->mapWidget->setMap(pixmap);
		// For caching
		lastPlanet = loc.planetName;
	}
	ui->mapWidget->setMarkerPos(loc.getLongitude(), loc.getLatitude());
}

void LocationDialog::populatePlanetList()
{
	Q_ASSERT(ui->planetNameComboBox);
	QComboBox* planetsCombo = ui->planetNameComboBox;
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	QList<PlanetP> ssList = ssystem->getAllPlanets();

	//Save the current selection to be restored later
	planetsCombo->blockSignals(true);
	int index = planetsCombo->currentIndex();
	QVariant selectedPlanetId = planetsCombo->itemData(index);
	planetsCombo->clear();
	//For each planet, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a loop.
	for (const auto& p : ssList)
	{
		planetsCombo->addItem(p->getNameI18n(), p->getEnglishName());
	}
	//Restore the selection
	index = planetsCombo->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	planetsCombo->setCurrentIndex(index);
	planetsCombo->model()->sort(0);
	planetsCombo->blockSignals(false);
}

void LocationDialog::populateRegionList(const QString& planet)
{
	Q_ASSERT(ui->regionNameComboBox);

	QComboBox* regionCombo = ui->regionNameComboBox;
	QStringList regionNames(StelApp::getInstance().getLocationMgr().getRegionNames(planet));

	//Save the current selection to be restored later
	regionCombo->blockSignals(true);
	int index = regionCombo->currentIndex();
	QVariant selectedRegionId = regionCombo->itemData(index);
	regionCombo->clear();
	//For each region, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a loop.
	for (const auto& name : qAsConst(regionNames))
		regionCombo->addItem(q_(name), name);

	regionCombo->addItem(QChar(0x2014), "");
	//Restore the selection
	index = regionCombo->findData(selectedRegionId, Qt::UserRole, Qt::MatchCaseSensitive);
	regionCombo->setCurrentIndex(index);
	regionCombo->model()->sort(0);
	regionCombo->blockSignals(false);
}

void LocationDialog::populateTimeZonesList()
{
	Q_ASSERT(ui->timeZoneNameComboBox);

	QComboBox* tzCombo = ui->timeZoneNameComboBox;
	// Return a list of all the known time zone names (from Qt)
	QStringList tzNames;
	auto tzList = QTimeZone::availableTimeZoneIds(); // System dependent set of IANA timezone names.
	for (const auto& tz : qAsConst(tzList))
	{
		tzNames.append(tz);
		// Activate this to get a list of known TZ names...
		//qDebug() << "Qt/IANA TZ entry from QTimeZone::available: " << tz;
	}
	tzNames.sort();

	// Special sort for UTC+XX:YY time zones: we want them
	// to go from the most negative to the most positive.
	static const QRegularExpression utcRegEx("^UTC([+-])(\\d\\d):(\\d\\d)$");
	std::vector<int> utcOffsets;
	for(int n = 0; n < tzNames.size();)
	{
		const auto match = utcRegEx.match(tzNames[n]);
		if(match.lastCapturedIndex() != 3)
		{
			++n;
			continue;
		}
		tzNames.removeAt(n);
		const int sign = match.captured(1)=="-" ? -1 : 1;
		const int hour      = match.captured(2).toInt();
		const int minute    = match.captured(3).toInt();
		// Encode with a shift by ±1 to distinguish UTC±00:00
		utcOffsets.push_back(sign * (100 * hour + minute + 1));
	}
	std::sort(utcOffsets.begin(), utcOffsets.end());
	for(const auto offset : utcOffsets)
	{
		const auto hm = std::abs(offset) - 1;
		const auto hour   = hm / 100;
		const auto minute = hm % 100;
		tzNames.push_back(QString("UTC%1%2:%3").arg(offset > 0 ? "+" : "-")
											   .arg(hour  , 2, 10, QChar('0'))
											   .arg(minute, 2, 10, QChar('0')));
	}

	if(!utcOffsets.empty())
	{
		// If we have UTC offsets, the Etc/GMT±x time zones are not useful. In
		// addition, they'd also need re-sorting, so let's just remove them.
		for(int n = 0; n < tzNames.size();)
		{
			if(tzNames[n].startsWith("Etc/GMT"))
				tzNames.removeAt(n);
			else
				++n;
		}
	}

	//Save the current selection to be restored later
	tzCombo->blockSignals(true);
	int index = tzCombo->currentIndex();
	QVariant selectedTzId = tzCombo->itemData(index);
	tzCombo->clear();
	//For each time zone, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a loop.
	for (const auto& name : tzNames)
	{
		tzCombo->addItem(name, name);
	}
	tzCombo->addItem(q_("Local Mean Solar Time"), "LMST");
	tzCombo->addItem(q_("Local True Solar Time"), "LTST");
	tzCombo->addItem(q_("System default"), "system_default");
	//Restore the selection
	index = tzCombo->findData(selectedTzId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index==-1) // the TZ is not found
		index=tzCombo->count()-1; // should point to system_default.

	Q_ASSERT(index!=-1);
	tzCombo->setCurrentIndex(index);
	tzCombo->blockSignals(false);
}

// Create a StelLocation instance from the fields
StelLocation LocationDialog::locationFromFields() const
{
	StelLocation loc;
	int index = ui->planetNameComboBox->currentIndex();
	if (index < 0)
	{
		qWarning() << "LocationDialog::locationFromFields(): no valid planet name from combo?";
		loc.planetName = QString("Earth"); //As returned by QComboBox::currentText()
	}
	else
		loc.planetName = ui->planetNameComboBox->itemData(index).toString();
	loc.name = ui->cityNameLineEdit->text().trimmed(); // avoid locations with leading whitespace
	loc.setLatitude(qBound(-90.0, ui->latitudeSpinBox->valueDegrees(), 90.0));
	loc.setLongitude(ui->longitudeSpinBox->valueDegrees());
	loc.altitude = ui->altitudeSpinBox->value();
	index = ui->regionNameComboBox->currentIndex();
	if (index < 0)
	{
		qWarning() << "LocationDialog::locationFromFields(): no valid region name from combo?";
		loc.region = QString();//As returned by QComboBox::currentText()
	}
	else
		loc.region = ui->regionNameComboBox->itemData(index).toString();

	index = ui->timeZoneNameComboBox->currentIndex();
	if (index < 0)
	{
		qWarning() << "LocationDialog::locationFromFields(): no valid timezone name from combo?";
		loc.ianaTimeZone = QString("UTC"); //Give at least some useful default
	}
	else
	{
		QString tz=ui->timeZoneNameComboBox->itemData(index).toString();
		loc.ianaTimeZone = tz;
	}
	return loc;
}

void LocationDialog::setLocationFromList(const QModelIndex& index)
{
	isEditingNew=false;
	ui->addLocationToListPushButton->setEnabled(false);
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForString(index.data().toString());
	setFieldsFromLocation(loc);
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.);
	// This calls indirectly updateFromProgram()
}

void LocationDialog::setLocationFromMap(double longitude, double latitude, const QColor &color)
{
	StelCore *core = StelApp::getInstance().getCore();
	if (core->getUseCustomTimeZone())
		customTimeZone=core->getCurrentTimeZone();
	reportEdit();
	StelLocation loc = locationFromFields();
	loc.setLatitude(latitude);
	loc.setLongitude(longitude);
	loc.name= QString("%1, %2").arg(loc.getLatitude()).arg(loc.getLongitude()); // Force a preliminary name
	setFieldsFromLocation(loc);
	if (loc.planetName=="Earth")
		core->moveObserverTo(loc, 0., 1., GETSTELMODULE(LandscapeMgr)->getFlagLandscapeAutoSelection() ? QString("ZeroColor(%1)").arg(Vec3f(color).toStr()) : QString());
	else
		core->moveObserverTo(loc, 0., 1., loc.planetName); // Loads a default landscape for the planet.

	// Only for locations on Earth: set zone to LMST.
	// TODO: Find a way to lookup (lon,lat)->country->timezone.

	if (core->getUseCustomTimeZone())
		ui->timeZoneNameComboBox->setCurrentIndex(ui->timeZoneNameComboBox->findData(customTimeZone, Qt::UserRole, Qt::MatchCaseSensitive));
	else
		ui->timeZoneNameComboBox->setCurrentIndex(ui->timeZoneNameComboBox->findData("LMST", Qt::UserRole, Qt::MatchCaseSensitive));

	// Filter location list for nearby sites. I assume Earth locations are better known. With only few locations on other planets in the list, 30 degrees seem OK.
	LocationMap results = StelApp::getInstance().getLocationMgr().pickLocationsNearby(loc.planetName, longitude, latitude, loc.planetName=="Earth" ? 5.0f: 30.0f);
	pickedModel->setStringList(results.keys());
	proxyModel->setSourceModel(pickedModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	ui->citySearchLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
}

// Called when the planet name is changed by hand
void LocationDialog::moveToAnotherPlanet()
{
	reportEdit();
	StelLocation loc = locationFromFields();
	StelCore* stelCore = StelApp::getInstance().getCore();
	LandscapeMgr *lMgr = GETSTELMODULE(LandscapeMgr);
	populateRegionList(loc.planetName);
	if (loc.planetName != stelCore->getCurrentLocation().planetName)
	{
		setFieldsFromLocation(loc);
		if (lMgr->getFlagLandscapeAutoSelection())
		{
			// If we have a landscape for selected planet then set it, otherwise use zero landscape
			// Details: https://bugs.launchpad.net/stellarium/+bug/1173254
			if (lMgr->getAllLandscapeNames().contains(loc.planetName))
				lMgr->setCurrentLandscapeName(loc.planetName); // TODO: Why not setCurrentLandscapeID?
			else
				//lMgr->setProperty("currentLandscapeID", lMgr->property("defaultLandscapeID"));
				lMgr->setCurrentLandscapeID("zero"); // harmonize with LandscapeMgr::onTargetLocationChanged()
		}

		// populate site list with sites only from that planet, or full list for Earth (faster than removing the ~50 non-Earth positions...).
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
			ui->regionNameComboBox->setCurrentIndex(ui->regionNameComboBox->findData("", Qt::UserRole, Qt::MatchCaseSensitive));
			// For 0.19, time zone should not change. When we can work out LMST for other planets, we can accept LMST.
			//if (customTimeZone.isEmpty())  // This is always true!
			//	ui->timeZoneNameComboBox->setCurrentIndex(ui->timeZoneNameComboBox->findData("LMST", Qt::UserRole, Qt::MatchCaseSensitive));
		}
		proxyModel->sort(0, Qt::AscendingOrder);
		ui->citySearchLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
		ui->citySearchLineEdit->setFocus();

		// If we change to an Observer "planet", auto-select and focus on the observed object.
		StelObjectMgr *soMgr=GETSTELMODULE(StelObjectMgr);
		SolarSystem *ss=GETSTELMODULE(SolarSystem);
		PlanetP planet=nullptr;
		if (ss)
			planet=GETSTELMODULE(SolarSystem)->searchByEnglishName(loc.planetName);
		if (planet && planet->getPlanetType()==Planet::isObserver)
		{
			setLocationUIvisible(false);

			loc.role=QChar('o'); // Mark this ad-hoc location as "observer".
			if (soMgr)
			{
				soMgr->findAndSelect(planet->getParent()->getEnglishName());
				GETSTELMODULE(StelMovementMgr)->setFlagTracking(true);
			}
		}
		else
		{
			GETSTELMODULE(StelMovementMgr)->setFlagTracking(false);
			setLocationUIvisible(true);
			GETSTELMODULE(StelMovementMgr)->resetInitViewPos();
			// Get rid of selection if we are standing on it...
			if (soMgr)
			{
				const QList<StelObjectP>& selection=soMgr->getSelectedObject();
				if (!selection.isEmpty() && selection[0]->getEnglishName()==loc.planetName)
						soMgr->unSelect();
			}
		}

		stelCore->moveObserverTo(loc, 0., 0., loc.planetName);
	}
	// Planet transition time also set to null to prevent ugliness when
	// "use landscape location" is enabled for that planet's landscape. --BM
	// NOTE: I think it also makes sense in the other cases. --BM
}

void LocationDialog::setLocationFromCoords(int i)
{
	Q_UNUSED(i)
	reportEdit();
	StelLocation loc = locationFromFields();
	StelApp::getInstance().getCore()->moveObserverTo(loc, 0.); // No landscape update: This may be used for finetuning within a landscape
	//Update the position of the map pointer
	ui->mapWidget->setMarkerPos(loc.getLongitude(), loc.getLatitude());
}

void LocationDialog::saveTimeZone()
{
	QString tz = ui->timeZoneNameComboBox->itemData(ui->timeZoneNameComboBox->currentIndex()).toString();
	StelCore* core = StelApp::getInstance().getCore();
	core->setCurrentTimeZone(tz);
	if (core->getUseCustomTimeZone())
	{
		StelApp::getInstance().getSettings()->setValue("localization/time_zone", tz);
	}
}

void LocationDialog::setTimezone(QString tz)
{
	disconnectEditSignals();

	int idx=ui->timeZoneNameComboBox->findData(tz, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx>=0)
	{
		ui->timeZoneNameComboBox->setCurrentIndex(idx);
	}
	else
	{
		qWarning() << "LocationDialog::setTimezone(): invalid name:" << tz;
	}

	connectEditSignals();
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
}

// Called when the user clicks on the save button
void LocationDialog::addCurrentLocationToList()
{
	const StelLocation& loc = locationFromFields();
	ui->citySearchLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
	StelApp::getInstance().getLocationMgr().saveUserLocation(loc);
	isEditingNew=false;
	ui->addLocationToListPushButton->setEnabled(false);

	const QAbstractItemModel* model = ui->citiesListView->model();
	const QString id = loc.getID();
	for (int i=0;i<model->rowCount();++i)
	{
		if (model->index(i,0).data()==id)
		{
			ui->citiesListView->selectionModel()->select(model->index(i,0), QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
			ui->citiesListView->scrollTo(model->index(i,0));
			setLocationFromList(model->index(i,0));
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
	StelCore* core = StelApp::getInstance().getCore();
	core->setUseCustomTimeZone(useCustomTimeZone);

	if (useCustomTimeZone)
	{
		saveTimeZone();
	}
	else
	{
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
}

// called when the user clicks on the IP Query button
void LocationDialog::ipQueryLocation(bool state)
{
	disconnectEditSignals();
	resetLocationList(); // in case we are on Moon/Mars, we must get list back to show all (earth) locations...
	StelLocationMgr &locMgr=StelApp::getInstance().getLocationMgr();
	locMgr.locationFromIP(); // This just triggers asynchronous lookup.
	// NOTE: These steps seem to assume IP lookup is successful!
	ui->useAsDefaultLocationCheckBox->setChecked(!state);
	ui->pushButtonReturnToDefault->setEnabled(!state);
	updateTimeZoneControls(!state);
	connectEditSignals();
	ui->citySearchLineEdit->setFocus();
	QSettings* conf = StelApp::getInstance().getSettings();
	if (state)
		conf->setValue("init_location/location", "auto");
	else
		conf->setValue("init_location/location", StelApp::getInstance().getCore()->getCurrentLocation().getID());
}

#ifdef ENABLE_GPS
// called when the user toggles the GPS Query button. Use gpsd or Qt's NMEA reader.
void LocationDialog::gpsEnableQueryLocation(bool running)
{
	if (running)
	{
		disconnectEditSignals();
		gpsCount=0;
		ui->gpsToolButton->setText(q_("GPS listening..."));
		StelApp::getInstance().getLocationMgr().locationFromGPS(3000);
	}
	else
	{
		// (edit signals restored by gpsReturn())
		StelApp::getInstance().getLocationMgr().locationFromGPS(0);
		ui->gpsToolButton->setText(q_("GPS disconnecting..."));
		QTimer::singleShot(1500, this, SLOT(resetGPSbuttonLabel()));
	}
}

void LocationDialog::gpsReturn(bool success)
{
	if (success)
	{
		StelCore* core = StelApp::getInstance().getCore();

		gpsCount++;
		ui->useAsDefaultLocationCheckBox->setChecked(false);
		ui->pushButtonReturnToDefault->setEnabled(true);
		//ui->useCustomTimeZoneCheckBox->setChecked(true); // done by updateTimeZoneControls(true) below.
		ui->useIpQueryCheckBox->setChecked(false); // Disable IP query option when GPS is used!
		resetLocationList(); // in case we come back from Moon/Mars, we must get list back to show all (earth) locations...
		updateTimeZoneControls(true);
		StelLocation loc=core->getCurrentLocation();
		setFieldsFromLocation(loc);
		if (loc.altitude==0) // give feedback of fix quality.
		{
			ui->gpsToolButton->setText(QString("%1 %2").arg(q_("2D location fix")).arg(gpsCount));
			ui->gpsToolButton->setStyleSheet(QString("QToolButton{ background: yellow; }"));
		}
		else
		{
			ui->gpsToolButton->setText(QString("%1 %2").arg(q_("3D location fix")).arg(gpsCount));
			ui->gpsToolButton->setStyleSheet(QString("QToolButton{ background: green; }"));
		}
		QSettings* conf = StelApp::getInstance().getSettings();
		conf->setValue("init_location/location", loc.getID());
		conf->setValue("init_location/last_location", QString("%1, %2, %3").arg(loc.getLatitude()).arg(loc.getLongitude()).arg(loc.altitude));
	}
	else
	{
		ui->gpsToolButton->setText(q_("GPS:FAILED"));
		ui->gpsToolButton->setStyleSheet(QString("QToolButton{ background: red; }"));
		// Use QTimer to reset the labels after 2 seconds.
		QTimer::singleShot(2000, this, SLOT(resetGPSbuttonLabel()));
	}
	connectEditSignals();
	ui->citySearchLineEdit->setFocus();
}

void LocationDialog::resetGPSbuttonLabel()
{
	ui->gpsToolButton->setChecked(false);
#ifdef Q_OS_WIN
	ui->gpsToolButton->setText(q_("Get location from GPS or system service"));
#else
	ui->gpsToolButton->setText(q_("Get location from GPS"));
#endif
	ui->gpsToolButton->setStyleSheet(QString("QToolButton{ background: gray; }"));
}
#endif

// called when user clicks "reset list"
void LocationDialog::resetLocationList()
{
	//reset search before setting model, prevents unnecessary search in full list
	ui->citySearchLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
	ui->citySearchLineEdit->setFocus();

	// populate site list with sites only from that planet, or full list for Earth (faster than removing the ~50 non-Earth positions...).
	StelLocationMgr &locMgr=StelApp::getInstance().getLocationMgr();
	StelLocation loc = locationFromFields();
	if (loc.planetName == "Earth")
	{
		proxyModel->setSourceModel(allModel);
	}
	else
	{
		LocationMap results = locMgr.pickLocationsNearby(loc.planetName, 0.0f, 0.0f, 180.0f);
		pickedModel->setStringList(results.keys());
		proxyModel->setSourceModel(pickedModel);
		ui->regionNameComboBox->setCurrentIndex(ui->regionNameComboBox->findData("", Qt::UserRole, Qt::MatchCaseSensitive));
	}

	proxyModel->sort(0, Qt::AscendingOrder);
}

// called when user clicks in the region combobox and selects a region. The locations in the list are updated to select only sites in that region.
void LocationDialog::filterSitesByRegion()
{
	QString region=ui->regionNameComboBox->currentData(Qt::UserRole).toString();
	StelLocationMgr &locMgr=StelApp::getInstance().getLocationMgr();

	LocationMap results = locMgr.pickLocationsInRegion(region);
	pickedModel->setStringList(results.keys());
	proxyModel->setSourceModel(pickedModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	ui->citySearchLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
	ui->citySearchLineEdit->setFocus();
}
