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
//! You can either connect via (JavaScript enabled) web browser (recommended in 2016: Firefox, Chrome) to a
//! configurable port (default: 8080) as in
//! http://localhost:8080[/index.html]
//! or for alternative GUI which may be better suited for small 7inch screens,
//! http://localhost:8080/tablet7in.html
//! The HTML pages for the interface reside in the /data/webroot directory inside the installation directory.
//! Alternative or derived HTML control GUIs must be placed into the same folder,
//! the web server cannot read data in the private Stellarium user directory.
//!
//! The RemoteControl plugin makes extensive use of the StelProperty system introduced with it. This allows not only to trigger actions,
//! but also set QVariant values, which is enough to control many things in the program.
//! A few dedicated modules have been implemented closely following the existing GUI for view motion, location setting,
//! landscape and skyculture selection, searching objects, etc.
//! It is possible to define simple action or property control interfaces which are only shown for activated plugins.
//!
//! It is also possible to send commands via commandline, e.g..
//! @code
//! wget -q --post-data 'id=double_stars.ssc' http://localhost:8080/api/scripts/run >/dev/null 2>&amp;1
//! curl --data 'id=double_stars.ssc' http://localhost:8080/api/scripts/run >/dev/null 2>&amp;1
//! curl -d     'id=double_stars.ssc' http://localhost:8080/api/scripts/run >/dev/null 2>&amp;1
//! @endcode
//! This allows triggering automatic show setups for museums etc.
//!
//! @author Florian Schaukowitsch
//! This plugin has been developed as project of ESA SoCiS 2015.
//!
//! TODO: Complete this documentation.
class RemoteControl : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled
		   READ getFlagEnabled
		   WRITE setFlagEnabled
		   NOTIFY flagEnabledChanged)
	Q_PROPERTY(bool autoStart
		   READ getFlagAutoStart
		   WRITE setFlagAutoStart
		   NOTIFY flagAutoStartChanged)
	Q_PROPERTY(bool usePassword
		   READ getFlagUsePassword
		   WRITE setFlagUsePassword
		   NOTIFY flagUsePasswordChanged)
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
	bool getFlagEnabled() const {return enabled;}
	bool getFlagAutoStart() const { return autoStart; }
	bool getFlagUsePassword() const { return usePassword; }

	QString getPassword() const { return password; }
	int getPort() const {return port; }

public slots:
	//property setters
	void setFlagEnabled(bool b);
	void setFlagAutoStart(bool b);
	void setFlagUsePassword(bool b);

	void setPassword(const QString& password);
	void setPort(const int port);

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "RemoteControl" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see saveSettings(), restoreDefaultSettings()
	void loadSettings();

	//! Save the plug-in's settings to the configuration file.
	//! @see loadSettings(), restoreDefaultSettings()
	void saveSettings();

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

	//! Starts the HTTP server and begins handling request
	void startServer();
	//! Stops the HTTP server
	void stopServer();

signals:
	//property notifiers
	void flagEnabledChanged(bool val);
	void flagAutoStartChanged(bool val);
	void flagUsePasswordChanged(bool val);

	void portChanged(int val);
	void passwordChanged(const QString& val);


private:

	//the http server
	HttpListener* httpListener;
	//the main request handler
	RequestHandler* requestHandler;

	bool enabled;
	bool autoStart;
	bool usePassword;
	QString password;

	int port;
	int minThreads;
	int maxThreads;

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

