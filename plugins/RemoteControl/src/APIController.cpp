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

#include "APIController.hpp"
#include <QJsonDocument>

void AbstractAPIService::get(const QList<QByteArray> &args, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	Q_UNUSED(args);
	Q_UNUSED(parameters);
	response.setStatus(405,"Method Not allowed");
	QString str(QStringLiteral("Method GET not allowed for service %2"));
	response.write(str.arg(QString::fromLatin1(serviceName())).toLatin1(),true);
}

void AbstractAPIService::post(const QList<QByteArray>& args, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, HttpResponse &response)
{
	Q_UNUSED(args);
	Q_UNUSED(parameters);
	response.setStatus(405,"Method Not allowed");
	QString str(QStringLiteral("Method POST not allowed for service %2"));
	response.write(str.arg(QString::fromLatin1(serviceName())).toLatin1(),true);
}

void AbstractAPIService::writeRequestError(const QByteArray &message, HttpResponse& response)
{
	response.setStatus(400,"Bad Request");
	response.write(message,true);
}

void AbstractAPIService::writeJSON(const QJsonDocument &doc, HttpResponse &response)
{
	QByteArray data = doc.toJson();
	response.setHeader("Content-Length",data.size());
	response.setHeader("Content-Type","application/json; charset=utf-8");
	response.write(data,true);
}

APIController::APIController(int prefixLength, QObject* parent) : HttpRequestHandler(parent), m_prefixLength(prefixLength)
{

}

APIController::~APIController()
{

}

void APIController::registerService(AbstractAPIService *service)
{
	m_serviceMap.insert(service->serviceName(), service);
}

void APIController::service(HttpRequest &request, HttpResponse &response)
{
	//disable caching by default for services
	response.setHeader("Cache-Control","no-cache");

	QByteArray path = request.getPath();
	QByteArray pathWithoutPrefix = path.right(path.size()-m_prefixLength);

	QList<QByteArray> splitPath = pathWithoutPrefix.split('/');

	//try to find service
	QMap<QByteArray,AbstractAPIService*>::iterator it = m_serviceMap.find(splitPath[0]);
	if(it!=m_serviceMap.end())
	{
		AbstractAPIService* sv = *it;

		QList<QByteArray> args;
		if(splitPath.size()>1)
		{
			args = splitPath.mid(1);
		}

		if(request.getMethod()=="GET")
		{
			sv->get(args, request.getParameterMap(),response);
		}
		else if (request.getMethod()=="POST")
		{
			sv->post(args, request.getParameterMap(),request.getBody(),response);
		}
		else
		{
			response.setStatus(405,"Method Not allowed");
			QString str(QStringLiteral("Method %1 not allowed for service %2"));
			response.write(str.arg(QString::fromLatin1(request.getMethod())).arg(QString::fromUtf8(pathWithoutPrefix)).toUtf8(),true);
		}
	}
	else
	{
		response.setStatus(400,"Bad Request");
		QString str(QStringLiteral("Unknown service: %1"));
		response.write(str.arg(QString::fromUtf8(pathWithoutPrefix)).toUtf8(),true);
	}
}
