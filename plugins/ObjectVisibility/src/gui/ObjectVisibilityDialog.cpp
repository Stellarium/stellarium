/*
 * Object Visibility plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
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

#include "ObjectVisibilityDialog.hpp"
#include "ObjectVisibility.hpp"
#include "ObjectVisibilityMapWidget.hpp"
#include "ui_objectVisibilityDialog.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelLocation.hpp"
#include "StelLocationMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "modules/Planet.hpp"
#include "modules/SolarSystem.hpp"

#include <QDebug>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <cmath>

ObjectVisibilityDialog::ObjectVisibilityDialog()
	: StelDialog("ObjectVisibility")
	, ui(new Ui_objectVisibilityDialog())
	, plugin(nullptr)
{
}

ObjectVisibilityDialog::~ObjectVisibilityDialog()
{
	delete ui;
	ui = nullptr;
}

void ObjectVisibilityDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		configurePlaceLabelControls();
		setAboutHtml();
		refreshTitleLabel();
		refreshTwilightLimits();
		updatePlaceLabels();
	}
}

//
// =================== Construction ====================
//

void ObjectVisibilityDialog::createDialogContent()
{
	plugin = GETSTELMODULE(ObjectVisibility);
	Q_ASSERT(plugin);
	ui->setupUi(dialog);

	// Standard kinetic scrolling for the About browser.
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui = static_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		// enableKineticScrolling is protected in StelDialog, so we
		// can't take its address from outside the class hierarchy.
		// Calling it from inside a lambda works because the lambda
		// is a member-function context with access to inherited
		// protected members.
		connect(gui, &StelGui::flagUseKineticScrollingChanged,
		        this, [this](bool b){ enableKineticScrolling(b); });
	}

	// Localisation + close button.
	connect(&StelApp::getInstance(), &StelApp::languageChanged,
	        this, &ObjectVisibilityDialog::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo,
	        this, &StelDialog::handleMovedTo);

	// "Good visibility" spinbox.
	ui->goodVisibilityLimitSpinBox->setRange(1, 89);
	ui->goodVisibilityLimitSpinBox->setSingleStep(1);
	ui->goodVisibilityLimitSpinBox->setValue(plugin->getGoodVisibilityLimit());
	connect(ui->goodVisibilityLimitSpinBox,
	        QOverload<int>::of(&QSpinBox::valueChanged),
	        this, &ObjectVisibilityDialog::onGoodVisibilityLimitChanged);
	// Push the same value back into the map widget at startup so the
	// dashed line uses the correct altitude limit straight away.
	ui->mapWidget->setGoodVisibilityAltitude(plugin->getGoodVisibilityLimit());
	ui->twilightMapWidget->setOverlayMode(ObjectVisibilityMapWidget::TwilightLimitsOverlay);
	ui->twilightMapWidget->setMarkerVisible(true);
	ui->twilightMapWidget->setMap(QPixmap(":/graphicGui/miscWorldMap.jpg"));
	configurePlaceLabelControls();

	// Buttons.
	connect(ui->calculatePushButton, &QPushButton::clicked,
	        this, &ObjectVisibilityDialog::calculate);
	connect(ui->setLocationByClickCheckBox, &QCheckBox::toggled,
	        this, &ObjectVisibilityDialog::onSetLocationByClickToggled);
	connect(ui->twilightSetLocationByClickCheckBox, &QCheckBox::toggled,
	        this, &ObjectVisibilityDialog::onTwilightSetLocationByClickToggled);
	connect(ui->resetSettingsPushButton, &QPushButton::clicked,
	        this, &ObjectVisibilityDialog::onResetSettings);
	connect(ui->placeLabelsCheckBox, &QCheckBox::toggled,
	        this, &ObjectVisibilityDialog::onPlaceLabelsToggled);
	connect(ui->twilightPlaceLabelsCheckBox, &QCheckBox::toggled,
	        this, &ObjectVisibilityDialog::onTwilightPlaceLabelsToggled);
	connect(ui->placeLabelsPopulationComboBox,
	        QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &ObjectVisibilityDialog::onPlaceLabelsPopulationChanged);
	connect(ui->twilightPlaceLabelsPopulationComboBox,
	        QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &ObjectVisibilityDialog::onTwilightPlaceLabelsPopulationChanged);
	connect(ui->placeLabelsNearLinesOnlyCheckBox, &QCheckBox::toggled,
	        this, &ObjectVisibilityDialog::onPlaceLabelsNearLinesOnlyToggled);
	connect(ui->twilightPlaceLabelsNearLinesOnlyCheckBox, &QCheckBox::toggled,
	        this, &ObjectVisibilityDialog::onTwilightPlaceLabelsNearLinesOnlyToggled);

	// Map clicks while in "set location" mode.
	connect(ui->mapWidget, &ObjectVisibilityMapWidget::locationPicked,
	        this, &ObjectVisibilityDialog::onLocationPicked);
	connect(ui->twilightMapWidget, &ObjectVisibilityMapWidget::locationPicked,
	        this, &ObjectVisibilityDialog::onLocationPicked);

	// Track Stellarium's selection so we can enable/disable Calculate
	// and update the prompt label.  The signal carries a
	// StelModuleSelectAction enum we don't care about — drop it with
	// a lambda so the slot can stay parameter-free.
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	if (objMgr)
	{
		connect(objMgr, &StelObjectMgr::selectedObjectChanged,
		        this, [this](StelModule::StelModuleSelectAction)
		        { onSelectedObjectChanged(); });
	}

	// Keep the marker on the map in sync with the observer's current
	// position — whether the user moved it from our dialog, from the
	// Location dialog, or from any other source.
	StelCore* core = StelApp::getInstance().getCore();
	connect(core, &StelCore::locationChanged,
	        this, &ObjectVisibilityDialog::syncMarkerToObserver);
	connect(core, &StelCore::dateChanged,
	        this, &ObjectVisibilityDialog::refreshTwilightLimits);
	syncMarkerToObserver();

	setAboutHtml();
	refreshTitleLabel();
	refreshTwilightLimits();
	updatePlaceLabels();
	updateCalculateButtonEnabled();
}

//
// =================== Actions ====================
//

bool ObjectVisibilityDialog::isAcceptableType(const QString& type)
{
	// "Star" covers normal stars (StarWrapper) and "Nebula" covers all
	// DSOs in Stellarium's nomenclature.  We deliberately exclude
	// planets, moons, comets, asteroids, satellites, etc.
	return (type == QStringLiteral("Star")) ||
	       (type == QStringLiteral("Nebula"));
}

void ObjectVisibilityDialog::onSelectedObjectChanged()
{
	updateCalculateButtonEnabled();
	// In the pre-Calculate state, the label depends on what's
	// selected — refresh it so the user gets immediate feedback
	// about whether the new selection is acceptable.  Once
	// Calculate has been pressed, the label shows the result
	// and is unaffected by subsequent selection changes.
	refreshTitleLabel();
}

void ObjectVisibilityDialog::updateCalculateButtonEnabled()
{
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	bool typeOk = false;
	if (objMgr && !objMgr->getSelectedObject().isEmpty())
	{
		const StelObjectP sel = objMgr->getSelectedObject().first();
		typeOk = isAcceptableType(sel->getType());
	}

	StelCore* core = StelApp::getInstance().getCore();
	const QString planet = core->getCurrentLocation().planetName;
	const bool planetOk = isSupportedPlanet(planet);

	const bool acceptable = typeOk && planetOk;
	ui->calculatePushButton->setEnabled(acceptable);

	QString tip;
	if (!typeOk)
		tip = q_("Only stars and DSOs may be used in this plugin!");
	else if (!planetOk)
		tip = q_("This plug-in supports observation from Earth, the "
		         "Moon, the eight planets, Pluto and the four Galilean "
		         "moons.  The current observing body (%1) is not "
		         "supported.").arg(planet);
	else
		tip = q_("Compute visibility for the selected object.");
	ui->calculatePushButton->setToolTip(tip);
}

void ObjectVisibilityDialog::calculate()
{
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	if (!objMgr || objMgr->getSelectedObject().isEmpty())
		return;

	const StelObjectP sel = objMgr->getSelectedObject().first();
	if (!isAcceptableType(sel->getType()))
		return;

	// Remember which object we computed for, so the title label can
	// be refreshed on retranslate without re-running the geometry.
	// We use getID() because it is the only identifier guaranteed to
	// be unique within a type (e.g. "HIP 32349" for Sirius).
	lockedObjectId          = sel->getID();
	lockedObjectType        = sel->getType();
	lockedObjectNameI18n    = sel->getNameI18n();
	if (lockedObjectNameI18n.isEmpty())
		lockedObjectNameI18n = sel->getEnglishName();
	if (lockedObjectNameI18n.isEmpty())
		lockedObjectNameI18n = lockedObjectId;

	StelCore* core = StelApp::getInstance().getCore();
	// getEquinoxEquatorialPos() returns the equatorial coordinates at
	// the *current* equinox (date), which is exactly what the spec requires:
	// precession and (where applicable) proper motion are folded in,
	// so plotting Sirius at 10000 BCE uses Sirius's position at that
	// time, not its J2000 position.
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, sel->getEquinoxEquatorialPos(core));
	const double decDeg = dec * 180.0 / M_PI;

	ui->mapWidget->setDeclination(decDeg);
	refreshTitleLabel();
}

void ObjectVisibilityDialog::onGoodVisibilityLimitChanged(int degrees)
{
	plugin->setGoodVisibilityLimit(degrees);
	ui->mapWidget->setGoodVisibilityAltitude(degrees);
}

void ObjectVisibilityDialog::onSetLocationByClickToggled(bool on)
{
	ui->mapWidget->setClickSetsLocationMode(on);

	if (ui->twilightSetLocationByClickCheckBox->isEnabled())
	{
		QSignalBlocker blocker(ui->twilightSetLocationByClickCheckBox);
		ui->twilightSetLocationByClickCheckBox->setChecked(on);
		ui->twilightMapWidget->setClickSetsLocationMode(on);
	}
}

void ObjectVisibilityDialog::onTwilightSetLocationByClickToggled(bool on)
{
	StelCore* core = StelApp::getInstance().getCore();
	const bool earth = core &&
	                   core->getCurrentLocation().planetName == QStringLiteral("Earth");

	if (on && !earth)
	{
		QSignalBlocker blocker(ui->twilightSetLocationByClickCheckBox);
		ui->twilightSetLocationByClickCheckBox->setChecked(false);
		ui->twilightMapWidget->setClickSetsLocationMode(false);
		return;
	}

	ui->twilightMapWidget->setClickSetsLocationMode(on);

	{
		QSignalBlocker blocker(ui->setLocationByClickCheckBox);
		ui->setLocationByClickCheckBox->setChecked(on);
	}
	ui->mapWidget->setClickSetsLocationMode(on);
}

void ObjectVisibilityDialog::onLocationPicked(double longitude,
                                              double latitude,
                                              const QColor &color)
{
	Q_UNUSED(color);
	// Note: we deliberately do NOT exit click-to-set mode here.
	// The user explicitly enabled the checkbox so they can pick as
	// many locations as they like; they uncheck when they're done.

	// Move the observer to the picked spot.  We keep the rest of the
	// current location (planet, altitude, name) untouched.
	StelCore* core = StelApp::getInstance().getCore();
	StelLocation loc = core->getCurrentLocation();
	loc.setLatitude(static_cast<float>(latitude));
	loc.setLongitude(static_cast<float>(longitude));
	loc.name = QString("%1, %2")
	           .arg(loc.getLatitude())
	           .arg(loc.getLongitude());
	// Quick move (no animation) so it feels responsive.
	core->moveObserverTo(loc, 0.0, 0.0);

	// Move the on-map marker immediately, without waiting for
	// StelCore::locationChanged to arrive — that signal is emitted
	// after the (async) move completes, which would feel laggy.
	ui->mapWidget->setMarkerPos(longitude, latitude);
	ui->twilightMapWidget->setMarkerPos(longitude, latitude);
}

void ObjectVisibilityDialog::onResetSettings()
{
	if (!askConfirmation())
		return;
	plugin->restoreDefaultSettings();
	// Refresh the visible controls so they match what we just wrote.
	const int g = plugin->getGoodVisibilityLimit();
	ui->goodVisibilityLimitSpinBox->setValue(g);
	ui->mapWidget->setGoodVisibilityAltitude(g);
}

void ObjectVisibilityDialog::onPlaceLabelsToggled(bool on)
{
	placeLabelsVisible = on;
	syncPlaceLabelControls();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::onTwilightPlaceLabelsToggled(bool on)
{
	placeLabelsVisible = on;
	syncPlaceLabelControls();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::onPlaceLabelsPopulationChanged(int index)
{
	const QVariant value = ui->placeLabelsPopulationComboBox->itemData(index);
	if (!value.isValid()) return;
	placeLabelsMinimumPopulation = value.toInt();
	syncPlaceLabelControls();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::onTwilightPlaceLabelsPopulationChanged(int index)
{
	const QVariant value = ui->twilightPlaceLabelsPopulationComboBox->itemData(index);
	if (!value.isValid()) return;
	placeLabelsMinimumPopulation = value.toInt();
	syncPlaceLabelControls();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::onPlaceLabelsNearLinesOnlyToggled(bool on)
{
	placeLabelsNearLinesOnly = on;
	syncPlaceLabelControls();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::onTwilightPlaceLabelsNearLinesOnlyToggled(bool on)
{
	placeLabelsNearLinesOnly = on;
	syncPlaceLabelControls();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::syncMarkerToObserver()
{
	if (!ui || !ui->mapWidget) return;
	const StelLocation& loc =
	    StelApp::getInstance().getCore()->getCurrentLocation();

	// (1) If the user has moved to a different planet since we last
	// loaded a texture, swap the map texture.  We mirror LocationDialog's
	// approach: Earth uses miscWorldMap.jpg (no clouds); other bodies
	// use Stellarium's stored planet texture identified by their
	// englishName.
	if (loc.planetName != cachedPlanetName)
	{
		QPixmap pixmap;
		if (loc.planetName == QStringLiteral("Earth"))
		{
			pixmap = QPixmap(":/graphicGui/miscWorldMap.jpg");
		}
		else
		{
			SolarSystem* ssm = GETSTELMODULE(SolarSystem);
			PlanetP p = ssm ? ssm->searchByEnglishName(loc.planetName) : PlanetP();
			if (p)
			{
				QString path = StelFileMgr::findFile(
					"textures/" + p->getTextMapName());
				if (!path.isEmpty())
					pixmap = QPixmap(path);
				else
					qWarning() << "[ObjectVisibility] "
					              "no texture for planet"
					           << loc.planetName;
			}
		}
		if (!pixmap.isNull())
			ui->mapWidget->setMap(pixmap);
		cachedPlanetName = loc.planetName;
	}

	// (2) Update the location marker.  getLatitude(true)/
	// getLongitude(true) returns the *configured* position even when
	// the observer is on a planet other than Earth, which is exactly
	// what we want here.
	const float lat = loc.getLatitude(true);
	const float lon = loc.getLongitude(true);
	ui->mapWidget->setMarkerPos(static_cast<double>(lon),
	                            static_cast<double>(lat));
	ui->twilightMapWidget->setMarkerPos(static_cast<double>(lon),
	                                    static_cast<double>(lat));

	// (3) Refresh the Calculate-enabled state and title label, since
	// both depend on whether the current planet is supported.
	updateCalculateButtonEnabled();
	refreshTitleLabel();
	refreshTwilightLimits();
	updatePlaceLabels();
}

void ObjectVisibilityDialog::refreshTwilightLimits()
{
	if (!ui || !ui->twilightMapWidget) return;

	StelCore* core = StelApp::getInstance().getCore();
	if (!core) return;

	const QString planet = core->getCurrentLocation().planetName;
	if (planet != QStringLiteral("Earth"))
	{
		ui->twilightMapWidget->clearTwilightLimits();
		ui->twilightMapWidget->setMarkerVisible(false);
		ui->twilightMapWidget->setClickSetsLocationMode(false);
		ui->twilightSetLocationByClickCheckBox->setEnabled(false);
		ui->twilightSetLocationByClickCheckBox->setToolTip(
			q_("Setting the observer location from the twilight map is "
			   "available on Earth only."));
		{
			QSignalBlocker blocker(ui->twilightSetLocationByClickCheckBox);
			ui->twilightSetLocationByClickCheckBox->setChecked(false);
		}
		ui->twilightTitleLabel->setText(
			QString(q_("Solstice twilight limits are available for Earth only. "
			           "Current observing body: %1"))
			.arg(q_(planet.toUtf8().constData())));
		return;
	}

	SolarSystem* ssm = GETSTELMODULE(SolarSystem);
	PlanetP earth = ssm ? ssm->getEarth() : PlanetP();
	if (!earth)
	{
		ui->twilightMapWidget->clearTwilightLimits();
		ui->twilightMapWidget->setMarkerVisible(false);
		ui->twilightMapWidget->setClickSetsLocationMode(false);
		ui->twilightSetLocationByClickCheckBox->setEnabled(false);
		ui->twilightTitleLabel->setText(
			q_("Solstice twilight limits on Earth are unavailable."));
		return;
	}

	ui->twilightMapWidget->setMarkerVisible(true);
	ui->twilightSetLocationByClickCheckBox->setEnabled(true);
	ui->twilightSetLocationByClickCheckBox->setToolTip(
		q_("Enable to move the observer by clicking on the map. "
		   "Stays active until you uncheck it."));

	const double obliquityDeg = earth->getRotObliquity(core->getJDE()) * 180.0 / M_PI;
	ui->twilightMapWidget->setTwilightObliquity(obliquityDeg);

	const int year = currentYear(core);
	ui->twilightTitleLabel->setText(
		QString(q_("Solstice twilight limits on Earth in %1 "
		           "(obliquity %2°)"))
		.arg(year)
		.arg(obliquityDeg, 0, 'f', 2));
}

void ObjectVisibilityDialog::configurePlaceLabelControls()
{
	QSignalBlocker b1(ui->placeLabelsPopulationComboBox);
	QSignalBlocker b2(ui->twilightPlaceLabelsPopulationComboBox);

	auto configureCombo = [](QComboBox* combo)
	{
		combo->clear();
		combo->addItem(q_("100,000"), 100000);
		combo->addItem(q_("500,000"), 500000);
		combo->addItem(q_("1,000,000"), 1000000);
		combo->addItem(q_("5,000,000"), 5000000);
	};

	configureCombo(ui->placeLabelsPopulationComboBox);
	configureCombo(ui->twilightPlaceLabelsPopulationComboBox);
	syncPlaceLabelControls();
}

void ObjectVisibilityDialog::syncPlaceLabelControls()
{
	QSignalBlocker b1(ui->placeLabelsCheckBox);
	QSignalBlocker b2(ui->twilightPlaceLabelsCheckBox);
	QSignalBlocker b3(ui->placeLabelsPopulationComboBox);
	QSignalBlocker b4(ui->twilightPlaceLabelsPopulationComboBox);
	QSignalBlocker b5(ui->placeLabelsNearLinesOnlyCheckBox);
	QSignalBlocker b6(ui->twilightPlaceLabelsNearLinesOnlyCheckBox);

	ui->placeLabelsCheckBox->setChecked(placeLabelsVisible);
	ui->twilightPlaceLabelsCheckBox->setChecked(placeLabelsVisible);
	ui->placeLabelsNearLinesOnlyCheckBox->setChecked(placeLabelsNearLinesOnly);
	ui->twilightPlaceLabelsNearLinesOnlyCheckBox->setChecked(placeLabelsNearLinesOnly);

	auto selectPopulation = [this](QComboBox* combo)
	{
		const int index = combo->findData(placeLabelsMinimumPopulation);
		if (index >= 0)
			combo->setCurrentIndex(index);
	};
	selectPopulation(ui->placeLabelsPopulationComboBox);
	selectPopulation(ui->twilightPlaceLabelsPopulationComboBox);
}

void ObjectVisibilityDialog::updatePlaceLabels()
{
	if (!ui || !ui->mapWidget || !ui->twilightMapWidget) return;

	StelCore* core = StelApp::getInstance().getCore();
	const bool earth = core &&
	                   core->getCurrentLocation().planetName == QStringLiteral("Earth");

	auto setControlsEnabled = [earth](QCheckBox* showCheckBox,
	                                  QLabel* populationLabel,
	                                  QComboBox* populationComboBox,
	                                  QCheckBox* nearLinesOnlyCheckBox)
	{
		showCheckBox->setEnabled(earth);
		populationLabel->setEnabled(earth);
		populationComboBox->setEnabled(earth);
		nearLinesOnlyCheckBox->setEnabled(earth);
	};

	setControlsEnabled(ui->placeLabelsCheckBox,
	                   ui->placeLabelsPopulationLabel,
	                   ui->placeLabelsPopulationComboBox,
	                   ui->placeLabelsNearLinesOnlyCheckBox);
	setControlsEnabled(ui->twilightPlaceLabelsCheckBox,
	                   ui->twilightPlaceLabelsPopulationLabel,
	                   ui->twilightPlaceLabelsPopulationComboBox,
	                   ui->twilightPlaceLabelsNearLinesOnlyCheckBox);

	if (!earth || !placeLabelsVisible)
	{
		ui->mapWidget->setPlaceLabelsVisible(false);
		ui->twilightMapWidget->setPlaceLabelsVisible(false);
		return;
	}

	QVector<ObjectVisibilityMapWidget::PlaceLabel> labels;
	const LocationList locations = StelApp::getInstance().getLocationMgr().getAll();
	labels.reserve(locations.size());
	for (const StelLocation& loc : locations)
	{
		if (loc.planetName != QStringLiteral("Earth")) continue;
		if (loc.name.isEmpty()) continue;

		ObjectVisibilityMapWidget::PlaceLabel label;
		label.name = loc.name;
		label.longitude = loc.getLongitude(true);
		label.latitude = loc.getLatitude(true);
		label.population = loc.population;
		label.role = loc.role;
		labels.append(label);
	}

	ui->mapWidget->setPlaceLabels(labels);
	ui->twilightMapWidget->setPlaceLabels(labels);
	ui->mapWidget->setPlaceLabelMinimumPopulation(placeLabelsMinimumPopulation);
	ui->twilightMapWidget->setPlaceLabelMinimumPopulation(placeLabelsMinimumPopulation);
	ui->mapWidget->setPlaceLabelsNearLinesOnly(placeLabelsNearLinesOnly);
	ui->twilightMapWidget->setPlaceLabelsNearLinesOnly(placeLabelsNearLinesOnly);
	ui->mapWidget->setPlaceLabelsVisible(true);
	ui->twilightMapWidget->setPlaceLabelsVisible(true);
}

bool ObjectVisibilityDialog::isSupportedPlanet(const QString& englishName)
{
	// Bodies whose rotation-pole orientation is reliably known in
	// Stellarium.  Limiting to these guarantees the visibility lines
	// reflect the body's real geometry.  Other bodies may have
	// approximate or default-equatorial poles which would give
	// misleading lines.
	static const QStringList supported = {
		QStringLiteral("Earth"),
		QStringLiteral("Moon"),
		QStringLiteral("Mercury"),
		QStringLiteral("Venus"),
		QStringLiteral("Mars"),
		QStringLiteral("Jupiter"),
		QStringLiteral("Saturn"),
		QStringLiteral("Uranus"),
		QStringLiteral("Neptune"),
		QStringLiteral("Pluto"),
		// Galilean moons
		QStringLiteral("Io"),
		QStringLiteral("Europa"),
		QStringLiteral("Ganymede"),
		QStringLiteral("Callisto")
	};
	return supported.contains(englishName);
}

//
// =================== Helpers ====================
//

int ObjectVisibilityDialog::currentYear(StelCore* core)
{
	int y = 2000, m = 1, d = 1;
	StelUtils::getDateFromJulianDay(core->getJD(), &y, &m, &d);
	Q_UNUSED(m);
	Q_UNUSED(d);
	return y;
}

void ObjectVisibilityDialog::refreshTitleLabel()
{
	if (!ui) return;

	// Pre-Calculate state: the label reflects what's currently selected
	// in Stellarium so the user knows why Calculate is enabled or not.
	// Three cases:
	//   1. nothing selected           → generic prompt
	//   2. acceptable object selected → confirm with name
	//   3. wrong-type object selected → say so explicitly, including
	//      the localised type name, so the user understands why
	//      Calculate is disabled.  Tooltips on disabled buttons are
	//      not always discoverable; the label is.
	if (lockedObjectId.isEmpty())
	{
		StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
		if (!objMgr || objMgr->getSelectedObject().isEmpty())
		{
			ui->titleLabel->setText(
				q_("Select a star or DSO, then click Calculate."));
			return;
		}
		const StelObjectP sel = objMgr->getSelectedObject().first();
		if (isAcceptableType(sel->getType()))
		{
			QString name = sel->getNameI18n();
			if (name.isEmpty()) name = sel->getEnglishName();
			if (name.isEmpty()) name = sel->getID();
			ui->titleLabel->setText(
				QString(q_("%1 is selected. Click Calculate."))
				.arg(name));
		}
		else
		{
			// Map Stellarium's internal type string to a user-friendly
			// localised description.  We could ask the object itself
			// via getObjectType() but that returns finer-grained
			// English text ("star", "open cluster", ...) that may not
			// be the most useful thing to surface in a "wrong type"
			// message; a class-level descriptor reads more naturally.
			const QString type = sel->getType();
			QString typeName;
			if      (type == QStringLiteral("Planet"))
				typeName = q_("a planet, moon or the Sun");
			else if (type == QStringLiteral("Comet"))
				typeName = q_("a comet");
			else if (type == QStringLiteral("MinorPlanet"))
				typeName = q_("a minor planet");
			else if (type == QStringLiteral("Satellite"))
				typeName = q_("an artificial satellite");
			else if (type == QStringLiteral("NomenclatureItem"))
				typeName = q_("a surface feature");
			else if (type == QStringLiteral("Meteor"))
				typeName = q_("a meteor");
			else
				typeName = q_("not a star or deep-sky object");

			QString name = sel->getNameI18n();
			if (name.isEmpty()) name = sel->getEnglishName();
			if (name.isEmpty()) name = sel->getID();
			ui->titleLabel->setText(
				QString(q_("%1 is %2. This plug-in only supports stars "
				           "and deep-sky objects."))
				.arg(name)
				.arg(typeName));
		}
		return;
	}

	// Post-Calculate state: show what we computed.
	StelCore* core = StelApp::getInstance().getCore();
	const int year = currentYear(core);
	const QString planet = core->getCurrentLocation().planetName;

	// Two translatable templates: the Earth case keeps the original
	// phrasing; the non-Earth case adds the planet name.  Splitting
	// them lets translators adapt word order independently for each
	// language.  %1 = object name, %2 = year, %3 = planet name.
	QString text;
	if (planet == QStringLiteral("Earth"))
	{
		text = QString(q_("Visibility of %1 in %2"))
		       .arg(lockedObjectNameI18n)
		       .arg(year);
	}
	else
	{
		text = QString(q_("Visibility of %1 from %3 in %2"))
		       .arg(lockedObjectNameI18n)
		       .arg(year)
		       .arg(q_(planet.toUtf8().constData()));
	}
	ui->titleLabel->setText(text);
}

void ObjectVisibilityDialog::setAboutHtml()
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Object Visibility Plug-in") + "</h2>";
	html += "<table class='layout' width=\"90%\">";
	html += "<tr><td><strong>" + q_("Version") + ":</strong></td><td>"
	        + QString(OBJECTVISIBILITY_PLUGIN_VERSION) + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>"
	        + QString(OBJECTVISIBILITY_PLUGIN_LICENSE) + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Atque</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plug-in shows on a planet map where on the "
	                   "observer's planet a selected star or deep-sky "
	                   "object is visible.  Given an object with declination "
	                   "&delta; at the current epoch (precession and proper "
	                   "motion taken into account), five lines are drawn at "
	                   "fixed geographic latitudes:") + "</p>";
	html += "<ul>";
	html += "<li><strong>" + q_("Limit of visibility") + "</strong>: "
	        + q_("&phi; = &delta; &plusmn; 90&deg;.  The object never rises "
	             "above the horizon at latitudes outside this band.")
	        + "</li>";
	html += "<li><strong>" + q_("Extinction free / good visibility") + "</strong>: "
	        + q_("&phi; = &delta; &plusmn; (90&deg; &minus; h).  At latitudes "
	             "inside this band the object reaches at least altitude h "
	             "at upper culmination.  The default h is 5&deg;; you may "
	             "increase it if you observe from a mountainous site or "
	             "want a larger safety margin.")
	        + "</li>";
	html += "<li><strong>" + q_("Passes zenith") + "</strong>: "
	        + q_("&phi; = &delta;.  At this latitude the object passes "
	             "through the zenith.")
	        + "</li>";
	html += "<li><strong>" + q_("Circumpolar limit, northern hemisphere") + "</strong>: "
	        + q_("&phi; = 90&deg; &minus; &delta;.  North of this latitude "
	             "the object never sets.")
	        + "</li>";
	html += "<li><strong>" + q_("Circumpolar limit, southern hemisphere") + "</strong>: "
	        + q_("&phi; = &minus;90&deg; &minus; &delta;.  South of this "
	             "latitude the object never sets.")
	        + "</li>";
	html += "</ul>";

	html += "<p>" + q_("Planets, moons, asteroids, comets and artificial "
	                   "satellites are intentionally not supported as the "
	                   "selected <em>target</em>: their rapid daily motion "
	                   "(and, for some, parallax) would make a single "
	                   "snapshot misleading.")
	        + "</p>";

	html += "<p>" + q_("The plug-in does support observation from any of "
	                   "the following bodies: Earth, the Moon, the eight "
	                   "planets (Mercury through Neptune), Pluto, and the "
	                   "four Galilean moons of Jupiter (Io, Europa, "
	                   "Ganymede, Callisto).  Change observer body via "
	                   "Stellarium's Location dialog; the map updates "
	                   "automatically.")
	        + "</p>";

	html += "<p>" + q_("Because stellar positions change over time through "
	                   "precession and proper motion, the lines are always "
	                   "computed for the date currently shown in the "
	                   "planetarium.  To see Sirius's visibility in "
	                   "10&thinsp;000 BCE, set the planetarium's date "
	                   "first, then press Calculate.")
	        + "</p>";

	html += "<p>" + q_("The Twilight limits tab is Earth-only.  It draws the "
	                   "equator, tropics, polar circles, and solstice "
	                   "latitudes where the Sun's lowest or highest altitude "
	                   "is -6&deg;, -12&deg;, or -18&deg;.  These lines use "
	                   "Earth's obliquity at the current epoch, so they shift "
	                   "when you change the planetarium date.  They are not "
	                   "shown for other observing bodies because civil, "
	                   "nautical, and astronomical twilight depend on Earth's "
	                   "atmosphere.")
	        + "</p>";

	html += "<h3>" + q_("Credits") + "</h3>";
	html += "<p>" + q_("The whole concept of these visibility lines on a "
	                   "world map – and in particular the choice of the "
	                   "five line types and their visual style – comes "
	                   "from Astro-Geo-GIS, in the article "
	                   "<em>The 49 brightest stars in the night sky – when "
	                   "and where can we see them?</em>")
	        + "</p>";
	html += "<p><a href=\"https://astro-geo-gis.com/the-49-brightest-stars"
	        "-in-the-night-sky-when-and-where-can-we-see-them/\">"
	        + q_("Read the original article")
	        + "</a><br/>"
	        + "<a href=\"https://astro-geo-gis.com/\">"
	        + q_("Astro-Geo-GIS website")
	        + "</a></p>";

	html += StelApp::getInstance().getModuleMgr()
	        .getStandardSupportLinksInfo("Object Visibility plug-in");

	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		QString stylesheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(stylesheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}
