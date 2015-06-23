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
#ifdef ENABLE_MEDIA
	#include <QDir>
	//#include <QVideoWidget> // Adapt to that when we finally switch to QtQuick2!
	#include <QGraphicsVideoItem>
	#include <QMediaPlayer>
	#include "StelApp.hpp"
	#include "StelFader.hpp"
#endif


StelVideoMgr::StelVideoMgr()
{
    setObjectName("StelVideoMgr");
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
	videoObjects[id]->duration=10000; // Actually this should be -1 to signal "unknown", but popup testing needs default setting, say 10s.
	videoObjects[id]->resolution=QSize();

	// N.B. A few connections are not needed, they are signals we don't use.
	connect(videoObjects[id]->player, SIGNAL(audioAvailableChanged(bool)), this, SLOT(handleAudioAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(bufferStatusChanged(int)), this, SLOT(handleBufferStatusChanged(int)));
	//connect(videoObjects[id]->player, SIGNAL(currentMediaChanged(QMediaContent)), this, SLOT(handleCurrentMediaChanged(QMediaContent)));
	connect(videoObjects[id]->player, SIGNAL(durationChanged(qint64)), this, SLOT(handleDurationChanged(qint64)));
	connect(videoObjects[id]->player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleError(QMediaPlayer::Error)));
	//connect(videoObjects[id]->player, SIGNAL(mediaChanged(QMediaContent)), this, SLOT(handleMediaChanged(QMediaContent)));
	connect(videoObjects[id]->player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus))); // debug-log messages
	connect(videoObjects[id]->player, SIGNAL(mutedChanged(bool)), this, SLOT(handleMutedChanged(bool)));
	//connect(videoObjects[id]->player, SIGNAL(networkConfigurationChanged(QNetworkConfiguration)), this, SLOT(handleNetworkConfigurationChanged(QNetworkConfiguration)));
	//connect(videoObjects[id]->player, SIGNAL(playbackRateChanged(qreal)), this, SLOT(handlePlaybackRateChanged(qreal)));
	connect(videoObjects[id]->player, SIGNAL(positionChanged(qint64)), this, SLOT(handlePositionChanged(qint64)));
	// we test isSeekable() anyway.
	//connect(videoObjects[id]->player, SIGNAL(seekableChanged(bool)), this, SLOT(handleSeekableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handleStateChanged(QMediaPlayer::State)));
	connect(videoObjects[id]->player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(handleVideoAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(volumeChanged(int)), this, SLOT(handleVolumeChanged(int)));

	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(bool)), this, SLOT(handleAvailabilityChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(QMultimedia::AvailabilityStatus)), this, SLOT(handleAvailabilityChanged(QMultimedia::AvailabilityStatus)));
	// Only this lets us read duration etc.
	connect(videoObjects[id]->player, SIGNAL(metaDataAvailableChanged(bool)), this, SLOT(handleMetaDataAvailableChanged(bool)));
	// I reconnect this for now: I see this is triggered, but no details delivered on Windows by the next signal? Needs further Windows test which one to keep.
	connect(videoObjects[id]->player, SIGNAL(metaDataChanged()), this, SLOT(handleMetaDataChanged()));
	connect(videoObjects[id]->player, SIGNAL(metaDataChanged(QString,QVariant)), this, SLOT(handleMetaDataChanged(QString,QVariant)));

	// Not needed because nothing from outside sets the interval.
	//connect(videoObjects[id]->player, SIGNAL(notifyIntervalChanged(int)), this, SLOT(handleNotifyIntervalChanged(int)));

	// we give the player the id as property to allow better tracking of log messages or to access their videoObjects.
	videoObjects[id]->player->setProperty("Stel_id", id);

	// We need an absolute pathname here.
	QMediaContent content(QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()));
	qDebug() << "Loaded " << content.canonicalUrl();
/*
	// On Windows, this reveals we have nothing loaded! No, it just does nothing.
	qDebug() << "MediaContent canonical resources:";
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
*/
	videoObjects[id]->player->setMedia(content);
	videoObjects[id]->player->setVideoOutput(videoObjects[id]->videoItem);
	// Maybe ask only player?
	qDebug() << "Media Resources queried from player:";
	qDebug() << "\tSTATUS:        " << videoObjects[id]->player->mediaStatus();
	//while (videoObjects[id]->player->mediaStatus() == QMediaPlayer::LoadingMedia) {}
	
	qDebug() << "\tSTATUS finally:        " << videoObjects[id]->player->mediaStatus();
	qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().canonicalUrl();
