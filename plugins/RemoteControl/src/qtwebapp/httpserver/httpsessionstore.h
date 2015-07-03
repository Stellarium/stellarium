/**
  @file
  @author Stefan Frings
*/

#ifndef HTTPSESSIONSTORE_H
#define HTTPSESSIONSTORE_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include "httpglobal.h"
#include "httpsession.h"
#include "httpresponse.h"
#include "httprequest.h"

struct HttpSessionStoreSettings
{
	HttpSessionStoreSettings()
		: expirationTime(3600000),cookieName("sessionid")
	{}

	/** The time in msec after which sessions expire. Default 3600000 (1 min). */
	int expirationTime;
	/** The name for the cookie that stores the session id. Default "sessionid" */
	QByteArray cookieName;
	/** A comment string for the cookie. Default empty. */
	QByteArray cookieComment;
	/** The path where the cookie is valid. Default empty (valid for whole domain). */
	QByteArray cookiePath;
	/** The domain where the cookie is valid. Default empty (valid for current domain). */
	QByteArray cookieDomain;
};

/**
  Stores HTTP sessions and deletes them when they have expired.
  The following configuration settings are required in the config file:
  <code><pre>
  expirationTime=3600000
  cookieName=sessionid
  </pre></code>
  The following additional configurations settings are optionally:
  <code><pre>
  cookiePath=/
  cookieComment=Session ID
  ;cookieDomain=stefanfrings.de
  </pre></code>
*/

class DECLSPEC HttpSessionStore : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(HttpSessionStore)
public:

    /** Constructor. */
    HttpSessionStore(const HttpSessionStoreSettings& settings, QObject* parent);

    /** Destructor */
    virtual ~HttpSessionStore();

    /**
       Get the ID of the current HTTP session, if it is valid.
       This method is thread safe.
       @warning Sessions may expire at any time, so subsequent calls of
       getSession() might return a new session with a different ID.
       @param request Used to get the session cookie
       @param response Used to get and set the new session cookie
       @return Empty string, if there is no valid session.
    */
    QByteArray getSessionId(HttpRequest& request, HttpResponse& response);

    /**
       Get the session of a HTTP request, eventually create a new one.
       This method is thread safe. New sessions can only be created before
       the first byte has been written to the HTTP response.
       @param request Used to get the session cookie
       @param response Used to get and set the new session cookie
       @param allowCreate can be set to false, to disable the automatic creation of a new session.
       @return If autoCreate is disabled, the function returns a null session if there is no session.
       @see HttpSession::isNull()
    */
    HttpSession getSession(HttpRequest& request, HttpResponse& response, bool allowCreate=true);

    /**
       Get a HTTP session by it's ID number.
       This method is thread safe.
       @return If there is no such session, the function returns a null session.
       @param id ID number of the session
       @see HttpSession::isNull()
    */
    HttpSession getSession(const QByteArray id);

    /** Delete a session */
    void removeSession(HttpSession session);

private:

    /** Configuration settings */
    HttpSessionStoreSettings settings;

    /** Storage for the sessions */
    QMap<QByteArray,HttpSession> sessions;

    /** Timer to remove expired sessions */
    QTimer cleanupTimer;

    /** Used to synchronize threads */
    QMutex mutex;

private slots:

    /** Called every minute to cleanup expired sessions. */
    void timerEvent();
};

#endif // HTTPSESSIONSTORE_H
