/*
 * Copyright (C) 2012 Sibi Antony (Phonon/QT4)
 * Copyright (C) 2015 Georg Zotti (Reactivate for QT5)
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelVideoMgr.hpp"
#include "StelMainView.hpp"
#include <QDebug>
#include <QDir>
//#include <QVideoWidget> // Adapt to that when we finally switch to QtQuick2!
#include <QGraphicsVideoItem>
#include <QMediaPlayer>


StelVideoMgr::StelVideoMgr()
{
}

#ifdef ENABLE_MEDIA
StelVideoMgr::~StelVideoMgr()
{
	foreach(QString id, videoObjects.keys())
	{
		dropVideo(id);
	}
}

void StelVideoMgr::loadVideo(const QString& filename, const QString& id, const float x, const float y, const bool show, const float alpha)
{
	if (videoObjects.contains(id))
	{
		qWarning() << "[StelVideoMgr] Video object with ID" << id << "already exists, dropping it";
		dropVideo(id);
	}

	videoObjects[id] = new VideoPlayer;
	videoObjects[id]->player = new QMediaPlayer();
	videoObjects[id]->videoItem= new QGraphicsVideoItem();

	// It seems we need an absolute pathname here.
	qDebug() << "Trying to load" << filename;
	QMediaContent content(QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()));
	qDebug() << "Loaded " << content.canonicalUrl();
	// On Windows, this reveals we have nothing loaded!
	qDebug() << "Media Resources:";
	qDebug() << "\tData size:     " << content.canonicalResource().dataSize();
	qDebug() << "\tMIME Type:     " << content.canonicalResource().mimeType();
	qDebug() << "\tVideo codec:   " << content.canonicalResource().videoCodec();
	qDebug() << "\tResolution:    " << content.canonicalResource().resolution();
	qDebug() << "\tVideo bitrate: " << content.canonicalResource().videoBitRate();
	qDebug() << "\tAudio codec:   " << content.canonicalResource().audioCodec();
	qDebug() << "\tChannel Count: " << content.canonicalResource().channelCount();
	qDebug() << "\tAudio bitrate: " << content.canonicalResource().audioBitRate();
	qDebug() << "\tSample rate:   " << content.canonicalResource().sampleRate();
	qDebug() << "\tlanguage:      " << content.canonicalResource().language();
	videoObjects[id]->player->setMedia(content);
	videoObjects[id]->player->setVideoOutput(videoObjects[id]->videoItem);
	// Maybe ask only player?
	qDebug() << "Media Resources from player:";
	qDebug() << "\tSTATUS:        " << videoObjects[id]->player->mediaStatus();
	qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().canonicalUrl();
	qDebug() << "\tData size:     " << videoObjects[id]->player->currentMedia().canonicalResource().dataSize();
	qDebug() << "\tMIME Type:     " << videoObjects[id]->player->currentMedia().canonicalResource().mimeType();
	qDebug() << "\tVideo codec:   " << videoObjects[id]->player->currentMedia().canonicalResource().videoCodec();
	qDebug() << "\tResolution:    " << videoObjects[id]->player->currentMedia().canonicalResource().resolution();
	qDebug() << "\tVideo bitrate: " << videoObjects[id]->player->currentMedia().canonicalResource().videoBitRate();
	qDebug() << "\tAudio codec:   " << videoObjects[id]->player->currentMedia().canonicalResource().audioCodec();
	qDebug() << "\tChannel Count: " << videoObjects[id]->player->currentMedia().canonicalResource().channelCount();
	qDebug() << "\tAudio bitrate: " << videoObjects[id]->player->currentMedia().canonicalResource().audioBitRate();
	qDebug() << "\tSample rate:   " << videoObjects[id]->player->currentMedia().canonicalResource().sampleRate();
	qDebug() << "\tlanguage:      " << videoObjects[id]->player->currentMedia().canonicalResource().language();

	StelMainView::getInstance().scene()->addItem(videoObjects[id]->videoItem);
	// TODO: Apparently we must wait until status=loaded. (?)

	videoObjects[id]->videoItem->setPos(x, y);
	//videoObjects[id]->videoItem->setSize(videoObjects[id]->player->media().resources()[0].resolution());
	videoObjects[id]->videoItem->setSize(content.resources().at(0).resolution());
	// DEBUG show a box. I had vieo playing in a tiny box already, once..!
	videoObjects[id]->videoItem->setSize(QSizeF(150, 150));

	videoObjects[id]->videoItem->setOpacity(alpha);
	videoObjects[id]->videoItem->setVisible(show);
	qDebug() << "Loaded video" << id << "for pos " << x << "/" << y << "Size" << videoObjects[id]->videoItem->size();

}

void StelVideoMgr::playVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			// if already playing, stop and play from the start
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
			{
				videoObjects[id]->player->stop();
			}

			// otherwise just play it, or resume playing paused video.
			qDebug() << "StelVideoMgr::playVideo: " << id;
			videoObjects[id]->player->play();
		}
	}
}

void StelVideoMgr::pauseVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->pause();
		}
	}
}

void StelVideoMgr::stopVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->stop();
		}
	}
}

void StelVideoMgr::seekVideo(const QString& id, const qint64 ms)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL) 
		{
			if (videoObjects[id]->player->isSeekable())
			{
				videoObjects[id]->player->setPosition(ms);
				// Seek capability depends on the backend used.
			}
			else
			{
				qDebug() << "[StelVideoMgr] Cannot seek media source.";
			}
		}
	}
}

void StelVideoMgr::dropVideo(const QString& id)
{
	if (!videoObjects.contains(id))
		return;
	if (videoObjects[id]->player!=NULL)
	{
		videoObjects[id]->player->stop();
		StelMainView::getInstance().scene()->removeItem(videoObjects[id]->videoItem);
		delete videoObjects[id]->player;
		delete videoObjects[id]->videoItem;
		delete videoObjects[id];

		videoObjects.remove(id); 
	}
}

void StelVideoMgr::setVideoXY(const QString& id, const float x, const float y)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			videoObjects[id]->videoItem->setPos(x, y);
		}
	}

}

void StelVideoMgr::setVideoAlpha(const QString& id, const float alpha)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			videoObjects[id]->videoItem->setOpacity(alpha);
		}
	}
}

void StelVideoMgr::resizeVideo(const QString& id, float w, float h)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			QSize videoSize=videoObjects[id]->player->currentMedia().resources()[1].resolution();
			float aspectRatio=videoSize.width()/videoSize.height();
			// TODO: Repair next lines, linker does not find function.
			if (w!=-1.0f && h!=-1.0f)
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::IgnoreAspectRatio);
			else
			{
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::KeepAspectRatio);
				if (w==-1.0f && h==-1.0f)
				{
					h=videoSize.height();
					w=videoSize.width();
				}
				else if (h==-1.0f)
					h=w/aspectRatio;
				else if (w==-1.0f)
					w=h*aspectRatio;
			}
			videoObjects[id]->videoItem->setSize(QSizeF(w, h));
		}
	}
}

void StelVideoMgr::showVideo(const QString& id, const bool show)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			videoObjects[id]->videoItem->setVisible(show);
		}
	}
}

#else 
void StelVideoMgr::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	qWarning() << "[StelVideoMgr] This build of Stellarium does not support video - cannot load video" << QDir::toNativeSeparators(filename) << id << x << y << show << alpha;
}
StelVideoMgr::~StelVideoMgr() {;}
void StelVideoMgr::playVideo(const QString&) {;}
void StelVideoMgr::pauseVideo(const QString&) {;}
void StelVideoMgr::stopVideo(const QString&) {;}
void StelVideoMgr::dropVideo(const QString&) {;}
void StelVideoMgr::seekVideo(const QString&, qint64) {;}
void StelVideoMgr::setVideoXY(const QString&, float, float) {;}
void StelVideoMgr::setVideoAlpha(const QString&, float) {;}
void StelVideoMgr::resizeVideo(const QString&, float, float) {;}
void StelVideoMgr::showVideo(const QString&, bool) {;}
#endif // ENABLE_MEDIA