/*
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
*/
	StelMainView::getInstance().scene()->addItem(videoObjects[id]->videoItem);
	// TODO: Apparently we must wait until status=loaded. (?)

	videoObjects[id]->videoItem->setPos(x, y);
	//videoObjects[id]->videoItem->setSize(videoObjects[id]->player->media().resources()[0].resolution());
	videoObjects[id]->videoItem->setSize(content.resources().at(0).resolution());
	// DEBUG show a box. I had video playing in a tiny box already, once..!
	videoObjects[id]->videoItem->setSize(QSizeF(650, 650));

	videoObjects[id]->videoItem->setOpacity(alpha);
	videoObjects[id]->videoItem->setVisible(show);
	qDebug() << "Loaded video" << id << "for pos " << x << "/" << y << "Size" << videoObjects[id]->videoItem->size();
	videoObjects[id]->player->setPosition(0); // This should force triggering a resolution setting (?)
	videoObjects[id]->player->pause();
    // This has been done by the signal handler now:
    //videoObjects[id]->videoItem->setSize(videoObjects[id]->resolution);

}

void StelVideoMgr::playVideo(const QString& id, bool keepLastFrame)
{
	if (videoObjects.contains(id))
	{
		videoObjects[id]->keepVisible=keepLastFrame; // Attempt with a flag. However, due to signal delay, this may not work.
		if (videoObjects[id]->player!=NULL)
		{
			// if already playing, stop and play from the start
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
			{
				videoObjects[id]->player->stop();
			}
			// otherwise just play it, or resume playing paused video.
			qDebug() << "StelVideoMgr::playVideo(): playing " << id;
			videoObjects[id]->player->play();
			qDebug() << "\tSTATUS: player: " << videoObjects[id]->player->state() << " - media: " << videoObjects[id]->player->mediaStatus();
			qDebug() << "\tFile:           " << videoObjects[id]->player->currentMedia().canonicalUrl();
		}
	}
	else qDebug() << "StelVideoMgr::playVideo(" << id << "): no such video";
}


