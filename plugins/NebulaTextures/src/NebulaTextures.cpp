/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024-2025 WANG Siliang
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

#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelModule.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include "NebulaTextures.hpp"
#include "NebulaTexturesDialog.hpp"

#include <stdexcept>
#include <QDebug>

// Returns a new instance of NebulaTextures module.
StelModule* NebulaTexturesStelPluginInterface::getStelModule() const
{
	return new NebulaTextures();
}

// Provides plugin information, including metadata like ID, authors, description, version, and license.
StelPluginInfo NebulaTexturesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(NebulaTextures);

	StelPluginInfo info;
	info.id = "NebulaTextures";
	info.displayedName = N_("Nebula Textures");
	info.authors = "WANG Siliang";
	info.contact = "bd7jay@outlook.com";
	info.description = N_("This plugin can plate-solve and position astronomical images, adding them as custom deep-sky object textures for rendering.");
	info.version = NEBULATEXTURES_PLUGIN_VERSION;
	info.license = NEBULATEXTURES_PLUGIN_LICENSE;
	return info;
}

// Constructor for NebulaTextures module, initializes GUI and connects signals.
NebulaTextures::NebulaTextures()
{
	setObjectName("NebulaTextures");
	configDialog = new NebulaTexturesDialog();
	connect(StelApp::getInstance().getModule("StelSkyLayerMgr"),
			SIGNAL(collectionLoaded()),configDialog,SLOT(initializeRefreshIfNeeded()));
}

// Destructor for NebulaTextures, cleaning up resources if necessary.
NebulaTextures::~NebulaTextures()
{
	// delete configDialog; // Resources are cleaned up automatically by Qt
}

// Configures the GUI, displaying the settings dialog if 'show' is true.
bool NebulaTextures::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

// Returns the call order for the "Draw" action, used to determine when the module should be drawn.
double NebulaTextures::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName) + 10.0;
	return 0;
}

// Initializes actions, UI elements, and toolbar button for the NebulaTextures module.
void NebulaTextures::init()
{
	// Create action for enable/disable & hook up signals
	addAction("actionShow_NebulaTextures", N_("Nebula Textures"), N_("Toggle Custom Nebula Textures"), "flagShow");
	addAction("actionShow_NebulaTextures_config_dialog", N_("Nebula Textures"), N_("Show settings dialog"), configDialog, "visible"); // no default hotkey

	// Add a toolbar button
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui != Q_NULLPTR)
		{
			StelButton* nebulaTexturesButton = new StelButton(Q_NULLPTR,
				QPixmap(":/NebulaTextures/btNebulaTextures-on.png"),
				QPixmap(":/NebulaTextures/btNebulaTextures-off.png"),
				QPixmap(":/graphicGui/miscGlow32x32.png"),
				"actionShow_NebulaTextures",
				false,
				"actionShow_NebulaTextures_config_dialog");
			gui->getButtonBar()->addButton(nebulaTexturesButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "[NebulaTextures] unable to manage toolbar buttons for NebulaTextures plugin!"
			<< e.what();
	}
}

// Draw method for the NebulaTextures module. Currently, it does nothing.
void NebulaTextures::draw(StelCore* core)
{
}

// Sets whether custom textures should be shown and refreshes the texture display.
void NebulaTextures::setShow(bool b)
{
	configDialog->setShowCustomTextures(b);
	configDialog->refreshTextures();
	emit showChanged(b);
}

// Returns whether custom textures are currently displayed.
bool NebulaTextures::getShow()
{
	return configDialog->getShowCustomTextures();
}
