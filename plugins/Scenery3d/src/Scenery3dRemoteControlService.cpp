/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2017 Florian Schaukowitsch
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

#include "Scenery3dRemoteControlService.hpp"

QLatin1String Scenery3dRemoteControlService::getPath() const
{
	return QLatin1String("scenery3d");
}

bool Scenery3dRemoteControlService::isThreadSafe() const
{
	return false;
}

void Scenery3dRemoteControlService::update(double deltaTime)
{
	Q_UNUSED(deltaTime);
}

void Scenery3dRemoteControlService::get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse &response)
{
	response.writeRequestError("not yet implemented");
}

void Scenery3dRemoteControlService::post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response)
{
	response.writeRequestError("not yet implemented");
}