//! Play a video which has previously been loaded with loadVideo with a complex effect.
//! The video appears to pop out from @param fromX/ @param fromY,
//! grows within @param popupDuration to size @param finalSizeX/@param finalSizeY, and
//! shrinks back towards @param fromX/@param fromY at the end during @param popdownDuration.
//! @param id the identifier used when loadVideo was called
//! @param fromX X position of starting point, counted from left of window. May be absolute (if >1) or relative (0<X<1)
//! @param fromY Y position of starting point, counted from top of window. May be absolute (if >1) or relative (0<Y<1)
//! @param atCenterX X position of center of final video frame, counted from left of window. May be absolute (if >1) or relative (0<X<1)
//! @param atCenterY Y position of center of final video frame, counted from top of window. May be absolute (if >1) or relative (0<Y<1)
//! @param finalSizeX X size (width)  of final video frame. May be absolute (if >1) or relative to window size (0<X<1). If -1, scale proportional from @param finalSizeY.
//! @param finalSizeY Y size (height) of final video frame. May be absolute (if >1) or relative to window size (0<Y<1). If -1, scale proportional from @param finalSizeX.
//! @param popupDuration duration of growing start transition (seconds)
//! @param popdownDuration duration of shrinking end transition (seconds)
//! @param frozenInTransition true if video should be paused during growing/shrinking transition.
void StelVideoMgr::playVideoPopout(const QString& id, float fromX, float fromY, float atCenterX, float atCenterY,
		     float finalSizeX, float finalSizeY, float popupDuration, bool frozenInTransition)
{
    qDebug() << "\n\n====playVideoPopout(): " << id;
    if (videoObjects.contains(id))
    {
	videoObjects[id]->keepVisible=frozenInTransition; // Attempt with a flag. However, due to signal delay, this may not work.
	if (videoObjects[id]->player!=NULL)
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

	    qDebug() << "playVideoPopout() finalSize: "<< finalSizeX << "x" << finalSizeY;
	    QSize videoSize=videoObjects[id]->resolution;
	    qDebug() << "playVideoPopout(): video resolution detected=" << videoSize;

	    if (!videoSize.isValid() && (finalSizeX==-1 || finalSizeY==-1))
	    {
		qDebug() << "StelVideoMgr::playVideoPopout()" << id << ": size (resolution) not yet determined, cannot resize with -1 argument. Sorry, command stops here...";
		return;
	    }
	    float aspectRatio=(float)videoSize.width()/(float)videoSize.height();
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
	    qDebug() << "StelVideoMgr::playVideoPopout(): Resetting target frame size to :" << finalSizeX << "x" << finalSizeY;
	    // TODO: Do something else in some update(ms) method to set frame size and position.
	    //videoObjects[id]->videoItem->setSize(QSizeF(w, h));
	    videoObjects[id]->popupFrameSize= QSizeF(finalSizeX, finalSizeY); // size in Pixels

	    // (2) start position:
	    if (fromX>0 && fromX<1)
		fromX *= viewportWidth;
	    if (fromY>0 && fromY<1)
		fromY *= viewportHeight;
	    videoObjects[id]->popupOrigin= QPointF(fromX, fromY); // Pixel coordinates of popout point.
	    qDebug() << "StelVideoMgr::playVideoPopout(): Resetting start position to :" << fromX << "/" << fromY;


	    // (3) center of target frame
	    if (atCenterX>0 && atCenterX<1)
		atCenterX *= viewportWidth;
	    if (atCenterY>0 && atCenterY<1)
		atCenterY *= viewportHeight;
	    videoObjects[id]->popupTargetCenter= QPointF(atCenterX, atCenterY); // Pixel coordinates of frame center
	    qDebug() << "StelVideoMgr::playVideoPopout(): Resetting target position to :" << atCenterX << "/" << atCenterY;

	    // (4) configure fader
	    videoObjects[id]->fader.setDuration(1000.0f*popupDuration);
	    videoObjects[id]->fader.setMinValue(0.0f);
	    videoObjects[id]->fader.setMaxValue(1.0f);

	    // (5) TRIGGER!
	    videoObjects[id]->fader=true;
	    videoObjects[id]->videoItem->setVisible(true);
	    videoObjects[id]->player->setPosition(0);
	    videoObjects[id]->player->play();

	    qDebug() << "StelVideoMgr::playVideoPopout(): fader triggered.";

	}
    }
    else qDebug() << "StelVideoMgr::playVideoPopout(" << id << "): no such video";
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
	else qDebug() << "StelVideoMgr::pauseVideo()" << id << ": no such video";
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
	else qDebug() << "StelVideoMgr::stopVideo()" << id << ": no such video";
}

void StelVideoMgr::seekVideo(const QString& id, const qint64 ms, bool pause)
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
				qDebug() << "[StelVideoMgr] Cannot seek media source" << id;
			}
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
	if (videoObjects[id]->player!=NULL)
	{
		qDebug() << "About to drop (unload) video.";
		qDebug() << "\tSTATUS finally:        " << videoObjects[id]->player->mediaStatus();
		qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().canonicalUrl();
		//	qDebug() << "\tData size:     " << videoObjects[id]->player->currentMedia().canonicalResource().dataSize();
		//	qDebug() << "\tMIME Type:     " << videoObjects[id]->player->currentMedia().canonicalResource().mimeType();
		//	qDebug() << "\tVideo codec:   " << videoObjects[id]->player->currentMedia().canonicalResource().videoCodec();
		//	qDebug() << "\tResolution:    " << videoObjects[id]->player->currentMedia().canonicalResource().resolution();
		//	qDebug() << "\tVideo bitrate: " << videoObjects[id]->player->currentMedia().canonicalResource().videoBitRate();
		//	qDebug() << "\tAudio codec:   " << videoObjects[id]->player->currentMedia().canonicalResource().audioCodec();
		//	qDebug() << "\tChannel Count: " << videoObjects[id]->player->currentMedia().canonicalResource().channelCount();
		//	qDebug() << "\tAudio bitrate: " << videoObjects[id]->player->currentMedia().canonicalResource().audioBitRate();
		//	qDebug() << "\tSample rate:   " << videoObjects[id]->player->currentMedia().canonicalResource().sampleRate();
		//	qDebug() << "\tlanguage:      " << videoObjects[id]->player->currentMedia().canonicalResource().language();

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
		if (videoObjects[id]->videoItem!=NULL)
		{
			// if w or h < 1 we scale proportional to mainview!
			int viewportWidth=StelMainView::getInstance().size().width();
			int viewportHeight=StelMainView::getInstance().size().height();
			float newX=x;
			float newY=y;
			if (x>0 && x<1)
				newX *= viewportWidth;
			if (y>0 && y<1)
				newY *= viewportHeight;
			QPointF pos = videoObjects[id]->videoItem->pos();
			float oldX=pos.x();
			float oldY=pos.y();
			if (relative)
				videoObjects[id]->videoItem->setPos(oldX+newX, oldY+newY);
			else
				videoObjects[id]->videoItem->setPos(newX, newY);
		}
	}
	else qDebug() << "StelVideoMgr::setVideoXY()" << id << ": no such video";
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
	else qDebug() << "StelVideoMgr::setVideoAlpha()" << id << ": no such video";
}

