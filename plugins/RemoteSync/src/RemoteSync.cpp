/*
 * Stellarium Remote Sync plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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

#include "RemoteSync.hpp"
#include "RemoteSyncDialog.hpp"

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"

#include <QDebug>
#include <QSettings>

//! This method is the one called automatically by the StelModuleMgr just after loading the dynamic library
StelModule* RemoteSyncStelPluginInterface::getStelModule() const
{
	return new RemoteSync();
}

StelPluginInfo RemoteSyncStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(RemoteSync);

	StelPluginInfo info;
	info.id = "RemoteSync";
	info.displayedName = N_("Remote Sync");
	info.authors = "Florian Schaukowitsch and Georg Zotti";
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("<p>Provides state synchronization for multiple Stellarium instances running in a network.</p> "
			      "<p>This can be used, for example, to create multi-screen/panorama setups using multiple physical PCs.</p>"
			      "<p>See manual for detailed description.</p>"
			      "<p>This plugin was developed during ESA SoCiS 2015.</p>");
	info.version = REMOTESYNC_VERSION;
	return info;
}

RemoteSync::RemoteSync()
{
	setObjectName("RemoteSync");

	configDialog = new RemoteSyncDialog();
	conf = StelApp::getInstance().getSettings();
}

RemoteSync::~RemoteSync()
{
	delete configDialog;
}

bool RemoteSync::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void RemoteSync::init()
{
	if (!conf->childGroups().contains("RemoteSync"))
		restoreDefaultSettings();

	loadSettings();


	// TODO create actions/buttons, if required
}

void RemoteSync::update(double deltaTime)
{
	//include update logic here
	Q_UNUSED(deltaTime);
}

void RemoteSync::setServerPort(const int port)
{
	if(port!= serverPort)
	{
		serverPort = port;
		emit serverPortChanged(port);
	}
}

void RemoteSync::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("RemoteSync");
	// ...load the default values...
	loadSettings();
	// ...and then save them.
	saveSettings();
}

void RemoteSync::loadSettings()
{
	conf->beginGroup("RemoteSync");
	setServerPort(conf->value("serverPort",20180).toInt());
	conf->endGroup();
}

void RemoteSync::saveSettings()
{
	conf->beginGroup("RemoteSync");
	conf->setValue("serverPort",serverPort);
	conf->endGroup();
}
