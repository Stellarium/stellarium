#include "httpconnectionhandlerpool.h"

HttpConnectionHandlerPool::HttpConnectionHandlerPool(const HttpConnectionHandlerPoolSettings &settings, HttpRequestHandler* requestHandler)
    : QObject()
{
    this->settings=settings;
    this->requestHandler=requestHandler;
    cleanupTimer.start(settings.cleanupInterval);
    connect(&cleanupTimer, SIGNAL(timeout()), SLOT(cleanup()));
}


HttpConnectionHandlerPool::~HttpConnectionHandlerPool() {
    // delete all connection handlers and wait until their threads are closed
    foreach(HttpConnectionHandler* handler, pool) {
       delete handler;
    }
    qDebug("HttpConnectionHandlerPool (%p): destroyed", this);
}


HttpConnectionHandler* HttpConnectionHandlerPool::getConnectionHandler() {   
    HttpConnectionHandler* freeHandler=0;
    mutex.lock();
    // find a free handler in pool
    foreach(HttpConnectionHandler* handler, pool) {
        if (!handler->isBusy()) {
            freeHandler=handler;
            freeHandler->setBusy();
            break;
        }
    }
    // create a new handler, if necessary
    if (!freeHandler) {
	int maxConnectionHandlers=settings.maxThreads;
        if (pool.count()<maxConnectionHandlers) {
            freeHandler=new HttpConnectionHandler(settings,requestHandler);
            freeHandler->setBusy();
            pool.append(freeHandler);
        }
    }
    mutex.unlock();
    return freeHandler;
}



void HttpConnectionHandlerPool::cleanup() {
    int maxIdleHandlers=settings.minThreads;
    int idleCounter=0;
    mutex.lock();
    foreach(HttpConnectionHandler* handler, pool) {
        if (!handler->isBusy()) {
            if (++idleCounter > maxIdleHandlers) {
                pool.removeOne(handler);                
                delete handler;
                qDebug("HttpConnectionHandlerPool: Removed connection handler (%p), pool size is now %i",handler,pool.size());
                break; // remove only one handler in each interval
            }
        }
    }
    mutex.unlock();
}
