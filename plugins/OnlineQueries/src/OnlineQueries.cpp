/*
 * Pointer Coordinates plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
 * Copyright (C) 2016 Georg Zotti (Constellation code)
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
#include "StelMainView.hpp"
#include "SkyGui.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "StelDialog.hpp"
#include "OnlineQueries.hpp"
#include "OnlineQueriesDialog.hpp"

#include <QFontMetrics>
#include <QSettings>
#include <QMetaEnum>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(onlineQueries,"stel.plugin.OnlineQueries")

StelModule* OnlineQueriesPluginInterface::getStelModule() const
{
	return new OnlineQueries();
}

StelPluginInfo OnlineQueriesPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(OnlineQueries);

	StelPluginInfo info;
	info.id = "OnlineQueries";
	info.displayedName = N_("Online Queries");
	info.authors = "Georg Zotti, Alexander Wolf";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plugin allows object information retrieval from selected online services.");
	info.version = ONLINEQUERIES_PLUGIN_VERSION;
	info.license = ONLINEQUERIES_PLUGIN_LICENSE;
	return info;
}

OnlineQueries::OnlineQueries() :
	enabled(false),
	textColor(1,0.5,0),
	fontSize(14),
	toolbarButton(Q_NULLPTR)
{
	setObjectName("OnlineQueries");
	dialog = new OnlineQueriesDialog();
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

OnlineQueries::~OnlineQueries()
{
	delete dialog;
}

void OnlineQueries::init()
{
	if (!conf->childGroups().contains("OnlineQueries"))
	{
		qCDebug(onlineQueries) << "OnlineQueries: no section exists in main config file - creating with defaults";
		restoreDefaultConfiguration();
	}
	// populate settings from main config file.
	loadConfiguration();

	addAction("actionShow_OnlineQueries", N_("Online Queries"), N_("Show window for Online Queries"), this, "enabled", "");
	createToolbarButton();

	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveConfiguration()));
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int))); // Probably not needed.
}

void OnlineQueries::deinit()
{
	//
}

void OnlineQueries::draw(StelCore *core)
{
	Q_UNUSED(core)
	if (!isEnabled())
		return;

}

void OnlineQueries::setEnabled(bool b)
{
	if (b!=enabled)
	{
		enabled = b;
		emit flagEnabledChanged(b);
	}
	configureGui(b);
}

// Default return 0 seems OK.
// double OnlineQueries::getCallOrder(StelModuleActionName actionName) const
// {
// 	if (actionName==StelModule::ActionDraw)
// 		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)+10.;
// 	return 0;
// }

bool OnlineQueries::configureGui(bool show)
{
	dialog->setVisible(show);
	return true;
}

void OnlineQueries::restoreDefaultConfiguration(void)
{
	// Remove the whole section from the configuration file
	conf->remove("OnlineQueries");
	// Load the default values...
	loadConfiguration();
	// ... then save them.
	saveConfiguration();
}

void OnlineQueries::loadConfiguration(void)
{
	conf->beginGroup("OnlineQueries");
	textColor = StelUtils::strToVec3f(conf->value("text_color", "1,0.5,0").toString());
	setFontSize(conf->value("font_size", 14).toInt());
	conf->endGroup();
}

void OnlineQueries::saveConfiguration(void)
{
	conf->beginGroup("OnlineQueries");
	conf->setValue("text_color", StelUtils::vec3fToStr(textColor));
	conf->setValue("font_size", getFontSize());
	conf->endGroup();
}

void OnlineQueries::createToolbarButton() const
{
	// Add toolbar button (copy/paste widely from AngleMeasure).
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		if (gui!=Q_NULLPTR)
		{
			StelButton* button =	new StelButton(Q_NULLPTR,
							       QPixmap(":/OnlineQueries/bt_OnlineQueries_On.png"),
							       QPixmap(":/OnlineQueries/bt_OnlineQueries_Off.png"),
							       QPixmap(":/graphicGui/glow32x32.png"),
							       "actionShow_OnlineQueries");
			qCDebug(onlineQueries) << "adding Button to toolbar ...";

			gui->getButtonBar()->addButton(button, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qCWarning(onlineQueries) << "WARNING: unable to create toolbar button for OnlineQueries plugin: " << e.what();
	}
}
