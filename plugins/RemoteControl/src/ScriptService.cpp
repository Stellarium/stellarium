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

#include "ScriptService.hpp"

#include "StelApp.hpp"
#include "StelScriptMgr.hpp"
#include "StelTranslator.hpp"

#include <QJsonDocument>

ScriptService::ScriptService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	//this is run in the main thread
	scriptMgr = &StelApp::getInstance().getScriptMgr();
}

void ScriptService::get(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	if(operation=="list")
	{
		//list all scripts, this should be thread safe
		QStringList allScripts = scriptMgr->getScriptList();

		writeJSON(QJsonDocument(QJsonArray::fromStringList(allScripts)),response);
	}
	else if (operation == "info")
	{
		if(parameters.contains("id"))
		{
			//retrieve detail about a single script
			QString scriptId = QString::fromUtf8(parameters.value("id"));

			QJsonObject obj;
			//if the script name is wrong, this will return empty strings
			obj.insert("id",scriptId);
			QString d = scriptMgr->getName(scriptId).trimmed();
			obj.insert("name",d);
			obj.insert("name_localized", StelTranslator::globalTranslator->qtranslate(d));
			d = scriptMgr->getDescription(scriptId).trimmed();
			obj.insert("description",d);
			obj.insert("description_localized", StelTranslator::globalTranslator->qtranslate(d));
			obj.insert("author",scriptMgr->getAuthor(scriptId).trimmed());
			obj.insert("license",scriptMgr->getLicense(scriptId).trimmed());
			//shortcut often causes a large delay because the whole file gets searched, and it is usually missing, so we ignore it
			//obj.insert("shortcut",scriptMgr->getShortcut(scriptId));

			writeJSON(QJsonDocument(obj),response);
		}
		else
		{
			writeRequestError("need parameter: id",response);
		}
	}
	else if(operation == "status")
	{
		//generic script status
		QJsonObject obj;
		obj.insert("scriptIsRunning",scriptMgr->scriptIsRunning());
		obj.insert("runningScriptId",scriptMgr->runningScriptId());

		writeJSON(QJsonDocument(obj),response);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: list,info,status POST: run,stop",response);
	}
}

void ScriptService::post(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	if(operation=="run")
	{
		//retrieve detail about a single script
		QString scriptId = QString::fromUtf8(parameters.value("id"));

		if(scriptMgr->scriptIsRunning())
		{
			response.write("error: a script is already running",true);
			return;
		}

		QString script;
		//Prepare the script. Through a refactor, this is separate from actually running the script,
		//and can be done in the HTTP thread. It can also notify the caller of an invalid script ID.
		bool ret = scriptMgr->prepareScript(script,scriptId);

		if(!ret)
		{
			response.write("error: could not prepare script, wrong id?",true);
			return;
		}

		//ret = scriptMgr->runScript(scriptId);
		//we are in another thread! for the actual execution of the script, we need to invoke the event queue
		//we can not use blocking connection here because runPreprocessedScript is blocking!
		//there is also no way to check if the script was actually started except for polling some time later
		QMetaObject::invokeMethod(scriptMgr,"runPreprocessedScript",Qt::QueuedConnection,Q_ARG(QString,script));
		response.write("ok",true);
	}
	else if(operation=="stop")
	{
		//scriptMgr->stopScript();
		//here a blocking connection should cause no problems
		QMetaObject::invokeMethod(scriptMgr,"stopScript",Qt::BlockingQueuedConnection);

		response.write("ok",true);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: list,info,status POST: run,stop",response);
	}
}
