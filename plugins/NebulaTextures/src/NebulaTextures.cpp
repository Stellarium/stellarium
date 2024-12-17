/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024 ultrapre@github.com
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
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "NebulaTextures.hpp"

#include "StelModule.hpp"
#include "NebulaTexturesDialog.hpp"

#include <QDebug>

StelModule* NebulaTexturesStelPluginInterface::getStelModule() const
{
   return new NebulaTextures();
}

StelPluginInfo NebulaTexturesStelPluginInterface::getPluginInfo() const
{
   // Allow to load the resources when used as a static plugin
   // Q_INIT_RESOURCE(NebulaTextures);

	StelPluginInfo info;
   info.id = "NebulaTextures";
   info.displayedName = "Nebula Textures";
	info.authors = "Stellarium team";
	info.contact = "www.stellarium.org";
   info.description = "User Define Nebula Textures.";
	return info;
}

NebulaTextures::NebulaTextures()
{
   setObjectName("NebulaTextures");
	font.setPixelSize(25);
   configDialog = new NebulaTexturesDialog();
}

NebulaTextures::~NebulaTextures()
{
   // delete configDialog;
}

bool NebulaTextures::configureGui(bool show)
{
   if (show)
      configDialog->setVisible(true);
   return true;
}

double NebulaTextures::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void NebulaTextures::init()
{
   qDebug() << "init called for NebulaTextures";
   // addAction("actionShow_NebulaTextures_ConfigDialog",
   //           N_("Nebula Textures"), N_("Open Nebula Textures configuration dialog"),
   //           configDialog, "visible");
   configDialog->reloadTextures();
}

void NebulaTextures::draw(StelCore* core)
{
}

