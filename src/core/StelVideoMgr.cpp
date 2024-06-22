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
#include "StelFader.hpp"
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	#include <QMediaFormat>
	#include <QAudioOutput>
	#include <QVideoSink>
	#include <QMediaMetaData>
#endif
#endif


StelVideoMgr::StelVideoMgr(bool withAudio) : StelModule(), audioEnabled(withAudio)
{
	setObjectName("StelVideoMgr");
#ifdef ENABLE_MEDIA
	// in case the property has not been set, getProperty() returns invalid.
	verbose= (qApp->property("verbose") == true);
	#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	if (verbose)
	{
		QMediaFormat fmt=QMediaFormat();
		qDebug() << "StelVideoMgr: Supported file formats: " << fmt.supportedFileFormats(QMediaFormat::Decode);
		qDebug() << "StelVideoMgr: Supported video codecs: " << fmt.supportedVideoCodecs(QMediaFormat::Decode);
		if (audioEnabled)
			qDebug() << "StelVideoMgr: Supported audio codecs: " << fmt.supportedAudioCodecs(QMediaFormat::Decode);
	}
	#endif
#endif
}

#ifdef ENABLE_MEDIA
StelVideoMgr::~StelVideoMgr()
{
	QMutableMapIterator<QString, StelVideoMgr::VideoPlayer*>it(videoObjects);
	while (it.hasNext())
	{
		it.next();
		if (it.value()!=nullptr)
		{
			it.value()->player->stop();
			StelMainView::getInstance().scene()->removeItem(it.value()->videoItem);
			delete it.value()->player;
			delete it.value()->videoItem;
			#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			delete it.value()->audioOutput;
			#endif
			it.remove();
		}
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

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	videoObjects[id]->player = new QMediaPlayer(nullptr);
	if (audioEnabled)
	{
		videoObjects[id]->audioOutput = new QAudioOutput();
		videoObjects[id]->player->setAudioOutput(videoObjects[id]->audioOutput);
	}
	else
		videoObjects[id]->audioOutput = nullptr;

	videoObjects[id]->resolution=QSize(200,200); // We must initialize with "valid" resolution, maybe resize when player is starting!
	videoObjects[id]->targetFrameSize=QSizeF(100, 200); // depends on parameters given in playVideo(), playPopoutVideo() and resolution detected only after playing started.
#else
	videoObjects[id]->player = new QMediaPlayer(nullptr, QMediaPlayer::VideoSurface);
	videoObjects[id]->player->setAudioRole(audioEnabled ? QAudio::VideoRole : QAudio::UnknownRole);
	if (!audioEnabled)
		videoObjects[id]->player->setMuted(true);
	videoObjects[id]->resolution=QSize(); // initialize with "invalid" empty resolution, we must detect this when player is starting!
	videoObjects[id]->targetFrameSize=QSizeF(); // start with invalid, depends on parameters given in playVideo(), playPopoutVideo() and resolution detected only after playing started.
#endif
	videoObjects[id]->duration=-1; // -1 to signal "unknown".
	videoObjects[id]->keepVisible=false;
	videoObjects[id]->needResize=true; // resolution and target frame not yet known.
	videoObjects[id]->simplePlay=true;
	videoObjects[id]->popupOrigin=QPointF();
	videoObjects[id]->popupTargetCenter=QPointF();
	videoObjects[id]->player->setProperty("Stel_id", id); // allow tracking of log messages and access of members.
	videoObjects[id]->player->setVideoOutput(videoObjects[id]->videoItem);
	videoObjects[id]->videoItem->setOpacity(alpha);
	// There was a notable difference: on Windows this causes a crash (Qt5.4?) or prevents being visible in Qt5.9! Qt5.12 is OK.
	// On Linux, it is required, else the movie frame is visible before proper resize.
	videoObjects[id]->videoItem->setVisible(show);
	videoObjects[id]->lastPos=-1;

	// A few connections are not really needed, they are signals we don't use. TBD: Remove or keep commented out?
	connect(videoObjects[id]->player, SIGNAL(durationChanged(qint64)), this, SLOT(handleDurationChanged(qint64))); // (CRITICALLY IMPORTANT!)
	connect(videoObjects[id]->player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus)));
	//connect(videoObjects[id]->player, SIGNAL(positionChanged(qint64)), this, SLOT(handlePositionChanged(qint64)));
	// we test isSeekable() where needed, so only debug log entry. --> And we may use the signal however now during blocking load below!
	connect(videoObjects[id]->player, SIGNAL(seekableChanged(bool)), this, SLOT(handleSeekableChanged(bool)));
	#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	connect(videoObjects[id]->player, SIGNAL(bufferProgressChanged(float)), this, SLOT(handleBufferProgressChanged(float)));
	connect(videoObjects[id]->player, SIGNAL(errorOccurred(QMediaPlayer::Error, const QString &)), this, SLOT(handleErrorMsg(QMediaPlayer::Error, const QString &)));
	connect(videoObjects[id]->player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)), this, SLOT(handleStateChanged(QMediaPlayer::PlaybackState)));
	connect(videoObjects[id]->player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus)));
	connect(videoObjects[id]->player, SIGNAL(hasVideoChanged(bool)), this, SLOT(handleVideoAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(hasAudioChanged(bool)), this, SLOT(handleAudioAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(sourceChanged(const QUrl &)), this, SLOT(handleSourceChanged(const QUrl &)));
	#else
	connect(videoObjects[id]->player, SIGNAL(bufferStatusChanged(int)), this, SLOT(handleBufferStatusChanged(int)));
	connect(videoObjects[id]->player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleError(QMediaPlayer::Error)));
	connect(videoObjects[id]->player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handleStateChanged(QMediaPlayer::State)));
	connect(videoObjects[id]->player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(handleVideoAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(audioAvailableChanged(bool)), this, SLOT(handleAudioAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(mutedChanged(bool)), this, SLOT(handleMutedChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(volumeChanged(int)), this, SLOT(handleVolumeChanged(int)));
	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(bool)), this, SLOT(handleAvailabilityChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(QMultimedia::AvailabilityStatus)), this, SLOT(handleAvailabilityChanged(QMultimedia::AvailabilityStatus)));
	#endif

	// Only this is triggered also on Windows. Lets us read resolution etc. (CRITICALLY IMPORTANT!)
	connect(videoObjects[id]->player, SIGNAL(metaDataChanged()), this, SLOT(handleMetaDataChanged()));

	// We need an absolute pathname here.
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	QUrl url(QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()));
	videoObjects[id]->player->setSource(url);
