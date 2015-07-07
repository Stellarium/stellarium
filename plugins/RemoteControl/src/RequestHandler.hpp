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

#ifndef REQUESTHANDLER_HPP_
#define REQUESTHANDLER_HPP_

#include "httpserver/httprequesthandler.h"
#include "httpserver/staticfilecontroller.h"

class APIController;
class StaticFileController;

class RequestHandler : public HttpRequestHandler
{
	Q_OBJECT
public:
	RequestHandler(const StaticFileControllerSettings& settings, QObject* parent = 0);
	virtual ~RequestHandler();

	//! Called in the main thread each frame
	void update(double deltaTime);

	virtual void service(HttpRequest& request, HttpResponse& response);

public slots:
	//Make sure to only call these when the server is offline because they are not synchronized
	void setUsePassword(bool v);
	bool getUsePassword() { return usePassword; }
	void setPassword(const QString& pw);

private slots:
	void refreshHtmlTemplate();

private:
	//Contains the translated index.html
	QString indexFile;
	QString indexFilePath;
	bool usePassword;
	QString password;
	QByteArray passwordReply;
	APIController* apiController;
	StaticFileController* staticFiles;

	static const QByteArray AUTH_REALM;
};

#endif
