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

#ifndef APIHANDLER_HPP_
#define APIHANDLER_HPP_

#include "httpserver/httprequesthandler.h"
#include "AbstractAPIService.hpp"

#include <QMutex>

//! @ingroup remoteControl
//! This class handles the API-specific requests and dispatches them to the correct service.
//! To see the default services used, see the RequestHandler::RequestHandler constructor.
class APIController : public HttpRequestHandler
{
	Q_OBJECT
public:
	APIController(int prefixLength, QObject* parent = 0);
	virtual ~APIController();

	//! Should be called each frame, like from StelModule::update.
	//! Passed on to each AbstractAPIService::update method for optional processing.
	void update(double deltaTime);

	virtual void service(HttpRequest& request, HttpResponse& response);

	//! Registers a service with the APIController.
	void registerService(AbstractAPIService* service);
private:
	int m_prefixLength;
	typedef QMap<QByteArray,AbstractAPIService*> ServiceMap;
	ServiceMap m_serviceMap;
	QMutex mutex;
};

#endif
