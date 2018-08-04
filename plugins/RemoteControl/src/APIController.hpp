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

#ifndef APIHANDLER_HPP
#define APIHANDLER_HPP

#include "httpserver/httprequesthandler.h"
#include "AbstractAPIService.hpp"

//! @ingroup remoteControl
//! This class handles the API-specific requests and dispatches them to the correct RemoteControlServiceInterface implementation.
//! Services are registered using registerService().
//! To see the default services used, see the RequestHandler::RequestHandler constructor.
class APIController : public HttpRequestHandler
{
	Q_OBJECT
public:
	//! Constructs an APIController
	//! @param prefixLength Determines how many characters to strip from the front of the request path
	//! @param parent passed on to QObject constructor
	APIController(int prefixLength, QObject* parent = Q_NULLPTR);
	virtual ~APIController();

	//! Should be called each frame from the main thread, like from StelModule::update.
	//! Passed on to each AbstractAPIService::update method for optional processing.
	void update(double deltaTime);

	//! Handles an API-specific request. It finds out which RemoteControlServiceInterface to use
	//! depending on the service name (first part of path until slash). An error is returned for invalid requests.
	//! If a service was found, the request is passed on to its RemoteControlServiceInterface::get or RemoteControlServiceInterface::post
	//! method depending on the HTTP request type.
	//! If RemoteControlServiceInterface::isThreadSafe is false, these methods are called in the Stellarium main thread
	//! using QMetaObject::invokeMethod, otherwise they are directly executed in the current thread (HTTP worker thread).
	virtual void service(HttpRequest& request, HttpResponse& response);

	//! Registers a service with the APIController.
	//! The RemoteControlServiceInterface::getPath() determines the request path of the service.
	void registerService(RemoteControlServiceInterface* service);
private slots:
	void performGet(RemoteControlServiceInterface* service, const QByteArray& operation, const APIParameters& parameters, APIServiceResponse* response);
	void performPost(RemoteControlServiceInterface* service, const QByteArray& operation, const APIParameters& parameters, const QByteArray& data, APIServiceResponse* response);
private:
	static void applyAPIResponse(const APIServiceResponse& apiresponse, HttpResponse& httpresponse);
	int m_prefixLength;
	typedef QMap<QByteArray,RemoteControlServiceInterface*> ServiceMap;
	ServiceMap m_serviceMap;
};

#endif
