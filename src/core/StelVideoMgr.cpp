/*
 * Copyright (C) 2012 Sibi Antony (Phonon/QT4 implementation)
 * Copyright (C) 2015 Georg Zotti (Reactivated with QT5 classes)
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
#ifdef ENABLE_MEDIA
	#include <QGraphicsVideoItem>
	#include <QMediaPlayer>
	#include <QTimer>
	#include <QApplication>
	#include "StelApp.hpp"
	#include "StelFader.hpp"
#endif


StelVideoMgr::StelVideoMgr() : StelModule()
{
    setObjectName("StelVideoMgr");
#ifdef ENABLE_MEDIA
    // in case the property has not been set, getProperty() returns invalid.
    verbose= (qApp->property("verbose") == true);
#endif
}

#ifdef ENABLE_MEDIA
StelVideoMgr::~StelVideoMgr()
{
	for (const auto& id : videoObjects.keys())
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
	videoObjects[id]->videoItem= new QGraphicsVideoItem();
	// This sets a tiny size so that if window should appear before proper resize, it should not disturb.
	videoObjects[id]->videoItem->setSize(QSizeF(1,1));

	videoObjects[id]->player = new QMediaPlayer(Q_NULLPTR, QMediaPlayer::VideoSurface);
	videoObjects[id]->duration=-1; // -1 to signal "unknown".
	videoObjects[id]->resolution=QSize(); // initialize with "invalid" empty resolution, we must detect this when player is starting!
	videoObjects[id]->keepVisible=false;
	videoObjects[id]->needResize=true; // resolution and target frame not yet known.
	videoObjects[id]->simplePlay=true;
	videoObjects[id]->targetFrameSize=QSizeF(); // start with invalid, depends on parameters given in playVideo(), playPopoutVideo() and resolution detected only after playing started.
	videoObjects[id]->popupOrigin=QPointF();
	videoObjects[id]->popupTargetCenter=QPointF();
	videoObjects[id]->player->setProperty("Stel_id", id); // allow tracking of log messages and access of members.
	videoObjects[id]->player->setVideoOutput(videoObjects[id]->videoItem);
	videoObjects[id]->videoItem->setOpacity(alpha);
#ifndef Q_OS_WIN
	// There is a notable difference: on Windows this causes a crash. On Linux, it is required, else the movie frame is visible before proper resize.
	videoObjects[id]->videoItem->setVisible(show);
#endif
	videoObjects[id]->lastPos=-1;

	// A few connections are not really needed, they are signals we don't use. TBD: Remove or keep commented out?
	connect(videoObjects[id]->player, SIGNAL(bufferStatusChanged(int)), this, SLOT(handleBufferStatusChanged(int)));
	connect(videoObjects[id]->player, SIGNAL(durationChanged(qint64)), this, SLOT(handleDurationChanged(qint64))); // (CRITICALLY IMPORTANT!)
	connect(videoObjects[id]->player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleError(QMediaPlayer::Error)));
	connect(videoObjects[id]->player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus)));
	//connect(videoObjects[id]->player, SIGNAL(positionChanged(qint64)), this, SLOT(handlePositionChanged(qint64)));
	// we test isSeekable() where needed, so only debug log entry. --> And we may use the signal however now during blocking load below!
	connect(videoObjects[id]->player, SIGNAL(seekableChanged(bool)), this, SLOT(handleSeekableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handleStateChanged(QMediaPlayer::State)));
	connect(videoObjects[id]->player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(handleVideoAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(audioAvailableChanged(bool)), this, SLOT(handleAudioAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(mutedChanged(bool)), this, SLOT(handleMutedChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(volumeChanged(int)), this, SLOT(handleVolumeChanged(int)));
	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(bool)), this, SLOT(handleAvailabilityChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(QMultimedia::AvailabilityStatus)), this, SLOT(handleAvailabilityChanged(QMultimedia::AvailabilityStatus)));

	// Only this is triggered also on Windows. Lets us read resolution etc. (CRITICALLY IMPORTANT!)
	connect(videoObjects[id]->player, SIGNAL(metaDataChanged()), this, SLOT(handleMetaDataChanged()));
	// That signal would require less overhead, but is not triggered under Windows and apparently also not on MacOS X. (QTBUG-42034.)
	// If this becomes available/bugfixed, it should be preferred as being more elegant.
	// connect(videoObjects[id]->player, SIGNAL(metaDataChanged(QString,QVariant)), this, SLOT(handleMetaDataChanged(QString,QVariant)));

	// We need an absolute pathname here.
	QMediaContent content(QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()));
	videoObjects[id]->player->setMedia(content);
	if (verbose)
	{
		qDebug() << "Loading " << content.canonicalUrl();
		qDebug() << "Media Resources queried from player:";
		qDebug() << "\tSTATUS:        " << videoObjects[id]->player->mediaStatus();
		qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().canonicalUrl();
	}
//	qDebug() << "scene->addItem...";
	StelMainView::getInstance().scene()->addItem(videoObjects[id]->videoItem);
//	qDebug() << "scene->addItem OK";

	videoObjects[id]->videoItem->setPos(x, y);
	// DEFAULT SIZE: show a tiny frame. This gets updated to native resolution as soon as resolution becomes known. Needed?
	//videoObjects[id]->videoItem->setSize(QSizeF(1, 1));

	// after many troubles with incompletely loaded files we attempt a blocking load from https://wiki.qt.io/Seek_in_Sound_File
	// This may be no longer required. But please keep the block here for testing/reactivation if necessary.
//	if (! videoObjects[id]->player->isSeekable())
//	{
//		qDebug() << "Not Seekable!";
//		if (verbose)
//			qDebug() << "Blocking load ...";
//		QEventLoop loop;
//		QTimer timer;
//		qDebug() << "Not Seekable: setSingleShot";
//		timer.setSingleShot(true);
//		timer.setInterval(5000); // 5 seconds, may be too long?
//		qDebug() << "Not Seekable: connect...";
//		loop.connect(&timer, SIGNAL (timeout()), &loop, SLOT (quit()) );
//		loop.connect(videoObjects[id]->player, SIGNAL (seekableChanged(bool)), &loop, SLOT (quit()));
//		qDebug() << "Not Seekable: loop...";
//		loop.exec();
//		if (verbose)
//			qDebug() << "Blocking load finished, should be seekable now or 5s are over.";
//	}

	if (verbose)
		qDebug() << "Loaded video" << id << "for pos " << x << "/" << y << "Size" << videoObjects[id]->videoItem->size();
	videoObjects[id]->player->setPosition(0); // This should force triggering a metadataAvailable() with resolution update.
	if (show)
		videoObjects[id]->player->play();
	else
		videoObjects[id]->player->pause();
}

void StelVideoMgr::playVideo(const QString& id, const bool keepVisibleAtEnd)
{
	if (videoObjects.contains(id))
	{
		videoObjects[id]->keepVisible=keepVisibleAtEnd;
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			// if already playing, stop and play from the start
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
			{
				videoObjects[id]->player->stop();
			}
#ifndef Q_OS_WIN
			// On Linux, we may have made movie frame invisible during loadVideo().
			videoObjects[id]->videoItem->setVisible(true);
#endif

			// otherwise just play it, or resume playing paused video.
			if (videoObjects[id]->player->state() == QMediaPlayer::PausedState)
				videoObjects[id]->lastPos=videoObjects[id]->player->position() - 1;
			else
				videoObjects[id]->lastPos=-1;

			videoObjects[id]->simplePlay=true;
			videoObjects[id]->player->play();
			if (verbose)
				qDebug() << "StelVideoMgr::playVideo(): playing " << id << videoObjects[id]->player->state() << " - media: " << videoObjects[id]->player->mediaStatus();
		}
	}
	else qDebug() << "StelVideoMgr::playVideo(" << id << "): no such video";
}


//! Play a video which has previously been loaded with loadVideo with a complex start/end effect.
//! The video appears to grow out from @param fromX/ @param fromY
//! within @param popupDuration to size @param finalSizeX/@param finalSizeY, and
//! shrinks back towards @param fromX/@param fromY at the end during @param popdownDuration.
//! @param id the identifier used when @name loadVideo() was called
//! @param fromX X position of starting point, counted from left of window. May be absolute pixel coordinate (if >1) or relative to screen size (0<X<1)
//! @param fromY Y position of starting point, counted from top of window. May be absolute pixel coordinate (if >1) or relative to screen size (0<Y<1)
//! @param atCenterX X position of center of final video frame, counted from left of window. May be absolute pixel coordinate (if >1) or relative to screen size (0<X<1)
//! @param atCenterY Y position of center of final video frame, counted from top of window. May be absolute pixel coordinate (if >1) or relative to screen size (0<Y<1)
//! @param finalSizeX X size (width)  of final video frame. May be absolute (if >1) or relative to window size (0<X<1). If -1, scale proportional from @param finalSizeY.
//! @param finalSizeY Y size (height) of final video frame. May be absolute (if >1) or relative to window size (0<Y<1). If -1, scale proportional from @param finalSizeX.
//! @param popupDuration duration of growing start transition (seconds)
//! @param popdownDuration duration of shrinking end transition (seconds)
//! @param frozenInTransition true if video should be paused during growing/shrinking transition.
void StelVideoMgr::playVideoPopout(const QString& id, float fromX, float fromY, float atCenterX, float atCenterY,
		     float finalSizeX, float finalSizeY, float popupDuration, bool frozenInTransition)
{
	if (verbose)
		qDebug() << "\n\n====Configuring playVideoPopout(): " << id;
	if (videoObjects.contains(id))
	{
		videoObjects[id]->keepVisible=frozenInTransition;
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			// if already playing, stop and play from the start
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
			{
				qDebug() << "playVideoPopout(): stop the playing video";
				videoObjects[id]->player->stop();
			}

			// prepare (1) target frame size, (2) XY start position, and (3) end position:
			// (1) Target frame size.
			// if finalSizeX or finalSizeY <= 1 we scale proportional to mainview!
			// Note that finalSizeX or finalSizeY thus cannot be set to 1.0, i.e. single-pixel rows/columns!
			// This is likely not tragic, else set to 1.0001
			int viewportWidth=StelMainView::getInstance().size().width();
			int viewportHeight=StelMainView::getInstance().size().height();

			if (finalSizeX>0 && finalSizeX<=1)
				finalSizeX*=viewportWidth;
			if (finalSizeY>0 && finalSizeY<=1)
				finalSizeY*=viewportHeight;

			if (verbose)
				qDebug() << "playVideoPopout() finalSize: "<< finalSizeX << "x" << finalSizeY;

			QSize videoSize=videoObjects[id]->resolution;
			if (verbose)
				qDebug() << "playVideoPopout(): video resolution detected=" << videoSize;

			if (!videoSize.isValid() && (finalSizeX==-1 || finalSizeY==-1))
			{
				// This should not happen, is a real problem.
				qDebug() << "StelVideoMgr::playVideoPopout()" << id << ": size (resolution) not yet determined, cannot resize with -1 argument. Sorry, command stops here...";
				return;
			}
			float aspectRatio=(float)videoSize.width()/(float)videoSize.height();
			if (verbose)
				qDebug() << "StelVideoMgr::playVideoPopout(): computed aspect ratio:" << aspectRatio;
			if (finalSizeX!=-1.0f && finalSizeY!=-1.0f)
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::IgnoreAspectRatio);
			else
			{
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::KeepAspectRatio);
				if (finalSizeX==-1.0f && finalSizeY==-1.0f)
				{
					finalSizeX=videoSize.height();
					finalSizeY=videoSize.width();
				}
				else if (finalSizeY==-1.0f)
					finalSizeY=finalSizeX/aspectRatio;
				else if (finalSizeX==-1.0f)
					finalSizeX=finalSizeY*aspectRatio;
			}
			if (verbose)
				qDebug() << "StelVideoMgr::playVideoPopout(): Resetting target frame size to :" << finalSizeX << "x" << finalSizeY;
			videoObjects[id]->targetFrameSize= QSizeF(finalSizeX, finalSizeY); // size in Pixels

			// (2) start position:
			if (fromX>0 && fromX<1)
				fromX *= viewportWidth;
			if (fromY>0 && fromY<1)
				fromY *= viewportHeight;
			videoObjects[id]->popupOrigin= QPointF(fromX, fromY); // Pixel coordinates of popout point.
			if (verbose)
				qDebug() << "StelVideoMgr::playVideoPopout(): Resetting start position to :" << fromX << "/" << fromY;


			// (3) center of target frame
			if (atCenterX>0 && atCenterX<1)
				atCenterX *= viewportWidth;
			if (atCenterY>0 && atCenterY<1)
				atCenterY *= viewportHeight;
			videoObjects[id]->popupTargetCenter= QPointF(atCenterX, atCenterY); // Pixel coordinates of frame center
			if (verbose)
				qDebug() << "StelVideoMgr::playVideoPopout(): Resetting target position to :" << atCenterX << "/" << atCenterY;

			// (4) configure fader
			videoObjects[id]->fader.setDuration(1000.0f*popupDuration);

			// (5) TRIGGER!
			videoObjects[id]->simplePlay=false;
			videoObjects[id]->fader=true;
			videoObjects[id]->videoItem->setVisible(true);
			videoObjects[id]->lastPos=-1;
			videoObjects[id]->player->setPosition(0);
			videoObjects[id]->player->play();

			if (verbose)
				qDebug() << "StelVideoMgr::playVideoPopout(): fader triggered.";
		}
	}
	else qDebug() << "StelVideoMgr::playVideoPopout(" << id << "): no such video";
}


void StelVideoMgr::pauseVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			// Maybe we can only pause while already playing?
			if (videoObjects[id]->player->state()==QMediaPlayer::StoppedState)
				videoObjects[id]->player->play();

			if (verbose)
				qDebug() << "StelVideoMgr::pauseVideo() ...";
			videoObjects[id]->player->pause();
		}
	}
	else qDebug() << "StelVideoMgr::pauseVideo()" << id << ": no such video";
}

void StelVideoMgr::stopVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			videoObjects[id]->player->stop();
		}
	}
	else qDebug() << "StelVideoMgr::stopVideo()" << id << ": no such video";
}

void StelVideoMgr::seekVideo(const QString& id, const qint64 ms, bool pause)
{
	if (verbose)
		qDebug() << "StelVideoMgr::seekVideo: " << id << " to:" << ms <<  (pause ? " (pausing)" : " (playing)");
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			if (videoObjects[id]->player->isSeekable())
			{
				videoObjects[id]->player->setPosition(ms);
				// Seek capability depends on the backend used and likely media codec.
			}
			else
			{
				qDebug() << "[StelVideoMgr] Cannot seek media source" << id;
			}
			// visual update only happens if we play. So even with pause set, we must play to freeze the frame!
			videoObjects[id]->player->play();
			if (pause)
				videoObjects[id]->player->pause();
		}
	}
	else qDebug() << "StelVideoMgr::seekVideo()" << id << ": no such video";
}

void StelVideoMgr::dropVideo(const QString& id)
{
	if (!videoObjects.contains(id))
		return;
	if (videoObjects[id]->player!=Q_NULLPTR)
	{
		if (verbose)
			qDebug() << "About to drop (unload) video " << id << "(" << videoObjects[id]->player->mediaStatus() << ")";
		videoObjects[id]->player->stop();
		StelMainView::getInstance().scene()->removeItem(videoObjects[id]->videoItem);
		delete videoObjects[id]->player;
		delete videoObjects[id]->videoItem;
		delete videoObjects[id];
		videoObjects.remove(id);
	}
}

// setVideoXY(100, 200, false): absolute, as before
// setVideoXY(100, 200, true): shift 100 right, 200 down
// setVideoXY(0.5, 0.25, false): (on fullHD), set to (960/270)
// setVideoXY(0.5, 0.25, true): (on fullHD), shift by (960/270)
// setVideoXY(600, 0.25, false): (on fullHD), set to (600/270)
// setVideoXY(600, 0.25, true): (on fullHD), shift by (600/270)
void StelVideoMgr::setVideoXY(const QString& id, const float x, const float y, const bool relative)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=Q_NULLPTR)
		{
			// if w or h < 1 we scale proportional to mainview!
			int viewportWidth=StelMainView::getInstance().size().width();
			int viewportHeight=StelMainView::getInstance().size().height();
			float newX=x;
			float newY=y;
			if (x>-1 && x<1)
				newX *= viewportWidth;
			if (y>-1 && y<1)
				newY *= viewportHeight;
			if (relative)
			{
				QPointF pos = videoObjects[id]->videoItem->pos();
				videoObjects[id]->videoItem->setPos(pos.x()+newX, pos.y()+newY);
			}
			else
				videoObjects[id]->videoItem->setPos(newX, newY);
			if (verbose)
				qDebug() << "Setting video XY= " << newX << "/" << newY << (relative? "(relative)":"");
		}
	}
	else qDebug() << "StelVideoMgr::setVideoXY()" << id << ": no such video";
}

void StelVideoMgr::setVideoAlpha(const QString& id, const float alpha)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=Q_NULLPTR)
		{
			videoObjects[id]->videoItem->setOpacity(alpha);
		}
	}
	else qDebug() << "StelVideoMgr::setVideoAlpha()" << id << ": no such video";
}

void StelVideoMgr::resizeVideo(const QString& id, float w, float h)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=Q_NULLPTR)
		{
			// if w or h <= 1 we scale proportional to mainview!
			// Note that w or h thus cannot be set to 1.0, i.e. single-pixel rows/columns!
			// This is likely not tragic, else set to 1.0001
			int viewportWidth=StelMainView::getInstance().size().width();
			int viewportHeight=StelMainView::getInstance().size().height();

			if (w>0 && w<=1)
				w*=viewportWidth;
			if (h>0 && h<=1)
				h*=viewportHeight;

			QSize videoSize=videoObjects[id]->resolution;
			if (verbose)
				qDebug() << "resizeVideo(): native resolution=" << videoSize;

			if (!videoSize.isValid() && (w==-1 || h==-1))
			{
				if (verbose)
					qDebug() << "StelVideoMgr::resizeVideo()" << id << ": size not yet determined, cannot resize with -1 argument. We do that in next update().";
				// mark necessity of deferred resize.
				videoObjects[id]->needResize=true;
				videoObjects[id]->targetFrameSize=QSizeF(w, h); // w|h can be -1 or >1, no longer 0<(w|h)<1.
				return;
			}
			float aspectRatio=(float)videoSize.width()/(float)videoSize.height();
			if (verbose)
				qDebug() << "aspect ratio:" << aspectRatio; // 1 for invalid size.
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
			if (verbose)
				qDebug() << "Resizing to:" << w << "x" << h;
			videoObjects[id]->targetFrameSize=QSizeF(w, h); // w|h cannot be -1 or >1 here.
			videoObjects[id]->videoItem->setSize(QSizeF(w, h));
			videoObjects[id]->needResize=false;
		}
	}
	else qDebug() << "StelVideoMgr::resizeVideo()" << id << ": no such video";
}

void StelVideoMgr::showVideo(const QString& id, const bool show)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=Q_NULLPTR)
		{
			videoObjects[id]->videoItem->setVisible(show);
		}
	}
	else qDebug() << "StelVideoMgr::showVideo()" << id << ": no such video";
}

qint64 StelVideoMgr::getVideoDuration(const QString& id) const
{
	if (videoObjects.contains(id))
	{
		return videoObjects[id]->duration;
	}
	else qDebug() << "StelVideoMgr::getDuration()" << id << ": no such video";
	return -1;
}

qint64 StelVideoMgr::getVideoPosition(const QString& id) const
{
	if (videoObjects.contains(id))
	{
		return videoObjects[id]->player->position();
	}
	else qDebug() << "StelVideoMgr::getPosition()" << id << ": no such video";
	return -1;
}

//! returns native resolution (in pixels) of loaded video. Returned value may be invalid before video has been fully loaded.
QSize StelVideoMgr::getVideoResolution(const QString& id) const
{
	if (videoObjects.contains(id))
	{
		return videoObjects[id]->resolution;
	}
	else qDebug() << "StelVideoMgr::getResolution()" << id << ": no such video";
	return QSize();
}

int StelVideoMgr::getVideoWidth(const QString& id) const
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->resolution.isValid())
			return videoObjects[id]->resolution.width();
		else
			return -1;
	}
	else qDebug() << "StelVideoMgr::getWidth()" << id << ": no such video";
	return -1;
}

int StelVideoMgr::getVideoHeight(const QString& id) const
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->resolution.isValid())
			return videoObjects[id]->resolution.height();
		else
			return -1;
	}
	else qDebug() << "StelVideoMgr::getHeight()" << id << ": no such video";
	return -1;
}


void StelVideoMgr::muteVideo(const QString& id, bool mute)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			videoObjects[id]->player->setMuted(mute);
		}
	}
	else qDebug() << "StelVideoMgr::mute()" << id << ": no such video";
}

void StelVideoMgr::setVideoVolume(const QString& id, int newVolume)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			videoObjects[id]->player->setVolume(newVolume);
		}
	}
	else qDebug() << "StelVideoMgr::setVolume()" << id << ": no such video";
}

int StelVideoMgr::getVideoVolume(const QString& id) const
{
	int volume=-1;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			volume=videoObjects[id]->player->volume();
		}
	}
	else qDebug() << "StelVideoMgr::getVolume()" << id << ": no such video";
	return volume;
}

bool StelVideoMgr::isVideoPlaying(const QString& id) const
{
	bool playing=false;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=Q_NULLPTR)
		{
			playing= (videoObjects[id]->player->state() == QMediaPlayer::PlayingState );
		}
	}
	else qDebug() << "StelVideoMgr::isPlaying()" << id << ": no such video";
	return playing;
}


/* *************************************************
 * Signal handlers for all signals of QMediaPlayer. Usually for now this only writes a message to logfile.
 */
