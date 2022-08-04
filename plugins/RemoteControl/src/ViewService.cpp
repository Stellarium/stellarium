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
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelTranslator.hpp"
#include "LandscapeMgr.hpp"

#include <QFile>
#include <QMimeDatabase>
#include <QJsonArray>
#include <QJsonDocument>

ViewService::ViewService(QObject *parent) : AbstractAPIService(parent)
{
	core = StelApp::getInstance().getCore();
	lsMgr = GETSTELMODULE(LandscapeMgr);
	skyCulMgr = &StelApp::getInstance().getSkyCultureMgr();
}

void ViewService::get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse &response)
{
	Q_UNUSED(parameters)

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
	else if (operation.startsWith("landscapedescription/"))
	{
		int startidx = operation.indexOf('/');
		//get the path after the name and map it to the landscapes' directory
		QByteArray path = operation.mid(startidx+1);

		if(path.isEmpty())
		{
			//return the HTML description of the current landscape
			QString str = lsMgr->getCurrentLandscapeHtmlDescription();
			response.writeWrappedHTML(str, lsMgr->getCurrentLandscapeName());
		}
		else
		{
			//map path to landscape dir and return files
			QString baseFolder = StelFileMgr::findFile("landscapes/" + lsMgr->getCurrentLandscapeID());
			QString pathString = baseFolder + '/' + QString::fromUtf8(path);

			response.writeFile(pathString);
		}
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
	else if (operation.startsWith("skyculturedescription/"))
	{
		int startidx = operation.indexOf('/');
		//get the path after the name and map it to the sky cultures' directory
		QByteArray path = operation.mid(startidx+1);

		if(path.isEmpty())
		{
			//return the HTML description of the current landscape
			QString str = skyCulMgr->getCurrentSkyCultureHtmlDescription();
			response.writeWrappedHTML(str, skyCulMgr->getCurrentSkyCultureNameI18());
		}
		else
		{
			//map path to sky cultures dir and return files
			QString baseFolder = StelFileMgr::findFile("skycultures/" + skyCulMgr->getCurrentSkyCultureID());
			QString pathString = baseFolder + '/' + QString::fromUtf8(path);

			response.writeFile(pathString);
		}
	}
	else if (operation=="listprojection")
	{
		//list projection types
		const QStringList keys = core->getAllProjectionTypeKeys();

		QJsonObject obj;

		for (auto &str : keys)
		{
			QString name = core->projectionTypeKeyToNameI18n(str);
			obj.insert(str,name);
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else if (operation=="projectiondescription")
	{
		//returns the description of the current projection
		QString str = core->getProjection(StelCore::FrameJ2000)->getHtmlSummary();
		response.writeWrappedHTML(str, core->getCurrentProjectionNameI18n());
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: listlandscape,landscapedescription/,listskyculture,skyculturedescription/,listprojection,projectiondescription");
	}
}
