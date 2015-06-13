/*
 * Copyright (C) 2012 Sibi Antony
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

#ifdef ENABLE_VIDEO
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

	QMediaContent content(QUrl::fromLocalFile(filename));
	videoObjects[id]->player->setMedia(content);
	StelMainView::getInstance().scene()->addItem(videoObjects[id]->videoItem);
	// TODO: See whether we also must remove the videoItem later?

	videoObjects[id]->videoItem->setPos(x, y);
	videoObjects[id]->videoItem->setOpacity(alpha);
	videoObjects[id]->videoItem->setVisible(show);
	//videoObjects[id]->player->show(); // ??

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
//			if (w!=-1.0f && h!=-1.0f)
//				videoObjects[id]->videoItem->setAspectRatioMode(Qt::IgnoreAspectRatio);
//			else
//			{
//				videoObjects[id]->videoItem->setAspectRatioMode(Qt::KeepAspectRatio);
//				if (w==-1.0f && h==-1.0f)
//				{
//					h=videoSize.height();
//					w=videoSize.width();
//				}
//				else if (h==-1.0f)
//					h=w/aspectRatio;
//				else if (w==-1.0f)
//					w=h*aspectRatio;
//			}
//			videoObjects[id]->videoItem->setSize(QSizeF(w, h));
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
#endif // ENABLE_VIDEO