void StelVideoMgr::resizeVideo(const QString& id, float w, float h)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
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
			qDebug() << "resizeVideo(): resolution=" << videoSize;

			if (!videoSize.isValid() && (w==-1 || h==-1))
			{
				qDebug() << "StelVideoMgr::resizeVideo()" << id << ": size not yet determined, cannot resize with -1 argument";
				return;
			}
			float aspectRatio=(float)videoSize.width()/(float)videoSize.height();
			qDebug() << "aspect ratio:" << aspectRatio;
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
			qDebug() << "Resetting size to :" << w << "x" << h;
			videoObjects[id]->videoItem->setSize(QSizeF(w, h));
		}
	}
	else qDebug() << "StelVideoMgr::resizeVideo()" << id << ": no such video";
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
	else qDebug() << "StelVideoMgr::showVideo()" << id << ": no such video";
}

qint64 StelVideoMgr::getVideoDuration(const QString& id)
{
	if (videoObjects.contains(id))
	{
		return videoObjects[id]->duration;
	}
	else qDebug() << "StelVideoMgr::getDuration()" << id << ": no such video";
	return -1;
}

qint64 StelVideoMgr::getVideoPosition(const QString& id)
{
	if (videoObjects.contains(id))
	{
		return videoObjects[id]->player->position();
	}
	else qDebug() << "StelVideoMgr::getPosition()" << id << ": no such video";
	return -1;
}

//! returns resolution (in pixels) of loaded video. Returned value may be invalid before video is playing.
QSize StelVideoMgr::getVideoResolution(const QString& id)
{
	if (videoObjects.contains(id))
	{
		return videoObjects[id]->resolution;
	}
	else qDebug() << "StelVideoMgr::getResolution()" << id << ": no such video";
	return QSize();
}

int StelVideoMgr::getVideoWidth(const QString& id)
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

int StelVideoMgr::getVideoHeight(const QString& id)
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
		if (videoObjects[id]->player!=NULL)
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
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->setVolume(newVolume);
		}
	}
	else qDebug() << "StelVideoMgr::setVolume()" << id << ": no such video";
}

int StelVideoMgr::getVideoVolume(const QString& id)
{
	int volume=-1;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			volume=videoObjects[id]->player->volume();
		}
	}
	else qDebug() << "StelVideoMgr::getVolume()" << id << ": no such video";
	return volume;
}

bool StelVideoMgr::isVideoPlaying(const QString& id)
{
	bool playing=false;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
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
	qDebug() << "QMediaplayer: " << this->sender()->property("Stel_id").toString() << ": Audio is now available:" << available;
}
void StelVideoMgr::handleBufferStatusChanged(int percentFilled)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": Buffer filled (%):" << percentFilled;
}

// This gets called in loadVideo with player->setMedia(content)
void StelVideoMgr::handleCurrentMediaChanged(const QMediaContent & media)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": Current media changed:" << media.canonicalUrl();
}
void StelVideoMgr::handleDurationChanged(qint64 duration)
{
	QString id=QObject::sender()->property("Stel_id").toString();
	qDebug() << "QMediaplayer: " << id << ": Duration changed to:" << duration;
	if (videoObjects.contains(id))
	{
		videoObjects[id]->duration=duration;
	}
}
void StelVideoMgr::handleError(QMediaPlayer::Error error)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  error:" << error;
}

