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
#include "StelActionMgr.hpp"
#include "StelCore.hpp"
#include "LandscapeMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelScriptMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


MainService::MainService(QObject *parent)
	: AbstractAPIService(parent),
	  moveX(0),moveY(0),lastMoveUpdateTime(0),
	  //100 should be more than enough
	  //this only has to encompass events that occur between 2 status updates
	  actionCache(100), propCache(100)
{
	//this is run in the main thread
	core = StelApp::getInstance().getCore();
	actionMgr =  StelApp::getInstance().getStelActionManager();
	lsMgr = GETSTELMODULE(LandscapeMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();
	objMgr = &StelApp::getInstance().getStelObjectMgr();
	mvmgr = GETSTELMODULE(StelMovementMgr);
	propMgr = StelApp::getInstance().getStelPropertyManager();
	scriptMgr = &StelApp::getInstance().getScriptMgr();
	skyCulMgr = &StelApp::getInstance().getSkyCultureMgr();

	connect(actionMgr,SIGNAL(actionToggled(QString,bool)),this,SLOT(actionToggled(QString,bool)));
	connect(propMgr,SIGNAL(stelPropertyChanged(StelProperty*,QVariant)),this,SLOT(propertyChanged(StelProperty*,QVariant)));

	Q_ASSERT(this->thread()==objMgr->thread());
}

void MainService::update(double deltaTime)
{
	bool xZero = qFuzzyIsNull(moveX);
	bool yZero = qFuzzyIsNull(moveY);

	//prevent sudden disconnects from moving endlessly
	if((QDateTime::currentMSecsSinceEpoch() - lastMoveUpdateTime) > 1000)
	{
		if(!xZero || !yZero)
			qDebug()<<"[MainService] move timeout";
		moveX = moveY = .0f;
	}

	//Similar to StelMovementMgr::updateMotion

	if(!xZero || !yZero)
	{
		//qDebug()<<moveX<<moveY;

		double currentFov = mvmgr->getCurrentFov();
		// the more it is zoomed, the lower the moving speed is (in angle)
		//0.0004 is the default key move speed
		double depl=0.0004 / 30 *deltaTime*1000*currentFov;

		double deltaAz = moveX*depl;
		double deltaAlt = moveY*depl;

		mvmgr->panView(deltaAz,deltaAlt);

		//this is required to enable maximal fps for smoothness
		StelMainView::getInstance().thereWasAnEvent();
	}
}

void MainService::get(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if(operation=="status")
	{
		//a listing of the most common stuff that can change often

		QString sActionId = QString::fromUtf8(parameters.value("actionId"));
		bool actionOk;
		int actionId = sActionId.toInt(&actionOk);

		QString sPropId = QString::fromUtf8(parameters.value("propId"));
		bool propOk;
		int propId = sPropId.toInt(&propOk);

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
			double jday = core->getJD();
			double deltaT = core->getDeltaT() * StelCore::JD_SECOND;

			double gmtShift = core->getUTCOffset(jday) / 24.0;

			QString utcIso = StelUtils::julianDayToISO8601String(jday,true).append('Z');
			QString localIso = StelUtils::julianDayToISO8601String(jday+gmtShift,true);

			//time zone string
			QString timeZone = localeMgr->getPrintableTimeZoneLocal(jday);

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

		//// Info about selected object (only primary)
		{
			QString infoStr;
			QMetaObject::invokeMethod(this,"getInfoString",SERVICE_DEFAULT_INVOKETYPE,
						  Q_RETURN_ARG(QString,infoStr));
			obj.insert("selectioninfo",infoStr);
		}

		//// Info about current view
		{
			QJsonObject obj2;

			// the aim fov may lie outside the min/max bounds, so constrain it
			double fov = mvmgr->getAimFov();
			if(fov < mvmgr->getMinFov())
				fov = mvmgr->getMinFov();
			else if (fov>mvmgr->getMaxFov())
				fov = mvmgr->getMaxFov();

			obj2.insert("fov",fov);

			obj.insert("view",obj2);
		}

		//// Info about changed actions & props (if requested)
		{
			if(actionOk)
				obj.insert("actionChanges",getActionChangesSinceID(actionId));
			if(propOk)
				obj.insert("propertyChanges",getPropertyChangesSinceID(propId));
		}

		response.writeJSON(QJsonDocument(obj));
	}
	else if(operation=="plugins")
	{
		// Retrieve list of plugins

		QJsonObject mainObj;

		StelModuleMgr& modMgr = StelApp::getInstance().getModuleMgr();
		for (const auto& desc : modMgr.getPluginsList())
		{
			QJsonObject pluginObj,infoObj;
			pluginObj.insert("loadAtStartup", desc.loadAtStartup);
			pluginObj.insert("loaded", desc.loaded);

			infoObj.insert("authors", desc.info.authors);
			infoObj.insert("contact", desc.info.contact);
			infoObj.insert("description", desc.info.description);
			infoObj.insert("displayedName", desc.info.displayedName);
			infoObj.insert("startByDefault", desc.info.startByDefault);
			infoObj.insert("version", desc.info.version);

			pluginObj.insert("info",infoObj);
			mainObj.insert(desc.info.id, pluginObj);
		}

		response.writeJSON(QJsonDocument(mainObj));
	}
	else if(operation=="view")
	{
		// JNow query can include a ref=(on|off|auto/anyOther) parameter
		StelCore::RefractionMode refMode=StelCore::RefractionAuto;
		QString refName = QString::fromUtf8(parameters.value("ref"));
		if (refName=="on")
			refMode=StelCore::RefractionOn;
		else if (refName=="off")
			refMode=StelCore::RefractionOff;

		// Retrieve Vector of view direction

		// Optional: limit answer to just one number.
		bool giveJ2000=true;
		bool giveJNow=true;
		bool giveAltAz=true;
		QString coordName = QString::fromUtf8(parameters.value("coord"));
		if (coordName=="j2000")
		{
			giveJNow=false;
			giveAltAz=false;
		}
		else if (coordName=="jNow")
		{
			giveJ2000=false;
			giveAltAz=false;
		}
		else if (coordName=="altAz")
		{
			giveJ2000=false;
			giveJNow=false;
		}

		QJsonObject mainObj;

		Vec3d viewJ2000=mvmgr->getViewDirectionJ2000();
		if (giveJ2000)
			mainObj.insert("j2000", viewJ2000.toString());
		if (giveJNow)
		{
			Vec3d viewJNow=core->j2000ToEquinoxEqu(viewJ2000, refMode);
			mainObj.insert("jNow", viewJNow.toString());
		}
		if (giveAltAz)
		{
			Vec3d viewAltAz=core->j2000ToAltAz(viewJ2000, StelCore::RefractionAuto);
			mainObj.insert("altAz", viewAltAz.toString());
		}

		response.writeJSON(QJsonDocument(mainObj));
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: status, plugins, view");
	}
}

void MainService::post(const QByteArray& operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{
	Q_UNUSED(data);

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
					//check for invalid double (NaN, inf...)
					//this will crash the app if it is allowed
					if(qIsNaN(jday) || qIsInf(jday))
					{
						qWarning()<<"[RemoteControl] Prevented setting invalid time"<<jday<<", does the web interface have a bug?";
						response.setData("error: invalid time value");
						return;
					}

					doneSomething = true;
					//set new time
					QMetaObject::invokeMethod(core,"setJD", SERVICE_DEFAULT_INVOKETYPE,
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
					QMetaObject::invokeMethod(core,"setTimeRate", SERVICE_DEFAULT_INVOKETYPE,
								  Q_ARG(double,rate));
				}
			}
		}

		if(doneSomething)
			response.setData("ok");
		else
			response.setData("error: invalid parameters, use time/timerate as double values");
	}
	else if(operation == "focus")
	{
		QString target = QString::fromUtf8(parameters.value("target"));
		SelectionMode selMode = Center;

		if(parameters.value("mode") == "zoom")
			selMode = Zoom;
		else if(parameters.value("mode") == "mark")
			selMode = Mark;

		//check target string first
		if(target.isEmpty())
		{
			if(parameters.value("position").isEmpty())
			{
				//no parameters = clear focus
				target = "";
			}
			else
			{
				//parse position
				QJsonDocument doc = QJsonDocument::fromJson(parameters.value("position"));
				QJsonArray arr = doc.array();
				if(arr.size() == 3)
				{
					Vec3d pos;
					pos[0] = arr.at(0).toDouble();
					pos[1] = arr.at(1).toDouble();
					pos[2] = arr.at(2).toDouble();

					//deselect and move
					QMetaObject::invokeMethod(this,"focusPosition", SERVICE_DEFAULT_INVOKETYPE,
								  Q_ARG(Vec3d,pos));
					response.setData("ok");
					return;
				}

				response.writeRequestError("invalid position format");
				return;
			}
		}

		bool result;
		QMetaObject::invokeMethod(this,"focusObject",SERVICE_DEFAULT_INVOKETYPE,
					  Q_RETURN_ARG(bool,result),
					  Q_ARG(QString,target),
					  Q_ARG(SelectionMode,selMode));

		response.setData(result ? "true" : "false");
	}
	else if(operation == "move")
	{
		bool xOk,yOk;

		double x = parameters.value("x").toDouble(&xOk);
		double y = parameters.value("y").toDouble(&yOk);

		if(xOk || yOk)
		{
			QMetaObject::invokeMethod(this,"updateMovement", SERVICE_DEFAULT_INVOKETYPE,
						  Q_ARG(double,x),
						  Q_ARG(double,y),
						  Q_ARG(bool,xOk),
						  Q_ARG(bool,yOk));

			response.setData("ok");
		}
		else
			response.writeRequestError("requires x or y parameter");
	}
	else if(operation == "view")
	{
		// JNow setting can include a ref=(on|off|auto/anyOther) parameter
		StelCore::RefractionMode refMode=StelCore::RefractionAuto;
		QString refName = QString::fromUtf8(parameters.value("ref"));
		if (refName=="on")
			refMode=StelCore::RefractionOn;
		else if (refName=="off")
			refMode=StelCore::RefractionOff;

		QByteArray j2000 = parameters.value("j2000");
		if(!j2000.isEmpty())
		{
			QJsonDocument doc = QJsonDocument::fromJson(j2000);
			QJsonArray arr = doc.array();
			if(arr.size() == 3)
			{
				Vec3d pos;
				pos[0] = arr.at(0).toDouble();
				pos[1] = arr.at(1).toDouble();
				pos[2] = arr.at(2).toDouble();

				mvmgr->setViewDirectionJ2000(pos);
				response.setData("ok");
			}
			else
			{
				response.writeRequestError("invalid j2000 format, use JSON array of 3 doubles");
			}
			return;
		}

		QByteArray jNow = parameters.value("jNow");
		if(!jNow.isEmpty())
		{
			QJsonDocument doc = QJsonDocument::fromJson(jNow);
			QJsonArray arr = doc.array();
			if(arr.size() == 3)
			{
				Vec3d posNow;
				posNow[0] = arr.at(0).toDouble();
				posNow[1] = arr.at(1).toDouble();
				posNow[2] = arr.at(2).toDouble();

				mvmgr->setViewDirectionJ2000(core->equinoxEquToJ2000(posNow, refMode));
				response.setData("ok");
			}
			else
			{
				response.writeRequestError("invalid jNow format, use JSON array of 3 doubles");
			}
			return;
		}

		QByteArray rect = parameters.value("altAz");
		if(!rect.isEmpty())
		{
			QJsonDocument doc = QJsonDocument::fromJson(rect);
			QJsonArray arr = doc.array();
			if(arr.size() == 3)
			{
				Vec3d pos;
				pos[0] = arr.at(0).toDouble();
				pos[1] = arr.at(1).toDouble();
				pos[2] = arr.at(2).toDouble();

				mvmgr->setViewDirectionJ2000(core->altAzToJ2000(pos,StelCore::RefractionOff));
				response.setData("ok");
			}
			else
			{
				response.writeRequestError("invalid altAz format, use JSON array of 3 doubles");
			}
			return;
		}

		QString azs = QString::fromUtf8(parameters.value("az"));
		QString alts = QString::fromUtf8(parameters.value("alt"));

		bool azOk,altOk;
		double az = azs.toDouble(&azOk);
		double alt = alts.toDouble(&altOk);

		if(azOk || altOk)
		{
			QMetaObject::invokeMethod(this,"updateView", SERVICE_DEFAULT_INVOKETYPE,
						  Q_ARG(double,az),
						  Q_ARG(double,alt),
						  Q_ARG(bool,azOk),
						  Q_ARG(bool,altOk));

			response.setData("ok");
		}
		else
			response.writeRequestError("requires at least one of az,alt,j2000,jNow parameters");
	}

	else if (operation == "fov")
	{
		QString fov = QString::fromUtf8(parameters.value("fov"));
		bool ok;
		double dFov = fov.toDouble(&ok);

		if(fov.isEmpty() || !ok)
		{
			response.writeRequestError("requires fov parameter");
			return;
		}

		QMetaObject::invokeMethod(this,"setFov",SERVICE_DEFAULT_INVOKETYPE,
					  Q_ARG(double,dFov));

		response.setData("ok");
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. POST: time,focus,move,view,fov");
	}
}

