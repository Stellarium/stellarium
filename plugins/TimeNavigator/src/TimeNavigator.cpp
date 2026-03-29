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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "TimeNavigator.hpp"
#include "TimeNavigatorDialog.hpp"

#include <QDebug>
#include <QPixmap>
#include <QSettings>

StelModule* TimeNavigatorStelPluginInterface::getStelModule() const
{
	return new TimeNavigator();
}

StelPluginInfo TimeNavigatorStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(TimeNavigator);

	StelPluginInfo info;
	info.id = "TimeNavigator";
	info.displayedName = N_("Time Navigator");
	info.authors = "Atque";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plugin provides a convenient panel for stepping the simulation time "
	                       "forward or backward by common astronomical periods, and for jumping to "
	                       "specific events such as risings, transits, settings, and twilight times.");
	info.version = TIMENAVIGATOR_PLUGIN_VERSION;
	info.license = TIMENAVIGATOR_PLUGIN_LICENSE;
	return info;
}

TimeNavigator::TimeNavigator()
	: flagShowButton(true)
	, flagSelectObject(true)
	, flagCenterView(false)
	, toolbarButton(Q_NULLPTR)
{
	setObjectName("TimeNavigator");
	mainWindow = new TimeNavigatorDialog();
	StelApp& app = StelApp::getInstance();
	conf = app.getSettings();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

TimeNavigator::~TimeNavigator()
{
	delete mainWindow;
}

void TimeNavigator::init()
{
	if (!conf->childGroups().contains("TimeNavigator"))
	{
		qDebug() << "[TimeNavigator] no TimeNavigator section in config - creating with defaults";
		restoreDefaultConfigIni();
	}

	readSettingsFromConfig();

	addAction("actionShow_TimeNavigator_dialog",
	          N_("Time Navigator"),
	          N_("Show Time Navigator window"),
	          mainWindow, "visible", "");

	setFlagShowButton(flagShowButton);

	connect(StelApp::getInstance().getCore(),
	        SIGNAL(configurationDataSaved()),
	        this, SLOT(saveSettings()));
}

void TimeNavigator::deinit()
{
	//
}

double TimeNavigator::getCallOrder(StelModuleActionName actionName) const
{
	Q_UNUSED(actionName)
	return 0.;
}

bool TimeNavigator::configureGui(bool show)
{
	if (show)
		mainWindow->setVisible(true);
	return true;
}

void TimeNavigator::restoreDefaults()
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void TimeNavigator::restoreDefaultConfigIni()
{
	conf->beginGroup("TimeNavigator");
	conf->remove("");
	conf->setValue("flag_show_button",    true);
	conf->setValue("flag_select_object",  true);
	conf->setValue("flag_center_view",    false);
	conf->endGroup();
}

void TimeNavigator::readSettingsFromConfig()
{
	conf->beginGroup("TimeNavigator");
	flagShowButton    = conf->value("flag_show_button",   true).toBool();
	flagSelectObject  = conf->value("flag_select_object", true).toBool();
	flagCenterView    = conf->value("flag_center_view",   false).toBool();
	conf->endGroup();
}

void TimeNavigator::saveSettingsToConfig() const
{
	conf->beginGroup("TimeNavigator");
	conf->setValue("flag_show_button",   getFlagShowButton());
	conf->setValue("flag_select_object", getFlagSelectObject());
	conf->setValue("flag_center_view",   getFlagCenterView());
	conf->endGroup();
}

void TimeNavigator::setFlagShowButton(bool b)
{
	if (b)
	{
		if (toolbarButton == Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR,
			                               QPixmap(":/TimeNavigator/bt_TimeNavigator_On.png"),
			                               QPixmap(":/TimeNavigator/bt_TimeNavigator_Off.png"),
			                               QPixmap(":/graphicGui/miscGlow32x32.png"),
			                               "actionShow_TimeNavigator_dialog",
			                               false);
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}
	else
	{
		gui->getButtonBar()->hideButton("actionShow_TimeNavigator_dialog");
	}
	flagShowButton = b;
	emit flagShowButtonChanged(b);
}
