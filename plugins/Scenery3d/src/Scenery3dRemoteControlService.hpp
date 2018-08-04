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

#ifndef SCENERY3DREMOTECONTROLSERVICE_HPP
#define SCENERY3DREMOTECONTROLSERVICE_HPP

#include "../../RemoteControl/include/RemoteControlServiceInterface.hpp"
#include "Scenery3d.hpp"


//! Provides a Scenery3d service for the \ref remoteControl plugin.
class Scenery3dRemoteControlService : public QObject, public RemoteControlServiceInterface
{
	Q_OBJECT
	Q_INTERFACES(RemoteControlServiceInterface)

public:
	//! Requires the Scenery3d module to be registered
	Scenery3dRemoteControlService();

	// RemoteControlServiceInterface interface
	virtual QLatin1String getPath() const Q_DECL_OVERRIDE;
	virtual bool isThreadSafe() const Q_DECL_OVERRIDE;
	virtual void get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse &response) Q_DECL_OVERRIDE;
	virtual void post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
protected:
	void performMove(double x,double y, double z);
private:
	Scenery3d* s3d;
	Vec3d move;
	qint64 lastMoveUpdateTime;
};


#endif // SCENERY3DREMOTECONTROLSERVICE_HPP