// This gets called in loadVideo with player->setMedia(content)
void StelVideoMgr::handleMediaChanged(const QMediaContent & media)
{
	qDebug() << "QMediaPlayer: " << QObject::sender()->property("Stel_id").toString() << ": Media changed:" << media.canonicalUrl();
}

// This may trigger the pause-at-end if it was fast enough.
// TODO: It seems that this is too slow! Window disappers, then reappears. This is ugly!
// Is there something that can be done?
void StelVideoMgr::handleMediaStatusChanged(QMediaPlayer::MediaStatus status) // debug-log messages
{
    QString id=QObject::sender()->property("Stel_id").toString();
    qDebug() << "QMediaplayer: " << id << ":  status changed:" << status;
    if ((status==QMediaPlayer::EndOfMedia) && videoObjects[id]->keepVisible)
    {
        //videoObjects[id]->player->setPosition(videoObjects[id]->duration - 1);
        //videoObjects[id]->player->pause();
        qDebug() << "handleMediaStatusChanged() no longer interacts here.";
    }
}

void StelVideoMgr::handleMutedChanged(bool muted)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  mute changed:" << muted;
}

/* // USELESS
void StelVideoMgr::handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration)
{
    qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  network configuration changed:" << configuration;
}
*/

/* // USELESS
void StelVideoMgr::handlePlaybackRateChanged(qreal rate)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": playback rate changed to:" << rate;
}
*/

void StelVideoMgr::handlePositionChanged(qint64 position)
{
    QString senderId=QObject::sender()->property("Stel_id").toString();
    qDebug() << "QMediaplayer: " << senderId << ":  position changed to (ms):" << position;
    // We can deal with the keep-visible here!
    if ((position==videoObjects[senderId]->duration) && (videoObjects[senderId]->keepVisible))
    {
        videoObjects[senderId]->player->setPosition(videoObjects[senderId]->duration - 1);
        videoObjects[senderId]->player->pause();
        qDebug() << " ---> paused at end as requested" ;
    }
}
void StelVideoMgr::handleSeekableChanged(bool seekable)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  seekable changed to:" << seekable;
}
void StelVideoMgr::handleStateChanged(QMediaPlayer::State state)
{
	QString senderId=QObject::sender()->property("Stel_id").toString();
	qDebug() << "QMediaplayer: " << senderId << ":  state changed to:" << state;
	qDebug() << "Media Resources from player:";
	qDebug() << "\tMedia Status:  " << videoObjects[senderId]->player->mediaStatus();
	qDebug() << "\tFile:          " << videoObjects[senderId]->player->currentMedia().canonicalUrl();
	// THIS IS ALL RUBBISH!
//	qDebug() << "\tData size:     " << videoObjects[senderId]->player->currentMedia().canonicalResource().dataSize();
//	qDebug() << "\tMIME Type:     " << videoObjects[senderId]->player->currentMedia().canonicalResource().mimeType();
//	qDebug() << "\tVideo codec:   " << videoObjects[senderId]->player->currentMedia().canonicalResource().videoCodec();
//	qDebug() << "\tResolution:    " << videoObjects[senderId]->player->currentMedia().canonicalResource().resolution();
//	qDebug() << "\tVideo bitrate: " << videoObjects[senderId]->player->currentMedia().canonicalResource().videoBitRate();
//	qDebug() << "\tAudio codec:   " << videoObjects[senderId]->player->currentMedia().canonicalResource().audioCodec();
//	qDebug() << "\tChannel Count: " << videoObjects[senderId]->player->currentMedia().canonicalResource().channelCount();
//	qDebug() << "\tAudio bitrate: " << videoObjects[senderId]->player->currentMedia().canonicalResource().audioBitRate();
//	qDebug() << "\tSample rate:   " << videoObjects[senderId]->player->currentMedia().canonicalResource().sampleRate();
//	qDebug() << "\tlanguage:      " << videoObjects[senderId]->player->currentMedia().canonicalResource().language();
}

void StelVideoMgr::handleVideoAvailableChanged(bool videoAvailable)
{
    qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Video available:" << videoAvailable;
}

void StelVideoMgr::handleVolumeChanged(int volume)
{
    qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  volume changed to:" << volume;
}

void StelVideoMgr::handleAvailabilityChanged(bool available)
{
    qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  available:" << available;
}

void StelVideoMgr::handleAvailabilityChanged(QMultimedia::AvailabilityStatus availability)
{
    qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  availability:" << availability;
}