void StelVideoMgr::handleAudioAvailableChanged(bool available)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << this->sender()->property("Stel_id").toString() << ": Audio is now available:" << available;
}
// It seems this is never called in practice.
void StelVideoMgr::handleBufferStatusChanged(int percentFilled)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ": Buffer filled (%):" << percentFilled;
}

void StelVideoMgr::handleDurationChanged(qint64 duration)
{
	QString id=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << id << ": Duration changed to:" << duration;
	if (videoObjects.contains(id))
	{
		videoObjects[id]->duration=duration;
	}
}
void StelVideoMgr::handleError(QMediaPlayer::Error error)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ":  error:" << error;
}

/*
// This gets called in loadVideo with player->setMedia(content). Unnecessary, disabled.
void StelVideoMgr::handleCurrentMediaChanged(const QMediaContent & media)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": Current media changed:" << media.canonicalUrl();
}

// This gets called in loadVideo with player->setMedia(content). Unnecessary, disabled.
void StelVideoMgr::handleMediaChanged(const QMediaContent & media)
{
	qDebug() << "QMediaPlayer: " << QObject::sender()->property("Stel_id").toString() << ": Media changed:" << media.canonicalUrl();
}
*/

// This may have been useful to trigger the pause-at-end if it was fast enough.
// It seems that this is too slow! At EndOfMedia the player window disappears, then reappears. This is ugly!
// Currently we deal with polling status changes within update().
void StelVideoMgr::handleMediaStatusChanged(QMediaPlayer::MediaStatus status) // debug-log messages
{
	QString id=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << id << ":  MediaStatus changed to:" << status;
}

