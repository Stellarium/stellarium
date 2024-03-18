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

void AbstractAPIService::update(double deltaTime)
{
	Q_UNUSED(deltaTime)
}

bool AbstractAPIService::isThreadSafe() const
{
	return false;
}

void AbstractAPIService::get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse& response)
{
	Q_UNUSED(operation)
	Q_UNUSED(parameters)

	response.setStatus(405,"Method Not allowed");
	QString str(QStringLiteral("Method GET not allowed for service %2"));

	response.setData(str.arg(getPath()).toLatin1());
}

void AbstractAPIService::post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse& response)
{
	Q_UNUSED(operation)
	Q_UNUSED(parameters)
	Q_UNUSED(data)

	response.setStatus(405,"Method Not allowed");
	QString str(QStringLiteral("Method POST not allowed for service %2"));
	response.setData(str.arg(getPath()).toLatin1());
}

#ifdef FORCE_THREADED_SERVICES
const Qt::ConnectionType AbstractAPIService::SERVICE_DEFAULT_INVOKETYPE = Qt::BlockingQueuedConnection;
#else
const Qt::ConnectionType AbstractAPIService::SERVICE_DEFAULT_INVOKETYPE = Qt::DirectConnection;
#endif
