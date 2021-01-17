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

#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelVertexArray.hpp"
#include "RemoteControl.hpp"
#include "RemoteControlDialog.hpp"
#include "RequestHandler.hpp"

#include "httpserver/httplistener.h"
#include "httpserver/staticfilecontroller.h"

#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QtNetwork>
#include <QSettings>
#include <stdexcept>

//! This method is the one called automatically by the StelModuleMgr just after loading the dynamic library
StelModule* RemoteControlStelPluginInterface::getStelModule() const
{
	return new RemoteControl();
}

StelPluginInfo RemoteControlStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(RemoteControl);

	StelPluginInfo info;
	info.id = "RemoteControl";
	info.displayedName = N_("Remote Control");
	info.authors = "Florian Schaukowitsch, Georg Zotti";
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("Provides remote control functionality using a webserver interface. See manual for detailed description.");
	info.acknowledgements += N_("This plugin was created in the 2015 campaign of the ESA Summer of Code in Space programme.");
	info.version = REMOTECONTROL_PLUGIN_VERSION;
	info.license = REMOTECONTROL_PLUGIN_LICENSE;
	return info;
}

RemoteControl::RemoteControl()
	: httpListener(Q_NULLPTR)
	, requestHandler(Q_NULLPTR)
	, enabled(false)
	, autoStart(false)
	, usePassword(false)
	, password("")
	, enableCors(false)
	, corsOrigin("")
	, port(8090)
	, minThreads(1)
	, maxThreads(30)
	, toolbarButton(Q_NULLPTR)
{
	setObjectName("RemoteControl");

	configDialog = new RemoteControlDialog();
	conf = StelApp::getInstance().getSettings();

	//needed to ensure clean shutdown of server before threading errors can occur
	connect(&StelApp::getInstance(), &StelApp::aboutToQuit, this, &RemoteControl::stopServer, Qt::DirectConnection);
}

RemoteControl::~RemoteControl()
{
	delete configDialog;
	if(httpListener)
	{
		//we manually delete the listener here to make sure
		//all connections are closed before the requesthandler is deleted
		delete httpListener;
		httpListener = Q_NULLPTR;
	}
}

bool RemoteControl::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

//! Determine which "layer" the plugin's drawing will happen on.
double RemoteControl::getCallOrder(StelModuleActionName actionName) const
{
	// TODO: if there is any graphics, this may have to be adjusted. Else maybe even delete?
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void RemoteControl::init()
{
	if (!conf->childGroups().contains("RemoteControl"))
		restoreDefaultSettings();

	loadSettings();

	qDebug()<<"RemoteControl using QtWebApp version"<<getQtWebAppLibVersion();

	StaticFileControllerSettings settings;
	//retrieve actual webroot through StelFileMgr
	//this prefers a webroot in user dir to the default one in the install dir
	QString path = StelFileMgr::findFile("webroot",StelFileMgr::Directory);
	if(path.isEmpty())
	{
		//webroot folder not found, probably running from IDE! try to use hardcoded path...
		path = REMOTECONTROL_WEBROOT_PATH;
	}
	//make sure its absolute, otherwise QtWebApp will look relative to working dir
	QDir dirPath(path);
	if(!dirPath.exists())
		qWarning()<<"[RemoteControl] Webroot folder invalid, can not use HTML interface";
	settings.path = dirPath.absolutePath();
#ifndef QT_NO_DEBUG
	//"disable" cache for development
	settings.cacheTime = 1;
	settings.maxAge = 1;
#endif
	requestHandler = new RequestHandler(settings, this);

	StelApp& app = StelApp::getInstance();

	// Create action for enable/disable & hook up signals	
	addAction("actionShow_Remote_Control", N_("Remote Control"), N_("Remote control"), "enabled", "");

	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));

	// Add a toolbar button. TODO:  decide whether a button is necessary at all. Maye the button should not only enable, but call the GUI dialog directly?
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/RemoteControl/resources/bt_remote_on.png"),
						       QPixmap(":/RemoteControl/resources/bt_remote_off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Remote_Control");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for RemoteControl plugin: " << e.what();
	}

	if(autoStart)
		setFlagEnabled(true);
}

void RemoteControl::update(double deltaTime)
{
	requestHandler->update(deltaTime);
}


//! Draw any parts on the screen which are for our module
void RemoteControl::draw(StelCore* core)
{
	Q_UNUSED(core)
}

void RemoteControl::setFlagEnabled(bool b)
{
	if(b!=enabled)
	{
		enabled = b;
		if (b)
		{
			qDebug() << "RemoteControl enabled";
			startServer();
		}
		else
			stopServer();

		emit flagEnabledChanged(b);
	}
}

void RemoteControl::setFlagAutoStart(bool b)
{
	if(b!=autoStart)
	{
		autoStart = b;
		emit flagAutoStartChanged(b);
	}
}

void RemoteControl::setFlagUsePassword(bool b)
{
	if(b!=usePassword)
	{
		usePassword = b;
		emit flagUsePasswordChanged(b);
	}
}

void RemoteControl::setPassword(const QString &password)
{
	if(password != this->password)
	{
		this->password = password;
		emit passwordChanged(password);
	}
}

void RemoteControl::setFlagEnableCors(bool b)
{
	if(b!=enableCors)
	{
		enableCors = b;
		emit flagEnableCorsChanged(b);
	}
}

void RemoteControl::setCorsOrigin(const QString &corsOrigin)
{
	if(corsOrigin != this->corsOrigin)
	{
		this->corsOrigin = corsOrigin;
		emit corsOriginChanged(corsOrigin);
	}
}

void RemoteControl::setPort(const int port)
{
	if(port!=this->port)
	{
		this->port = port;
		emit portChanged(port);
	}
}

void RemoteControl::startServer()
{
	Q_ASSERT(httpListener == Q_NULLPTR);

	//set request handler password settings
	requestHandler->setPassword(password);
	requestHandler->setUsePassword(usePassword);
	requestHandler->setEnableCors(enableCors);
	requestHandler->setCorsOrigin(corsOrigin);
	HttpListenerSettings settings;
	settings.port = port;
	settings.minThreads = minThreads;
	settings.maxThreads = maxThreads;
	httpListener = new HttpListener(settings,requestHandler);
}

void RemoteControl::stopServer()
{
	if(httpListener)
	{
		delete httpListener;
		httpListener = Q_NULLPTR;
	}
}

void RemoteControl::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("RemoteControl");
	// ...load the default values...
	loadSettings();
	// ...and then save them.
	saveSettings();
}

void RemoteControl::loadSettings()
{
	conf->beginGroup("RemoteControl");
	setFlagAutoStart(conf->value("autostart", false).toBool()); // disable autostart for security reason
	setFlagUsePassword(conf->value("use_password", false).toBool());
	setPassword(conf->value("password", "").toString());
	setFlagEnableCors(conf->value("enable_cors", false).toBool());
	setCorsOrigin(conf->value("cors_origin", "").toString());
	setPort(conf->value("port", 8090).toInt());
	minThreads = conf->value("min_threads", 1).toInt();
	maxThreads = conf->value("max_threads", 30).toInt();
	conf->endGroup();
}

void RemoteControl::saveSettings()
{
	conf->beginGroup("RemoteControl");
	conf->setValue("autostart", autoStart);
	conf->setValue("use_password", usePassword);
	conf->setValue("password", password);
	conf->setValue("enable_cors", enableCors);
	conf->setValue("cors_origin", corsOrigin);
	conf->setValue("port", port);
	conf->setValue("min_threads", minThreads);
	conf->setValue("max_threads", maxThreads);
	conf->endGroup();
}
