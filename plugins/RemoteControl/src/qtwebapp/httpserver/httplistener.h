/**
  @file
  @author Stefan Frings
*/

#ifndef LISTENER_H
#define LISTENER_H

#include <QTcpServer>
#include <QSettings>
#include <QBasicTimer>
#include "httpglobal.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include "httprequesthandler.h"

/**
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
  @see HttpConnectionHandlerPool for description of config settings minThreads, maxThreads and cleanupInterval
  @see HttpConnectionHandler for description of the ssl settings and readTimeout
  @see HttpRequest for description of config settings maxRequestSize and maxMultiPartSize
*/

class DECLSPEC HttpListener : public QTcpServer {
    Q_OBJECT
    Q_DISABLE_COPY(HttpListener)
public:

    /**
      Constructor.
      @param settings Configuration settings for the HTTP server. Must not be 0.
      @param requestHandler Processes each received HTTP request, usually by dispatching to controller classes.
      @param parent Parent object.
    */
    HttpListener(QSettings* settings, HttpRequestHandler* requestHandler, QObject* parent = 0);

    /** Destructor */
    virtual ~HttpListener();

protected:

    /** Serves new incoming connection requests */
    void incomingConnection(tSocketDescriptor socketDescriptor);

private:

    /** Configuration settings for the HTTP server */
    QSettings* settings;

    /** Pool of connection handlers */
    HttpConnectionHandlerPool* pool;

signals:

    /**
      Emitted when the connection handler shall process a new incoming onnection.
      @param socketDescriptor references the accepted connection.
    */

    void handleConnection(tSocketDescriptor socketDescriptor);

};

#endif // LISTENER_H
