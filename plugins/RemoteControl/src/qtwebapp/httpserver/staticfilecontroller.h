/**
  @file
  @author Stefan Frings
*/

#ifndef STATICFILECONTROLLER_H
#define STATICFILECONTROLLER_H

#include <QCache>
#include <QMutex>
#include "httpglobal.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httprequesthandler.h"

/**
  Delivers static files. It is usually called by the applications main request handler when
  the caller request a path that is mapped to static files.
  <p>
  The following settings are required in the config file:
  <code><pre>
  path=../docroot
  encoding=UTF-8
  maxAge=60000
  cacheTime=60000
  cacheSize=1000000
  maxCachedFileSize=65536
  </pre></code>
  The path is relative to the directory of the config file. In case of windows, if the
  settings are in the registry, the path is relative to the current working directory.
  <p>
  The encoding is sent to the web browser in case of text and html files.
  <p>
  The cache improves performance of small files when loaded from a network
  drive. Large files are not cached. Files are cached as long as possible,
  when cacheTime=0. The maxAge value (in msec!) controls the remote browsers cache.
  <p>
  Do not instantiate this class in each request, because this would make the file cache
  useless. Better create one instance during start-up and call it when the application
  received a related HTTP request.
*/

class DECLSPEC StaticFileController : public HttpRequestHandler  {
    Q_OBJECT
    Q_DISABLE_COPY(StaticFileController)
public:

    /** Constructor */
    StaticFileController(QSettings* settings, QObject* parent = 0);

    /** Generates the response */
    void service(HttpRequest& request, HttpResponse& response);

private:

    /** Encoding of text files */
    QString encoding;

    /** Root directory of documents */
    QString docroot;

    /** Maximum age of files in the browser cache */
    int maxAge;    

    struct CacheEntry {
        QByteArray document;
        qint64 created;
        QByteArray filename;
    };

    /** Timeout for each cached file */
    int cacheTimeout;

    /** Maximum size of files in cache, larger files are not cached */
    int maxCachedFileSize;

    /** Cache storage */
    QCache<QString,CacheEntry> cache;

    /** Used to synchronize cache access for threads */
    QMutex mutex;

    /** Set a content-type header in the response depending on the ending of the filename */
    void setContentType(QString file, HttpResponse& response) const;
};

#endif // STATICFILECONTROLLER_H