void StelVideoMgr::handleMetaDataAvailableChanged(bool available)
{
    qDebug() << "handleMetaDataAvailableChanged():" << available << " -- Is this called on Windows?";

    QString id=QObject::sender()->property("Stel_id").toString();
    qDebug() << "QMediaplayer: " << id << ":  Metadata available:" << available;
    if (videoObjects.contains(id) && available)
    {
	QStringList metadataList=videoObjects[id]->player->availableMetaData();
	QStringList::const_iterator metadataIterator;
	for (metadataIterator=metadataList.constBegin(); metadataIterator!=metadataList.constEnd(); ++metadataIterator)
	{
	    QString key=(*metadataIterator).toLocal8Bit().constData();
	    qDebug() << "\t" << key << "==>" << videoObjects[id]->player->metaData(key).toString();
	}

    }
    else if (!available)
	qDebug() << "Metadata availability ceased.";
    else
	qDebug() << "StelVideoMgr::handleMetaDataChanged()" << id << ": no such video - this is absurd.";

}

void StelVideoMgr::handleMetaDataChanged()
{
    qDebug() << "!!! StelVideoMgr::handleMetadataChanged(): Is this called on Windows? ";

    QString id=QObject::sender()->property("Stel_id").toString();
    qDebug() << "QMediaplayer: " << id << ":  Metadata changed (global notification).";

    if (videoObjects.contains(id) && videoObjects[id]->player->isMetaDataAvailable())
    {
	qDebug() << "QMediaplayer: " << id << ":  Following metadata are available:";
	QStringList metadataList=videoObjects[id]->player->availableMetaData();
	QStringList::const_iterator mdIter;
	for (mdIter=metadataList.constBegin(); mdIter!=metadataList.constEnd(); ++mdIter)
	{
	    QString key=(*mdIter).toLocal8Bit().constData();
	    qDebug() << "\t" << key << "==>" << videoObjects[id]->player->metaData(key);
	}

    }
    else if (videoObjects.contains(id) && !(videoObjects[id]->player->isMetaDataAvailable()))
	qDebug() << "StelVideoMgr::handleMetaDataChanged()" << id << ": no metadata now.";
    else
	qDebug() << "StelVideoMgr::handleMetaDataChanged()" << id << ": no such video - this is absurd.";

}

