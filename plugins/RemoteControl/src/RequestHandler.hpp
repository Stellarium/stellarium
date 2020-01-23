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

#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "httpserver/httprequesthandler.h"
#include "httpserver/staticfilecontroller.h"

class APIController;
class StaticFileController;

//! This is the main request handler for the remote control plugin, receiving and dispatching the HTTP requests.
//! It also handles the optional simple HTTP authentication. See #service to find out how the requests are processed.
//! @ingroup remoteControl
class RequestHandler : public HttpRequestHandler
{
	Q_OBJECT
public:
	//! Constructs the request handler. This also creates an StaticFileController for the \c webroot folder,
	//! and an APIController.
	//!
	//! To see the default services that are registered here, see \ref rcApiReference.
	RequestHandler(const StaticFileControllerSettings& settings, QObject* parent = Q_NULLPTR);
	//! The internal APIController, and all registered services are deleted
	virtual ~RequestHandler();

	//! Called in the main thread each frame, only passed on to APIController::update
	void update(double deltaTime);

	//! Receives the HttpRequest from the HttpListener.
	//! It checks the optional HTTP authentication and sets the keep-alive header if requested
	//! by the client.
	//!
	//! If the authentication is correct, the request is processed according to the following rules:
	//!  - If the request path starts with the string @c "/api/", then the request is passed to
	//! the \ref APIController without further processing.
	//!  - If a file specified in the special \c translate_files file is requested, the cached translated version
	//! of this file is returned. This cache is updated each time the app language changes.
	//!  - Otherwise, it is passed to a StaticFileController that has been set up for the \c data/webroot folder.
	//!
	//! @note This method runs in an HTTP worker thread, not in the Stellarium main thread, so take caution.
	virtual void service(HttpRequest& request, HttpResponse& response);

public slots:
	//! Sets wether a password set with setPassword() is required by all requests.
	//! It uses HTTP Basic authorization, with an empty username.
	//! @warning Make sure to only call this only when the server is offline because they are not synchronized
	void setUsePassword(bool v);
	//! Returns if a password is required to access the remote control
	bool getUsePassword() { return usePassword; }
	//! @warning Make sure to only call this only when the server is offline because they are not synchronized
	void setPassword(const QString& pw);

	//! Sets wether CORS is enabled.
	//! @warning Make sure to only call this only when the server is offline because they are not synchronized
	void setEnableCors(bool v);
	//! Returns if CORS is enabled
	bool getEnableCors() const { return enableCors; }
	//! Set the host for which CORS is enabled. Specify "*" to let any website take control.
	//! @warning Make sure to only call this only when the server is offline because they are not synchronized
	void setCorsOrigin(const QString& origin);




private slots:
	void refreshTemplates();

	void addExtensionServices(QObjectList services);

private:
	//Contains the translated templates loaded from the file "translate_files" in the webroot folder
	QMap<QByteArray,QString> templateMap;

	bool usePassword;
	QString password;
	bool enableCors;
	QString corsOrigin;
	QByteArray passwordReply;
	APIController* apiController;
	StaticFileController* staticFiles;
	QMutex templateMutex;

	static const QByteArray AUTH_REALM;

	void handleCorsPreflightRequest(HttpRequest &request, HttpResponse &response);
};

#endif
