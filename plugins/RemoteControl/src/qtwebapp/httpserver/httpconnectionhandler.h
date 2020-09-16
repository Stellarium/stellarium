/**
  @file
  @author Stefan Frings
*/

#ifndef HTTPCONNECTIONHANDLER_H
#define HTTPCONNECTIONHANDLER_H

#ifndef QT_NO_OPENSSL
   #include <QSslConfiguration>
#endif
#include <QTcpSocket>
#include <QTimer>
#include <QThread>
#include "httpglobal.h"
#include "httprequest.h"
#include "httprequesthandler.h"

/**
  @ingroup qtWebApp
  Alias type definition, for compatibility to different Qt versions
*/
#if QT_VERSION >= 0x050000
    typedef qintptr tSocketDescriptor;
#else
    typedef int tSocketDescriptor;
#endif

/**
  @ingroup qtWebApp
  Alias for QSslConfiguration if OpenSSL is not supported
*/
#ifdef QT_NO_OPENSSL
typedef QObject HttpSslConfiguration;
#else
typedef QSslConfiguration HttpSslConfiguration;
#endif

/**
  @ingroup qtWebApp
  Contains all settings for the connection handler
 */
struct HttpConnectionHandlerSettings {
	HttpConnectionHandlerSettings()
		: readTimeout(10000),maxRequestSize(16384),maxMultipartSize(1048576)
	{}

	/** Defines the maximum time to wait for a complete HTTP request in msec. Default 10000. */
	int readTimeout;
	/** Maximum size of a request in bytes. Default 16384. */
	int maxRequestSize;
	/** Maximum size of a multipart request in bytes. Default 1048576 (1MB) */
	int maxMultipartSize;
};


/**
  @ingroup qtWebApp
  The connection handler accepts incoming connections and dispatches incoming requests to to a
  request mapper. Since HTTP clients can send multiple requests before waiting for the response,
  the incoming requests are queued and processed one after the other.
  <p>
  Example for the required configuration settings:
  <code><pre>
  readTimeout=60000
  maxRequestSize=16000
  maxMultiPartSize=1000000
  </pre></code>
  <p>
  The readTimeout value defines the maximum time to wait for a complete HTTP request.
  @see HttpRequest for description of config settings maxRequestSize and maxMultiPartSize.
*/
class DECLSPEC HttpConnectionHandler : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY(HttpConnectionHandler)

public:
    /**
      Constructor.
      @param settings Configuration settings of the HTTP webserver
      @param requestHandler Handler that will process each incoming HTTP request
      @param sslConfiguration SSL (HTTPS) will be used if not Q_NULLPTR
    */
    HttpConnectionHandler(const HttpConnectionHandlerSettings& settings, HttpRequestHandler* requestHandler, HttpSslConfiguration* sslConfiguration=Q_NULLPTR);

    /** Destructor */
    virtual ~HttpConnectionHandler();

    /** Returns true, if this handler is in use. */
    bool isBusy();

    /** Mark this handler as busy */
    void setBusy();

private:
    /** Configuration settings */
    HttpConnectionHandlerSettings settings;

    /** TCP socket of the current connection  */
    QTcpSocket* socket;

    /** Time for read timeout detection */
    QTimer readTimer;

    /** Storage for the current incoming HTTP request */
    HttpRequest* currentRequest;

    /** Dispatches received requests to services */
    HttpRequestHandler* requestHandler;

    /** This shows the busy-state from a very early time */
    bool busy;

    /** Configuration for SSL */
    HttpSslConfiguration* sslConfiguration;

    /** Executes the threads own event loop */
    void run();

    /**  Create SSL or TCP socket */
    void createSocket();

public slots:
    /**
      Received from from the listener, when the handler shall start processing a new connection.
      @param socketDescriptor references the accepted connection.
    */
    void handleConnection(tSocketDescriptor socketDescriptor);

private slots:
    /** Received from the socket when a read-timeout occured */
    void readTimeout();

    /** Received from the socket when incoming data can be read */
    void read();

    /** Received from the socket when a connection has been closed */
    void disconnected();
};

#endif // HTTPCONNECTIONHANDLER_H
