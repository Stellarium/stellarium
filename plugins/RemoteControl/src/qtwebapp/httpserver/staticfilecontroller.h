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
 * @ingroup qtWebApp
 * Contains all the settings for the StaticFileController
*/
struct StaticFileControllerSettings
{
	StaticFileControllerSettings()
		: encoding("UTF-8"),maxAge(60),cacheTime(60000),cacheSize(1048576),maxCachedFileSize(65536)
	{}

	/** The path to the document root. Default empty (= current working directory). */
	QString path;
	/** The assumed encoding of textual files (txt, html). Default UTF-8 */
	QString encoding;
	/** Maximum age of files in the browser cache (HTTP Cache-Control: max-age), in seconds. Default 60. */
	int maxAge;
	/** Maximum age of files in the serverside cache (in msec). Default 60000. */
	int cacheTime;
	/** Total serverside cache size in bytes. Default 1048576 (1MB) */
	int cacheSize;
	/** Maximum size of a single file in the serverside cache. Default 65536 (64KB) */
	int maxCachedFileSize;
};


/**
  @ingroup qtWebApp
  Delivers static files. It is usually called by the applications main request handler when
  the caller requests a path that is mapped to static files.
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
    StaticFileController(const StaticFileControllerSettings& settings, QObject* parent = Q_NULLPTR);

    /** Generates the response */
    void service(HttpRequest& request, HttpResponse& response);

    QString getDocRoot() const { return docroot; }

    /** Returns the content type of this file. This is currently only determined by file name. */
    static QByteArray getContentType(QString fileName, QString encoding);

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
