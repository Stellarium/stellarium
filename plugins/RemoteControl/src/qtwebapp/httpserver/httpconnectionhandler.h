/**
  @file
  @author Stefan Frings
*/

#ifndef HTTPCONNECTIONHANDLER_H
#define HTTPCONNECTIONHANDLER_H

#include <QTcpSocket>
#include <QTimer>
#include <QThread>
#include <QSslConfiguration>
#include "httpglobal.h"
#include "httprequest.h"
#include "httprequesthandler.h"

/** Alias type definition, for compatibility to different Qt versions */
#if QT_VERSION >= 0x050000
    typedef qintptr tSocketDescriptor;
#else
    typedef int tSocketDescriptor;
#endif

/**
  Contains all settings for the connection handler
 */
struct HttpConnectionHandlerSettings {
	HttpConnectionHandlerSettings()
		: readTimeout(10000),maxRequestSize(16384),maxMultipartSize(1048576)
	{}

	/** Defines the maximum time to wait for a complete HTTP request in msec. Default 10000. */
	int readTimeout;
	/** The file path to the SSL key file. Default empty. */
	QString sslKeyFile;
	/** The file path to the SSL cert file. Default empty. */
	QString sslCertFile;
	/** Maximum size of a request in bytes. Default 16384. */
	int maxRequestSize;
	/** Maximum size of a multipart request in bytes. Default 1048576 (1MB) */
	int maxMultipartSize;
};

/**
  The connection handler accepts incoming connections and dispatches incoming requests to to a
  request mapper. Since HTTP clients can send multiple requests before waiting for the response,
  the incoming requests are queued and processed one after the other.
  <p>
  Example for the required configuration settings:
  <code><pre>
  readTimeout=60000
  ;sslKeyFile=ssl/my.key
  ;sslCertFile=ssl/my.cert
  maxRequestSize=16000
  maxMultiPartSize=1000000
  </pre></code>
  <p>
  The readTimeout value defines the maximum time to wait for a complete HTTP request.
  <p>
  For SSL support, you need an openssl certificate file and a key file.
Both can be created with the command
  <code><pre>
  openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout my.key -out my.cert
  </pre></code>
  <p>
  Please not that a connection handler with ssl settings can only handle HTTPS protocol. To
  support bth HTTP and HTTPS simultaneously, you need to start two listeners on different ports -
  one with ssl and one without ssl.
  @see HttpRequest for description of config settings maxRequestSize and maxMultiPartSize
*/
class DECLSPEC HttpConnectionHandler : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY(HttpConnectionHandler)

public:

    /**
      Constructor.
      @param settings Configuration settings of the HTTP webserver
      @param requestHandler handler that will process each incomin HTTP request
    */
    HttpConnectionHandler(const HttpConnectionHandlerSettings& settings, HttpRequestHandler* requestHandler);

    /** Destructor */
    virtual ~HttpConnectionHandler();

    /** Returns true, if this handler is in use. */
    bool isBusy();

    /** Mark this handler as busy */
    void setBusy();

private:

    /** Configuration settings */
    HttpConnectionHandlerSettings settings;

    /** TCP socket of the current connection (used for both HTTP and HTTPS) */
    QSslSocket* socket;

    /** Holds the configuration settings for SSL */
    QSslConfiguration sslConfiguration;

    /** Whether SSL is enabled */
    bool useSsl;

    /** Time for read timeout detection */
    QTimer readTimer;

    /** Storage for the current incoming HTTP request */
    HttpRequest* currentRequest;

    /** Dispatches received requests to services */
    HttpRequestHandler* requestHandler;

    /** This shows the busy-state from a very early time */
    bool busy;

    /** Executes the threads own event loop */
    void run();

    /** Load the SSL certificate and key files */
    void loadSslConfig();

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
