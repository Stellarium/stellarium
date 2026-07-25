/*
 * Object Visibility plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * Concept inspired by:
 *   Astro-Geo-GIS, "The 49 brightest stars in the night sky – when and
 *   where can we see them?", https://astro-geo-gis.com/
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

#include "ObjectVisibility.hpp"
#include "gui/ObjectVisibilityDialog.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QDebug>
#include <QPixmap>
#include <QSettings>
#include <algorithm>
#include <stdexcept>

//
// =================== Plug-in interface (Qt plug-in system) ===================
//

StelModule* ObjectVisibilityStelPluginInterface::getStelModule() const
{
	return new ObjectVisibility();
}

StelPluginInfo ObjectVisibilityStelPluginInterface::getPluginInfo() const
{
	// Required when used as a static plug-in.
	Q_INIT_RESOURCE(ObjectVisibility);

	StelPluginInfo info;
	info.id          = "ObjectVisibility";
	info.displayedName = N_("Object Visibility");
	info.authors     = "Atque";
	info.contact     = "https://github.com/Atque";
	info.description = N_("Shows on a planet map where a selected star or "
	                      "deep-sky object is visible.  Supports observation "
	                      "from Earth, the Moon, the eight planets, Pluto, "
	                      "and the four Galilean moons.  Also shows "
	                      "Earth-only solstice twilight latitude limits "
	                      "computed from obliquity of date.  Based on the "
	                      "geometric criteria of the Astro-Geo-GIS article "
	                      "\"The 49 brightest stars in the night sky – when "
	                      "and where can we see them?\"");
	info.version     = OBJECTVISIBILITY_PLUGIN_VERSION;
	info.license     = OBJECTVISIBILITY_PLUGIN_LICENSE;
	return info;
}

//
// =================== ObjectVisibility ====================
//

ObjectVisibility::ObjectVisibility()
	: goodVisibilityLimit(5)
	, conf(nullptr)
#ifndef NO_GUI
	, configDialog(nullptr)
	, toolbarButton(nullptr)
#endif
{
	setObjectName("ObjectVisibility");

#ifndef NO_GUI
	configDialog = new ObjectVisibilityDialog();
#endif
	conf = StelApp::getInstance().getSettings();
}

ObjectVisibility::~ObjectVisibility()
{
#ifndef NO_GUI
	delete configDialog;
	configDialog = nullptr;
#endif
}

double ObjectVisibility::getCallOrder(StelModuleActionName actionName) const
{
	Q_UNUSED(actionName);
	// We don't draw on the sky, so the order doesn't really matter.
	return 0.0;
}

bool ObjectVisibility::configureGui(bool show)
{
#ifdef NO_GUI
	Q_UNUSED(show);
	return false;
#else
	if (show)
		configDialog->setVisible(true);
	return true;
#endif
}

void ObjectVisibility::init()
{
	loadSettings();

	// Defensive: ensure our qrc is registered.  Q_INIT_RESOURCE is
	// idempotent — calling it twice is safe.  We do this here in
	// addition to in getPluginInfo() so that even if some unusual
	// loading order happens, our pixmaps are guaranteed to be
	// accessible when the toolbar button is built below.
#ifndef NO_GUI
	Q_INIT_RESOURCE(ObjectVisibility);
#endif

	const QString section = N_("Object Visibility");
	// A StelAction that toggles the dialog's visibility.  Note that
	// the dialog itself is a StelDialog with a `visible` property, so
	// we hook the action directly to it.
#ifndef NO_GUI
	addAction("actionShow_ObjectVisibility_dialog",
	          section,
	          N_("Object Visibility plug-in dialog"),
	          configDialog,
	          "visible",
	          "");  // no default shortcut

	// Toolbar button (only if we have the standard GUI).
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui != nullptr)
		{
			// Icon PNGs are supplied at 5x scale (160x160) because
			// Stellarium's GUI uses GUI_INPUT_PIXMAPS_SCALE = 5 to
			// downscale into the actual ~32px logical button size.
			// Supplying smaller PNGs makes the button appear tiny.
			toolbarButton = new StelButton(
				nullptr,
				QPixmap(":/ObjectVisibility/bt_object_visibility_on.png"),
				QPixmap(":/ObjectVisibility/bt_object_visibility_off.png"),
				QPixmap(":/graphicGui/miscGlow32x32.png"),
				"actionShow_ObjectVisibility_dialog");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "[ObjectVisibility] Unable to create toolbar button:"
		           << e.what();
	}
#endif
}

void ObjectVisibility::loadSettings()
{
	Q_ASSERT(conf);
	conf->beginGroup("ObjectVisibility");
	const int v = conf->value("good_visibility_limit", 5).toInt();
	conf->endGroup();
	setGoodVisibilityLimit(std::clamp(v, 1, 89));
}

void ObjectVisibility::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	conf->beginGroup("ObjectVisibility");
	conf->remove("");      // wipe section
	conf->setValue("good_visibility_limit", 5);
	conf->endGroup();
	loadSettings();
}

void ObjectVisibility::setGoodVisibilityLimit(int degrees)
{
	degrees = std::clamp(degrees, 1, 89);
	if (degrees == goodVisibilityLimit)
		return;
	goodVisibilityLimit = degrees;

	if (conf)
	{
		conf->beginGroup("ObjectVisibility");
		conf->setValue("good_visibility_limit", goodVisibilityLimit);
		conf->endGroup();
	}
	emit goodVisibilityLimitChanged(goodVisibilityLimit);
}
