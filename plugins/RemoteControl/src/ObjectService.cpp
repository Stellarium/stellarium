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

#include "ObjectService.hpp"

#include "SearchDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "LandscapeMgr.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QMutex>
#include <QThreadPool>
#include <QRunnable>
#include <QWaitCondition>

ObjectService::ObjectService(QObject *parent) : AbstractAPIService(parent)
{
	//this is run in the main thread
	core = StelApp::getInstance().getCore();
	objMgr = &StelApp::getInstance().getStelObjectMgr();
	useStartOfWords = StelApp::getInstance().getSettings()->value("search/flag_start_words", false).toBool();
}

QStringList ObjectService::performSearch(const QString &text)
{
	//perform substitution greek text --> symbol
	QString greekText = substituteGreek(text);

	QStringList matches;
	if(greekText != text)
	{
		matches = objMgr->listMatchingObjects(text, 3, useStartOfWords);
		matches += objMgr->listMatchingObjects(greekText, (8 - matches.size()), useStartOfWords);
	}
	else //no greek replaced, saves 1 call
		matches = objMgr->listMatchingObjects(text, 5, useStartOfWords);

	return matches;
}

QString ObjectService::substituteGreek(const QString &text)
{
	//use the searchdialog static method for that
	return SearchDialog::substituteGreek(text);
}

void ObjectService::get(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	//make sure the object still "lives" in the main Stel thread, even though
	//we may currently be in the HTTP thread
	Q_ASSERT(this->thread() == objMgr->thread());

	if(operation=="find")
	{
		//this may contain greek or other unicode letters
		QString str = QString::fromUtf8(parameters.value("str"));
		str = str.trimmed().toLower();

		if(str.isEmpty())
		{
			response.writeRequestError("empty search string");
			return;
		}

		//qDebug()<<"Search string"<<str;

		QStringList results;
		QMetaObject::invokeMethod(this,"performSearch",SERVICE_DEFAULT_INVOKETYPE,
					  Q_RETURN_ARG(QStringList,results),
					  Q_ARG(QString,str));

		//remove duplicates here
		results.removeDuplicates();

		results.sort(Qt::CaseInsensitive);
		// objects with short names should be searched first
		// examples: Moon, Hydra (moon); Jupiter, Ghost of Jupiter
		stringLengthCompare comparator;
		std::sort(results.begin(), results.end(), comparator);

		//return as json
		response.writeJSON(QJsonDocument(QJsonArray::fromStringList(results)));
	}
	else if (operation == "info")
	{
		//retrieve HTML info string about a specific object
		//if no parameter is given, uses the currently selected object

		QString name = QString::fromUtf8(parameters.value("name"));
		QString formatStr = QString::fromUtf8(parameters.value("format"));
		bool formatHtml=true;
		if (formatStr == "map" || formatStr == "json")
			formatHtml=false;

		StelObjectP obj;
		if(!name.isEmpty())
		{
			QMetaObject::invokeMethod(this,"findObject",SERVICE_DEFAULT_INVOKETYPE,
						  Q_RETURN_ARG(StelObjectP,obj),
						  Q_ARG(QString,name));

			if(!obj)
			{
				response.setStatus(404,"not found");
				response.setData("object name not found");
				return;
			}
		}
		else
		{
			//use first selected object
			const QList<StelObjectP> selection = objMgr->getSelectedObject();
			if(selection.isEmpty())
			{
				response.setStatus(404,"not found");
				response.setData("no current selection, and no name parameter given");
				return;
			}
			obj = selection[0];
		}

		if (formatHtml)
		{
			QString infoStr;
			QMetaObject::invokeMethod(this,"getInfoString",SERVICE_DEFAULT_INVOKETYPE,
						  Q_RETURN_ARG(QString,infoStr),
						  Q_ARG(StelObjectP,obj));

			response.setData(infoStr.toUtf8());
		}
		else
		{
			QVariantMap infoMap=StelObjectMgr::getObjectInfo(obj);
			QJsonObject infoObj=QJsonObject::fromVariantMap(infoMap);

			// We make use of 2 extra values for linked applications. These govern sky brightness and can be used for ambient settings.
			LandscapeMgr *lmgr=GETSTELMODULE(LandscapeMgr);
			infoObj.insert("ambientLum", QJsonValue(static_cast<double>(lmgr->getAtmosphereAverageLuminance())));
			infoObj.insert("ambientInt", QJsonValue(lmgr->getCurrentLandscape()->getBrightness()));
			response.writeJSON(QJsonDocument(infoObj));
		}
	}
	else if (operation == "listobjecttypes")
	{
		//lists the available types of objects

		QMap<QString,QString> map = objMgr->objectModulesMap();
		QMapIterator<QString,QString> it(map);

		StelTranslator& trans = *StelTranslator::globalTranslator;
		QJsonArray arr;
		while(it.hasNext())
		{
			it.next();

			//check if this object type has any items first
			if(!objMgr->listAllModuleObjects(it.key(), true).isEmpty())
			{
				QJsonObject obj;
				obj.insert("key",it.key());
				obj.insert("name",it.value());
				obj.insert("name_i18n", trans.qtranslate(it.value()));
				arr.append(obj);
			}
		}

		response.writeJSON(QJsonDocument(arr));
	}
	else if(operation == "listobjectsbytype")
	{
		QString type = QString::fromUtf8(parameters.value("type"));
		QString engString = QString::fromUtf8(parameters.value("english"));

		bool ok;
		bool eng = engString.toInt(&ok);
		if(!ok)
			eng = false;

		if(!type.isEmpty())
		{
			QStringList list = objMgr->listAllModuleObjects(type,eng);

			//sort
			list.sort();
			response.writeJSON(QJsonDocument(QJsonArray::fromStringList(list)));
		}
		else
		{
			response.writeRequestError("missing type parameter");
		}
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: find,info,listobjecttypes,listobjectsbytype");
	}
}

StelObjectP ObjectService::findObject(const QString &name)
{
	StelObjectP obj = objMgr->searchByNameI18n(name);
	if(!obj)
		obj = objMgr->searchByName(name);
	return obj;
}

QString ObjectService::getInfoString(const StelObjectP obj)
{
	return obj->getInfoString(core);
}
