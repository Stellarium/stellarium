/*
 * Stellarium Remote Control plugin
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

#ifndef REMOTECONTROLSERVICEINTERFACE_HPP_
#define REMOTECONTROLSERVICEINTERFACE_HPP_

#include <QByteArray>
#include <QJsonDocument>
#include <QMap>
#include <QObject>


//! \addtogroup remoteControl
//! @{

//! Thread-safe version of HttpResponse that can be passed around through QMetaObject::invokeMethod.
//! It contains the data that will be sent back to the client in the HTTP thread, when control returns to the APIController.
struct APIServiceResponse
{
public:
	//! Constructs an invalid response
	APIServiceResponse() : status(-1)
	{

	}

	//! Sets a specific HTTP header to the specified value
	void setHeader(const QByteArray &name, const QByteArray &val)
	{
		headers.insert(name,val);
	}
	//! Shortcut for int header values
	void setHeader(const QByteArray& name, const int val)
	{
		headers.insert(name,QByteArray::number(val));
	}
	//! Sets the HTTP status type and status text
	void setStatus(int status, const QByteArray& text)
	{
		this->status = status;
		this->statusText = text;
	}

	//! Replaces the current return data
	void setData(const QByteArray& data)
	{
		this->responseData = data;
	}
	//! Appends to the current return data
	void appendData(const QByteArray& data)
	{
		this->responseData.append(data);
	}

	//! Sets the HTTP status to 400, and sets the response data to the message
	void writeRequestError(const QByteArray& msg)
	{
		setStatus(400,"Bad Request");
		responseData = msg;
	}
	//! Sets the Content-Type to "application/json" and serializes the given document
	//! into JSON text format
	void writeJSON(const QJsonDocument& doc)
	{
	#ifdef QT_NO_DEBUG
		//Use compact JSON format for release builds for smaller files
		QByteArray data = doc.toJson(QJsonDocument::Compact);
	#else
		//Use indented JSON format in debug builds for easier human reading
		QByteArray data = doc.toJson(QJsonDocument::Indented);
	#endif
		//setHeader("Content-Length",data.size());
		setHeader("Content-Type","application/json; charset=utf-8");
		setData(data);
	}

private:

	int status;
	QByteArray statusText;
	QMap<QByteArray,QByteArray> headers;
	QByteArray responseData;

	static int metaTypeId;
	static int parametersMetaTypeId;
	friend class APIController;
};

//! Defines the HTTP request parameters for the service
typedef QMultiMap<QByteArray, QByteArray> APIParameters;

Q_DECLARE_METATYPE(APIServiceResponse)
Q_DECLARE_METATYPE(APIParameters)

//! Interface for all \ref remoteControl services. Each implementation is mapped to a separate
//! HTTP request path. The get() or post() method is called to handle each request.
//! @note This interface can be used through the Qt plugin system, for use in other plugins.
class RemoteControlServiceInterface
{
public:
	virtual ~RemoteControlServiceInterface() {}

	//! Returns the desired path mapping
	//! If there is a conflict, only the first object is mapped.
	virtual QLatin1String getPath() const = 0;
	//! Return true if the service's get() and post() methods can safely be run in the HTTP handler thread,
	//! instead of having to queue it into the Stellarium main thread. This can
	//! result in better performance if done correctly.
	//! Unless you are sure, return false here.
	virtual bool isThreadSafe() const = 0;
	//! Implement this to define reactions to HTTP GET requests.
	//! GET requests generally should only query data or program state, and not change it.
	//! If there is an error with the request, use APIServiceResponse::writeRequestError to notify the client.
	//! @param operation The operation string of the request (i.e. the part of the request URL after the service name, without parameters)
	//! @param parameters The extracted service parameters (extracted from the URL)
	//! @param response The response object, write your response into this
	//! @note The thread this is called in depends on the supportThreadedOperation() return value
	virtual void get(const QByteArray& operation, const APIParameters& parameters, APIServiceResponse& response) = 0;
	//! Implement this to define reactions to HTTP POST requests.
	//! POST requests generally should change data or perform some action.
	//! If there is an error with the request, use APIServiceResponse::writeRequestError to notify the client.
	//! @param operation The operation string of the request (i.e. the part of the request URL after the service name, without parameters)
	//! @param parameters The extracted service parameters (extracted from the URL, and form data, if applicable)
	//! @param data The unmodified data as sent from the client
	//! @param response The response object, write your response into this
	//! @note The thread this is called in depends on the supportThreadedOperation() return value
	virtual void post(const QByteArray& operation, const APIParameters& parameters, const QByteArray& data, APIServiceResponse& response) = 0;
	//! Called in the main thread each frame.
	//! Can be used for ongoing actions, for example movement control.
	virtual void update(double deltaTime) = 0;
};

// Qt plugin setup
#define RemoteControlServiceInterface_iid "org.stellarium.plugin.RemoteSync.RemoteControlServiceInterface/1.0"
Q_DECLARE_INTERFACE(RemoteControlServiceInterface, RemoteControlServiceInterface_iid)

//! @}

#endif
