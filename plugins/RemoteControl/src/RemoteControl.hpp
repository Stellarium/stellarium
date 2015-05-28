/*
 * Stellarium Remote Control plugin
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
 
#ifndef REMOTECONTROL_HPP_
#define REMOTECONTROL_HPP_

#include <QFont>
#include <QKeyEvent>

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include "StelCore.hpp"

class QTimer;
class QPixmap;
class StelButton;
class RemoteControlDialog;
class HttpListener;
class RequestHandler;

//! Main class of the RemoteControl plug-in.
//! Provides a remote control using a webserver interface, usable for single or even synchronized cluster of clients (via the RemoteSync plugin).
//! This plugin has been developed during ESA SoCiS 2015.
class RemoteControl : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled
		   READ isEnabled
		   WRITE enableRemoteControl)
public:
	RemoteControl();
	virtual ~RemoteControl();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(QKeyEvent* event){event->setAccepted(false);}

	//virtual void handleMouseClicks(class QMouseEvent* event);
	//virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	virtual bool configureGui(bool show=true);
	///////////////////////////////////////////////////////////////////////////
	// Property getters
	bool isEnabled() const {return enabled;}


	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "RemoteControl" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see saveSettings(), restoreSettings()
	void loadSettings();

	//! Save the plug-in's settings to the configuration file.
	//! @see loadSettings(), restoreSettings()
	void saveSettings();

public slots:
	//property setters
	void enableRemoteControl(bool b);

private slots:
	void updateMessageText();
	void clearMessage();

	//! Starts the HTTP server and begins handling request
	void startServer();
	//! Stops the HTTP server
	void stopServer();

private:

	//the http server
	HttpListener* httpListener;
	//the main request handler
	RequestHandler* requestHandler;

	bool enabled;
	QFont font;
	LinearFader messageFader;
	QTimer* messageTimer;
	QString messageEnabled;

	StelButton* toolbarButton;


	QSettings* conf;

	// GUI
	RemoteControlDialog* configDialog;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class RemoteControlStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*REMOTECONTROL_HPP_*/

