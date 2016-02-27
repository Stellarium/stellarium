/**
  @file
  @author Stefan Frings
*/

#include "httpconnectionhandler.h"
#include "httpresponse.h"

HttpConnectionHandler::HttpConnectionHandler(const HttpConnectionHandlerSettings& settings, HttpRequestHandler* requestHandler, QSslConfiguration* sslConfiguration)
    : QThread()
{
    Q_ASSERT(requestHandler!=0);
    this->settings=settings;
    this->requestHandler=requestHandler;
    this->sslConfiguration=sslConfiguration;
    currentRequest=0;
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


HttpConnectionHandler::~HttpConnectionHandler() {
    quit();
    wait();
    delete socket;
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): destroyed", this);
#endif
}


void HttpConnectionHandler::createSocket() {
    // If SSL is supported and configured, then create an instance of QSslSocket
    #ifndef QT_NO_OPENSSL
        if (sslConfiguration) {
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


void HttpConnectionHandler::run() {
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): thread started", this);
#endif
    try {
        exec();
    }
    catch (...) {
	qCritical("HttpConnectionHandler (%p): an uncaught exception occurred in the thread",this);
    }
    socket->close();
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): thread stopped", this);
#endif
}


void HttpConnectionHandler::handleConnection(tSocketDescriptor socketDescriptor) {
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): handle new connection", this);
#endif
    busy = true;
    Q_ASSERT(socket->isOpen()==false); // if not, then the handler is already busy

    //UGLY workaround - we need to clear writebuffer before reusing this socket
    //https://bugreports.qt-project.org/browse/QTBUG-28914
    socket->connectToHost("",0);
    socket->abort();

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        qCritical("HttpConnectionHandler (%p): cannot initialize socket: %s", this,qPrintable(socket->errorString()));
        return;
    }

    #ifndef QT_NO_OPENSSL
        // Switch on encryption, if SSL is configured
        if (sslConfiguration) {
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
    currentRequest=0;
}


bool HttpConnectionHandler::isBusy() {
    return busy;
}

void HttpConnectionHandler::setBusy() {
    this->busy = true;
}


void HttpConnectionHandler::readTimeout() {
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): read timeout occured",this);
#endif
    //Commented out because QWebView cannot handle this.
    //socket->write("HTTP/1.1 408 request timeout\r\nConnection: close\r\n\r\n408 request timeout\r\n");

    socket->flush();
    socket->disconnectFromHost();
    delete currentRequest;
    currentRequest=0;
}


void HttpConnectionHandler::disconnected() {
#ifndef NDEBUG
    qDebug("HttpConnectionHandler (%p): disconnected", this);
#endif
    socket->close();
    readTimer.stop();
    busy = false;
}

void HttpConnectionHandler::read() {
	// The loop adds support for HTTP pipelinig
	while (socket->bytesAvailable()) {
	#ifdef SUPERVERBOSE
		qDebug("HttpConnectionHandler (%p): read input",this);
	#endif

		// Create new HttpRequest object if necessary
		if (!currentRequest) {
			currentRequest=new HttpRequest(settings.maxRequestSize,settings.maxMultipartSize);
		}

		// Collect data for the request object
		while (socket->bytesAvailable() && currentRequest->getStatus()!=HttpRequest::complete && currentRequest->getStatus()!=HttpRequest::abort) {
			currentRequest->readFromSocket(socket);
			if (currentRequest->getStatus()==HttpRequest::waitForBody) {
				// Restart timer for read timeout, otherwise it would
				// expire during large file uploads.
				int readTimeout=settings.readTimeout;
				readTimer.start(readTimeout);
			}
		}

		// If the request is aborted, return error message and close the connection
		if (currentRequest->getStatus()==HttpRequest::abort) {
			socket->write("HTTP/1.1 413 entity too large\r\nConnection: close\r\n\r\n413 Entity too large\r\n");
			socket->flush();
			socket->disconnectFromHost();
			delete currentRequest;
			currentRequest=0;
			return;
		}

		// If the request is complete, let the request mapper dispatch it
		if (currentRequest->getStatus()==HttpRequest::complete) {
			readTimer.stop();
			#ifndef NDEBUG
			qDebug("HttpConnectionHandler (%p): received request",this);
			#endif
			HttpResponse response(socket);
			try {
				requestHandler->service(*currentRequest, response);
			}
			catch (...) {
				qCritical("HttpConnectionHandler (%p): An uncatched exception occured in the request handler",this);
			}

			// Finalize sending the response if not already done
			if (!response.hasSentLastPart()) {
				response.write(QByteArray(),true);
			}

			#ifndef NDEBUG
			qDebug("HttpConnectionHandler (%p): finished request",this);
			#endif

			// Close the connection after delivering the response, if requested
			if (QString::compare(currentRequest->getHeader("Connection"),"close",Qt::CaseInsensitive)==0) {
				socket->flush();
				socket->disconnectFromHost();
			}
			else {
				// Start timer for next request
				int readTimeout=settings.readTimeout;
				readTimer.start(readTimeout);
			}
			// Prepare for next request
			delete currentRequest;
			currentRequest=0;
		}
	}
}
