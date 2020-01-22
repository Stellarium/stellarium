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

#ifndef REMOTECONTROL_HPP
#define REMOTECONTROL_HPP

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

//! @ingroup remoteControl
//! Main class of the %RemoteControl plug-in, implementing the StelModule interface.
//! Manages the settings and the starting/stopping of the <a href="http://stefanfrings.de/qtwebapp/index-en.html">QtWebApp</a> web server.
//! The RequestHandler class is used for request processing.
//! @author Florian Schaukowitsch
class RemoteControl : public StelModule
{
	Q_OBJECT
	//! Determines if the web server is running, and can be used to start/stop the server
	Q_PROPERTY(bool enabled
		   READ getFlagEnabled
		   WRITE setFlagEnabled
		   NOTIFY flagEnabledChanged)
	//! If true, the server is automatically started when init() is called
	Q_PROPERTY(bool autoStart
		   READ getFlagAutoStart
		   WRITE setFlagAutoStart
		   NOTIFY flagAutoStartChanged)
	//! If true, the password set with setPassword() is required for all requests.
	//! The password is passed on to the RequestHandler.
	Q_PROPERTY(bool usePassword
		   READ getFlagUsePassword
		   WRITE setFlagUsePassword
		   NOTIFY flagUsePasswordChanged)
	Q_PROPERTY(bool enableCors
		   READ getFlagEnableCors
		   WRITE setFlagEnableCors
		   NOTIFY flagEnableCorsChanged)
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
	bool getFlagEnableCors() const { return enableCors; }

	QString getPassword() const { return password; }
	QString getCorsOrigin() const { return corsOrigin; }
	int getPort() const {return port; }

public slots:
	//property setters
	//! Starts/stops the web server
	void setFlagEnabled(bool b);
	//! If true, the server is automatically started when init() is called
	void setFlagAutoStart(bool b);
	//! If true, the password from setPassword() is required for all web requests
	void setFlagUsePassword(bool b);

	//! Sets the password that is optionally enabled with setFlagUsePassword().
	//! The password is required by RequestHandler for all HTTP requests.
	//! Basic HTTP auth is used, without a user name.
	void setPassword(const QString& password);

	//! If true, Access-Control-Allow-Origin header will be appended to responses.
	void setFlagEnableCors(bool b);

	//! Sets the CORS origin that is optionally enabled with setFlagEnableCors().
	void setCorsOrigin(const QString& corsOrigin);


	//! Sets the port where the server listens. Must be done before startServer() is called,
	//! or restart the server to use the new setting.
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

	//! Starts the HTTP server using the current settings and begins handling requests.
	//! Uses the RequestHandler class for processing.
	//! @see RequestHandler
	void startServer();
	//! Stops the HTTP server gracefully
	void stopServer();

signals:
	//property notifiers
	void flagEnabledChanged(bool val);
	void flagAutoStartChanged(bool val);
	void flagUsePasswordChanged(bool val);
	void flagEnableCorsChanged(bool val);

	void portChanged(int val);
	void passwordChanged(const QString& val);
	void corsOriginChanged(const QString& val);


private:
	//the http server
	HttpListener* httpListener;
	//the main request handler
	RequestHandler* requestHandler;

	bool enabled;
	bool autoStart;
	bool usePassword;
	QString password;
	bool enableCors;
	QString corsOrigin;

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

//! @ingroup remoteControl
//! This class defines the plugin interface with the main Stellarium program
class RemoteControlStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*REMOTECONTROL_HPP*/

