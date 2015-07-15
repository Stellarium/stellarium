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
 
#ifndef REMOTESYNC_HPP_
#define REMOTESYNC_HPP_

#include <QFont>
#include <QKeyEvent>

#include "StelModule.hpp"

class RemoteSyncDialog;

//! Main class of the RemoteSync plug-in.
//! Provides a synchronization mechanism for multiple Stellarium instances in a network.
//! This plugin has been developed during ESA SoCiS 2015.
class RemoteSync : public StelModule
{
	Q_OBJECT

public:
	RemoteSync();
	virtual ~RemoteSync();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);

	//virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(QKeyEvent* event){event->setAccepted(false);}

	virtual bool configureGui(bool show=true);
	///////////////////////////////////////////////////////////////////////////

	int getServerPort() const { return serverPort; }

public slots:

	void setServerPort(const int port);


	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "RemoteSync" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see saveSettings(), restoreSettings()
	void loadSettings();

	//! Save the plug-in's settings to the configuration file.
	//! @see loadSettings(), restoreSettings()
	void saveSettings();

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

signals:
	void serverPortChanged(const int val);

private:
	int serverPort;

	QSettings* conf;

	// GUI
	RemoteSyncDialog* configDialog;
};


#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class RemoteSyncStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*REMOTESYNC_HPP_*/

