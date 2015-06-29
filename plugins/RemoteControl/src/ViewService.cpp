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

#include "ViewService.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelTranslator.hpp"
#include "LandscapeMgr.hpp"

#include <QJsonArray>
#include <QJsonDocument>

ViewService::ViewService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	core = StelApp::getInstance().getCore();
	lsMgr = GETSTELMODULE(LandscapeMgr);
	skyCulMgr = &StelApp::getInstance().getSkyCultureMgr();
}

QString ViewService::wrapHtml(QString text) const
{
	const QString head = "<html><head>"
			     "<link type=\"text/css\" rel=\"stylesheet\" href=\"/iframestyle.css\">"
			     "<base target=\"_blank\">"
			     "</head><body>";
	const QString tail = "</body></html>";

	return text.prepend(head).append(tail);
}

void ViewService::getImpl(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if(operation=="listlandscape")
	{
		//list all installed landscapes

		QMap<QString,QString> map = lsMgr->getNameToDirMap();
		QJsonObject obj;

		QMapIterator<QString,QString> it(map);
		while(it.hasNext())
		{
			it.next();
			//value is the id here, key is name
			obj.insert(it.value(), StelTranslator::globalTranslator->qtranslate(it.key()));
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else if (operation=="landscapedescription")
	{
		//return the HTML description of the current landscape
		QString str = lsMgr->getCurrentLandscapeHtmlDescription();
		response.setHeader("Content-Type","text/html; charset=UTF-8");
		response.setData(wrapHtml(str).toUtf8());
	}
	else if (operation=="listskyculture")
	{
		//list installed skycultures
		QMap<QString, StelSkyCulture> map = skyCulMgr->getDirToNameMap();

		QJsonObject obj;
		QMapIterator<QString,StelSkyCulture> it(map);
		while(it.hasNext())
		{
			it.next();
			obj.insert(it.key(),StelTranslator::globalTranslator->qtranslate(it.value().englishName));
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else if (operation=="skyculturedescription")
	{
		//return the HTML description of the current landscape
		QString str = skyCulMgr->getCurrentSkyCultureHtmlDescription();
		response.setHeader("Content-Type","text/html; charset=UTF-8");
		response.setData(wrapHtml(str).toUtf8());
	}
	else if (operation=="listprojection")
	{
		//list projection types
		QStringList keys = core->getAllProjectionTypeKeys();

		QJsonObject obj;

		foreach(QString str,keys)
		{
			QString name = core->projectionTypeKeyToNameI18n(str);
			obj.insert(str,name);
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: listlandscape,listskyculture,listprojection");
	}
}

void ViewService::postImpl(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{
	if(operation == "setlandscape")
	{
		QString id = QString::fromUtf8(parameters.value("id"));

		if(id.isEmpty())
		{
			response.writeRequestError("missing id parameter");
			return;
		}

		bool ok;
		QMetaObject::invokeMethod(lsMgr,"setCurrentLandscapeID", SERVICE_DEFAULT_INVOKETYPE,
					  Q_RETURN_ARG(bool, ok),
					  Q_ARG(QString,id),
					  Q_ARG(double,1.0));

		if(ok)
			response.setData("ok");
		else
			response.setData("error: landscape not found");
	}
	else if (operation == "setskyculture")
	{
		QString id = QString::fromUtf8(parameters.value("id"));

		if(id.isEmpty())
		{
			response.writeRequestError("missing id parameter");
			return;
		}

		bool ok;
		QMetaObject::invokeMethod(skyCulMgr,"setCurrentSkyCultureID", SERVICE_DEFAULT_INVOKETYPE,
					  Q_RETURN_ARG(bool, ok),
					  Q_ARG(QString,id));

		if(ok)
			response.setData("ok");
		else
			response.setData("error: skyculture not found");
	}
	else if(operation=="setprojection")
	{
		QString type = QString::fromUtf8(parameters.value("type"));

		if(type.isEmpty())
		{
			response.writeRequestError("missing type parameter");
			return;
		}

		QMetaObject::invokeMethod(core,"setCurrentProjectionTypeKey", SERVICE_DEFAULT_INVOKETYPE,
					  Q_ARG(QString,type));

		response.setData("ok");
	}
	else
	{
		response.writeRequestError("unsupported operation. POST: setprojection");
	}
}
