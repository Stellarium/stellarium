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
#include "ui_timeNavigatorDialog.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelModule.hpp"
#include "StelObjectMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelTranslator.hpp"
#include "SpecificTimeMgr.hpp"

TimeNavigatorDialog::TimeNavigatorDialog()
	: StelDialog("TimeNavigator")
	, ui(new Ui_timeNavigatorDialog)
	, core(Q_NULLPTR)
	, specMgr(Q_NULLPTR)
	, objMgr(Q_NULLPTR)
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
	connect(ui->btnTodayRising,     &QPushButton::clicked, this, [=](){specMgr->todayRising();});
	connect(ui->btnNextRising,      &QPushButton::clicked, this, [=](){specMgr->nextRising();});
	connect(ui->btnPreviousTransit, &QPushButton::clicked, this, [=](){specMgr->previousTransit();});
	connect(ui->btnTodayTransit,    &QPushButton::clicked, this, [=](){specMgr->todayTransit();});
	connect(ui->btnNextTransit,     &QPushButton::clicked, this, [=](){specMgr->nextTransit();});
	connect(ui->btnPreviousSetting, &QPushButton::clicked, this, [=](){specMgr->previousSetting();});
	connect(ui->btnTodaySetting,    &QPushButton::clicked, this, [=](){specMgr->todaySetting();});
	connect(ui->btnNextSetting,     &QPushButton::clicked, this, [=](){specMgr->nextSetting();});

	// Twilight - wire the altitude spinbox directly to SpecificTimeMgr property
	connectTwilightAltitudeSpinBox();
	connect(ui->btnPreviousMorningTwilight, &QPushButton::clicked, this, [=](){specMgr->previousMorningTwilight();});
	connect(ui->btnTodayMorningTwilight,    &QPushButton::clicked, this, [=](){specMgr->todayMorningTwilight();});
	connect(ui->btnNextMorningTwilight,     &QPushButton::clicked, this, [=](){specMgr->nextMorningTwilight();});
	connect(ui->btnPreviousEveningTwilight, &QPushButton::clicked, this, [=](){specMgr->previousEveningTwilight();});
	connect(ui->btnTodayEveningTwilight,    &QPushButton::clicked, this, [=](){specMgr->todayEveningTwilight();});
	connect(ui->btnNextEveningTwilight,     &QPushButton::clicked, this, [=](){specMgr->nextEveningTwilight();});

	// At altitude
	connect(ui->btnPreviousMorningAtAltitude, &QPushButton::clicked, this, [=](){specMgr->previousMorningAtAltitude();});
	connect(ui->btnTodayMorningAtAltitude,    &QPushButton::clicked, this, [=](){specMgr->todayMorningAtAltitude();});
	connect(ui->btnNextMorningAtAltitude,     &QPushButton::clicked, this, [=](){specMgr->nextMorningAtAltitude();});
	connect(ui->btnPreviousEveningAtAltitude, &QPushButton::clicked, this, [=](){specMgr->previousEveningAtAltitude();});
	connect(ui->btnTodayEveningAtAltitude,    &QPushButton::clicked, this, [=](){specMgr->todayEveningAtAltitude();});
	connect(ui->btnNextEveningAtAltitude,     &QPushButton::clicked, this, [=](){specMgr->nextEveningAtAltitude();});

	// Enable/disable object-dependent groups when selection changes
	connect(objMgr, &StelObjectMgr::selectedObjectChanged,
	        this, &TimeNavigatorDialog::updateSelectedObjectState);
	updateSelectedObjectState();

	// ── Tab: Settings ────────────────────────────────────────────────────
	connectBoolProperty(ui->checkBoxShowButton, "TimeNavigator.flagShowButton");
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