void StelVideoMgr::handleMutedChanged(bool muted)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ":  mute changed:" << muted;
}



/* // USELESS. This is called not often enough (could be configured), but update() is better suited to check for video-at-end.
void StelVideoMgr::handlePositionChanged(qint64 position)
{
    QString senderId=QObject::sender()->property("Stel_id").toString();
    qDebug() << "StelVideoMgr: " << senderId << ":  position changed to (ms):" << position;
    // We could deal with the keep-visible here, however this is not called often enough by default, and we have update anyways
    if ((position==videoObjects[senderId]->duration) && (videoObjects[senderId]->keepVisible))
    {
        videoObjects[senderId]->player->setPosition(videoObjects[senderId]->duration - 1);
        videoObjects[senderId]->player->pause();
        qDebug() << " ---> paused at end as requested" ;
    }
}
*/

// USELESS?
void StelVideoMgr::handleSeekableChanged(bool seekable)
{
	if (verbose)
		qDebug() << "StelVideoMgr: handleSeekableChanged()" << QObject::sender()->property("Stel_id").toString() << ":  seekable changed to:" << seekable;
}
void StelVideoMgr::handleStateChanged(QMediaPlayer::State state)
{
	QString senderId=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << senderId << ":  state changed to:" << state
			 << "(Media Status: " << videoObjects[senderId]->player->mediaStatus() << ")";
}

