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
#include "LandscapeMgr.hpp"

#include <QJsonArray>
#include <QJsonDocument>

ViewService::ViewService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	core = StelApp::getInstance().getCore();
	lsMgr = GETSTELMODULE(LandscapeMgr);
	skyCulMgr = &StelApp::getInstance().getSkyCultureMgr();
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
			obj.insert(it.value(),it.key());
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else if (operation=="listskyculture")
	{
		//list installed skycultures
		QStringList names = skyCulMgr->getSkyCultureListI18();
		names.sort();

		response.writeJSON(QJsonDocument(QJsonArray::fromStringList(names)));
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
