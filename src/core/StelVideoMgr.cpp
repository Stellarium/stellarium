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
#include "StelMainGraphicsView.hpp"
#include <QDebug>


StelVideoMgr::StelVideoMgr()
{
}

#ifdef HAVE_QT_PHONON
StelVideoMgr::~StelVideoMgr()
{
	foreach(QString id, videoObjects.keys())
	{
		dropVideo(id);
	}
}

void StelVideoMgr::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	if (videoObjects.contains(id))
	{
		qWarning() << "[StelVideoMgr] Video object with ID" << id << "already exists, dropping it";
		dropVideo(id);
	}

	videoObjects[id] = new VideoPlayer;
	videoObjects[id]->widget = new QWidget();
	videoObjects[id]->player = new Phonon::VideoPlayer(Phonon::VideoCategory, videoObjects[id]->widget);

	videoObjects[id]->player->load(Phonon::MediaSource(filename));
	videoObjects[id]->pWidget = 
		StelMainGraphicsView::getInstance().scene()->addWidget(videoObjects[id]->widget, Qt::FramelessWindowHint);

	videoObjects[id]->pWidget->setPos(x, y);
	videoObjects[id]->pWidget->setOpacity(alpha);
	videoObjects[id]->pWidget->setVisible(show);
	videoObjects[id]->player->show();

}

void StelVideoMgr::playVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			// if already playing, stop and play from the start
			if (videoObjects[id]->player->isPlaying() == true)
			{
				videoObjects[id]->player->stop();
			}

			// otherwise just play it
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

void StelVideoMgr::seekVideo(const QString& id, qint64 ms)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL) 
		{
			if (videoObjects[id]->player->mediaObject()->isSeekable())
			{
				videoObjects[id]->player->seek(ms);
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
		delete videoObjects[id]->pWidget;
		delete videoObjects[id];

		videoObjects.remove(id); 
	}
}

void StelVideoMgr::setVideoXY(const QString& id, float x, float y)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->pWidget!=NULL)
		{
			videoObjects[id]->pWidget->setPos(x, y);
		}
	}

}

void StelVideoMgr::setVideoAlpha(const QString& id, float alpha)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->pWidget!=NULL)
		{
			videoObjects[id]->pWidget->setOpacity(alpha);
		}
	}
}

void StelVideoMgr::resizeVideo(const QString& id, float w, float h)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->pWidget!=NULL)
		{
			videoObjects[id]->pWidget->resize(w, h); 
			videoObjects[id]->player->resize(w, h); 
		}
	}
}

void StelVideoMgr::showVideo(const QString& id, bool show) 
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->pWidget!=NULL)
		{
			videoObjects[id]->pWidget->setVisible(show);
		}
	}
}

#else  // HAVE_QT_PHONON
void StelVideoMgr::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	qWarning() << "[StelVideoMgr] This build of Stellarium does not support video - cannot load video" << filename << id << x << y << show << alpha;
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
#endif // HAVE_QT_PHONON


