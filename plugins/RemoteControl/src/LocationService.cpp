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

#include "LocationService.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocationMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelTranslator.hpp"
#include "SolarSystem.hpp"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringListModel>

LocationService::LocationService(QObject *parent) : AbstractAPIService(parent)
{
	//this is run in the main thread
	core = StelApp::getInstance().getCore();
	locMgr = &StelApp::getInstance().getLocationMgr();
	ssys = GETSTELMODULE(SolarSystem);
}

void LocationService::get(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if(operation=="list")
	{
		//for now, this does not return location objects (which would require a much larger data transfer), but only the location strings
		//same as in the LocationDialog list

		//TODO not fully thread safe
		QJsonArray list = QJsonArray::fromStringList(QStringList(locMgr->getAllMap().keys()));

		response.writeJSON(QJsonDocument(list));
	}
	else if(operation == "regionlist")
	{
		const StelTranslator& trans = *StelTranslator::globalTranslator;

		QStringList allRegions = StelApp::getInstance().getLocationMgr().getRegionNames();
		QJsonArray list;
		for (const auto &str : qAsConst(allRegions))
		{
			QJsonObject obj;
			obj.insert("name",str);
			obj.insert("name_i18n",trans.qtranslate(str));
			list.append(obj);
		}

		response.writeJSON(QJsonDocument(list));
	}
	else if(operation == "planetlist")
	{
		QList<PlanetP> ss = ssys->getAllPlanets();
		QJsonArray list;
		for (const auto& p : ss)
		{
			QJsonObject obj;
			obj.insert("name", p->getEnglishName());
			obj.insert("name_i18n", p->getNameI18n());
			list.append(obj);
		}

		response.writeJSON(QJsonDocument(list));
	}
	else if(operation=="planetimage")
	{
		//return the image file for the specified planet
		//logic from LocationDialog

		QString planet = QString::fromUtf8(parameters.value("planet"));

		if(planet.isEmpty())
		{
			response.writeRequestError("requires 'planet' parameter");
			return;
		}


		SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		PlanetP p = ssm->searchByEnglishName(planet);
		if (p)
		{
			QString path = StelFileMgr::findFile("textures/"+p->getTextMapName());
			response.writeFile(path, true);
		}
		else
		{
			response.setStatus(404,"not found");
			response.setData("planet id not found");
			qWarning() << "ERROR - could not find planet " << planet;
			return;
		}
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: list, countrylist, planetlist, planetimage");
	}
}

void LocationService::post(const QByteArray& operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{
	Q_UNUSED(data)

	if (operation == "setlocationfields")
	{
		QString id = QString::fromUtf8(parameters.value("id"));

		StelLocation loc;
		if(!id.isEmpty())
		{
			//if an id is set, ignore everything else
			//use an invoke here for reasons
			QMetaObject::invokeMethod(locMgr,"locationForString",SERVICE_DEFAULT_INVOKETYPE,
						  Q_RETURN_ARG(StelLocation, loc),
						  Q_ARG(QString,id));

			if(loc.isValid())
			{
				//set location
				QMetaObject::invokeMethod(core, "moveObserverTo", SERVICE_DEFAULT_INVOKETYPE,
							  Q_ARG(StelLocation, loc),
							  Q_ARG(double,0.0) );

				response.setData("ok");
				return;
			}
			else
			{
				response.writeRequestError("invalid location id");
				return;
			}
		}

		//copy current loc
		loc = core->getCurrentLocation();

		//similar to StelScriptMainAPI::setObserverLocation
		QString sLatitude = QString::fromUtf8(parameters.value("latitude"));
		QString sLongitude = QString::fromUtf8(parameters.value("longitude"));
		QString sAltitude = QString::fromUtf8(parameters.value("altitude"));

		QString name = QString::fromUtf8(parameters.value("name"));
		//QString country = QString::fromUtf8(parameters.value("country"));
		QString region = QString::fromUtf8(parameters.value("region"));
		QString planet = QString::fromUtf8(parameters.value("planet"));

		//check each field
		bool doneSomething = false;
		bool ok = false;
		float latitude = sLatitude.toFloat(&ok);
		if(ok && (latitude - loc.latitude) != 0.0f)
		{
			loc.latitude = latitude;
			doneSomething = true;
		}
		float longitude = sLongitude.toFloat(&ok);
		if(ok && (longitude - loc.longitude) != 0.0f)
		{
			loc.longitude = longitude;
			doneSomething = true;
		}
		int altitude = sAltitude.toInt(&ok);
		if(ok && altitude != loc.altitude)
		{
			loc.altitude = altitude;
			doneSomething = true;
		}
		if(parameters.contains("name") && name != loc.name)
		{
			loc.name = name;
			doneSomething = true;
		}
		if(!region.isEmpty() && region != loc.region)
		{
			loc.region = region;
			doneSomething = true;
		}

		if(!planet.isEmpty() && loc.planetName != planet)
		{
			if(ssys->searchByName(planet))
			{
				//set planet
				loc.planetName = planet;
				doneSomething = true;
			}
		}

		if(doneSomething)
		{
			//update the core
			QMetaObject::invokeMethod(core,"moveObserverTo", SERVICE_DEFAULT_INVOKETYPE,
						  Q_ARG(StelLocation, loc),
						  Q_ARG(double, 0.0),
						  Q_ARG(double, 0.0));
		}

		response.setData("ok");
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. POST: setlocation, setlocationfields");
	}
}
