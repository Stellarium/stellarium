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

#include "MainService.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelScriptMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QJsonDocument>

MainService::MainService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	//this is run in the main thread
	core = StelApp::getInstance().getCore();
	localeMgr = &StelApp::getInstance().getLocaleMgr();
	objMgr = &StelApp::getInstance().getStelObjectMgr();
	mvmgr = GETSTELMODULE(StelMovementMgr);
	scriptMgr = &StelApp::getInstance().getScriptMgr();

	Q_ASSERT(this->thread()==objMgr->thread());
}

void MainService::get(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	if(operation=="status")
	{
		//a listing of the most common stuff that can change often

		QJsonObject obj;

		//// Location
		const StelLocation& loc = core->getCurrentLocation();
		{
			QJsonObject obj2;
			obj2.insert("name",loc.name);
			obj2.insert("role",QString(loc.role));
			obj2.insert("planet",loc.planetName);
			obj2.insert("latitude",loc.latitude);
			obj2.insert("longitude",loc.longitude);
			obj2.insert("altitude",loc.altitude);
			obj2.insert("country",loc.country);
			obj2.insert("state",loc.state);
			obj2.insert("landscapeKey",loc.landscapeKey);
			obj.insert("location",obj2);
		}

		//// Time related stuff
		{
			double jday = core->getJDay();
			double deltaT = 0;
			if(loc.planetName == "Earth")
			{
				//apply deltaT on Earth
				deltaT = core->getDeltaT(jday) * StelCore::JD_SECOND;
			}
			double correctedTime = jday - deltaT;
			double gmtShift = localeMgr->getGMTShift(correctedTime) / 24.0;

			QString utcIso = StelUtils::julianDayToISO8601String(correctedTime,true).append('Z');
			QString localIso = StelUtils::julianDayToISO8601String(correctedTime+gmtShift,true);

			//time zone string
			QString timeZone = localeMgr->getPrintableTimeZoneLocal(correctedTime);

			QJsonObject obj2;
			obj2.insert("jday",jday);
			obj2.insert("deltaT",deltaT);
			obj2.insert("gmtShift",gmtShift);
			obj2.insert("timeZone",timeZone);
			obj2.insert("utc",utcIso);
			obj2.insert("local",localIso);
			obj2.insert("isTimeNow",core->getIsTimeNow());
			obj2.insert("timerate",core->getTimeRate());
			obj.insert("time",obj2);
		}

		//// Info about current script (same as ScriptService/status)
		{
			QJsonObject obj2;
			obj2.insert("scriptIsRunning",scriptMgr->scriptIsRunning());
			obj2.insert("runningScriptId",scriptMgr->runningScriptId());
			obj.insert("script",obj2);
		}

		writeJSON(QJsonDocument(obj),response);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: status",response);
	}
}

void MainService::post(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, HttpResponse &response)
{
	//this is run in an HTTP worker thread
	if(operation == "time")
	{
		bool doneSomething = false;
		bool ok;

		//set the time + timerate
		{
			const QByteArray& raw = parameters.value("time");
			if(!raw.isEmpty())
			{
				//parse time and set it
				double jday = QString(raw).toDouble(&ok);
				if(ok)
				{
					doneSomething = true;
					//set new time
					QMetaObject::invokeMethod(core,"setJDay", Qt::BlockingQueuedConnection,
								  Q_ARG(double,jday));
				}
			}
		}
		{
			const QByteArray& raw = parameters.value("timerate");
			if(!raw.isEmpty())
			{
				//parse timerate and set it
				double rate = QString(raw).toDouble(&ok);
				if(ok)
				{
					doneSomething = true;
					//set new time rate
					QMetaObject::invokeMethod(core,"setTimeRate", Qt::BlockingQueuedConnection,
								  Q_ARG(double,rate));
				}
			}
		}

		if(doneSomething)
			response.write("ok",true);
		else
			response.write("error: invalid parameters, use time/timerate as double values");
	}
	else if(operation == "focus")
	{
		QString target = QString::fromUtf8(parameters.value("target"));

		if(target.isEmpty())
		{
			writeRequestError("missing target parameter",response);
			return;
		}

		bool result;
		QMetaObject::invokeMethod(this,"focusObject",Qt::BlockingQueuedConnection,
					  Q_RETURN_ARG(bool,result),
					  Q_ARG(QString,target));

		response.write(result ? "true" : "false", true);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. POST: time,focus",response);
	}
}

bool MainService::focusObject(const QString &name)
{
	bool result = false;
	if (objMgr->findAndSelectI18n(name) || objMgr->findAndSelect(name))
	{
		const QList<StelObjectP> newSelected = objMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=core->getCurrentLocation().planetName)
			{
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
				result = true;
			}
			else
			{
				objMgr->unSelect();
			}
		}
	}
	return result;
}
