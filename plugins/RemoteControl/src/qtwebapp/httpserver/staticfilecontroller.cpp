/**
  @file
  @author Stefan Frings
*/

#include "staticfilecontroller.h"
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMimeDatabase>

StaticFileController::StaticFileController(const StaticFileControllerSettings& settings, QObject* parent)
    :HttpRequestHandler(parent)
{
    maxAge=settings.maxAge;
    encoding=settings.encoding;
    docroot=settings.path;
    if(!(docroot.startsWith(":/") || docroot.startsWith("qrc://")))
    {
        // Convert relative path to absolute, based on the current working directory
        if (QDir::isRelativePath(docroot))
        {
            docroot=QDir(docroot).absolutePath();
        }
    }
    qDebug("StaticFileController: docroot=%s, encoding=%s, maxAge=%i",qPrintable(docroot),qPrintable(encoding),maxAge);
    maxCachedFileSize=settings.maxCachedFileSize;
    cache.setMaxCost(settings.cacheSize);
    cacheTimeout=settings.cacheTime;
    qDebug("StaticFileController: cache timeout=%i, size=%i",cacheTimeout,cache.maxCost());
}


void StaticFileController::service(HttpRequest& request, HttpResponse& response)
{
    QByteArray path=request.getPath();
    // Check if we have the file in cache
    qint64 now=QDateTime::currentMSecsSinceEpoch();
    mutex.lock();
    CacheEntry* entry=cache.object(path);
    if (entry && (cacheTimeout==0 || entry->created>now-cacheTimeout))
    {
        QByteArray document=entry->document; //copy the cached document, because other threads may destroy the cached entry immediately after mutex unlock.
        QByteArray filename=entry->filename;
        mutex.unlock();
#ifndef NDEBUG
        qDebug("StaticFileController: Cache hit for %s",path.data());
#endif
        setContentType(filename,response);
        response.setHeader("Cache-Control","max-age="+QByteArray::number(maxAge));
        response.write(document);
    }
    else
    {
        mutex.unlock();
        // The file is not in cache.
#ifndef NDEBUG
        qDebug("StaticFileController: Cache miss for %s",path.data());
#endif
        // Forbid access to files outside the docroot directory
        if (path.contains("/.."))
        {
            qWarning("StaticFileController: detected forbidden characters in path %s",path.data());
            response.setStatus(403,"forbidden");
            response.write("403 forbidden",true);
            return;
        }
        // If the filename is a directory, append index.html.
        if (QFileInfo(docroot+path).isDir())
        {
            path+="/index.html";
        }
        // Try to open the file
        QFile file(docroot+path);
#ifndef NDEBUG
        qDebug("StaticFileController: Open file %s",qPrintable(file.fileName()));
#endif
        if (file.open(QIODevice::ReadOnly))
        {
            setContentType(path,response);
            response.setHeader("Cache-Control","max-age="+QByteArray::number(maxAge));
            if (file.size()<=maxCachedFileSize)
            {
                // Return the file content and store it also in the cache
                entry=new CacheEntry();
                while (!file.atEnd() && !file.error())
                {
                    QByteArray buffer=file.read(65536);
                    response.write(buffer);
                    entry->document.append(buffer);
                }
                entry->created=now;
                entry->filename=path;
                mutex.lock();
                cache.insert(request.getPath(),entry,entry->document.size());
                mutex.unlock();
            }
            else
            {
                // Return the file content, do not store in cache
                while (!file.atEnd() && !file.error())
                {
                    response.write(file.read(65536));
                }
            }
            file.close();
        }
        else {
            if (file.exists())
            {
                qWarning("StaticFileController: Cannot open existing file %s for reading",qPrintable(file.fileName()));
                response.setStatus(403,"forbidden");
                response.write("403 forbidden",true);
            }
            else
            {
                response.setStatus(404,"not found");
                response.write("404 not found",true);
            }
        }
    }
}

QByteArray StaticFileController::getContentType(QString fileName, QString encoding)
{
	//Directly return the most commonly used types
	if (fileName.endsWith(".png")) {
		return "image/png";
	}
	else if (fileName.endsWith(".jpg")) {
		return "image/jpeg";
	}
	else if (fileName.endsWith(".gif")) {
		return "image/gif";
	}
	else if (fileName.endsWith(".pdf")) {
		return "application/pdf";
	}
	else if (fileName.endsWith(".txt")) {
		return qPrintable("text/plain; charset="+encoding);
	}
	else if (fileName.endsWith(".html") || fileName.endsWith(".htm")) {
		return qPrintable("text/html; charset="+encoding);
	}
	else if (fileName.endsWith(".css")) {
		return "text/css";
	}
	else if (fileName.endsWith(".js")) {
		return qPrintable("text/javascript; charset="+encoding);
	}
	else if (fileName.endsWith(".woff")) {
		return qPrintable("application/font-woff");
	}
	else if (fileName.endsWith(".woff2")) {
		return qPrintable("application/font-woff2");
	}
	else if (fileName.endsWith(".ttf")) {
		return qPrintable("application/x-font-ttf");
	}
	else if (fileName.endsWith(".otf")) {
		return qPrintable("application/x-font-otf");
	}
	else if (fileName.endsWith(".eot")) {
		return qPrintable("application/vnd.ms-fontobject");
	}
	else if (fileName.endsWith(".svg")) {
		return qPrintable("image/svg+xml");
	}
	else{
		//Query Qt for file type, using only the name for now
		QMimeDatabase mimeData;
		QMimeType type = mimeData.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);

		if(type.isValid())
			return qPrintable(type.name());
		return QByteArray();
	}
}

void StaticFileController::setContentType(QString fileName, HttpResponse& response) const 
{
	QByteArray contentType = getContentType(fileName,encoding);
	if(!contentType.isEmpty())
		response.setHeader("Content-Type", contentType);
}
