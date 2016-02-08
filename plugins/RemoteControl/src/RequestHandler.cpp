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
#include "StelPropertyService.hpp"
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
	apiController->registerService(new StelPropertyService("stelproperty",apiController));
	apiController->registerService(new LocationService("location",apiController));
	apiController->registerService(new LocationSearchService("locationsearch",apiController));
	apiController->registerService(new ViewService("view",apiController));

	staticFiles = new StaticFileController(settings,this);
	connect(&StelApp::getInstance(),SIGNAL(languageChanged()),this,SLOT(refreshHtmlTemplate()));
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

	//QByteArray rawPath = request.getRawPath();
	QByteArray path = request.getPath();
	//qDebug()<<"Request path:"<<rawPath<<" decoded:"<<path;

	if(path.startsWith("/api/"))
	{
		//this is an API request, pass it on
		apiController->service(request,response);
	}
	else
	{
		if(path.isEmpty() || path == "/" || path == "/index.html")
		{
			//transparently redirect to index.html
			path = "/index.html";
		}

		if(templateMap.contains(path))
		{
#ifndef QT_NO_DEBUG
			//force fresh loading for each request in debug mode
			//to allow for immediate display of changes
			refreshHtmlTemplate();
#endif
			//get a mime type
			QByteArray mime = StaticFileController::getContentType(path,"utf-8");
			if(!mime.isEmpty())
				response.setHeader("Content-Type",mime);

			//serve the stored template
			response.write(templateMap[path].toUtf8(),true);
		}
		else
		{
			//let the static file controller handle the request
			staticFiles->service(request,response);
		}
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
	//remove old tranlations
	templateMap.clear();

	StelTemplateTranslationProvider transProv;
	QDir docRoot = QDir(staticFiles->getDocRoot());
	//load the translate_files list
	QFile transFileList(docRoot.absoluteFilePath("translate_files"));
	if(transFileList.open(QFile::ReadOnly))
	{
		QTextStream text(&transFileList);
		//read line by line, ignoring whitespace and comments
		while(!text.atEnd())
		{
			QString line = text.readLine().trimmed();
			if(line.isEmpty() || line.startsWith('#'))
				continue;

			//load file and translate
			QFile f(docRoot.absoluteFilePath(line));
			if(f.exists())
			{
				Template tmp(f);
				tmp.translate(transProv);
				//check if the file was correctly loaded
				if(tmp.size()>0)
				{
					templateMap.insert('/'+line.toUtf8(),tmp);
				}
			}
			else
				qWarning()<<"[RemoteControl] Translatable file"<<f.fileName()<<"does not exist!";
		}
		transFileList.close();
	}
	else
	{
		qWarning()<<"[RemoteControl] "<<transFileList.fileName()<<" could not be opened, can not automatically translate files with StelTranslator!";
	}
}
