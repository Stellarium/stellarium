/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2016 Florian Schaukowitsch
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

#include "StelPropertyService.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocationMgr.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

StelPropertyService::StelPropertyService(QObject *parent)
	: AbstractAPIService(parent)
{
	propMgr = StelApp::getInstance().getStelPropertyManager();
}

void StelPropertyService::get(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	Q_UNUSED(parameters)

	if(operation=="list")
	{
		QJsonObject rootObj;

		const auto& map = propMgr->getPropertyMap();
		for (auto it = map.constBegin(); it != map.constEnd(); ++it)
		{
			QJsonObject item;
			const StelProperty* prop = *it;
			QVariant val = prop->getValue();
			QMetaProperty metaProp = prop->getMetaProp();
			item.insert("value", QJsonValue::fromVariant(val));
			//The actual data type the variant was converted to, may be different than typeString (for example enums/flags, or user types)
			item.insert("variantType", val.typeName());
			item.insert("typeString", metaProp.typeName());
			item.insert("typeEnum", static_cast<qint64>(metaProp.type()));
			item.insert("canNotify", metaProp.hasNotifySignal());
			item.insert("isWritable", metaProp.isWritable());

			rootObj.insert(it.key(),item);
		}

		response.writeJSON(QJsonDocument(rootObj));
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: list POST: set");
	}
}

void StelPropertyService::post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{
	Q_UNUSED(data)

	if(operation=="set")
	{
		QString id = QString::fromUtf8(parameters.value("id"));
		QString val = QString::fromUtf8(parameters.value("value"));

		QMetaProperty prop = propMgr->getMetaProperty(id);
		if(!prop.isValid())
		{
			response.setData("error: unknown property");
		}
		else
		{
			QVariant newValue(val);
			//make sure that enum types are always interpreted numerically
			if(prop.isEnumType())
			{
				bool ok;
				uint uiVal = val.toUInt(&ok);
				if(ok)
					newValue = uiVal;
			}
			//rely on automatic QVariant conversions otherwise if possible
			if(propMgr->setStelPropertyValue(id,newValue))
			{
				response.setData("ok");
			}
			else
			{
				response.setData("error: could not set property, invalid data type?");
			}
		}
	}
	else
	{
		response.writeRequestError("unsupported operation. GET: list POST: set");
	}
}
