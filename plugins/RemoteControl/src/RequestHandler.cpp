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

#include "RequestHandler.hpp"
#include "httpserver/staticfilecontroller.h"

#include "StelFileMgr.hpp"

#include <QDir>

RequestHandler::RequestHandler(QSettings *settings, QObject* parent) : HttpRequestHandler(parent)
{
	//retrieve actual webroot through StelFileMgr
	QString path = StelFileMgr::findFile("data/webroot",StelFileMgr::Directory);
	//make sure its absolute, otherwise QtWebApp will look relative to config dir
	QDir dir(path);
	settings->setValue("path",dir.absolutePath());

	staticFiles = new StaticFileController(settings,this);
}

RequestHandler::~RequestHandler()
{

}

void RequestHandler::service(HttpRequest &request, HttpResponse &response)
{
	staticFiles->service(request,response);
}