void StelVideoMgr::handleVideoAvailableChanged(bool videoAvailable)
{
	QString senderId=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << senderId << ":  Video available:" << videoAvailable;
	// Sometimes it appears the video has not fully loaded when popup stars, and the movie is not shown.
	// Maybe force showing here? --> NO, breaks our own logic...
	//videoObjects[senderId]->videoItem->setVisible(videoAvailable);
}

void StelVideoMgr::handleVolumeChanged(int volume)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ":  volume changed to:" << volume;
}

void StelVideoMgr::handleAvailabilityChanged(bool available)
{
	if (verbose)
		qDebug() << "StelVideoMgr::handleAvailabilityChanged(bool) " << QObject::sender()->property("Stel_id").toString() << ":  available:" << available;
}

void StelVideoMgr::handleAvailabilityChanged(QMultimedia::AvailabilityStatus availability)
{
	if (verbose)
		qDebug() << "StelVideoMgr::availabilityChanged(QMultimedia::AvailabilityStatus) " << QObject::sender()->property("Stel_id").toString() << ":  availability:" << availability;
}



// The signal sequence (at least on Windows7/MinGW/Qt5.4) seems to be:
// metadatachanged() as soon as video has been loaded. But Result: no metadata.
// After media has started playing and frame is already growing,
// audioAvailable() (but no audio in this movie...)
// videoAvailable() (true)
// durationChanged() --> only now duration becomes known!
// status changed: QMediaPlayer::BufferedMedia
// metadataAvailablechanged(true): Duration, PixelAspectRatio(unknown!), Resolution(unknown!), Videobitrate, VideoFramerate
// metadataChanged() now true, and finally here also PixelAspectRatio and Resolution are known.
// Then periodically, positionChanged(). We can skip that because in update() we can check position() if required.
// Sequence is the same on Win/MSVC and Qt5.3.2, so metadataChanged(.,.) is NOT called on Windows.
// This is also already observed on MacOSX and listed as QTBUG-42034.
// This signal is sent several times during replay because min/max rates often change, and we must go through the metadata list each time. We avoid setting resolution more than once.
void StelVideoMgr::handleMetaDataChanged()
{
	QString id=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << id << ":  Metadata changed (global notification).";

	if (videoObjects.contains(id) && videoObjects[id]->player->isMetaDataAvailable())
	{
		if (verbose)
			qDebug() << "StelVideoMgr: " << id << ":  Following metadata are available:";
		QStringList metadataList=videoObjects[id]->player->availableMetaData();
		for (const auto& md : metadataList)
		{
			QString key = md.toLocal8Bit().constData();
			if (verbose)
				qDebug() << "\t" << key << "==>" << videoObjects[id]->player->metaData(key);

			if ((key=="Resolution") && !(videoObjects[id]->resolution.isValid()))
			{
				if (verbose)
					qDebug() << "StelVideoMgr: Resolution becomes available: " << videoObjects[id]->player->metaData(key).toSize();
				videoObjects[id]->resolution=videoObjects[id]->player->metaData(key).toSize();
			}
		}
	}
	else if (videoObjects.contains(id) && !(videoObjects[id]->player->isMetaDataAvailable()) &&verbose)
		qDebug() << "StelVideoMgr::handleMetaDataChanged()" << id << ": no metadata now.";
	else
		qDebug() << "StelVideoMgr::handleMetaDataChanged()" << id << ": no such video - this is absurd.";
}



