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

#ifdef ENABLE_SCRIPTING

#include "ScriptService.hpp"

#include "StelApp.hpp"
#include "StelScriptMgr.hpp"
#include "StelTranslator.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

ScriptService::ScriptService(QObject *parent) : AbstractAPIService(parent)
{
	//this is run in the main thread
	scriptMgr = &StelApp::getInstance().getScriptMgr();
}

void ScriptService::get(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if(operation=="list")
	{
		//list all scripts, this should be thread safe
		QStringList allScripts = scriptMgr->getScriptList();

		response.writeJSON(QJsonDocument(QJsonArray::fromStringList(allScripts)));
	}
	else if (operation == "info")
	{
		if(parameters.contains("id"))
		{
			//retrieve detail about a single script
			QString scriptId = QString::fromUtf8(parameters.value("id"));

			if(parameters.contains("html"))
			{
				QString html = scriptMgr->getHtmlDescription(scriptId, false);
				response.writeWrappedHTML(html, scriptId);
				return;
			}

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
			obj.insert("version",scriptMgr->getVersion(scriptId).trimmed());
			//shortcut often causes a large delay because the whole file gets searched, and it is usually missing, so we ignore it
			//obj.insert("shortcut",scriptMgr->getShortcut(scriptId));

			response.writeJSON(QJsonDocument(obj));
		}
		else
		{
			response.writeRequestError("need parameter: id");
		}
	}
	else if(operation == "status")
	{
		//generic script status
		QJsonObject obj;
		obj.insert("scriptIsRunning",scriptMgr->scriptIsRunning());
		obj.insert("runningScriptId",scriptMgr->runningScriptId());

		response.writeJSON(QJsonDocument(obj));
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: list,info,status POST: run,stop");
	}
}

void ScriptService::post(const QByteArray& operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{
	Q_UNUSED(data)

	if(operation=="run")
	{
		//retrieve detail about a single script
		QString scriptId = QString::fromUtf8(parameters.value("id"));

		if(scriptMgr->scriptIsRunning())
		{
			response.setData("error: a script is already running");
			return;
		}

		QString script;
		//Prepare the script. Through a refactor, this is separate from actually running the script,
		//and can be done in the HTTP thread. It can also notify the caller of an invalid script ID.
		bool ret = scriptMgr->prepareScript(script,scriptId);

		if(!ret)
		{
			response.setData("error: could not prepare script, wrong id?");
			return;
		}

		//ret = scriptMgr->runScript(scriptId);
		//we may be in another thread! for the actual execution of the script, we need to invoke the event queue
		//we can not use blocking connection here because runPreprocessedScript is blocking!
		//there is also no way to check if the script was actually started except for polling some time later
		QMetaObject::invokeMethod(scriptMgr,"runPreprocessedScript",
					  Qt::QueuedConnection,Q_ARG(QString,script),Q_ARG(QString,scriptId));
		response.setData("ok");
	}
	else if(operation=="direct")
	{
		//directly runs the given script code
		QString code = QString::fromUtf8(parameters.value("code"));
		QString useIncludes = QString::fromUtf8(parameters.value("useIncludes"));

		if(code.isEmpty())
		{
			response.writeRequestError("need parameter: code");
		}
		else
		{
			if(scriptMgr->scriptIsRunning())
			{
				response.setData("error: a script is already running");
				return;
			}

			//use QVariant logic to convert to bool
			bool bUseIncludes = QVariant(useIncludes).toBool();

			//also requires queued connection
			QMetaObject::invokeMethod(scriptMgr,"runScriptDirect", Qt::QueuedConnection,
						  Q_ARG(QString,code),
						  Q_ARG(QString,bUseIncludes ? QStringLiteral("") : QString() ));
			response.setData("ok");
		}
	}
	else if(operation=="stop")
	{
		//scriptMgr->stopScript();
		QMetaObject::invokeMethod(scriptMgr,"stopScript",SERVICE_DEFAULT_INVOKETYPE);

		response.setData("ok");
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: list,info,status POST: run,direct,stop");
	}
}

#endif
