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

#include "AbstractAPIService.hpp"
#include <QJsonDocument>

#include "httpserver/httpresponse.h"

int APIServiceResponse::metaTypeId = qRegisterMetaType<APIServiceResponse>();
int APIServiceResponse::parametersMetaTypeId = qRegisterMetaType<APIParameters>();

void APIServiceResponse::setHeader(const QByteArray &name, const QByteArray &val)
{
	headers.insert(name,val);
}

void APIServiceResponse::setHeader(const QByteArray &name, const int val)
{
	headers.insert(name,QByteArray::number(val));
}

void APIServiceResponse::setStatus(int status, const QByteArray &text)
{
	this->status = status;
	this->statusText = text;
}

void APIServiceResponse::applyResponse(HttpResponse *response) const
{
	if(status != -1)
	{
		response->setStatus(status,statusText);
	}

	//apply headers
	response->getHeaders().unite(headers);

	//send response data, if any
	if(responseData.isEmpty())
	{
		response->getHeaders().clear();
		response->setStatus(500,"Internal Server Error");
		response->write("Service provided no response",true);
	}
	response->write(responseData,true);
}

void APIServiceResponse::setData(const QByteArray &data)
{
	this->responseData = data;
}

void APIServiceResponse::appendData(const QByteArray &data)
{
	this->responseData.append(data);
}

void APIServiceResponse::writeRequestError(const QByteArray &msg)
{
	setStatus(400,"Bad Request");
	responseData = msg;
}

void APIServiceResponse::writeJSON(const QJsonDocument &doc)
{
#ifdef QT_NO_DEBUG
	//Use compact JSON format for release builds for smaller files
	QByteArray data = doc.toJson(QJsonDocument::Compact);
#else
	//Use indented JSON format in debug builds for easier human reading
	QByteArray data = doc.toJson(QJsonDocument::Indented);
#endif
	//setHeader("Content-Length",data.size());
	setHeader("Content-Type","application/json; charset=utf-8");
	setData(data);
}


void AbstractAPIService::update(double deltaTime)
{
	Q_UNUSED(deltaTime);
}

bool AbstractAPIService::supportsThreadedOperation() const
{
	return false;
}

APIServiceResponse AbstractAPIService::get(const QByteArray &operation, const APIParameters &parameters)
{
	APIServiceResponse response;
	getImpl(operation,parameters,response);
	return response;
}

APIServiceResponse AbstractAPIService::post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data)
{
	APIServiceResponse response;
	postImpl(operation,parameters,data,response);
	return response;
}

#ifdef FORCE_THREADED_SERVICES
const Qt::ConnectionType AbstractAPIService::SERVICE_DEFAULT_INVOKETYPE = Qt::BlockingQueuedConnection;
#else
const Qt::ConnectionType AbstractAPIService::SERVICE_DEFAULT_INVOKETYPE = Qt::DirectConnection;
#endif

void AbstractAPIService::getImpl(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, APIServiceResponse &response)
{
	Q_UNUSED(operation);
	Q_UNUSED(parameters);

	response.setStatus(405,"Method Not allowed");
	QString str(QStringLiteral("Method GET not allowed for service %2"));

	response.setData(str.arg(QString::fromLatin1(serviceName())).toLatin1());
}

void AbstractAPIService::postImpl(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, APIServiceResponse &response)
{
	Q_UNUSED(operation);
	Q_UNUSED(parameters);
	Q_UNUSED(data);

	response.setStatus(405,"Method Not allowed");
	QString str(QStringLiteral("Method POST not allowed for service %2"));
	response.setData(str.arg(QString::fromLatin1(serviceName())).toLatin1());
}



