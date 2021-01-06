/*
 * Copyright (C) 2012 Ivan Marti-Vidal
 * Copyright (C) 2021 Thomas Heinrichs
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

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QSettings>
#include <QString>
#include <QTimer>

#include "Observability.hpp"
#include "ObservabilityDialog.hpp"

#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StarMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFader.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelObserver.hpp"
#include "StelProjector.hpp"
#include "StelSkyDrawer.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "ZoneArray.hpp"


StelModule* ObservabilityStelPluginInterface::getStelModule() const
{
	return new Observability();
}

StelPluginInfo ObservabilityStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Observability);

	StelPluginInfo info;
	info.id = "Observability";
	info.displayedName = N_("Observability Analysis");
	info.authors = "Ivan Marti-Vidal (Onsala Space Observatory)"; // non-translatable field
	info.contact = "i.martividal@gmail.com";
	info.description = N_("Displays an analysis of a selected object's observability (rise, set, and transit times) for the current date, as well as when it is observable through the year. An object is assumed to be observable if it is above the horizon during a fraction of the night. Also included are the dates of the largest separation from the Sun and acronychal and cosmical rising and setting. (Explanations are provided in the 'About' tab of the plugin's configuration window.)");
	info.version = OBSERVABILITY_PLUGIN_VERSION;
	info.license = OBSERVABILITY_PLUGIN_LICENSE;
	return info;
}

Observability::Observability()
{
	setObjectName("Observability");
	config = StelApp::getInstance().getSettings();
	configDialog = new ObservabilityDialog();
}

Observability::~Observability()
{

}

void Observability::init()
{
	loadSettings();

	StelAction* actionShow = addAction("actionShow_Observability", N_("Observability"), N_("Observability"), "flagShowReport");
	addAction("actionShow_Observability_ConfigDialog", N_("Observability"), N_("Observability configuration window"), configDialog, "visible", "");
}

void Observability::draw(StelCore* core)
{

}

void Observability::update(double x)
{

}

double Observability::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 10.;
	return 0.;
}

void Observability::deinit()
{
	if (configDialog != nullptr)
	{
		delete configDialog;
	}
}

bool Observability::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Observability::showGoodNights(bool b)
{
	flagShowGoodNights = b;
	// save setting
	config->setValue("Observability/show_good_nights", flagShowGoodNights);
	emit flagShowGoodNightsChanged(b);
}

void Observability::showBestNight(bool b)
{
	flagShowBestNight = b;
	// save setting
	config->setValue("Observability/show_best_night", flagShowBestNight);
	emit flagShowBestNightChanged(b);
}

void Observability::showToday(bool b)
{
	flagShowToday = b;
	// save setting
	config->setValue("Observability/show_today", flagShowToday);
	emit flagShowTodayChanged(b);
}

void Observability::showFullMoon(bool b)
{
	flagShowFullMoon = b;
	// save setting
	config->setValue("Observability/show_full_moon", flagShowFullMoon);
	emit flagShowFullMoonChanged(b);
}

void Observability::enableReport(bool b)
{
	if (b != flagShowReport)
	{
		flagShowReport = b;
		emit flagReportVisibilityChanged(b);
	}
}

void Observability::restoreDefaultSettings()
{
	Q_ASSERT(config);
	// Remove the old values
	config->remove("Observability");
	// load default settings
	loadSettings();
	
	config->beginGroup("Observability");

	config->endGroup();
}

void Observability::loadSettings()
{
	showGoodNights(config->value("Observability/show_good_nights", true).toBool());
	showBestNight(config->value("Observability/show_best_night", true).toBool());
	showToday(config->value("Observability/show_today", true).toBool());
	showFullMoon(config->value("Observability/show_full_moon", true).toBool());
}