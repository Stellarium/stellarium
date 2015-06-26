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

//! Thread-safe version of HttpResponse that can be passed around through invokeMethod
struct APIServiceResponse
{
public:
	APIServiceResponse() : status(-1)
	{

	}

	void setHeader(const QByteArray& name, const QByteArray& val);
	void setHeader(const QByteArray& name, const int val);
	void setStatus(int status, const QByteArray& text);

	void setData(const QByteArray& data);
	void appendData(const QByteArray& data);

	void writeRequestError(const QByteArray& msg);
	void writeJSON(const QJsonDocument& doc);

private:
	int status;
	QByteArray statusText;
	QMap<QByteArray,QByteArray> headers;
	QByteArray responseData;

	//! Applies the data in this APIServiceResponse onto the HttpResponse
	void applyResponse(HttpResponse* response) const;

	static int metaTypeId;
	static int parametersMetaTypeId;
	friend class APIController;

};

typedef QMultiMap<QByteArray, QByteArray> APIParameters;

Q_DECLARE_METATYPE(APIServiceResponse)
Q_DECLARE_METATYPE(APIParameters)

class AbstractAPIService : public QObject
{
	Q_OBJECT
public:
	AbstractAPIService(const QByteArray& serviceName, QObject* parent = 0) : QObject(parent), m_serviceName(serviceName)
	{

	}

	virtual ~AbstractAPIService() { }

	//! Returns the service name
	QByteArray serviceName() { return m_serviceName; }

	//! Returns true if the service can safely be run in the HTTP handler thread.
	//! Default implementation returns false.
	virtual bool supportsThreadedOperation() const;

	//! Called in the main thread each frame. Default implementation does nothing.
	virtual void update(double deltaTime);

	//Wrappers aroung getImpl/postImpl, which do the actual service
	//The thread these are called in depends on the supportThreadedOperation return value
	//If FORCE_THREADED_SERVICES is set, all services will be run in the HTTP threads for testing
	Q_INVOKABLE APIServiceResponse get(const QByteArray &operation, const APIParameters &parameters);
	Q_INVOKABLE APIServiceResponse post(const QByteArray &operation, const APIParameters &parameters, const QByteArray& data);

protected:
	virtual void getImpl(const QByteArray& operation, const APIParameters& parameters, APIServiceResponse& response);
	virtual void postImpl(const QByteArray& operation, const APIParameters& parameters, const QByteArray& data, APIServiceResponse& response);

	//!This defines the connection type QMetaObject::invokeMethod has to use inside a service: either Qt::DirectConnection for main thread handling, or
	//! Qt::BlockingQueuedConnection for HTTP thread handling
	static const Qt::ConnectionType SERVICE_DEFAULT_INVOKETYPE;

private:
	QByteArray m_serviceName;
};

#endif
