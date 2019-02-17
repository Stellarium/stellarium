#ifndef HTTPCONNECTIONHANDLERPOOL_H
#define HTTPCONNECTIONHANDLERPOOL_H

#include <QList>
#include <QTimer>
#include <QObject>
#include <QMutex>
#include "httpglobal.h"
#include "httpconnectionhandler.h"

/**
  @ingroup qtWebApp
  Contains all settings for the connection handler pool and the child connection handlers.
 */
struct HttpConnectionHandlerPoolSettings : public HttpConnectionHandlerSettings
{
	HttpConnectionHandlerPoolSettings()
		: minThreads(1), maxThreads(100), cleanupInterval(1000)
	{}

	/** The minimal number of threads kept in the pool at all times. Default 1. */
	int minThreads;
	/** The maximal number of threads. If a new connection arrives when this amount of threads is already busy,
	 * the new connection will be dropped. Default 100. */
	int maxThreads;
	/** The time after which inactive threads are stopped in msec. Default 1000.  */
	int cleanupInterval;
	/** The file path to the SSL key file. Default empty. */
	QString sslKeyFile;
	/** The file path to the SSL cert file. Default empty. */
	QString sslCertFile;
};

/**
  @ingroup qtWebApp
  Pool of http connection handlers. The size of the pool grows and
  shrinks on demand.
  <p>
  Example for the required configuration settings:
  <code><pre>
  minThreads=4
  maxThreads=100
  cleanupInterval=60000
  readTimeout=60000
  ;sslKeyFile=ssl/my.key
  ;sslCertFile=ssl/my.cert
  maxRequestSize=16000
  maxMultiPartSize=1000000
  </pre></code>
  After server start, the size of the thread pool is always 0. Threads
  are started on demand when requests come in. The cleanup timer reduces
  the number of idle threads slowly by closing one thread in each interval.
  But the configured minimum number of threads are kept running.
  <p>
  For SSL support, you need an OpenSSL certificate file and a key file.
  Both can be created with the command
  <code><pre>
      openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout my.key -out my.cert
  </pre></code>
  <p>
  Visit http://slproweb.com/products/Win32OpenSSL.html to download the Light version of OpenSSL for Windows.
  <p>
  Please note that a listener with SSL settings can only handle HTTPS protocol. To
  support both HTTP and HTTPS simultaneously, you need to start two listeners on different ports -
  one with SLL and one without SSL.
  @see HttpConnectionHandler for description of the readTimeout
  @see HttpRequest for description of config settings maxRequestSize and maxMultiPartSize
*/

class DECLSPEC HttpConnectionHandlerPool : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(HttpConnectionHandlerPool)

public:
    /**
      Constructor.
      @param settings Configuration settings for the HTTP server. Must not be 0.
      @param requestHandler The handler that will process each received HTTP request.
      @warning The requestMapper gets deleted by the destructor of this pool
    */
    HttpConnectionHandlerPool(const HttpConnectionHandlerPoolSettings& settings, HttpRequestHandler* requestHandler);

    /** Destructor */
    virtual ~HttpConnectionHandlerPool();

    /** Get a free connection handler, or 0 if not available. */
    HttpConnectionHandler* getConnectionHandler();

private:
    /** Settings for this pool */
    HttpConnectionHandlerPoolSettings settings;

    /** Will be assigned to each Connectionhandler during their creation */
    HttpRequestHandler* requestHandler;

    /** Pool of connection handlers */
    QList<HttpConnectionHandler*> pool;

    /** Timer to clean-up unused connection handler */
    QTimer cleanupTimer;

    /** Used to synchronize threads */
    QMutex mutex;

    /** The SSL configuration (certificate, key and other settings) */
    HttpSslConfiguration* sslConfiguration;

    /** Load SSL configuration */
    void loadSslConfig();

private slots:
    /** Received from the clean-up timer.  */
    void cleanup();
};

#endif // HTTPCONNECTIONHANDLERPOOL_H