/*
// Either this signal or metadataChanged() must be evaluated. On Linux, both are fired, on Windows only the (void) version.
// I (GZ) cannot say which may work on MacOSX, but will disable this for now, the required data handling is done in handleMetaDataChanged(void).
void StelVideoMgr::handleMetaDataChanged(const QString & key, const QVariant & value)
{
    qDebug() << "!!! StelVideoMgr::handleMetadataChanged(.,.): Is this called on Windows when built with MSVC? ";  // NOT WITH MinGW and Qt5.4!!!
    qDebug() << "THIS IS TO ENSURE YOU SEE A CRASH! (If you see it, be happy!) CURRENTLY THE SIGNAL IS NOT SENT ON WINDOWS WHEN BUILT WITH minGW Qt5.4 and not with MSVC on Qt5.3.2";
    Q_ASSERT(0); // Remove the Q_ASSERT and write a comment that it works on (which) Windows/Mac/...!
    QString id=QObject::sender()->property("Stel_id").toString();
    qDebug() << "StelVideoMgr: " << id << ":  Metadata change:" << key << "=>" << value;
    if (key=="Resolution")
    {
        qDebug() << "hah, resolution becomes available!";
        if (videoObjects.contains(id))
        {
            videoObjects[id]->resolution=value.toSize();
            videoObjects[id]->videoItem->setSize(videoObjects[id]->resolution);
        }
	else qDebug() << "StelVideoMgr::handleMetaDataChanged(.,.)" << id << ": no such video - this is absurd.";
    }
}
*/


