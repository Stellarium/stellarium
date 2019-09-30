/**
  @file
  @author Stefan Frings
*/

#include "httpconnectionhandler.h"
#include "httpresponse.h"

HttpConnectionHandler::HttpConnectionHandler(const HttpConnectionHandlerSettings& settings, HttpRequestHandler* requestHandler, HttpSslConfiguration *sslConfiguration)
    : QThread()
{
    Q_ASSERT(requestHandler!=Q_NULLPTR);
    this->settings=settings;
    this->requestHandler=requestHandler;
    this->sslConfiguration=sslConfiguration;
    currentRequest=Q_NULLPTR;
    busy=false;

    // Create TCP or SSL socket
    createSocket();

    // execute signals in my own thread
    moveToThread(this);
    socket->moveToThread(this);
    readTimer.moveToThread(this);

    // Connect signals
    connect(socket, SIGNAL(readyRead()), SLOT(read()));
    connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
    connect(&readTimer, SIGNAL(timeout()), SLOT(readTimeout()));
    readTimer.setSingleShot(true);

#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): constructed", this);
#endif
    this->start();
}


HttpConnectionHandler::~HttpConnectionHandler()
{
    quit();
    wait();
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): destroyed", this);
#endif
}


void HttpConnectionHandler::createSocket()
{
    // If SSL is supported and configured, then create an instance of QSslSocket
    #ifndef QT_NO_OPENSSL
        if (sslConfiguration)
        {
            QSslSocket* sslSocket=new QSslSocket();
            sslSocket->setSslConfiguration(*sslConfiguration);
            socket=sslSocket;
	#ifndef NDEBUG
            qDebug("HttpConnectionHandler (%p): SSL is enabled", this);
	#endif
            return;
        }
    #endif
    // else create an instance of QTcpSocket
    socket=new QTcpSocket();
}


void HttpConnectionHandler::run()
{
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): thread started", this);
#endif
    try 
    {
        exec();
    }
    catch (...)
    {
        qCritical("HttpConnectionHandler (%p): an uncaught exception occurred in the thread",this);
    }
    socket->close();
    delete socket;
    readTimer.stop();
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): thread stopped", this);
#endif
}


void HttpConnectionHandler::handleConnection(tSocketDescriptor socketDescriptor)
{
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): handle new connection", this);
#endif
    busy = true;
    Q_ASSERT(socket->isOpen()==false); // if not, then the handler is already busy

    //UGLY workaround - we need to clear writebuffer before reusing this socket
    //https://bugreports.qt-project.org/browse/QTBUG-28914
    socket->connectToHost("",0);
    socket->abort();

    if (!socket->setSocketDescriptor(socketDescriptor))
    {
        qCritical("HttpConnectionHandler (%p): cannot initialize socket: %s", this,qPrintable(socket->errorString()));
        return;
    }

    #ifndef QT_NO_OPENSSL
        // Switch on encryption, if SSL is configured
        if (sslConfiguration)
        {
#ifndef NDEBUG
            qDebug("HttpConnectionHandler (%p): Starting encryption", this);
#endif
            ((QSslSocket*)socket)->startServerEncryption();
        }
    #endif

    // Start timer for read timeout
    int readTimeout=settings.readTimeout;
    readTimer.start(readTimeout);
    // delete previous request
    delete currentRequest;
    currentRequest=Q_NULLPTR;
}


bool HttpConnectionHandler::isBusy()
{
    return busy;
}

void HttpConnectionHandler::setBusy()
{
    this->busy = true;
}


void HttpConnectionHandler::readTimeout()
{
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): read timeout occured",this);
#endif
    //Commented out because QWebView cannot handle this.
    //socket->write("HTTP/1.1 408 request timeout\r\nConnection: close\r\n\r\n408 request timeout\r\n");

    socket->flush();
    socket->disconnectFromHost();
    delete currentRequest;
    currentRequest=Q_NULLPTR;
}


void HttpConnectionHandler::disconnected()
{
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): disconnected", this);
#endif
    socket->close();
    readTimer.stop();
    busy = false;
}

