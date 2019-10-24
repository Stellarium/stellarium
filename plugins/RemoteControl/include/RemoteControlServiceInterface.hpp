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

#ifndef REMOTECONTROLSERVICEINTERFACE_HPP
#define REMOTECONTROLSERVICEINTERFACE_HPP

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QMap>
#include <QMimeDatabase>
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
		headers[name]=val;
	}
	//! Shortcut for int header values
	void setHeader(const QByteArray& name, const int val)
	{
		headers[name]=QByteArray::number(val);
	}

	//! Sets the time in seconds for which the browser is allowed to cache the reply
	void setCacheTime(int seconds)
	{
		setHeader("Cache-Control","max-age="+QByteArray::number(seconds));
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

	//! Because the HTML descriptions in Stellarium are often not compatible
	//! with "clean" HTML5 which is used for the main interface,
	//! this method can be used to explicitely wrap the given HTML snippet
	//! in a valid HTML 4.01 transitional document for better results, and include the stylesheet
	//! \c iframestyle.css for consistent styling when used in iframes of the RemoteControl web interface
	//! @param html The HTML snippet to wrap with HTML document tags
	//! @param title The title of the page
	void writeWrappedHTML(const QString& html, const QString& title)
	{
		QString wrapped = QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n<html><head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n<title>")
				+ title + QStringLiteral("</title>\n<link type=\"text/css\" rel=\"stylesheet\" href=\"/iframestyle.css\">\n<base target=\"_blank\">\n</head><body>\n")
				+ html + QStringLiteral("</body></html>");

		setHeader("Content-Type","text/html; charset=UTF-8");
		setData(wrapped.toUtf8());
	}

	//! Writes the specified file contents into the response.
	//! @param path The (preferably absolute) path to a file
	//! @param allowCaching if true, the browser is allowed to cache the file for one hour
	void writeFile(const QString& path, bool allowCaching = false)
	{
		QFile file(path);
		if (path.isEmpty() || !file.exists())
		{
			setStatus(404,"not found");
			setData("requested file resource not found");
			return;
		}

		QMimeType mime = QMimeDatabase().mimeTypeForFile(path);

		if(file.open(QIODevice::ReadOnly))
		{
			if(allowCaching)
				setHeader("Cache-Control","max-age="+QByteArray::number(60*60));

			//reply with correct mime type if possible
			if(!mime.isDefault())
			{
				setHeader("Content-Type", mime.name().toLatin1());
			}

			//load and write data
			setData(file.readAll());
		}
		else
		{
			qWarning()<<"Could not open requested file resource"<<path<<file.errorString();
			setStatus(500,"internal server error");
			setData("could not open file resource");
		}
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
//! Instances of this class which are provided through the StelModuleMgr's extension mechanism
//! (by adding it into the list returned by StelPluginInterface::getExtensionList()) are automatically
//! discovered and registered with the APIController.
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

// Q_DECLARE_INTERFACE enables qobject_cast for the interface
#define RemoteControlServiceInterface_iid "org.stellarium.plugin.RemoteSync.RemoteControlServiceInterface/1.0"
Q_DECLARE_INTERFACE(RemoteControlServiceInterface, RemoteControlServiceInterface_iid)

//! @}

#endif