// update() has only to deal with the faders in all videos, and (re)set positions and sizes of video windows.
void StelVideoMgr::update(double deltaTime)
{
	for (auto voIter = videoObjects.constBegin(); voIter != videoObjects.constEnd(); ++voIter)
	{
		QMediaPlayer::MediaStatus mediaStatus = (*voIter)->player->mediaStatus();
		QString id=voIter.key();
		// Maybe we have verbose as int with levels of verbosity, and output the next line with verbose>=2?
		if (verbose)
			qDebug() << "StelVideoMgr::update() for" << id << ": PlayerState:" << (*voIter)->player->state() << "MediaStatus: " << mediaStatus;

		// fader must be updated here, else the video may not be visible when not yet fully loaded?
		(*voIter)->fader.update(static_cast<int>(deltaTime*1000));

		// It seems we need a more thorough analysis of MediaStatus!
		// In all not-ready status we immediately leave further handling, usually in the hope that loading is successful really soon.
		switch (mediaStatus)
		{
			case QMediaPlayer::UnknownMediaStatus:
			case QMediaPlayer::NoMedia:
			case QMediaPlayer::InvalidMedia:
			case QMediaPlayer::LoadingMedia:
			case QMediaPlayer::StalledMedia:
				if (verbose)
					qDebug() << "StelVideoMgr::update(): " << id  << mediaStatus << "\t==> no further update action";
				continue;
			case QMediaPlayer::LoadedMedia:
			case QMediaPlayer::BufferingMedia:
			case QMediaPlayer::BufferedMedia:
			case QMediaPlayer::EndOfMedia:
			default:
				break;
		}

//		if (verbose)
//			qDebug() << "update() Still alive";

		// First fix targetFrameSize if needed and possible.
		if ((*voIter)->needResize && ((*voIter)->resolution.isValid()))
		{
			// we have buffered our final size (which may be relative to screen size) in targetFrameSize. So, simply
			resizeVideo(voIter.key(), (*voIter)->targetFrameSize.width(), (*voIter)->targetFrameSize.height());
		}

		// if keepVisible, pause video if already close to end.
		if ((*voIter)->simplePlay)
		{
			if ( ((*voIter)->keepVisible) &&  ((*voIter)->duration > 0) &&  ((*voIter)->player->position() >= ((*voIter)->duration - 250))   )
			{
				if (verbose)
					qDebug() << "update(): pausing" << id << "at end";
				(*voIter)->player->pause();
			}

			continue;
		}

		if ((*voIter)->player->state()==QMediaPlayer::StoppedState)
			continue;

		// the rest of the loop is only needed if we are in popup playing mode.

		QSizeF currentFrameSize= (*voIter)->fader.getInterstate() * (*voIter)->targetFrameSize;
		QPointF frameCenter=  (*voIter)->popupOrigin +  ( (*voIter)->popupTargetCenter - (*voIter)->popupOrigin ) *  (*voIter)->fader.getInterstate();
		QPointF frameXY=frameCenter - 0.5*QPointF(currentFrameSize.width(), currentFrameSize.height());
		(*voIter)->videoItem->setPos(frameXY);
		(*voIter)->videoItem->setSize(currentFrameSize);
		if (verbose)
			qDebug() << "StelVideoMgr::update(): Video" << id << "at" << (*voIter)->player->position()
				 << ", player state=" << (*voIter)->player->state() << ", media status=" << (*voIter)->player->mediaStatus();
		int newPos=(*voIter)->player->position();
//		if ((newPos==(*voIter)->lastPos) && ((*voIter)->player->state()==QMediaPlayer::PlayingState))
//		{
//			// I (GZ) have no idea how this can happen, but it does, every couple of runs.
//			// I see this happen only in the grow-while-play phase, but I see no logical error.
//			// It seemed to indicate not-fully-loaded, but this would have been caught in the intro test.
//			// But this even can happen if MediaStatus==BufferedMedia, i.e. fully loaded. Really a shame!
//			// TODO: check with every version of Qt whether this can still happen.
//			// SILLY! Of course if Stellarium framerate > video framerate...
//			if (verbose)
//			{
//				qDebug() << "StelVideoMgr::update(): player state" << (*voIter)->player->state() << "with MediaStatus" << mediaStatus;
//				qDebug() << "This should not be: video should play but position" << newPos << "does not increase? Bumping video.";
//			}
//			//(*voIter)->player->stop(); // GZ Dec2: Do we really need to stop?
//			(*voIter)->player->setPosition(newPos+1); // GZ Dec2 flipped 2 lines.
//			(*voIter)->player->play();
//			qDebug() << "We had an issue with a stuck mediaplayer, should play again!";
//		}
		(*voIter)->lastPos=newPos;

		if (verbose)
			qDebug() << "StelVideoMgr::update(): fader at " << (*voIter)->fader.getInterstate()  << "; update frame size " << currentFrameSize << "to XY" << frameXY;

		// if we want still videos during transition, we must grow/shrink paused, and run only when reached full size.
		// We must detect a frame close to end, pause there, and trigger the fader back to 0.
		if ((*voIter)->keepVisible)
		{
			if ((*voIter)->fader.getInterstate()==0.0f)
			{ // interstate is >0 on startup, so 0 is reached only at end of loop. we can send stop here.
				(*voIter)->player->stop(); // this immediately hides the video window.
				(*voIter)->simplePlay=true; // to reset
				(*voIter)->keepVisible=false;
			}
			else if ((*voIter)->fader.getInterstate()<1.0f)
			{
				if (verbose)
					qDebug() << "StelVideoMgr::update(): not fully grown: pausing video at start or end!";
				(*voIter)->player->pause();
			}
			else if (((*voIter)->duration > 0) && ((*voIter)->player->position() >= ((*voIter)->duration - 250))) // allow stop 250ms before end. 100ms was too short!
			{
				if (verbose)
					qDebug() << "StelVideoMgr::update(): position " << (*voIter)->player->position() << "close to end of duration " << (*voIter)->duration << "--> pause and shutdown with fader ";
				(*voIter)->player->pause();
				// TBD: If we set some very last frame position here, it takes a very long while to seek (may disturb/flicker!)
				// (*voIter)->player->setPosition((*voIter)->duration-10);
				(*voIter)->fader=false; // trigger shutdown (sending again does not disturb)
			}
			else if (((*voIter)->player->state() != QMediaPlayer::PlayingState))
			{
				if (verbose)
					qDebug() << "StelVideoMgr::update(): fully grown: play!";
				(*voIter)->player->play();
			}
		}
		// If the videos come with their own fade-in/-out, this can be played during transitions. (keepVisible configured to false)
		// In this case we must trigger shrinking *before* end of video!
		else
		{
			if ((*voIter)->fader.getInterstate()>0.0f)
			{
				if (((*voIter)->player->state() != QMediaPlayer::PlayingState))
					(*voIter)->player->play();
				if (( ((*voIter)->duration > 0) &&  ((*voIter)->player->position() >= ((*voIter)->duration - (*voIter)->fader.getDuration() - 200))))
				{
					if (verbose)
						qDebug() << "StelVideoMgr::update(): position " << (*voIter)->player->position() << "close to end of duration " << (*voIter)->duration <<
							    "minus fader duration " << (*voIter)->fader.getDuration() << "--> shutdown with fader ";
					(*voIter)->fader=false; // trigger shutdown (sending again does not disturb)
				}
			}
			else // interstate==0: end of everything.
			{
				if (verbose)
					qDebug() << "StelVideoMgr::update(): Stopping at Interstate " << (*voIter)->fader.getInterstate();
				(*voIter)->player->stop();  // immediately hides video window.
				(*voIter)->simplePlay=true; // reset
			}
		}
	}
}



