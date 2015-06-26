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


APIController::APIController(int prefixLength, QObject* parent) : HttpRequestHandler(parent), m_prefixLength(prefixLength)
{

}

APIController::~APIController()
{

}

void APIController::update(double deltaTime)
{
	mutex.lock();
	foreach(AbstractAPIService* service, m_serviceMap)
	{
		service->update(deltaTime);
	}
	mutex.unlock();
}

void APIController::registerService(AbstractAPIService *service)
{
	m_serviceMap.insert(service->serviceName(), service);
}

void APIController::service(HttpRequest &request, HttpResponse &response)
{
	//disable caching by default for services
	response.setHeader("Cache-Control","no-cache");
	//default content type is text
	response.setHeader("Content-Type","text/plain");

	//use the raw path here
	QByteArray path = request.getRawPath();
	QByteArray pathWithoutPrefix = path.right(path.size()-m_prefixLength);

	int slashIdx = pathWithoutPrefix.indexOf('/');

	QByteArray serviceString = pathWithoutPrefix;
	QByteArray operation;
	if(slashIdx>=0)
	{
		serviceString = pathWithoutPrefix.mid(0,slashIdx);
		operation = pathWithoutPrefix.mid(slashIdx+1);
	}

	//try to find service
	//the mutex here is to prevent a very strange crash I had, in which sv is an invalid pointer (but m_serviceMap is ok)
	//probably caused by the foreach loop in update in the main thread?
	mutex.lock();
	QMap<QByteArray,AbstractAPIService*>::iterator it = m_serviceMap.find(serviceString);
	if(it!=m_serviceMap.end())
	{
		AbstractAPIService* sv = *it;
		mutex.unlock();

		//create the response object
		APIServiceResponse apiresponse;
		if(request.getMethod()=="GET")
		{
#ifdef FORCE_THREADED_SERVICES
			apiresponse = sv->get(operation, request.getParameterMap());
#else
			if(sv->supportsThreadedOperation())
			{
				apiresponse = sv->get(operation, request.getParameterMap());
			}
			else
			{
				//invoke it in the main thread!
				QMetaObject::invokeMethod(sv,"get",Qt::BlockingQueuedConnection,
							  Q_RETURN_ARG(APIServiceResponse, apiresponse),
							  Q_ARG(QByteArray, operation),
							  Q_ARG(APIParameters, request.getParameterMap()));
			}
#endif
			apiresponse.applyResponse(&response);
		}
		else if (request.getMethod()=="POST")
		{
#ifdef FORCE_THREADED_SERVICES
			apiresponse = sv->post(operation, request.getParameterMap(), request.getBody());
#else
			if(sv->supportsThreadedOperation())
			{
				apiresponse = sv->post(operation, request.getParameterMap(), request.getBody());
			}
			else
			{
				QMetaObject::invokeMethod(sv,"post",Qt::BlockingQueuedConnection,
							  Q_RETURN_ARG(APIServiceResponse, apiresponse),
							  Q_ARG(QByteArray, operation),
							  Q_ARG(APIParameters, request.getParameterMap()),
							  Q_ARG(QByteArray, request.getBody()));
			}
#endif
			apiresponse.applyResponse(&response);
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
		mutex.unlock();
		response.setStatus(400,"Bad Request");
		QString str(QStringLiteral("Unknown service: %1"));
		response.write(str.arg(QString::fromUtf8(pathWithoutPrefix)).toUtf8(),true);
	}
}
