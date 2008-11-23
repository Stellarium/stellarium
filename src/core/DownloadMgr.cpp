/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
 
#include "DownloadMgr.hpp"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QFile>

DownloadMgr::~DownloadMgr()
{
	if(reply)
	{
		delete reply;
		delete target;
		if(barVisible)
			delete progressBar;
	}
}

void DownloadMgr::get(const QString& addr, const QString& filePath, quint16 csum)
{
	if(inProgress)
	{
		qDebug() << "Can't begin another download while" << addr << "is still in progress.";
		return;
	}
	
	if(reply)
	{
		delete target;
		delete reply;
	}
	
	address = addr;
	path = filePath;
	target = new QFile(filePath);
	stream = new QDataStream(target);
	checksum = csum;
	received = 0;
	total = 0;
	inProgress = true;
	
	target->open(QIODevice::WriteOnly | QIODevice::Truncate);
	reply = networkManager->get(QNetworkRequest(QUrl(address)));
	
	if(barVisible)
	{
		if(!progressBar)
			progressBar = StelMainGraphicsView::getInstance().addProgressBar();
		progressBar->setValue(0);
		progressBar->setMaximum(0);
		progressBar->setVisible(true);
		progressBar->setFormat(barFormat);
	}
	connect(reply, SIGNAL(readyRead()), this, SLOT(readData()));
	connect(reply, SIGNAL(finished()), this, SLOT(fin()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(err(QNetworkReply::NetworkError)));
	if(barVisible)
		connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(updateDownloadBar(qint64, qint64)));
}

void DownloadMgr::abort()
{
	inProgress = false;
	if(barVisible)
	{
		progressBar->setValue(0);
		progressBar->setMaximum(0);
		progressBar->setVisible(false);
	}
	reply->abort();
	target->close();
}

void DownloadMgr::readData()
{
	int size = reply->bytesAvailable();
	QByteArray data = reply->read(size);
	stream->writeRawData(data.constData(), size);
}

void DownloadMgr::updateDownloadBar(qint64 received, qint64 total)
{
	progressBar->setMaximum(total);
	progressBar->setValue(received);
}

void DownloadMgr::fin()
{
	if(reply->error())
		return;
	
	if(barVisible)
	{
		progressBar->setValue(0);
		progressBar->setMaximum(0);
		progressBar->setVisible(false);
	}
	
	inProgress = false;
	if(total-received == 0)
	{
		target->close();
		
		if(useChecksum)
		{
			emit verifying();
			
			target->open(QIODevice::ReadOnly);
			quint16 fileChecksum = qChecksum(target->readAll().constData(), target->size());
			target->close();
			
			if(fileChecksum != checksum)
				emit badChecksum();
		}
		emit finished();
	} else
		if(!target->remove())
			qDebug() << "Error deleting incomplete file" << path;
}

void DownloadMgr::err(QNetworkReply::NetworkError code)
{
	if(code != QNetworkReply::NoError)
	{
		inProgress = false;
		target->close();
		if(!target->remove())
			qDebug() << "Error deleting incomplete file" << path;
		if(barVisible)
		{
			progressBar->setValue(0);
			progressBar->setMaximum(0);
			progressBar->setVisible(false);
		}
	}
	
	if(code != QNetworkReply::OperationCanceledError)
		emit error(code, reply->errorString());
}