#else 
void StelVideoMgr::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	qWarning() << "[StelVideoMgr] This build of Stellarium does not support video - cannot load video" << QDir::toNativeSeparators(filename) << id << x << y << show << alpha;
}
StelVideoMgr::~StelVideoMgr() {;}
void StelVideoMgr::update(double){;}
void StelVideoMgr::playVideo(const QString&, const bool) {;}
void StelVideoMgr::playVideoPopout(const QString&, float, float, float, float, float, float, float, bool){;}
void StelVideoMgr::pauseVideo(const QString&) {;}
void StelVideoMgr::stopVideo(const QString&) {;}
void StelVideoMgr::dropVideo(const QString&) {;}
void StelVideoMgr::seekVideo(const QString&, qint64, bool) {;}
void StelVideoMgr::setVideoXY(const QString&, float, float, const bool) {;}
void StelVideoMgr::setVideoAlpha(const QString&, float) {;}
void StelVideoMgr::resizeVideo(const QString&, float, float) {;}
void StelVideoMgr::showVideo(const QString&, bool) {;}
// New functions for 0.15
qint64 StelVideoMgr::getVideoDuration(const QString&)const{return -1;}
qint64 StelVideoMgr::getVideoPosition(const QString&)const{return -1;}
QSize StelVideoMgr::getVideoResolution(const QString&)const{return QSize(0,0);}
int StelVideoMgr::getVideoWidth(const QString&)const{return -1;}
int StelVideoMgr::getVideoHeight(const QString&)const{return -1;}
void StelVideoMgr::muteVideo(const QString&, bool){;}
void StelVideoMgr::setVideoVolume(const QString&, int){;}
int StelVideoMgr::getVideoVolume(const QString&)const{return -1;}
bool StelVideoMgr::isVideoPlaying(const QString& id) const
{
	Q_UNUSED(id)
	return false;
}

#endif // ENABLE_MEDIA