void HttpConnectionHandler::read()
{
    // The loop adds support for HTTP pipelinig
    while (socket->bytesAvailable())
    {
        #ifdef SUPERVERBOSE
            qDebug("HttpConnectionHandler (%p): read input",this);
        #endif

        // Create new HttpRequest object if necessary
        if (!currentRequest)
        {
            currentRequest=new HttpRequest(settings.maxRequestSize,settings.maxMultipartSize);
        }

        // Collect data for the request object
        while (socket->bytesAvailable() && currentRequest->getStatus()!=HttpRequest::complete && currentRequest->getStatus()!=HttpRequest::abort)
        {
            currentRequest->readFromSocket(socket);
            if (currentRequest->getStatus()==HttpRequest::waitForBody)
            {
                // Restart timer for read timeout, otherwise it would
                // expire during large file uploads.
                int readTimeout=settings.readTimeout;
                readTimer.start(readTimeout);
            }
        }

        // If the request is aborted, return error message and close the connection
        if (currentRequest->getStatus()==HttpRequest::abort)
        {
            socket->write("HTTP/1.1 413 entity too large\r\nConnection: close\r\n\r\n413 Entity too large\r\n");
            socket->flush();
            socket->disconnectFromHost();
            delete currentRequest;
	    currentRequest=Q_NULLPTR;
            return;
        }

        // If the request is complete, let the request mapper dispatch it
        if (currentRequest->getStatus()==HttpRequest::complete)
        {
            readTimer.stop();
#ifndef NDEBUG
	    //qDebug("HttpConnectionHandler (%p): received request",this);
#endif

            // Copy the Connection:close header to the response
            HttpResponse response(socket);
            bool closeConnection=QString::compare(currentRequest->getHeader("Connection"),"close",Qt::CaseInsensitive)==0;
            if (closeConnection)
            {
                response.setHeader("Connection","close");
            }

            // In case of HTTP 1.0 protocol add the Connection:close header.
            // This ensures that the HttpResponse does not activate chunked mode, which is not spported by HTTP 1.0.
            else
            {
                bool http1_0=QString::compare(currentRequest->getVersion(),"HTTP/1.0",Qt::CaseInsensitive)==0;
                if (http1_0)
                {
                    closeConnection=true;
                    response.setHeader("Connection","close");
                }
            }

            // Call the request mapper
            try
            {
                requestHandler->service(*currentRequest, response);
            }
            catch (...)
            {
                qCritical("HttpConnectionHandler (%p): An uncatched exception occured in the request handler",this);
            }

            // Finalize sending the response if not already done
            if (!response.hasSentLastPart())
            {
                response.write(QByteArray(),true);
            }

#ifndef NDEBUG
	    //qDebug("HttpConnectionHandler (%p): finished request",this);
#endif

            // Find out whether the connection must be closed
            if (!closeConnection)
            {
                // Maybe the request handler or mapper added a Connection:close header in the meantime
                bool closeResponse=QString::compare(response.getHeaders().value("Connection"),"close",Qt::CaseInsensitive)==0;
                if (closeResponse==true)
                {
                    closeConnection=true;
                }
                else
                {
                    // If we have no Content-Length header and did not use chunked mode, then we have to close the
                    // connection to tell the HTTP client that the end of the response has been reached.
                    bool hasContentLength=response.getHeaders().contains("Content-Length");
                    if (!hasContentLength)
                    {
                        bool hasChunkedMode=QString::compare(response.getHeaders().value("Transfer-Encoding"),"chunked",Qt::CaseInsensitive)==0;
                        if (!hasChunkedMode)
                        {
                            closeConnection=true;
                        }
                    }
                }
            }

            // Close the connection or prepare for the next request on the same connection.
            if (closeConnection)
            {
                socket->flush();
                socket->disconnectFromHost();
            }
            else
            {
                // Start timer for next request
                int readTimeout=settings.readTimeout;
                readTimer.start(readTimeout);
            }
            delete currentRequest;
	    currentRequest=Q_NULLPTR;
        }
    }
}
