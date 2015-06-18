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

LocationService::LocationService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	//this is run in the main thread
	core = StelApp::getInstance().getCore();
	locMgr = &StelApp::getInstance().getLocationMgr();
	ssys = GETSTELMODULE(SolarSystem);
}

void LocationService::get(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	if(operation=="list")
	{
		//for now, this does not return location objects (which would require a much larger data transfer), but only the location strings
		//same as in the LocationDialog list

		//TODO not fully thread safe
		QStringListModel* mdl = locMgr->getModelAll();
		QJsonArray list = QJsonArray::fromStringList(mdl->stringList());

		writeJSON(QJsonDocument(list),response);
	}
	else if(operation == "countrylist")
	{
		const StelTranslator& trans = *StelTranslator::globalTranslator;

		QStringList allCountries = StelApp::getInstance().getLocaleMgr().getAllCountryNames();
		QJsonArray list;
		foreach(QString str, allCountries)
		{
			QJsonObject obj;
			obj.insert("name",str);
			obj.insert("name_i18n",trans.qtranslate(str));
			list.append(obj);
		}

		writeJSON(QJsonDocument(list),response);
	}
	else if(operation == "planetlist")
	{
		const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

		QStringList allPlanets = ssys->getAllPlanetEnglishNames();
		QJsonArray list;
		foreach(QString str, allPlanets)
		{
			QJsonObject obj;
			obj.insert("name",str);
			obj.insert("name_i18n",trans.qtranslate(str));
			list.append(obj);
		}

		writeJSON(QJsonDocument(list),response);
	}
	else if(operation=="planetimage")
	{
		//return the image file for the specified planet
		//logic from LocationDialog

		QString planet = QString::fromUtf8(parameters.value("planet"));

		if(planet.isEmpty())
		{
			writeRequestError("requires 'planet' parameter",response);
			return;
		}


		SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		PlanetP p = ssm->searchByEnglishName(planet);
		if (p)
		{
			QString path = StelFileMgr::findFile("textures/"+p->getTextMapName());
			QFile file(path);
			if (path.isEmpty() || !file.exists())
			{
				response.setStatus(404,"not found");
				response.write("planet image not available",true);
				qWarning() << "ERROR - could not find planet map for " << planet;
				return;
			}

			QMimeType mime = QMimeDatabase().mimeTypeForFile(path);

			if(file.open(QIODevice::ReadOnly))
			{
				//allow the image to be cached by browser (1 hour)
				response.setHeader("Cache-Control","max-age="+QByteArray::number(60*60));
				response.setHeader("Content-Length",static_cast<int>(file.size()));
				if(!mime.isDefault())
				{
					response.setHeader("Content-Type", mime.name().toLatin1());
				}

				//load and write data
				response.write(file.readAll(),true);
			}
			else
			{
				response.setStatus(500,"internal server error");
				response.write("could not open image file",true);
			}
		}
		else
		{
			response.setStatus(404,"not found");
			response.write("planet id not found",true);
			qWarning() << "ERROR - could not find planet " << planet;
			return;
		}
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: list, countrylist, planetlist, planetimage",response);
	}
}

void LocationService::post(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, HttpResponse &response)
{
	//this is run in an HTTP worker thread
	if (operation == "setlocationfields")
	{
		QString id = QString::fromUtf8(parameters.value("id"));

		StelLocation loc;
		if(!id.isEmpty())
		{
			//if an id is set, ignore everything else
			//use an invoke here for reasons
			QMetaObject::invokeMethod(locMgr,"locationForString",Qt::BlockingQueuedConnection,
						  Q_RETURN_ARG(StelLocation, loc),
						  Q_ARG(QString,id));

			if(loc.isValid())
			{
				//set location
				QMetaObject::invokeMethod(core, "moveObserverTo", Qt::BlockingQueuedConnection,
							  Q_ARG(StelLocation, loc),
							  Q_ARG(double,0.0) );

				response.write("ok",true);
				return;
			}
			else
			{
				writeRequestError("invalid location id", response);
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
		QString country = QString::fromUtf8(parameters.value("country"));
		QString planet = QString::fromUtf8(parameters.value("planet"));

		//check each field
		bool doneSomething = false;
		bool ok = false;
		float latitude = sLatitude.toFloat(&ok);
		if(ok && latitude != loc.latitude)
		{
			loc.latitude = latitude;
			doneSomething = true;
		}
		float longitude = sLongitude.toFloat(&ok);
		if(ok && longitude != loc.longitude)
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
		if(!country.isEmpty() && country != loc.country)
		{
			loc.country = country;
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
			QMetaObject::invokeMethod(core,"moveObserverTo", Qt::BlockingQueuedConnection,
						  Q_ARG(StelLocation, loc),
						  Q_ARG(double, 0.0),
						  Q_ARG(double, 0.0));
		}

		response.write("ok",true);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. POST: setlocation, setlocationfields",response);
	}
}