void StelVideoMgr::handleMetaDataChanged(const QString & key, const QVariant & value)
{
    qDebug() << "!!! StelVideoMgr::handleMetadataChanged(.,.): Is this called on Windows? ";

    QString id=QObject::sender()->property("Stel_id").toString();
    qDebug() << "QMediaplayer: " << id << ":  Metadata change:" << key << "=>" << value;
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

void StelVideoMgr::handleNotifyIntervalChanged(int milliseconds)
{
    qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Notify interval changed to:" << milliseconds;
}



// update() has only to deal with the faders in all videos, and set positions and sizes of video windows.
void StelVideoMgr::update(double deltaTime)
{
    QMap<QString, VideoPlayer*>::const_iterator voIter;
    for (voIter=videoObjects.constBegin(); voIter!=videoObjects.constEnd(); ++voIter)
    {
	(*voIter)->fader.update((int)(deltaTime*1000));
	QSizeF currentFrameSize= /* QSizeF(1.0f, 1.0f) + */ (*voIter)->fader.getInterstate() * (*voIter)->popupFrameSize;
	QPointF frameCenter=  (*voIter)->popupOrigin +  ( (*voIter)->popupTargetCenter - (*voIter)->popupOrigin ) *  (*voIter)->fader.getInterstate();
	QPointF frameXY=frameCenter - 0.5*QPointF(currentFrameSize.width(), currentFrameSize.height());
	(*voIter)->videoItem->setPos(frameXY);
	(*voIter)->videoItem->setSize(currentFrameSize);
	qDebug() << "StelVideoMgr::update():" << (*voIter)->fader.getInterstate()  << "update frame " << currentFrameSize << "to" << frameXY;
	qDebug() << "StelVideoMgr::update(): Video at" << (*voIter)->player->position();

	// if we want still videos during transition, we must grow/shrink paused, and run only when reached full size.
	// TODO: We must detect a frame close to end, pause there, and trigger the fader back to 0.
	if ((*voIter)->keepVisible)
	{
	    if ((*voIter)->fader.getInterstate()<1.0f)
	    {
		qDebug() << "StelVideoMgr::update(): not fully grown: pause at start or end!";
		(*voIter)->player->pause();
	    }
	    else if ((*voIter)->player->position() >= ((*voIter)->duration - 100)) // allow stop 100ms before end...
	    {
		qDebug() << "StelVideoMgr::update(): position " << (*voIter)->player->position() << "close to end of duration " << (*voIter)->duration << "--> pause and shutdown with fader ";
		(*voIter)->player->setPosition((*voIter)->duration-10);
		(*voIter)->player->pause();
		(*voIter)->fader=false; // trigger shutdown
	    }
	    else if (((*voIter)->player->state() != QMediaPlayer::PlayingState))
	    {
		qDebug() << "StelVideoMgr::update(): fully grown: play!";
		(*voIter)->player->play();
	    }

	}
	// If the videos come with their own fade-in/-out, this can be played during transitions. (keepVisible configured to false)
	// In this case we must trigger *before* end of video!
	else if ((*voIter)->fader.getInterstate()>0.0f)
	{
	    if (((*voIter)->player->state() != QMediaPlayer::PlayingState))
		(*voIter)->player->play();
	    if ((*voIter)->player->position() >= ((*voIter)->duration - (*voIter)->fader.getDuration()))
	    {
		qDebug() << "StelVideoMgr::update(): position " << (*voIter)->player->position() << "close to end of duration " << (*voIter)->duration <<
			    "minus fader duration " << (*voIter)->fader.getDuration() << "--> shutdown with fader ";
		(*voIter)->fader=false; // trigger shutdown
	    }
	}
	else
	    qDebug() << "StelVideoMgr::update(): Unwanted situation: Interstate" << (*voIter)->fader.getInterstate();


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
void StelVideoMgr::seekVideo(const QString&, qint64, bool pause=true) {Q_UNUSED(pause);}
void StelVideoMgr::setVideoXY(const QString&, float, float) {;}
void StelVideoMgr::setVideoAlpha(const QString&, float) {;}
void StelVideoMgr::resizeVideo(const QString&, float, float) {;}
void StelVideoMgr::showVideo(const QString&, bool) {;}
// New functions for 0.14
int StelVideoMgr::getDuration(const QString&){return -1;}
QSize StelVideoMgr::getResolution(const QString&){return QSize(0,0);}
int StelVideoMgr::getWidth(const QString&){return -1;}
int StelVideoMgr::getHeight(const QString&){return -1;}
void StelVideoMgr::mute(const QString&, bool){;}
void StelVideoMgr::setVolume(const QString&, int){;}
int StelVideoMgr::getVolume(const QString&){return -1;}

// New functions for QT5
void StelVideoMgr::handleAudioAvailableChanged(bool available){;}
void StelVideoMgr::handleBufferStatusChanged(int percentFilled){;}
void StelVideoMgr::handleCurrentMediaChanged(const QMediaContent & media){;}
void StelVideoMgr::handleDurationChanged(qint64 duration){;}
void StelVideoMgr::handleError(QMediaPlayer::Error error){;}
void StelVideoMgr::handleMediaChanged(const QMediaContent & media){;}
void StelVideoMgr::handleMediaStatusChanged(QMediaPlayer::MediaStatus status){;}
void StelVideoMgr::handleMutedChanged(bool muted){;}
void StelVideoMgr::handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration){;}
void StelVideoMgr::handlePlaybackRateChanged(qreal){;}
void StelVideoMgr::handlePositionChanged(qint64){;}
void StelVideoMgr::handleSeekableChanged(bool){;}
void StelVideoMgr::handleStateChanged(QMediaPlayer::State){;}
void StelVideoMgr::handleVideoAvailableChanged(bool){;}
void StelVideoMgr::handleVolumeChanged(int){;}
void StelVideoMgr::handleAvailabilityChanged(bool){;}
void StelVideoMgr::handleAvailabilityChanged(QMultimedia::AvailabilityStatus){;}
void StelVideoMgr::handleMetaDataAvailableChanged(bool){;}
void StelVideoMgr::handleMetaDataChanged(){;}
void StelVideoMgr::handleMetaDataChanged(const QString&, const QVariant&){;}
void StelVideoMgr::handleNotifyIntervalChanged(int){;}

#endif // ENABLE_MEDIA


