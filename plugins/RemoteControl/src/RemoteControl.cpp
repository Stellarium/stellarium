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

#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QtNetwork>
#include <QSettings>
#include <cmath>

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
			      "<p>See manual for detailed description.</p>"
			      "<p>This plugin was developed during ESA SoCiS 2015.</p>");
	info.version = REMOTECONTROL_VERSION;
	return info;
}

RemoteControl::RemoteControl()
	: enabled(false)
	, toolbarButton(NULL), httpListener(NULL), requestHandler(NULL)
{
	setObjectName("RemoteControl");
	font.setPixelSize(16);

	configDialog = new RemoteControlDialog();
	conf = StelApp::getInstance().getSettings();

	messageTimer = new QTimer(this);
	messageTimer->setInterval(7000);
	messageTimer->setSingleShot(true);

	connect(messageTimer, SIGNAL(timeout()), this, SLOT(clearMessage()));
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

	QSettings* staticSettings = new QSettings(conf->fileName(),StelIniFormat,this);
	staticSettings->beginGroup("RemoteControl");
	staticSettings->beginGroup("staticfiles");
	requestHandler = new RequestHandler(staticSettings);

	StelApp& app = StelApp::getInstance();

	// Create action for enable/disable & hook up signals	
	addAction("actionShow_Remote_Control", N_("Remote Control"), N_("Remote control"), "enabled", "Ctrl+N");

	// Initialize the message strings and make sure they are translated when
	// the language changes.
	updateMessageText();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateMessageText()));

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
}

void RemoteControl::update(double deltaTime)
{
	messageFader.update((int)(deltaTime*1000));
	// TODO: GZ I think most processing will be triggered here...
}


//! Draw any parts on the screen which are for our module
void RemoteControl::draw(StelCore* core)
{
	// TODO: Likely nothing.
	Q_UNUSED(core);
}

void RemoteControl::enableRemoteControl(bool b)
{
	enabled = b;
	messageFader = b;
	if (b)
	{
		qDebug() << "RemoteControl enabled";
		messageTimer->start();
		startServer();
	}
	else
		stopServer();
}

void RemoteControl::startServer()
{
	//use the Stellarium config file, but a separate conf object
	QSettings* settings = new QSettings(conf->fileName(),StelIniFormat,this);
	settings->beginGroup("RemoteControl");
	settings->beginGroup("listener");
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


void RemoteControl::updateMessageText()
{
	// TRANSLATORS: instructions for using the RemoteControl plugin.
	messageEnabled = q_("Remote Control enabled.");
}

void RemoteControl::clearMessage()
{
	//qDebug() << "RemoteControl::clearMessage";
	messageFader = false;
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

	//save the QtWebApp settings
	conf->beginGroup("RemoteControl");
	conf->beginGroup("listener");
	conf->setValue("port",8080);
	conf->setValue("minThreads",1);
	conf->setValue("maxThreads",10);
	conf->setValue("cleanupInterval",1000);
	conf->setValue("readTimeout", 60000);
	conf->setValue("maxRequestSize",16000);
	conf->setValue("maxMultiPartSize",10000000);
	conf->endGroup();
	conf->beginGroup("staticfiles");
	conf->setValue("encoding","UTF-8");
	conf->setValue("maxAge",120000);
	conf->setValue("cacheTime",120000);
	conf->setValue("cacheSize",1000000);
	conf->setValue("maxCachedFileSize",65536);
	conf->endGroup();
	conf->endGroup();
}

void RemoteControl::loadSettings()
{
	conf->beginGroup("RemoteControl");

	conf->endGroup();
}

void RemoteControl::saveSettings()
{
	conf->beginGroup("RemoteControl");
	// TODO whatever has to be stored here
	conf->endGroup();
}
