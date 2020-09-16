/**
  @file
  @author Stefan Frings
*/

#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H

#include <QTcpServer>
#include <QBasicTimer>
#include "httpglobal.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include "httprequesthandler.h"

/**
 @ingroup qtWebApp
 Contains all settings for HttpListener and supporting classes.
*/
struct HttpListenerSettings : public HttpConnectionHandlerPoolSettings
{
	HttpListenerSettings()
		:port(8080)
	{}

	/** The local IP address to bind to. Default empty (listen on all interfaces). */
	QString host;
	/** The HTTP port to use. Default 8080. */
	int port;
};

/**
  @ingroup qtWebApp
  Listens for incoming TCP connections and and passes all incoming HTTP requests to your implementation of HttpRequestHandler,
  which processes the request and generates the response (usually a HTML document).
  <p>
  Example for the required settings in the config file:
  <code><pre>
  ;host=192.168.0.100
  port=8080
  minThreads=1
  maxThreads=10
  cleanupInterval=1000
  readTimeout=60000
  ;sslKeyFile=ssl/my.key
  ;sslCertFile=ssl/my.cert
  maxRequestSize=16000
  maxMultiPartSize=1000000
  </pre></code>
  The optional host parameter binds the listener to one network interface.
  The listener handles all network interfaces if no host is configured.
  The port number specifies the incoming TCP port that this listener listens to.
  @see HttpConnectionHandlerPool for description of config settings minThreads, maxThreads, cleanupInterval and ssl settings
  @see HttpConnectionHandler for description of the readTimeout
  @see HttpRequest for description of config settings maxRequestSize and maxMultiPartSize
*/
class DECLSPEC HttpListener : public QTcpServer {
    Q_OBJECT
    Q_DISABLE_COPY(HttpListener)

public:
    /**
      Constructor.
      Creates a connection pool and starts listening on the configured host and port.
      @param settings Configuration settings for the HTTP server. Must not be 0.
      @param requestHandler Processes each received HTTP request, usually by dispatching to controller classes.
      @param parent Parent object.
      @warning Ensure to close or delete the listener before deleting the request handler.
    */
    HttpListener(const HttpListenerSettings& settings, HttpRequestHandler* requestHandler, QObject* parent = Q_NULLPTR);

    /** Destructor */
    virtual ~HttpListener();

    /**
      Restart listeing after close().
    */
    void listen();

    /**
     Closes the listener, waits until all pending requests are processed,
     then closes the connection pool.
    */
    void close();

protected:
    /** Serves new incoming connection requests */
    void incomingConnection(tSocketDescriptor socketDescriptor);

private:
    /** Configuration settings for the HTTP server */
    HttpListenerSettings settings;

    /** Point to the reuqest handler which processes all HTTP requests */
    HttpRequestHandler* requestHandler;

    /** Pool of connection handlers */
    HttpConnectionHandlerPool* pool;

signals:
    /**
      Sent to the connection handler to process a new incoming connection.
      @param socketDescriptor references the accepted connection.
    */

    void handleConnection(tSocketDescriptor socketDescriptor);
};

#endif // HTTPLISTENER_H
