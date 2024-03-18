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

#ifndef ABSTRACTAPISERVICE_HPP
#define ABSTRACTAPISERVICE_HPP

#include "RemoteControlServiceInterface.hpp"

//! \addtogroup remoteControl
//! @{

//! Abstract base class for all RemoteControlServiceInterface implementations which are provided by the \ref remoteControl plugin directly.
class AbstractAPIService : public QObject, public RemoteControlServiceInterface
{
	Q_OBJECT
	//Probably not really necessary to do this here but it probably won't hurt either
	Q_INTERFACES(RemoteControlServiceInterface)
public:
	//! Only calls QObject constructor
	AbstractAPIService(QObject* parent = nullptr) : QObject(parent)
	{
	}

	// Provides a default implementation which returns false.
	bool isThreadSafe() const override;
	//! Called in the main thread each frame. Default implementation does nothing.
	//! Can be used for ongoing actions, for example movement control.
	void update(double deltaTime) override;
	//! Provides a default implementation which returns an error message.
	void get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse& response) override;
	//! Provides a default implementation which returns an error message.
	void post(const QByteArray &operation, const APIParameters &parameters, const QByteArray& data, APIServiceResponse& response) override;

protected:
	//! This defines the connection type QMetaObject::invokeMethod has to use inside a service: either Qt::DirectConnection for main thread handling, or
	//! Qt::BlockingQueuedConnection for HTTP thread handling
	static const Qt::ConnectionType SERVICE_DEFAULT_INVOKETYPE;
};

//! @}

#endif
