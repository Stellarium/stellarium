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
	info.authors = "Florian Schaukowitsch and Georg Zotti";
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("<p>Provides remote control functionality using a webserver interface.</p> "
			      "<p>See manual for detailed description.</p><br/>"
			      "<p>This plugin was developed during ESA SoCiS 2015.</p>"
			      "<p>This plugin uses the <a href=\"http://stefanfrings.de/qtwebapp/index-en.html\">QtWebApp HTTP server</a> by Stefan Frings.</p>"
			      );
	info.version = REMOTECONTROL_VERSION;
	return info;
}

RemoteControl::RemoteControl()
	: httpListener(NULL), requestHandler(NULL), enabled(false), autoStart(false), toolbarButton(NULL)
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
		delete httpListener;
	}
	if(requestHandler)
		requestHandler->deleteLater();
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
	QString path = StelFileMgr::findFile("data/webroot",StelFileMgr::Directory);
	//make sure its absolute, otherwise QtWebApp will look relative to working dir
	settings.path = QDir(path).absolutePath();
#ifndef QT_NO_DEBUG
	//"disable" cache for development
	settings.cacheTime = 1;
	settings.maxAge = 1;
#endif
	requestHandler = new RequestHandler(settings);

	StelApp& app = StelApp::getInstance();

	// Create action for enable/disable & hook up signals	
	addAction("actionShow_Remote_Control", N_("Remote Control"), N_("Remote control"), "enabled", "Ctrl+N");

	// Add a toolbar button. TODO:  decide whether a button is necessary at all. Maye the button should not only enable, but call the GUI dialog directly?
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=NULL)
		{
			toolbarButton = new StelButton(NULL,
						       QPixmap(":/RemoteControl/resources/bt_network_on.png"),
						       QPixmap(":/RemoteControl/resources/bt_network_off.png"),
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_Remote_Control");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable create toolbar button for RemoteControl plugin: " << e.what();
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
	Q_UNUSED(core);
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
	Q_ASSERT(httpListener == NULL);

	//set request handler password settings
	requestHandler->setPassword(password);
	requestHandler->setUsePassword(usePassword);
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
		httpListener = NULL;
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
	setFlagAutoStart(conf->value("autoStart",true).toBool());
	setFlagUsePassword(conf->value("usePassword",false).toBool());
	setPassword(conf->value("password","").toString());
	setPort(conf->value("port",8090).toInt());
	minThreads = conf->value("minThreads",1).toInt();
	maxThreads = conf->value("maxThreads",30).toInt();
	conf->endGroup();
}

void RemoteControl::saveSettings()
{
	conf->beginGroup("RemoteControl");
	conf->setValue("autoStart",autoStart);
	conf->setValue("usePassword",usePassword);
	conf->setValue("password",password);
	conf->setValue("port",port);
	conf->setValue("minThreads",minThreads);
	conf->setValue("maxThreads",maxThreads);
	conf->endGroup();
}
