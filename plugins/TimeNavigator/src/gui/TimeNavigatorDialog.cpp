/*
 * Time Navigator plug-in for Stellarium
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TimeNavigator.hpp"
#include "TimeNavigatorDialog.hpp"
#include "PlanetaryEventsMgr.hpp"
#include "ui_timeNavigatorDialog.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelModule.hpp"
#include "StelObjectMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "SpecificTimeMgr.hpp"
#include "StelLocation.hpp"
#include "StelMovementMgr.hpp"
#include "SolarSystem.hpp"

#include <QDate>
#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QSizePolicy>

TimeNavigatorDialog::TimeNavigatorDialog()
	: StelDialog("TimeNavigator")
	, ui(new Ui_timeNavigatorDialog)
	, core(Q_NULLPTR)
	, specMgr(Q_NULLPTR)
	, objMgr(Q_NULLPTR)
	, planetaryMgr(Q_NULLPTR)
	, notOnEarthLabel(Q_NULLPTR)
{
}

TimeNavigatorDialog::~TimeNavigatorDialog()
{
	delete ui;
}

void TimeNavigatorDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void TimeNavigatorDialog::createDialogContent()
{
	core    = StelApp::getInstance().getCore();
	specMgr = GETSTELMODULE(SpecificTimeMgr);
	objMgr  = GETSTELMODULE(StelObjectMgr);

	ui->setupUi(dialog);

	// ── Earth-only guard ─────────────────────────────────────────────────────
	// All functionality in this plugin is only meaningful from Earth.
	// Build a message label that replaces the tab widget when the observer
	// travels to another body.
	notOnEarthLabel = new QLabel(
	    q_("The Time Navigator is only available when observing from Earth."),
	    dialog);
	notOnEarthLabel->setAlignment(Qt::AlignCenter);
	notOnEarthLabel->setWordWrap(true);
	// Insert it into the same top-level layout as the tab widget, but hide
	// it initially; updateEarthOnlyState() will show/hide as needed.
	ui->mainVBox->insertWidget(1, notOnEarthLabel);
	notOnEarthLabel->hide();

	connect(core, SIGNAL(locationChanged(StelLocation)),
	        this, SLOT(updateEarthOnlyState()));
	updateEarthOnlyState();   // apply correct initial state

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui = static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// ── Tab: Time Steps ──────────────────────────────────────────────────

	// Days group
	connect(ui->btnSubSolarDay,    &QToolButton::clicked, core, &StelCore::subtractDay);
	connect(ui->btnAddSolarDay,    &QToolButton::clicked, core, &StelCore::addDay);
	connect(ui->btnSubSolarWeek,   &QToolButton::clicked, core, &StelCore::subtractWeek);
	connect(ui->btnAddSolarWeek,   &QToolButton::clicked, core, &StelCore::addWeek);
	connect(ui->btnSubSiderealDay, &QToolButton::clicked, core, &StelCore::subtractSiderealDay);
	connect(ui->btnAddSiderealDay, &QToolButton::clicked, core, &StelCore::addSiderealDay);
	connect(ui->btnSubSiderealWeek,&QToolButton::clicked, core, &StelCore::subtractSiderealWeek);
	connect(ui->btnAddSiderealWeek,&QToolButton::clicked, core, &StelCore::addSiderealWeek);

	// Months group
	connect(ui->btnSubSynodicMonth,      &QToolButton::clicked, core, &StelCore::subtractSynodicMonth);
	connect(ui->btnAddSynodicMonth,      &QToolButton::clicked, core, &StelCore::addSynodicMonth);
	connect(ui->btnSubMeanTropicalMonth, &QToolButton::clicked, core, &StelCore::subtractMeanTropicalMonth);
	connect(ui->btnAddMeanTropicalMonth, &QToolButton::clicked, core, &StelCore::addMeanTropicalMonth);
	connect(ui->btnSubDraconicMonth,     &QToolButton::clicked, core, &StelCore::subtractDraconicMonth);
	connect(ui->btnAddDraconicMonth,     &QToolButton::clicked, core, &StelCore::addDraconicMonth);
	connect(ui->btnSubAnomalisticMonth,  &QToolButton::clicked, core, &StelCore::subtractAnomalisticMonth);
	connect(ui->btnAddAnomalisticMonth,  &QToolButton::clicked, core, &StelCore::addAnomalisticMonth);
	connect(ui->btnSubCalendarMonth,     &QToolButton::clicked, core, &StelCore::subtractCalendarMonth);
	connect(ui->btnAddCalendarMonth,     &QToolButton::clicked, core, &StelCore::addCalendarMonth);

	// Years group
	connect(ui->btnSubMeanTropicalYear,  &QToolButton::clicked, core, &StelCore::subtractMeanTropicalYear);
	connect(ui->btnAddMeanTropicalYear,  &QToolButton::clicked, core, &StelCore::addMeanTropicalYear);
	connect(ui->btnSubTropicalYear,      &QToolButton::clicked, core, &StelCore::subtractTropicalYear);
	connect(ui->btnAddTropicalYear,      &QToolButton::clicked, core, &StelCore::addTropicalYear);
	connect(ui->btnSubSiderealYear,      &QToolButton::clicked, core, &StelCore::subtractSiderealYear);
	connect(ui->btnAddSiderealYear,      &QToolButton::clicked, core, &StelCore::addSiderealYear);
	connect(ui->btnSubDraconicYear,      &QToolButton::clicked, core, &StelCore::subtractDraconicYear);
	connect(ui->btnAddDraconicYear,      &QToolButton::clicked, core, &StelCore::addDraconicYear);
	connect(ui->btnSubAnomalisticYear,   &QToolButton::clicked, core, &StelCore::subtractAnomalisticYear);
	connect(ui->btnAddAnomalisticYear,   &QToolButton::clicked, core, &StelCore::addAnomalisticYear);
	connect(ui->btnSubJulianYear,        &QToolButton::clicked, core, &StelCore::subtractJulianYear);
	connect(ui->btnAddJulianYear,        &QToolButton::clicked, core, &StelCore::addJulianYear);
	connect(ui->btnSubGaussianYear,      &QToolButton::clicked, core, &StelCore::subtractGaussianYear);
	connect(ui->btnAddGaussianYear,      &QToolButton::clicked, core, &StelCore::addGaussianYear);
	connect(ui->btnSubCalendarYear,      &QToolButton::clicked, core, &StelCore::subtractCalendarYear);
	connect(ui->btnAddCalendarYear,      &QToolButton::clicked, core, &StelCore::addCalendarYear);

	// Long cycles group
	connect(ui->btnSubMetonicCycle, &QToolButton::clicked, core, &StelCore::subtractMetonicCycle);
	connect(ui->btnAddMetonicCycle, &QToolButton::clicked, core, &StelCore::addMetonicCycle);
	connect(ui->btnSubSaros,        &QToolButton::clicked, core, &StelCore::subtractSaros);
	connect(ui->btnAddSaros,        &QToolButton::clicked, core, &StelCore::addSaros);

	// ── Tab: Specific Time ───────────────────────────────────────────────

	// Selected object: Rising / Transit / Setting
	connect(ui->btnPreviousRising,  &QPushButton::clicked, this, [=](){specMgr->previousRising();});
	connect(ui->btnThisDateRising,     &QPushButton::clicked, this, [=](){specMgr->todayRising();});
	connect(ui->btnNextRising,      &QPushButton::clicked, this, [=](){specMgr->nextRising();});
	connect(ui->btnPreviousTransit, &QPushButton::clicked, this, [=](){specMgr->previousTransit();});
	connect(ui->btnThisDateTransit,    &QPushButton::clicked, this, [=](){specMgr->todayTransit();});
	connect(ui->btnNextTransit,     &QPushButton::clicked, this, [=](){specMgr->nextTransit();});
	connect(ui->btnPreviousSetting, &QPushButton::clicked, this, [=](){specMgr->previousSetting();});
	connect(ui->btnThisDateSetting,    &QPushButton::clicked, this, [=](){specMgr->todaySetting();});
	connect(ui->btnNextSetting,     &QPushButton::clicked, this, [=](){specMgr->nextSetting();});

	// Twilight - wire the altitude spinbox directly to SpecificTimeMgr property
	connectTwilightAltitudeSpinBox();
	connect(ui->btnPreviousMorningTwilight, &QPushButton::clicked, this, [=](){specMgr->previousMorningTwilight();});
	connect(ui->btnThisDateMorningTwilight,    &QPushButton::clicked, this, [=](){specMgr->todayMorningTwilight();});
	connect(ui->btnNextMorningTwilight,     &QPushButton::clicked, this, [=](){specMgr->nextMorningTwilight();});
	connect(ui->btnPreviousEveningTwilight, &QPushButton::clicked, this, [=](){specMgr->previousEveningTwilight();});
	connect(ui->btnThisDateEveningTwilight,    &QPushButton::clicked, this, [=](){specMgr->todayEveningTwilight();});
	connect(ui->btnNextEveningTwilight,     &QPushButton::clicked, this, [=](){specMgr->nextEveningTwilight();});

	// At altitude
	connect(ui->btnPreviousMorningAtAltitude, &QPushButton::clicked, this, [=](){specMgr->previousMorningAtAltitude();});
	connect(ui->btnThisDateMorningAtAltitude,    &QPushButton::clicked, this, [=](){specMgr->todayMorningAtAltitude();});
	connect(ui->btnNextMorningAtAltitude,     &QPushButton::clicked, this, [=](){specMgr->nextMorningAtAltitude();});
	connect(ui->btnPreviousEveningAtAltitude, &QPushButton::clicked, this, [=](){specMgr->previousEveningAtAltitude();});
	connect(ui->btnThisDateEveningAtAltitude,    &QPushButton::clicked, this, [=](){specMgr->todayEveningAtAltitude();});
	connect(ui->btnNextEveningAtAltitude,     &QPushButton::clicked, this, [=](){specMgr->nextEveningAtAltitude();});

	// Enable/disable object-dependent groups when selection changes
	connect(objMgr, &StelObjectMgr::selectedObjectChanged,
	        this, &TimeNavigatorDialog::updateSelectedObjectState);
	updateSelectedObjectState();

	// ── Tab: Planetary Events ────────────────────────────────────────────────
	planetaryMgr = new PlanetaryEventsMgr(this);
	buildPlanetaryTab();

	// ── Tab: Settings ────────────────────────────────────────────────────
	connectBoolProperty(ui->checkBoxShowButton,    "TimeNavigator.flagShowButton");
	connectBoolProperty(ui->checkBoxSelectObject,  "TimeNavigator.flagSelectObject");
	connectBoolProperty(ui->checkBoxCenterView,    "TimeNavigator.flagCenterView");

	// "Center view" only makes sense when "Select object" is enabled.
	auto updateCenterEnabled = [this]() {
		ui->checkBoxCenterView->setEnabled(
		    ui->checkBoxSelectObject->isChecked());
	};
	connect(ui->checkBoxSelectObject, &QCheckBox::toggled,
	        this, [updateCenterEnabled](bool) { updateCenterEnabled(); });
	updateCenterEnabled();

	connect(ui->btnSaveSettings,    &QPushButton::clicked, this, &TimeNavigatorDialog::saveSettings);
	connect(ui->btnRestoreDefaults, &QPushButton::clicked, this, &TimeNavigatorDialog::restoreDefaults);

	setAboutHtml();
}

void TimeNavigatorDialog::connectTwilightAltitudeSpinBox()
{
	// connectDoubleProperty is protected in StelDialog, so we replicate its
	// logic directly using the public StelPropertyMgr API.
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()
	                         ->getProperty("SpecificTimeMgr.twilightAltitude");
	if (prop)
	{
		ui->spinBoxTwilightAltitude->setValue(prop->getValue().toDouble());
		connect(ui->spinBoxTwilightAltitude,
		        static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		        prop, &StelProperty::setValue);
		connect(prop, &StelProperty::changed, ui->spinBoxTwilightAltitude,
		        [this](const QVariant& val) {
		            bool b = ui->spinBoxTwilightAltitude->blockSignals(true);
		            ui->spinBoxTwilightAltitude->setValue(val.toDouble());
		            ui->spinBoxTwilightAltitude->blockSignals(b);
		        });
	}
}

void TimeNavigatorDialog::updateSelectedObjectState()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	const bool hasObject = !selected.isEmpty() && selected[0]->getType() != "Satellite";

	ui->labelSelectedObject->setText(hasObject ? selected[0]->getNameI18n()
	                                            : q_("No object selected"));
	ui->groupRTS->setEnabled(hasObject);
	ui->groupAtAltitude->setEnabled(hasObject);
}

void TimeNavigatorDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[TimeNavigator] restore defaults...";
		GETSTELMODULE(TimeNavigator)->restoreDefaults();
	}
	else
		qDebug() << "[TimeNavigator] restore defaults canceled.";
}

void TimeNavigatorDialog::saveSettings()
{
	GETSTELMODULE(TimeNavigator)->saveSettingsToConfig();
}

void TimeNavigatorDialog::setAboutHtml()
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Time Navigator Plug-in") + "</h2>";
	html += "<table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + TIMENAVIGATOR_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + TIMENAVIGATOR_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Authors") + ":</strong></td><td>Atque</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Time Navigator plugin provides a convenient window for stepping "
	                   "the simulation time forward or backward by common astronomical periods "
	                   "(solar day, sidereal day, synodic month, tropical year, Saros cycle, etc.), "
	                   "as well as for jumping to specific events for a selected object "
	                   "(rising, transit, setting) and to twilight times.") + "</p>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Time Navigator plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui != Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}


// ── Planetary Events tab ─────────────────────────────────────────────────────

void TimeNavigatorDialog::buildPlanetaryTab()
{
	QWidget*     tabW      = ui->tabWidget->findChild<QWidget*>("tabPlanetaryEvents");
	QVBoxLayout* outerVBox = qobject_cast<QVBoxLayout*>(tabW->layout());

	QScrollArea* scroll = new QScrollArea(tabW);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	QWidget*     contents = new QWidget(scroll);
	QVBoxLayout* vbox     = new QVBoxLayout(contents);
	vbox->setSpacing(6);
	vbox->setContentsMargins(0, 0, 0, 0);
	scroll->setWidget(contents);
	outerVBox->addWidget(scroll);

	// Helper: add one group of event rows.
	// rows = list of { display label, eventType key }
	auto addGroup = [&](const QString& title,
	                    std::initializer_list<QPair<QString,QString>> rows)
	{
		QGroupBox*   group = new QGroupBox(title, contents);
		QGridLayout* grid  = new QGridLayout(group);
		grid->setSpacing(2);
		grid->setContentsMargins(4, 4, 4, 4);

		// Column headers
		grid->addWidget(new QLabel(QString("◀ ") + q_("Previous"), group), 0, 1, Qt::AlignCenter);
		grid->addWidget(new QLabel(q_("Next") + QString(" ▶"),     group), 0, 2, Qt::AlignCenter);

		int r = 1;
		for (const auto& row : rows)
		{
			const QString rowLabel  = row.first;
			const QString eventType = row.second;

			grid->addWidget(new QLabel(rowLabel, group), r, 0);

			QPushButton* btnP = new QPushButton("◀", group);
			btnP->setToolTip(QString(q_("Previous %1")).arg(rowLabel));
			btnP->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			grid->addWidget(btnP, r, 1);

			QPushButton* btnN = new QPushButton("▶", group);
			btnN->setToolTip(QString(q_("Next %1")).arg(rowLabel));
			btnN->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			grid->addWidget(btnN, r, 2);

			connect(btnP, &QPushButton::clicked, this, [this, eventType]() {
				double jd = planetaryMgr->findPrev(core->getJD(), eventType);
				if (jd > 0.0) { core->setJD(jd); applySelectCenter(eventType); }
			});
			connect(btnN, &QPushButton::clicked, this, [this, eventType]() {
				double jd = planetaryMgr->findNext(core->getJD(), eventType);
				if (jd > 0.0) { core->setJD(jd); applySelectCenter(eventType); }
			});
			++r;
		}
		vbox->addWidget(group);
	};

	addGroup(q_("Mercury"), {
		{ q_("Inferior conjunction"),        PE::MercuryInfConj  },
		{ q_("Superior conjunction"),        PE::MercurySupConj  },
		{ q_("Greatest eastern elongation"), PE::MercuryElongE   },
		{ q_("Greatest western elongation"), PE::MercuryElongW   },
	});
	addGroup(q_("Venus"), {
		{ q_("Inferior conjunction"),        PE::VenusInfConj  },
		{ q_("Superior conjunction"),        PE::VenusSupConj  },
		{ q_("Greatest eastern elongation"), PE::VenusElongE   },
		{ q_("Greatest western elongation"), PE::VenusElongW   },
	});
	addGroup(q_("Mars"), {
		{ q_("Opposition"),         PE::MarsOpposition },
		{ q_("Superior conjunction"),PE::MarsSupConj   },
	});
	addGroup(q_("Jupiter"), {
		{ q_("Opposition"),          PE::JupiterOpposition },
		{ q_("Superior conjunction"),PE::JupiterSupConj    },
	});
	addGroup(q_("Saturn"), {
		{ q_("Opposition"),          PE::SaturnOpposition },
		{ q_("Superior conjunction"),PE::SaturnSupConj    },
	});
	addGroup(q_("Uranus"), {
		{ q_("Opposition"),          PE::UranusOpposition },
		{ q_("Superior conjunction"),PE::UranusSupConj    },
	});
	addGroup(q_("Neptune"), {
		{ q_("Opposition"),          PE::NeptuneOpposition },
		{ q_("Superior conjunction"),PE::NeptuneSupConj    },
	});
	addGroup(q_("Moon"), {
		{ q_("New Moon"),      PE::MoonNew          },
		{ q_("First quarter"), PE::MoonFirstQuarter },
		{ q_("Full Moon"),     PE::MoonFull         },
		{ q_("Last quarter"),  PE::MoonLastQuarter  },
		{ q_("Perigee"),       PE::MoonPerigee      },
		{ q_("Apogee"),        PE::MoonApogee       },
	});
	addGroup(q_("Earth"), {
		{ q_("Perihelion"), PE::EarthPerihelion },
		{ q_("Aphelion"),   PE::EarthAphelion   },
	});

	vbox->addStretch();
}

// ── Earth-only guard ─────────────────────────────────────────────────────────

void TimeNavigatorDialog::updateEarthOnlyState()
{
	const bool onEarth = (core->getCurrentPlanet()->getEnglishName() == "Earth");
	ui->tabWidget->setVisible(onEarth);
	notOnEarthLabel->setVisible(!onEarth);
}

// ── Select / center on event ──────────────────────────────────────────────────

void TimeNavigatorDialog::applySelectCenter(const QString& eventType) const
{
	// Extract the planet name: everything before the first space.
	// e.g. "Jupiter opposition" → "Jupiter", "Moon new" → "Moon"
	const QString planetName = eventType.section(' ', 0, 0);

	// Earth events (perihelion/aphelion): observer IS Earth, nothing to select.
	if (planetName == "Earth")
		return;

	const TimeNavigator* tn = GETSTELMODULE(TimeNavigator);
	if (!tn->getFlagSelectObject())
		return;

	SolarSystem*    ss      = GETSTELMODULE(SolarSystem);
	StelObjectMgr*  objMgr  = GETSTELMODULE(StelObjectMgr);
	StelMovementMgr* mvMgr  = GETSTELMODULE(StelMovementMgr);

	PlanetP planet = ss->searchByEnglishName(planetName);
	if (!planet)
		return;

	objMgr->setSelectedObject(qSharedPointerCast<StelObject>(planet),
	                          StelModule::ReplaceSelection);

	if (tn->getFlagCenterView())
	{
		const QList<StelObjectP> sel = objMgr->getSelectedObject();
		if (!sel.isEmpty())
		{
			mvMgr->moveToObject(sel[0], mvMgr->getAutoMoveDuration());
			mvMgr->setFlagTracking(true);
		}
	}
}
