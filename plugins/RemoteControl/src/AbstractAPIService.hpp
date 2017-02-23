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

#ifndef ABSTRACTAPISERVICE_HPP_
#define ABSTRACTAPISERVICE_HPP_

#include "RemoteControlServiceInterface.hpp"

//! \addtogroup remoteControl
//! @{

//! Abstract base class for all RemoteControlServiceInterface implementations which are provided by the \ref remoteControl plugin directly.
class AbstractAPIService : public QObject, public RemoteControlServiceInterface
{
	Q_OBJECT
	Q_INTERFACES(RemoteControlServiceInterface)
public:
	//! Only calls QObject constructor
	AbstractAPIService(QObject* parent = 0) : QObject(parent)
	{
	}

	// Provides a default implementation which returns false.
	virtual bool isThreadSafe() const Q_DECL_OVERRIDE;
	//! Called in the main thread each frame. Default implementation does nothing.
	//! Can be used for ongoing actions, for example movement control.
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
	//! Provides a default implementation which returns an error message.
	virtual void get(const QByteArray &operation, const APIParameters &parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	//! Provides a default implementation which returns an error message.
	virtual void post(const QByteArray &operation, const APIParameters &parameters, const QByteArray& data, APIServiceResponse& response) Q_DECL_OVERRIDE;

protected:
	//! This defines the connection type QMetaObject::invokeMethod has to use inside a service: either Qt::DirectConnection for main thread handling, or
	//! Qt::BlockingQueuedConnection for HTTP thread handling
	static const Qt::ConnectionType SERVICE_DEFAULT_INVOKETYPE;

	//! Because the HTML descriptions in Stellarium are often not compatible
	//! with "clean" HTML5 which is used for the main interface,
	//! this method can be used to explicitely set the doctype
	//! to 4.01 transitional for better results, and include the stylesheet
	//! \c iframestyle.css
	//! @param text The text to wrap with HTML document tags
	//! @param title The title of the page
	QString wrapHtml(QString& text, const QString& title) const;
};

//! @}

#endif