#else
	QMediaContent content(QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()));
	videoObjects[id]->player->setMedia(content);
#endif
	if (verbose)
	{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		qDebug() << "Loading " << url;
#elif (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
		qDebug() << "Loading " << content.request().url();
#else
		qDebug() << "Loading " << content.canonicalUrl();
#endif
		qDebug() << "Media Resources queried from player:";
		qDebug() << "\tSTATUS:        " << videoObjects[id]->player->mediaStatus();
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		qDebug() << "\tFile:          " << videoObjects[id]->player->source();
#elif (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
		qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().request().url();
#else
		qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().canonicalUrl();
#endif
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
		if (videoObjects[id]->player!=nullptr)
		{
			// if already playing, stop and play from the start
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			if (videoObjects[id]->player->playbackState() == QMediaPlayer::PlayingState)
#else
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
#endif
			{
				videoObjects[id]->player->stop();
			}
			// otherwise just play it, or resume playing paused video.
			videoObjects[id]->videoItem->setVisible(true);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			if (videoObjects[id]->player->playbackState() == QMediaPlayer::PausedState)
#else
			if (videoObjects[id]->player->state() == QMediaPlayer::PausedState)
#endif
				videoObjects[id]->lastPos=videoObjects[id]->player->position() - 1;
			else
				videoObjects[id]->lastPos=-1;

			videoObjects[id]->simplePlay=true;
			videoObjects[id]->player->play();
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			videoObjects[id]->videoItem->show();
			if (verbose)
				qDebug() << "StelVideoMgr::playVideo(): playing " << id << videoObjects[id]->player->playbackState() << " - media: " << videoObjects[id]->player->mediaStatus();
#else
			if (verbose)
				qDebug() << "StelVideoMgr::playVideo(): playing " << id << videoObjects[id]->player->state() << " - media: " << videoObjects[id]->player->mediaStatus();
#endif
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
		if (videoObjects[id]->player!=nullptr)
		{
			// if already playing, stop and play from the start
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			if (videoObjects[id]->player->playbackState() == QMediaPlayer::PlayingState)
#else
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
#endif
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
			float aspectRatio=static_cast<float>(videoSize.width())/static_cast<float>(videoSize.height());
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
				fromX *= static_cast<float>(viewportWidth);
			if (fromY>0 && fromY<1)
				fromY *= static_cast<float>(viewportHeight);
			videoObjects[id]->popupOrigin= QPointF(fromX, fromY); // Pixel coordinates of popout point.
			if (verbose)
				qDebug() << "StelVideoMgr::playVideoPopout(): Resetting start position to :" << fromX << "/" << fromY;


			// (3) center of target frame
			if (atCenterX>0 && atCenterX<1)
				atCenterX *= static_cast<float>(viewportWidth);
			if (atCenterY>0 && atCenterY<1)
				atCenterY *= static_cast<float>(viewportHeight);
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
		if (videoObjects[id]->player!=nullptr)
		{
			// Maybe we can only pause while already playing?
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			if (videoObjects[id]->player->playbackState() == QMediaPlayer::StoppedState)
#else
			if (videoObjects[id]->player->state()==QMediaPlayer::StoppedState)
#endif
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
		if (videoObjects[id]->player!=nullptr)
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
		if (videoObjects[id]->player!=nullptr)
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
	if (videoObjects[id]->player!=nullptr)
	{
		if (verbose)
			qDebug() << "About to drop (unload) video " << id << "(" << videoObjects[id]->player->mediaStatus() << ")";
		videoObjects[id]->player->stop();
		StelMainView::getInstance().scene()->removeItem(videoObjects[id]->videoItem);
		delete videoObjects[id]->player;
		delete videoObjects[id]->videoItem;
		#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		delete videoObjects[id]->audioOutput;
		#endif
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
		if (videoObjects[id]->videoItem!=nullptr)
		{
			// if w or h < 1 we scale proportional to mainview!
			int viewportWidth=StelMainView::getInstance().size().width();
			int viewportHeight=StelMainView::getInstance().size().height();
			float newX=x;
			float newY=y;
			if (x>-1 && x<1)
				newX *= static_cast<float>(viewportWidth);
			if (y>-1 && y<1)
				newY *= static_cast<float>(viewportHeight);
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
		if (videoObjects[id]->videoItem!=nullptr)
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
		if (videoObjects[id]->videoItem!=nullptr)
		{
			// if w or h <= 1 we scale proportional to mainview!
			// Note that w or h thus cannot be set to 1.0, i.e. single-pixel rows/columns!
			// This is likely not tragic, else set to 1.0001
			int viewportWidth=StelMainView::getInstance().size().width();
			int viewportHeight=StelMainView::getInstance().size().height();

			if (w>0 && w<=1)
				w*=static_cast<float>(viewportWidth);
			if (h>0 && h<=1)
				h*=static_cast<float>(viewportHeight);

			QSize videoSize=videoObjects[id]->resolution;
			if (verbose)
				qDebug() << "resizeVideo(): old resolution=" << videoSize;

			if (!videoSize.isValid() && (w==-1 || h==-1))
			{
				if (verbose)
					qDebug() << "StelVideoMgr::resizeVideo()" << id << ": size not yet determined, cannot resize with -1 argument. We do that in next update().";
				// mark necessity of deferred resize.
				videoObjects[id]->needResize=true;
				videoObjects[id]->targetFrameSize=QSizeF(w, h); // w|h can be -1 or >1, no longer 0<(w|h)<1.
				return;
			}
			float aspectRatio=static_cast<float>(videoSize.width())/static_cast<float>(videoSize.height());
			if (verbose)
				qDebug() << "aspect ratio:" << aspectRatio; // 1 for invalid size.
			if (w!=-1.0f && h!=-1.0f)
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::IgnoreAspectRatio);
			else
			{
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::KeepAspectRatio);
				if (w==-1.0f && h==-1.0f)
				{
					h=static_cast<float>(videoSize.height());
					w=static_cast<float>(videoSize.width());
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
		if (videoObjects[id]->videoItem!=nullptr)
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
	if (!audioEnabled)
		return;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=nullptr)
		{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			videoObjects[id]->player->audioOutput()->setMuted(mute);
#else
			videoObjects[id]->player->setMuted(mute);
#endif
		}
	}
	else qDebug() << "StelVideoMgr::mute()" << id << ": no such video";
}

void StelVideoMgr::setVideoVolume(const QString& id, int newVolume)
{
	if (!audioEnabled)
		return;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=nullptr)
		{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			videoObjects[id]->player->audioOutput()->setVolume(qBound(0,newVolume,100)/100.);
#else
			videoObjects[id]->player->setVolume(newVolume);
#endif
		}
	}
	else qDebug() << "StelVideoMgr::setVolume()" << id << ": no such video";
}

int StelVideoMgr::getVideoVolume(const QString& id) const
{
	if (!audioEnabled)
		return 0;

	int volume=-1;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=nullptr)
		{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			volume=int(videoObjects[id]->player->audioOutput()->volume()*100.);
#else
			volume=videoObjects[id]->player->volume();
#endif
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
		if (videoObjects[id]->player!=nullptr)
		{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			playing= (videoObjects[id]->player->playbackState() == QMediaPlayer::PlayingState );
#else
			playing= (videoObjects[id]->player->state() == QMediaPlayer::PlayingState );
#endif
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

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
void StelVideoMgr::handleBufferProgressChanged(float filled)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ": Buffer filled (fraction):" << filled;
}
void StelVideoMgr::handleErrorMsg(QMediaPlayer::Error error, const QString &errorString)
{
	qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ":  error:" << error << ":" << errorString;
}
#else
// It seems this is never called in practice.
void StelVideoMgr::handleBufferStatusChanged(int percentFilled)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ": Buffer filled (%):" << percentFilled;
}
void StelVideoMgr::handleError(QMediaPlayer::Error error)
{
	qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ":  error:" << error;
}
#endif

void StelVideoMgr::handleDurationChanged(qint64 duration)
{
	const QString id=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << id << ": Duration changed to:" << duration;
	if (videoObjects.contains(id))
	{
		videoObjects[id]->duration=duration;
	}
}

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
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
void StelVideoMgr::handleStateChanged(QMediaPlayer::PlaybackState state)
#else
void StelVideoMgr::handleStateChanged(QMediaPlayer::State state)
#endif
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void StelVideoMgr::handleMetaDataChanged()
{
	QString id=QObject::sender()->property("Stel_id").toString();
	if (verbose)
		qDebug() << "StelVideoMgr: " << id << ":  Metadata changed (global notification).";

	if (videoObjects.contains(id))
	{
		const QMediaMetaData metaData=videoObjects[id]->player->metaData();

		if (verbose)
			qDebug() << "StelVideoMgr: " << id << ":  Following metadata are available:";
		for (const auto& mdKey : metaData.keys())
		{
			QString key = metaData.stringValue(mdKey);
			if (verbose)
				qDebug() << "\t" << mdKey << "==>" << key;

			if ((key=="Resolution"))
			{
				if (verbose)
					qDebug() << "StelVideoMgr: Resolution becomes available: " << metaData.stringValue(mdKey);
				videoObjects[id]->resolution=metaData.value(mdKey).toSize();
			}
		}
	}
	else
		qDebug() << "StelVideoMgr::handleMetaDataChanged()" << id << ": no such video - this is absurd.";
}
void StelVideoMgr::handleSourceChanged(const QUrl &media)
{
	if (verbose)
		qDebug() << "StelVideoMgr: " << QObject::sender()->property("Stel_id").toString() << ": source changed to:" << media;
}
#else
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
		const QStringList metadataList=videoObjects[id]->player->availableMetaData();
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
#endif

// update() has only to deal with the faders in all videos, and (re)set positions and sizes of video windows.
void StelVideoMgr::update(double deltaTime)
{
	for (auto voIter = videoObjects.constBegin(); voIter != videoObjects.constEnd(); ++voIter)
	{
		QMediaPlayer::MediaStatus mediaStatus = (*voIter)->player->mediaStatus();
		QString id=voIter.key();
		// Maybe we have verbose as int with levels of verbosity, and output the next line with verbose>=2?
		if (verbose)
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			qDebug() << "StelVideoMgr::update() for" << id << ": PlayerState:" << (*voIter)->player->playbackState() << "MediaStatus: " << mediaStatus;
#else
			qDebug() << "StelVideoMgr::update() for" << id << ": PlayerState:" << (*voIter)->player->state() << "MediaStatus: " << mediaStatus;
#endif

		// fader must be updated here, else the video may not be visible when not yet fully loaded?
		(*voIter)->fader.update(static_cast<int>(deltaTime*1000));

		// It seems we need a more thorough analysis of MediaStatus!
		// In all not-ready status we immediately leave further handling, usually in the hope that loading is successful really soon.
		switch (mediaStatus)
		{
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
			case QMediaPlayer::UnknownMediaStatus:
#endif
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

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		if ((*voIter)->player->playbackState()==QMediaPlayer::StoppedState)
#else
		if ((*voIter)->player->state()==QMediaPlayer::StoppedState)
#endif
			continue;

		// the rest of the loop is only needed if we are in popup playing mode.

		QSizeF currentFrameSize= (*voIter)->fader.getInterstate() * (*voIter)->targetFrameSize;
		QPointF frameCenter=  (*voIter)->popupOrigin +  ( (*voIter)->popupTargetCenter - (*voIter)->popupOrigin ) *  (*voIter)->fader.getInterstate();
		QPointF frameXY=frameCenter - 0.5*QPointF(currentFrameSize.width(), currentFrameSize.height());
		(*voIter)->videoItem->setPos(frameXY);
		(*voIter)->videoItem->setSize(currentFrameSize);
		if (verbose)
			qDebug() << "StelVideoMgr::update(): Video" << id << "at" << (*voIter)->player->position()
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
				 << ", player state=" << (*voIter)->player->playbackState() << ", media status=" << (*voIter)->player->mediaStatus();
#else
				 << ", player state=" << (*voIter)->player->state() << ", media status=" << (*voIter)->player->mediaStatus();
#endif

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
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			else if (((*voIter)->player->playbackState() != QMediaPlayer::PlayingState))
#else
			else if (((*voIter)->player->state() != QMediaPlayer::PlayingState))
#endif
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
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
				if (((*voIter)->player->playbackState() != QMediaPlayer::PlayingState))
#else
				if (((*voIter)->player->state() != QMediaPlayer::PlayingState))
#endif
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