StelObjectP MainService::getSelectedObject()
{
	const QList<StelObjectP>& list = objMgr->getSelectedObject();
	if(list.isEmpty())
		return StelObjectP();
	return list.first();
}

QString MainService::getInfoString()
{
	StelObjectP selectedObject = getSelectedObject();
	if(selectedObject.isNull())
		return QString();
	return selectedObject->getInfoString(core,StelObject::AllInfo | StelObject::NoFont);
}

bool MainService::focusObject(const QString &name, SelectionMode mode)
{
	//StelDialog::gotoObject

	//if name is empty, unselect
	if(name.isEmpty())
	{
		objMgr->unSelect();
		if(mode == Zoom)
			mvmgr->autoZoomOut();
		return true;
	}

	bool result = false;
	if (objMgr->findAndSelectI18n(name) || objMgr->findAndSelect(name))
	{
		const QList<StelObjectP> newSelected = objMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=core->getCurrentLocation().planetName)
			{
				if(mode != Mark)
				{
					mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
					mvmgr->setFlagTracking(true);
					if(mode == Zoom)
					{
						mvmgr->autoZoomIn();
					}
				}
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

void MainService::focusPosition(const Vec3d &pos)
{
	objMgr->unSelect();
	mvmgr->moveToJ2000(pos, mvmgr->mountFrameToJ2000(Vec3d(0., 0., 1.)), mvmgr->getAutoMoveDuration());
}

void MainService::updateMovement(double x, double y, bool xUpdated, bool yUpdated)
{
	if(xUpdated)
	{
		this->moveX = x;
	}
	if(yUpdated)
	{
		this->moveY = y;
	}
	qint64 curTime = QDateTime::currentMSecsSinceEpoch();
	//qDebug()<<"updateMove"<<x<<y<<(curTime-lastMoveUpdateTime);
	lastMoveUpdateTime = curTime;
}

void MainService::updateView(double az, double alt, bool azUpdated, bool altUpdated)
{
	Vec3d viewDirJ2000=mvmgr->getViewDirectionJ2000();
	Vec3d viewDirAltAz=core->j2000ToAltAz(viewDirJ2000, StelCore::RefractionOff);
	double anAz, anAlt;
	StelUtils::rectToSphe(&anAz, &anAlt, viewDirAltAz);
	if (azUpdated)
	{
		anAz=az;
	}
	if (altUpdated)
	{
		anAlt=alt;
	}
	StelUtils::spheToRect(anAz, anAlt, viewDirAltAz);
	mvmgr->setViewDirectionJ2000(core->altAzToJ2000(viewDirAltAz, StelCore::RefractionOff));
}

void MainService::setFov(double fov)
{
	//TODO calculate a better move duration here
	mvmgr->zoomTo(fov,0.25f);
}

void MainService::actionToggled(const QString &id, bool val)
{
	actionMutex.lock();
	actionCache.append(ActionCacheEntry(id,val));
	if(!actionCache.areIndexesValid())
	{
		//in theory, this can happen, but practically not so much
		qWarning()<<"Action cache indices invalid";
		actionCache.clear();
	}
	actionMutex.unlock();
}

void MainService::propertyChanged(StelProperty* prop, const QVariant& val)
{
	propMutex.lock();
	propCache.append(PropertyCacheEntry(prop->getId(),val));
	if(!propCache.areIndexesValid())
	{
		//in theory, this can happen, but practically not so much
		qWarning()<<"Property cache indices invalid";
		propCache.clear();
	}
	propMutex.unlock();
}

QJsonObject MainService::getActionChangesSinceID(int changeId)
{
	//changeId is the last id the interface is available
	//or -2 if the interface just started
	// -1 means the initial state was set

	QJsonObject obj;
	QJsonObject changes;
	int newId = changeId;


	actionMutex.lock();
	if(actionCache.isEmpty())
	{
		if(changeId!=-1)
		{
			//this is either the initial state (-2) or
			//something is "broken", probably from an existing web interface that reconnected after restart
			//force a full reload

			for (const auto* ac : actionMgr->getActionList())
			{
				if(ac->isCheckable())
				{
					changes.insert(ac->getId(),ac->isChecked());
				}
			}
			newId = -1;
		}
	}
	else
	{
		if(changeId > actionCache.lastIndex() || changeId < (actionCache.firstIndex()-1))
		{
			//this is either the initial state (-2) or
			//"broken" state again, force full reload
			for (const auto* ac : actionMgr->getActionList())
			{
				if(ac->isCheckable())
				{
					changes.insert(ac->getId(),ac->isChecked());
				}
			}
			newId = actionCache.lastIndex();
		}
		else if(changeId < actionCache.lastIndex())
		{
			//create a "diff" between changeId to lastIndex
			for(int i = changeId+1;i<=actionCache.lastIndex();++i)
			{
				const ActionCacheEntry& e = actionCache.at(i);
				changes.insert(e.action,e.val);
			}
			newId = actionCache.lastIndex();
		}
		//else no changes happened, interface is at current state!
	}
	actionMutex.unlock();

	obj.insert("changes",changes);
	obj.insert("id",newId);

	return obj;
}

QJsonObject MainService::getPropertyChangesSinceID(int changeId)
{
	//changeId is the last id the interface is available
	//or -2 if the interface just started
	// -1 means the initial state was set
	QJsonObject obj;
	QJsonObject changes;
	int newId = changeId;

	propMutex.lock();
	if(propCache.isEmpty())
	{
		if(changeId!=-1)
		{
			//this is either the initial state (-2) or
			//something is "broken", probably from an existing web interface that reconnected after restart
			//force a full reload
			const StelPropertyMgr::StelPropertyMap& map = propMgr->getPropertyMap();
			for(StelPropertyMgr::StelPropertyMap::const_iterator it = map.constBegin();
			    it!=map.constEnd();++it)
			{
				changes.insert(it.key(), QJsonValue::fromVariant((*it)->getValue()));
			}
			newId = -1;
		}
	}
	else
	{
		if(changeId > propCache.lastIndex() || changeId < (propCache.firstIndex()-1))
		{
			//this is either the initial state (-2) or
			//"broken" state again, force full reload
			const StelPropertyMgr::StelPropertyMap& map = propMgr->getPropertyMap();
			for(StelPropertyMgr::StelPropertyMap::const_iterator it = map.constBegin();
			    it!=map.constEnd();++it)
			{
				changes.insert(it.key(), QJsonValue::fromVariant((*it)->getValue()));
			}
			newId = propCache.lastIndex();
		}
		else if(changeId < propCache.lastIndex())
		{
			//create a "diff" between changeId to lastIndex
			for(int i = changeId+1;i<=propCache.lastIndex();++i)
			{
				const PropertyCacheEntry& e = propCache.at(i);
				changes.insert(e.id,QJsonValue::fromVariant(e.val));
			}
			newId = propCache.lastIndex();
		}
		//else no changes happened, interface is at current state!
	}
	propMutex.unlock();

	obj.insert("changes",changes);
	obj.insert("id",newId);

	return obj;
}
