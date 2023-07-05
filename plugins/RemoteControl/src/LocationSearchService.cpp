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

#include "LocationSearchService.hpp"

#include "StelApp.hpp"
#include "StelLocationMgr.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

LocationSearchService::LocationSearchService(QObject *parent)
	: AbstractAPIService(parent), locMgr(LocationList())
{
	//this is run in the main thread
	connect(&StelApp::getInstance().getLocationMgr(), SIGNAL(locationListChanged()), this, SLOT(mainLocationManagerUpdated()));
	mainLocationManagerUpdated();
}

void LocationSearchService::mainLocationManagerUpdated()
{
	//this is run in the main thread
	locMgrMutex.lock();
	//copy the contents of the location manager
	locMgr.setLocations(StelApp::getInstance().getLocationMgr().getAll());
	locMgrMutex.unlock();
}

void LocationSearchService::get(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if(operation=="search")
	{
		//parameter must be named "term" to be compatible with jQuery UI autocomplete without further JS code
		QString term = QString::fromUtf8(parameters.value("term"));

		if(term.isEmpty())
		{
			response.writeRequestError("needs non-empty 'term' parameter");
			return;
		}

		//the filtering in the app is provided by QSortFilterProxyModel in the view
		//we dont have that luxury, but we make sure the filtering happens in the separate HTTP thread
		locMgrMutex.lock();
		LocationMap allItems = locMgr.getAllMap();
		locMgrMutex.unlock();

		QJsonArray results;
		const QList<QString>& list = allItems.keys();

		//use a regexp in wildcard mode, the app does the same
		QRegularExpression exp(QRegularExpression::wildcardToRegularExpression(term), QRegularExpression::CaseInsensitiveOption);
		for(const auto& str : list)
		{
			if (str.contains(exp))
				results.append(str);
		}
		response.writeJSON(QJsonDocument(results));
	}
	else if(operation=="nearby")
	{
		QString sPlanet = QString::fromUtf8(parameters.value("planet"));
		QString sLatitude = QString::fromUtf8(parameters.value("latitude"));
		QString sLongitude = QString::fromUtf8(parameters.value("longitude"));
		QString sRadius = QString::fromUtf8(parameters.value("radius"));

		float latitude = sLatitude.toFloat();
		float longitude = sLongitude.toFloat();
		float radius = sRadius.toFloat();

		locMgrMutex.lock();
		LocationMap results = locMgr.pickLocationsNearby(sPlanet,longitude,latitude,radius);
		locMgrMutex.unlock();

		response.writeJSON(QJsonDocument(QJsonArray::fromStringList(results.keys())));
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: search,nearby");
	}
}
