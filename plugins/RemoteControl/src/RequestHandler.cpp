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

#include "RequestHandler.hpp"
#include "httpserver/staticfilecontroller.h"
#include "templateengine/template.h"

#include "APIController.hpp"
#include "LocationService.hpp"
#include "LocationSearchService.hpp"
#include "MainService.hpp"
#include "ObjectService.hpp"
#include "ScriptService.hpp"
#include "SimbadService.hpp"
#include "StelActionService.hpp"
#include "ViewService.hpp"

#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelFileMgr.hpp"

#include <QDir>
#include <QFile>

const QByteArray RequestHandler::AUTH_REALM = "Basic realm=\"Stellarium remote control\"";

class StelTemplateTranslationProvider : public ITemplateTranslationProvider
{
public:
	StelTemplateTranslationProvider()
	{
		//create a translator for remote control specific stuff
		rcTranslator = new StelTranslator("stellarium-remotecontrol",StelTranslator::globalTranslator->getTrueLocaleName());
	}

	~StelTemplateTranslationProvider()
	{
		delete rcTranslator;
	}

	QString getTranslation(const QString &key) Q_DECL_OVERRIDE
	{
		//try to get a RemoteControl specific translation first
		QString trans = rcTranslator->tryQtranslate(key);
		if(trans.isNull())
			return StelTranslator::globalTranslator->qtranslate(key);
		return trans;
	}
private:
	StelTranslator* rcTranslator;
};


RequestHandler::RequestHandler(const StaticFileControllerSettings& settings, QObject* parent) : HttpRequestHandler(parent), usePassword(false)
{
	apiController = new APIController(QByteArray("/api/").size(),this);

	//register the services
	//they "live" in the main thread in the QObject sense, but their service methods are actually
	//executed in the HTTP handler threads
	apiController->registerService(new MainService("main",apiController));
	apiController->registerService(new ObjectService("objects",apiController));
	apiController->registerService(new ScriptService("scripts",apiController));
	apiController->registerService(new SimbadService("simbad",apiController));
	apiController->registerService(new StelActionService("stelaction",apiController));
	apiController->registerService(new LocationService("location",apiController));
	apiController->registerService(new LocationSearchService("locationsearch",apiController));
	apiController->registerService(new ViewService("view",apiController));

	connect(&StelApp::getInstance(),SIGNAL(languageChanged()),this,SLOT(refreshHtmlTemplate()));

	staticFiles = new StaticFileController(settings,this);
	indexFilePath = staticFiles->getDocRoot() + "/index.html";
	transDataPath = staticFiles->getDocRoot() + "/translationdata.js";
	qDebug()<<"Index file located at "<<indexFilePath;

	refreshHtmlTemplate();
}

RequestHandler::~RequestHandler()
{

}

void RequestHandler::update(double deltaTime)
{
	apiController->update(deltaTime);
}

void RequestHandler::service(HttpRequest &request, HttpResponse &response)
{

#define SERVER_HEADER "Stellarium RemoteControl " REMOTECONTROL_VERSION
	response.setHeader("Server",SERVER_HEADER);

	//try to support keep-alive connections
	if(QString::compare(request.getHeader("Connection"),"keep-alive",Qt::CaseInsensitive)==0)
		response.setHeader("Connection","keep-alive");
	else
		response.setHeader("Connection","close");

	if(usePassword)
	{
		//Check if the browser provided correct password, else reject request
		if(request.getHeader("Authorization") != passwordReply)
		{
			response.setStatus(401,"Not Authorized");
			response.setHeader("WWW-Authenticate",AUTH_REALM);
			response.write("HTTP 401 Not Authorized",true);
			return;
		}
	}

	QByteArray rawPath = request.getRawPath();
	QByteArray path = request.getPath();
	qDebug()<<"Request path:"<<rawPath<<" decoded:"<<path;

	if(path.startsWith("/api/"))
		apiController->service(request,response);
	else
	{
#ifndef QT_NO_DEBUG
		if(path.startsWith("/external/"))
		{
			//let browser cache external stuff even if in development
			response.setHeader("Cache-Control","max-age=86400");
		}
#endif

		if(path.isEmpty() || path == "/" || path == "/index.html")
		{
			//special handling for indexFile
			response.setHeader("Content-Type", qPrintable("text/html; charset=UTF-8"));
#ifndef QT_NO_DEBUG
			//force fresh loading for each request
			refreshHtmlTemplate();
#endif
			response.write(indexFile.toUtf8(),true);
		}
		else if (path == "/translationdata.js")
		{
			response.setHeader("Content-Type", qPrintable("text/javascript; charset=UTF-8"));

#ifndef QT_NO_DEBUG
			//force fresh loading for each request
			refreshHtmlTemplate();
#endif

			response.write(transData.toUtf8(),true);
		}
		else
			staticFiles->service(request,response);
	}
}

void RequestHandler::setUsePassword(bool v)
{
	usePassword = v;
}

void RequestHandler::setPassword(const QString &pw)
{
	password = pw;

	//pre-create the expected response string
	QByteArray arr = password.toUtf8();
	arr.prepend(':');
	passwordReply = "Basic " + arr.toBase64();
}

void RequestHandler::refreshHtmlTemplate()
{
	StelTemplateTranslationProvider transProv;
	QFile file(indexFilePath);
	Template tmp(file);
	tmp.translate(transProv);
	indexFile = tmp;

	file.close();
	file.setFileName(transDataPath);
	Template tmp2(file);
	tmp2.translate(transProv);
	transData = tmp2;
}
