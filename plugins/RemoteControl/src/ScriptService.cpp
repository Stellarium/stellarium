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

void ScriptService::get(const QList<QByteArray>& args, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	if(args.isEmpty())
	{
		//list all scripts, this should be thread safe
		QStringList allScripts = scriptMgr->getScriptList();

		writeJSON(QJsonDocument(QJsonArray::fromStringList(allScripts)),response);
	}
	else if (args.size() == 1)
	{
		//retrieve detail about a single script
		QString scriptId = QString::fromUtf8(args[0]);

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
		writeRequestError("invalid number of arguments",response);
	}
}
