/**
  @file
  @author Stefan Frings
*/

#include <QSslKey>
#include <QSslCertificate>
#include <QDir>
#include "httpconnectionhandler.h"
#include "httpresponse.h"

HttpConnectionHandler::HttpConnectionHandler(QSettings* settings, HttpRequestHandler* requestHandler)
    : QThread()
{
    Q_ASSERT(settings!=0);
    Q_ASSERT(requestHandler!=0);
    this->settings=settings;
    this->requestHandler=requestHandler;
    currentRequest=0;
    busy = false;
    useSsl = false;

    // Load SSL configuration
    loadSslConfig();

    // Create a socket object, used for both HTTP and HTTPS
    socket=new QSslSocket();

    // execute signals in my own thread
    moveToThread(this);
    socket->moveToThread(this);
    readTimer.moveToThread(this);
    connect(socket, SIGNAL(readyRead()), SLOT(read()));
    connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
    connect(&readTimer, SIGNAL(timeout()), SLOT(readTimeout()));
    readTimer.setSingleShot(true);
    qDebug("HttpConnectionHandler (%p): constructed", this);
    this->start();
}


HttpConnectionHandler::~HttpConnectionHandler() {
    quit();
    wait();
    qDebug("HttpConnectionHandler (%p): destroyed", this);
}


void HttpConnectionHandler::loadSslConfig() {
    QString sslKeyFileName=settings->value("sslKeyFile","").toString();
    QString sslCertFileName=settings->value("sslCertFile","").toString();
    if (!sslKeyFileName.isEmpty() && !sslCertFileName.isEmpty()) {
        // Convert relative fileNames to absolute, based on the directory of the config file.
        QFileInfo configFile(settings->fileName());
        #ifdef Q_OS_WIN32
            if (QDir::isRelativePath(sslKeyFileName) && settings->format()!=QSettings::NativeFormat)
        #else
            if (QDir::isRelativePath(sslKeyFileName))
        #endif
        {
            sslKeyFileName=QFileInfo(configFile.absolutePath(),sslKeyFileName).absoluteFilePath();
        }
        #ifdef Q_OS_WIN32
            if (QDir::isRelativePath(sslCertFileName) && settings->format()!=QSettings::NativeFormat)
        #else
            if (QDir::isRelativePath(sslCertFileName))
        #endif
        {
            sslCertFileName=QFileInfo(configFile.absolutePath(),sslCertFileName).absoluteFilePath();
        }
        // Load the two files
        QFile certFile(sslCertFileName);
        if (!certFile.open(QIODevice::ReadOnly)) {
            qCritical("HttpConnectionHandler (%p): cannot open sslCertFile %s", this, qPrintable(sslCertFileName));
            return;
        }
        QSslCertificate certificate(&certFile, QSsl::Pem);
        certFile.close();
        QFile keyFile(sslKeyFileName);
        if (!keyFile.open(QIODevice::ReadOnly)) {
            qCritical("HttpConnectionHandler (%p): cannot open sslKeyFile %s", this, qPrintable(sslKeyFileName));
            return;
        }
        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
        keyFile.close();
        sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfiguration.setLocalCertificate(certificate);
        sslConfiguration.setPrivateKey(sslKey);
        sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
        useSsl=true;
        qDebug("HttpConnectionHandler (%p): SSL is enabled", this);
    }
}


void HttpConnectionHandler::run() {
    qDebug("HttpConnectionHandler (%p): thread started", this);
    try {
        exec();

	socket->close();
	delete socket;
    }
    catch (...) {
        qCritical("HttpConnectionHandler (%p): an uncatched exception occured in the thread",this);
    }
    qDebug("HttpConnectionHandler (%p): thread stopped", this);
}


void HttpConnectionHandler::handleConnection(tSocketDescriptor socketDescriptor) {
	Q_ASSERT(QThread::currentThread() == this);
	Q_ASSERT(this->thread() == this);

    qDebug("HttpConnectionHandler (%p): handle new connection", this);
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

    // Switch on encryption, if enabled
    if (useSsl) {
        socket->setSslConfiguration(sslConfiguration);
        socket->startServerEncryption();
    }

    // Start timer for read timeout
    int readTimeout=settings->value("readTimeout",10000).toInt();
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
	Q_ASSERT(QThread::currentThread() == this);

    qDebug("HttpConnectionHandler (%p): read timeout occured",this);

    //Commented out because QWebView cannot handle this.
    //socket->write("HTTP/1.1 408 request timeout\r\nConnection: close\r\n\r\n408 request timeout\r\n");

    socket->disconnectFromHost();
    delete currentRequest;
    currentRequest=0;
}


void HttpConnectionHandler::disconnected() {
	Q_ASSERT(QThread::currentThread() == this);

    qDebug("HttpConnectionHandler (%p): disconnected", this);
    socket->close();
    readTimer.stop();
    busy = false;
}

void HttpConnectionHandler::read() {
	Q_ASSERT(QThread::currentThread() == this);

    // The loop adds support for HTTP pipelinig
    while (socket->bytesAvailable()) {
        #ifdef SUPERVERBOSE
            qDebug("HttpConnectionHandler (%p): read input",this);
        #endif

        // Create new HttpRequest object if necessary
        if (!currentRequest) {
            currentRequest=new HttpRequest(settings);
        }

        // Collect data for the request object
        while (socket->bytesAvailable() && currentRequest->getStatus()!=HttpRequest::complete && currentRequest->getStatus()!=HttpRequest::abort) {
            currentRequest->readFromSocket(socket);
            if (currentRequest->getStatus()==HttpRequest::waitForBody) {
                // Restart timer for read timeout, otherwise it would
                // expire during large file uploads.
                int readTimeout=settings->value("readTimeout",10000).toInt();
                readTimer.start(readTimeout);
            }
        }

        // If the request is aborted, return error message and close the connection
        if (currentRequest->getStatus()==HttpRequest::abort) {
            socket->write("HTTP/1.1 413 entity too large\r\nConnection: close\r\n\r\n413 Entity too large\r\n");
            socket->disconnectFromHost();
            delete currentRequest;
            currentRequest=0;
            return;
        }

        // If the request is complete, let the request mapper dispatch it
        if (currentRequest->getStatus()==HttpRequest::complete) {
            readTimer.stop();
            qDebug("HttpConnectionHandler (%p): received request",this);
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
            // Close the connection after delivering the response, if requested
            if (QString::compare(currentRequest->getHeader("Connection"),"close",Qt::CaseInsensitive)==0) {
                socket->disconnectFromHost();
            }
            else {
                // Start timer for next request
                int readTimeout=settings->value("readTimeout",10000).toInt();
                readTimer.start(readTimeout);
            }
            // Prepare for next request
            delete currentRequest;
            currentRequest=0;
        }
    }
}
