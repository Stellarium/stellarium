/**
  @file
  @author Stefan Frings
*/

#include "httpsessionstore.h"
#include <QDateTime>
#include <QUuid>

HttpSessionStore::HttpSessionStore(QSettings* settings, QObject* parent)
    :QObject(parent)
{
    this->settings=settings;
    connect(&cleanupTimer,SIGNAL(timeout()),this,SLOT(timerEvent()));
    cleanupTimer.start(60000);
    cookieName=settings->value("cookieName","sessionid").toByteArray();
    expirationTime=settings->value("expirationTime",3600000).toInt();
    qDebug("HttpSessionStore: Sessions expire after %i milliseconds",expirationTime);
}

HttpSessionStore::~HttpSessionStore()
{
    cleanupTimer.stop();
}

QByteArray HttpSessionStore::getSessionId(HttpRequest& request, HttpResponse& response) {
    // The session ID in the response has priority because this one will be used in the next request.
    mutex.lock();
    // Get the session ID from the response cookie
    QByteArray sessionId=response.getCookies().value(cookieName).getValue();
    if (sessionId.isEmpty()) {
        // Get the session ID from the request cookie
        sessionId=request.getCookie(cookieName);
    }
    // Clear the session ID if there is no such session in the storage.
    if (!sessionId.isEmpty()) {
        if (!sessions.contains(sessionId)) {
            qDebug("HttpSessionStore: received invalid session cookie with ID %s",sessionId.data());
            sessionId.clear();
        }
    }
    mutex.unlock();
    return sessionId;
}

HttpSession HttpSessionStore::getSession(HttpRequest& request, HttpResponse& response, bool allowCreate) {
    QByteArray sessionId=getSessionId(request,response);
    mutex.lock();
    if (!sessionId.isEmpty()) {
        HttpSession session=sessions.value(sessionId);
        if (!session.isNull()) {
            mutex.unlock();
            // Refresh the session cookie
            QByteArray cookieName=settings->value("cookieName","sessionid").toByteArray();
            QByteArray cookiePath=settings->value("cookiePath").toByteArray();
            QByteArray cookieComment=settings->value("cookieComment").toByteArray();
            QByteArray cookieDomain=settings->value("cookieDomain").toByteArray();
            response.setCookie(HttpCookie(cookieName,session.getId(),expirationTime/1000,cookiePath,cookieComment,cookieDomain));
            session.setLastAccess();
            return session;
        }
    }
    // Need to create a new session
    if (allowCreate) {
        QByteArray cookieName=settings->value("cookieName","sessionid").toByteArray();
        QByteArray cookiePath=settings->value("cookiePath").toByteArray();
        QByteArray cookieComment=settings->value("cookieComment").toByteArray();
        QByteArray cookieDomain=settings->value("cookieDomain").toByteArray();
        HttpSession session(true);
        qDebug("HttpSessionStore: create new session with ID %s",session.getId().data());
        sessions.insert(session.getId(),session);
        response.setCookie(HttpCookie(cookieName,session.getId(),expirationTime/1000,cookiePath,cookieComment,cookieDomain));
        mutex.unlock();
        return session;
    }
    // Return a null session
    mutex.unlock();
    return HttpSession();
}

HttpSession HttpSessionStore::getSession(const QByteArray id) {
    mutex.lock();
    HttpSession session=sessions.value(id);
    mutex.unlock();
    session.setLastAccess();
    return session;
}

void HttpSessionStore::timerEvent() {
    // Todo: find a way to delete sessions only if no controller is accessing them
    mutex.lock();
    qint64 now=QDateTime::currentMSecsSinceEpoch();
    QMap<QByteArray,HttpSession>::iterator i = sessions.begin();
    while (i != sessions.end()) {
        QMap<QByteArray,HttpSession>::iterator prev = i;
        ++i;
        HttpSession session=prev.value();
        qint64 lastAccess=session.getLastAccess();
        if (now-lastAccess>expirationTime) {
            qDebug("HttpSessionStore: session %s expired",session.getId().data());
            sessions.erase(prev);
        }
    }
    mutex.unlock();
}


/** Delete a session */
void HttpSessionStore::removeSession(HttpSession session) {
    mutex.lock();
    sessions.remove(session.getId());
    mutex.unlock();
}
