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

#include <QByteArray>
#include <QMap>
#include <QObject>

//Uncomment to force each service to run in the HTTP handling threads themselves, like the initial versions
//#define FORCE_THREADED_SERVICES

class HttpResponse;
class APIController;

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
	void setHeader(const QByteArray& name, const QByteArray& val);
	//! Shortcut for int header values
	void setHeader(const QByteArray& name, const int val);
	//! Sets the HTTP status type and status text
	void setStatus(int status, const QByteArray& text);

	//! Replaces the current return data
	void setData(const QByteArray& data);
	//! Appends to the current return data
	void appendData(const QByteArray& data);

	//! Sets the HTTP status to 400, and sets the response data to the message
	void writeRequestError(const QByteArray& msg);
	//! Sets the Content-Type to "application/json" and serializes the given document
	//! into JSON text format
	void writeJSON(const QJsonDocument& doc);

private:
	int status;
	QByteArray statusText;
	QMap<QByteArray,QByteArray> headers;
	QByteArray responseData;

	//! Applies the data in this APIServiceResponse onto the HttpResponse
	//! Must be called in the HTTP thread, done by APIController
	void applyResponse(HttpResponse* response) const;

	static int metaTypeId;
	static int parametersMetaTypeId;
	friend class APIController;

};

//! Defines the HTTP request parameters for the service
typedef QMultiMap<QByteArray, QByteArray> APIParameters;

Q_DECLARE_METATYPE(APIServiceResponse)
Q_DECLARE_METATYPE(APIParameters)

//! Abstract base class for all \ref remoteControl service implementations. Different implementations of this class are mapped to
//! different HTTP request paths. For each API request, the APIController finds out which service to use, and calls its get() or post() method.
class AbstractAPIService : public QObject
{
	Q_OBJECT
public:
	//! Abstract constructor. The service name is used by the APIController for request path mapping.
	AbstractAPIService(const QByteArray& serviceName, QObject* parent = 0) : QObject(parent), m_serviceName(serviceName)
	{

	}

	virtual ~AbstractAPIService() { }

	//! Returns the service name, used for request path mapping by the APIController
	QByteArray serviceName() { return m_serviceName; }

	//! Return true if the service can safely be run in the HTTP handler thread,
	//! instead of having to queue it into the Stellarium main thread.
	//! Default implementation returns false, i.e. no special threading precautions have to be used
	//! in the get() and post() implementation.
	//! @warning If the macro \c FORCE_THREADED_SERVICES is set, all services will be run
	//! in the HTTP threads for testing, and this method will be ignored.
	virtual bool supportsThreadedOperation() const;

	//! Called in the main thread each frame. Default implementation does nothing.
	//! Can be used for ongoing actions, for example movement control.
	virtual void update(double deltaTime);

	//! Wrapper around getImpl(), constructs an APIServiceResponse object for the response and passes it on
	//! @note The thread this is called in depends on the supportThreadedOperation() return value
	Q_INVOKABLE APIServiceResponse get(const QByteArray &operation, const APIParameters &parameters);
	//! Wrapper around postImpl(), constructs an APIServiceResponse object for the response and passes it on
	//! @note The thread this is called in depends on the supportThreadedOperation() return value
	Q_INVOKABLE APIServiceResponse post(const QByteArray &operation, const APIParameters &parameters, const QByteArray& data);

protected:
	//! Subclasses should implement this to define reactions to HTTP GET requests.
	//! GET requests generally should only query data or program state, and not change it.
	//! If there is an error with the request, use APIServiceResponse::writeRequestError to notify the client.
	//! @param operation The operation string of the request (i.e. the part of the request URL after the service name, without parameters)
	//! @param parameters The extracted service parameters (extracted from the URL)
	//! @param response The response object, write your response into this
	//! @note The thread this is called in depends on the supportThreadedOperation() return value
	virtual void getImpl(const QByteArray& operation, const APIParameters& parameters, APIServiceResponse& response);
	//! Subclasses should implement this to define reactions to HTTP POST requests.
	//! POST requests generally should change data or perform some action.
	//! If there is an error with the request, use APIServiceResponse::writeRequestError to notify the client.
	//! @param operation The operation string of the request (i.e. the part of the request URL after the service name, without parameters)
	//! @param parameters The extracted service parameters (extracted from the URL, and form data, if applicable)
	//! @param data The unmodified data as sent from the client
	//! @param response The response object, write your response into this
	//! @note The thread this is called in depends on the supportThreadedOperation() return value
	virtual void postImpl(const QByteArray& operation, const APIParameters& parameters, const QByteArray& data, APIServiceResponse& response);

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
private:
	QByteArray m_serviceName;
};

//! @}

#endif
