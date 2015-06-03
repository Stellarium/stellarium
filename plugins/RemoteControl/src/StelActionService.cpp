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

#include "StelActionService.hpp"

#include "StelApp.hpp"
#include "StelActionMgr.hpp"
#include "StelTranslator.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

StelActionService::StelActionService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	//this is run in the main thread
	actionMgr = StelApp::getInstance().getStelActionManager();
}

void StelActionService::get(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	if(operation=="list")
	{
		//list all registered StelActions, this should be thread safe
		QJsonObject groupObject;
		foreach(QString group, actionMgr->getGroupList())
		{
			QJsonArray itemArray;
			foreach(StelAction* action, actionMgr->getActionList(group))
			{
				QJsonObject item;
				item.insert("id",action->getId());
				item.insert("isCheckable",action->isCheckable());
				if(action->isCheckable())
					item.insert("isChecked",action->isChecked());
				item.insert("text",action->getText());
				itemArray.append(item);
			}
			groupObject.insert(StelTranslator::globalTranslator->qtranslate(group),itemArray);
		}

		writeJSON(QJsonDocument(groupObject),response);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: list POST: do",response);
	}
}

void StelActionService::post(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, HttpResponse &response)
{
	//this is run in an HTTP worker thread
	if(operation == "do")
	{
		QString id = QString::fromUtf8(parameters.value("id"));

		StelAction* action = actionMgr->findAction(id);
		if(!action)
		{
			writeRequestError("error: invalid id parameter",response);
		}
		else
		{
			//queue a trigger event
			//some special handling for quit and stop server event (async to prevent deadlock)

			bool isClosing = (id=="actionQuit_Global" || id == "actionShow_Remote_Control");
			Qt::ConnectionType type = Qt::BlockingQueuedConnection;
			if(isClosing);
				type = Qt::QueuedConnection;

			QMetaObject::invokeMethod(action,"trigger",type);

			if(isClosing)
			{
				response.write("error: Stellarium or remote control closing!",true);
				return;
			}

			if(action->isCheckable())
			{
				//return the new state of the action
				response.write(action->isChecked() ? "true" : "false", true);
			}
			else
				response.write("ok",true);
		}
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: list POST: do",response);
	}
}
