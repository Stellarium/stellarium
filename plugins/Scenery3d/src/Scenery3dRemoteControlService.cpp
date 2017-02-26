/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2017 Florian Schaukowitsch
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

#include "Scenery3dRemoteControlService.hpp"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

Scenery3dRemoteControlService::Scenery3dRemoteControlService()
{
	s3d = GETSTELMODULE(Scenery3d);
}

QLatin1String Scenery3dRemoteControlService::getPath() const
{
	return QLatin1String("scenery3d");
}

bool Scenery3dRemoteControlService::isThreadSafe() const
{
	return false;
}

void Scenery3dRemoteControlService::update(double deltaTime)
{
	Q_UNUSED(deltaTime);
}

void Scenery3dRemoteControlService::get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if(operation == "listscene")
	{
		QMap<QString,QString> map = SceneInfo::getNameToIDMap();

		QJsonObject obj;

		for(QMap<QString,QString>::const_iterator it = map.constBegin(); it!=map.constEnd();++it)
		{
			obj.insert(it.value(), StelTranslator::globalTranslator->qtranslate(it.key()));
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else if (operation.startsWith("scenedescription/"))
	{
		//extract scene id
		int startidx = operation.indexOf('/');
		int endidx = operation.indexOf('/', startidx+1);

		if(endidx == -1)
		{
			response.writeRequestError("requires /-separated scene ID");
			return;
		}

		QString id = operation.mid(startidx+1,(endidx - startidx - 1));
		SceneInfo si;
		SceneInfo::loadByID(id,si);

		if(!si.isValid)
		{
			response.writeRequestError("invalid scene ID");
			return;
		}

		// allow caching of response
		response.setCacheTime(60*60);

		//get the path after the name and map it to the scene's directory
		QByteArray path = operation.mid(endidx+1);
		if(path.isEmpty())
		{
			//no path, return HTML description
			//first, try to find a localized description
			QString desc = si.getLocalizedHTMLDescription();
			if(desc.isEmpty())
			{
				//use the "old" way to create an description from data contained in the .ini
				desc = QString("<h3>%1</h3>").arg(si.name);
				desc+= si.description;
				desc+="<br><br>";
				desc+="<b>"+q_("Author: ")+"</b>";
				desc+= si.author;
			}
			response.writeWrappedHTML(desc,si.name);
		}
		else
		{
			//map path to scene dir and return files
			QString baseFolder = si.fullPath;
			QString pathString = baseFolder + '/' + QString::fromUtf8(path);

			response.writeFile(pathString);
		}
	}
	else
	{
		response.writeRequestError("unsupported operation. GET: listscene,scenedescription/");
	}
}

void Scenery3dRemoteControlService::post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{

	if(operation == "loadscene")
	{
		QString id(parameters.value("id"));

		if(id.isEmpty())
		{
			response.writeRequestError("requires valid id parameter");
			return;
		}

		if(id == s3d->getCurrentSceneID())
		{
			if(s3d->getLoadingSceneID().isEmpty())
			{
				response.writeRequestError("scene already loaded");
				return;
			}
		}
		else if(id == s3d->getLoadingSceneID())
		{
			response.writeRequestError("scene already being loaded");
			return;
		}

		SceneInfo si = s3d->loadScenery3dByID(id);
		if(si.isValid)
			response.setData("ok");
		else
			response.writeRequestError("invalid id parameter");
	}
	else
	{
		response.writeRequestError("unsupported operation. POST: loadscene");
	}
}
