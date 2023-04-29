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
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"

#include <QDir>
#include <QFile>
#include <QPluginLoader>

const QByteArray RequestHandler::AUTH_REALM = "Basic realm=\"Stellarium remote control\"";

class HtmlTranslationProvider : public ITemplateTranslationProvider
{
public:
	HtmlTranslationProvider(StelTranslator* localInstance)
	{
		rcTranslator = localInstance;
	}
	QString getTranslation(const QString &key) Q_DECL_OVERRIDE
	{
		//try to get a RemoteControl specific translation first
		QString trans = rcTranslator->tryQtranslate(key);
		if(trans.isNull())
			trans = StelTranslator::globalTranslator->qtranslate(key);
		//HTML escape + single quote escape
		return trans.toHtmlEscaped().replace('\'',"&#39;");
	}
private:
	StelTranslator* rcTranslator;
};

class JsTranslationProvider : public ITemplateTranslationProvider
{
public:
	JsTranslationProvider(StelTranslator* localInstance)
	{
		rcTranslator = localInstance;
	}

	QString getTranslation(const QString &key) Q_DECL_OVERRIDE
	{
		//try to get a RemoteControl specific translation first
		QString trans = rcTranslator->tryQtranslate(key);
		if(trans.isNull())
			trans = StelTranslator::globalTranslator->qtranslate(key);
		//JS escape single/double quotes
		return trans.replace('\'',"\\'").replace('"',"\\\"");
	}
private:
	StelTranslator* rcTranslator;
};

#if (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
RequestHandler::RequestHandler(const StaticFileControllerSettings& settings, QObject* parent) : HttpRequestHandler(parent), usePassword(false), enableCors(false), templateMutex()
#else
RequestHandler::RequestHandler(const StaticFileControllerSettings& settings, QObject* parent) : HttpRequestHandler(parent), usePassword(false), enableCors(false), templateMutex(QMutex::Recursive)
#endif
{
	apiController = new APIController(QByteArray("/api/").size(),this);

	//register the services
	//they "live" in the main thread in the QObject sense, but their service methods are actually
	//executed in the HTTP handler threads
	apiController->registerService(new MainService(apiController));
	apiController->registerService(new ObjectService(apiController));
#ifdef ENABLE_SCRIPTING
	apiController->registerService(new ScriptService(apiController));
#endif
	apiController->registerService(new SimbadService(apiController));
	apiController->registerService(new StelActionService(apiController));
	apiController->registerService(new StelPropertyService(apiController));
	apiController->registerService(new LocationService(apiController));
	apiController->registerService(new LocationSearchService(apiController));
	apiController->registerService(new ViewService(apiController));

	connect(&StelApp::getInstance().getModuleMgr(), SIGNAL(extensionsAdded(QObjectList)), this, SLOT(addExtensionServices(QObjectList)));
	addExtensionServices(StelApp::getInstance().getModuleMgr().getExtensionList());

	staticFiles = new StaticFileController(settings,this);
	connect(&StelApp::getInstance(),SIGNAL(languageChanged()),this,SLOT(refreshTemplates()));
	refreshTemplates();
}

RequestHandler::~RequestHandler()
{
}

void RequestHandler::addExtensionServices(QObjectList services)
{
	for (auto* obj : services)
	{
		RemoteControlServiceInterface* sv = qobject_cast<RemoteControlServiceInterface*>(obj);
		if(sv)
		{
			qDebug()<<"Registering RemoteControl extension service:"<<sv->getPath();
			apiController->registerService(sv);
		}
	}
}

void RequestHandler::update(double deltaTime)
{
	apiController->update(deltaTime);
}

void RequestHandler::service(HttpRequest &request, HttpResponse &response)
{
#define SERVER_HEADER "Stellarium RemoteControl " REMOTECONTROL_PLUGIN_VERSION
	response.setHeader("Server",SERVER_HEADER);

	if(QString::compare(request.getMethod(),"OPTIONS",Qt::CaseInsensitive)==0) {
		handleCorsPreflightRequest(request,response);
		return;
	}

	//try to support keep-alive connections
	if(QString::compare(request.getHeader("Connection"),"keep-alive",Qt::CaseInsensitive)==0)
		response.setHeader("Connection","keep-alive");
	else
		response.setHeader("Connection","close");

	if(enableCors)
	{
		response.setHeader("Access-Control-Allow-Origin",corsOrigin.toUtf8());
		response.setHeader("Access-Control-Allow-Methods","GET, PUT, POST, HEAD, OPTIONS");
		response.setHeader("Vary","Origin");
	}

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

		//make sure we can access the template map
		templateMutex.lock();
		if(templateMap.contains(path))
		{
#ifndef QT_NO_DEBUG
			//force fresh loading for each request in debug mode
			//to allow for immediate display of changes
			refreshTemplates();
#endif
			QByteArray content = templateMap[path].toUtf8();
			templateMutex.unlock();

			//get a mime type
			QByteArray mime = StaticFileController::getContentType(path,"utf-8");
			if(!mime.isEmpty())
				response.setHeader("Content-Type",mime);

			//serve the stored template
			response.write(content,true);
		}
		else
		{
			templateMutex.unlock();
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

void RequestHandler::setEnableCors(bool v)
{
	enableCors = v;
}

void RequestHandler::setCorsOrigin(const QString &origin)
{
	corsOrigin = origin;
}


void RequestHandler::refreshTemplates()
{
	//multiple threads can potentially enter here,
	//so this requires locking
	QMutexLocker locker(&templateMutex);
	//remove old translations
	templateMap.clear();
	//create a translator for remote control specific stuff, with the current language
	StelTranslator rcTrans("stellarium-remotecontrol",StelTranslator::globalTranslator->getTrueLocaleName());
	JsTranslationProvider jsTranslator(&rcTrans);
	HtmlTranslationProvider htmlTranslator(&rcTrans);

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
				//use the HTML escapes by default,
				//but use JS escapes for js files
				ITemplateTranslationProvider* transProv = &htmlTranslator;
				if(line.endsWith(".js"))
					transProv = &jsTranslator;

				Template tmp(f);
				tmp.translate(*transProv);
				//check if the file was correctly loaded
				if(!tmp.isEmpty())
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

void RequestHandler::handleCorsPreflightRequest(HttpRequest &request, HttpResponse &response) {
	response.setStatus(204,"No Content");
	response.setHeader("Access-Control-Allow-Origin",corsOrigin.toUtf8());
	response.setHeader("Access-Control-Allow-Methods","GET, POST");
	response.setHeader("Access-Control-Allow-Headers",request.getHeader("Access-Control-Request-Headers"));
	response.setHeader("Access-Control-Max-Age","0");
	response.setHeader("Vary","Origin");
}
