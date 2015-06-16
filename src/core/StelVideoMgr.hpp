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

#ifndef _STELVIDEOMGR_HPP_
#define _STELVIDEOMGR_HPP_

#include <QObject>
#include <QMap>
#include <QString>
#include <QMediaContent>
#include <QMediaPlayer>

class QGraphicsVideoItem;

class StelVideoMgr : public QObject
{
	Q_OBJECT

public:
	StelVideoMgr();
	~StelVideoMgr();

public slots:
	//! load a video from @name filename, assign an @name id for it for later reference.
	//! Prepare replay at upper-left corner @name x/@name y in natural resolution,
	//! decide whether to @name show the box already (paused at Frame 1), and set opacity @name alpha.
	//! If you want non-native resolution, load with @name show set to false, and use @name resizeVideo() and @name showVideo().
	void loadVideo(const QString& filename, const QString& id, const float x, const float y, const bool show, const float alpha);
	//! play video from current position. If @param keepLastFrame is true, video pauses at last frame.
	void playVideo(const QString& id, bool keepLastFrame=false);
	void pauseVideo(const QString& id);
	//! Stop playing, resets video and hides video output window
	void stopVideo(const QString& id);
	void dropVideo(const QString& id);
	void seekVideo(const QString& id, const qint64 ms, bool pause=true);
	//! move lower left corner of video @name id to @name x, @name y.
	void setVideoXY(const QString& id, const float x, const float y);
	//! sets opacity
	//! @param alpha opacity for the video.
	//! @note This seems not to work on Linux (GStreamer0.10 backend) and Windows (DirectShow backend).
	void setVideoAlpha(const QString& id, const float alpha);
	//! set video size to width @name w and height @name h.
	//! Use w=-1 or h=-1 for autoscale from the other dimension, or use w=h=-1 for native size of video.
	void resizeVideo(const QString& id, float w = -1.0f, float h = -1.0f);
	void showVideo(const QString& id, const bool show);
	// TODO: New functions:
//	//! returns duration (in seconds) of loaded video (maybe change to qint64/ms?)
//	int getDuration(const QString& id);
//	//! returns resolution (in pixels) of loaded video
//	QSize getResolution(const QString& id);
//	//! returns width (in pixels) of loaded video
//	int getWidth(const QString& id);
//	//! returns height (in pixels) of loaded video
//	int getHeight(const QString& id);
	//! set mute state of video player
	//! @param mute true to silence the video, false to hear audio.
	void mute(const QString& id, bool mute=true);
	//! set volume for video
	void setVolume(const QString& id, int newVolume);
	//! return currently set volume of media player, or -1 in case of some error.
	int getVolume(const QString& id);
	//! return whether video is currently playing
	bool isPlaying(const QString& id);


	// Slots to handle QMediaPlayer change signals:
	// These might hopefully be useful to understand media handling and how to get to crucial information like native resolution during loading of media.
	// We start by simple debug output...
	void handleAudioAvailableChanged(bool available);
	void handleBufferStatusChanged(int percentFilled);
	void handleCurrentMediaChanged(const QMediaContent & media);
	void handleDurationChanged(qint64 duration);
	void handleError(QMediaPlayer::Error error);
	void handleMediaChanged(const QMediaContent & media);
	void handleMediaStatusChanged(QMediaPlayer::MediaStatus status); // debug-log messages
	void handleMutedChanged(bool muted);
	//void handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration);
	void handlePlaybackRateChanged(qreal rate);
	void handlePositionChanged(qint64 position);
	void handleSeekableChanged(bool seekable);
	void handleStateChanged(QMediaPlayer::State state);
	void handleVideoAvailableChanged(bool videoAvailable);
	void handleVolumeChanged(int volume);
	// Slots to handle QMediaPlayer change signals inherited from QMediaObject
	void handleAvailabilityChanged(bool available);
	void handleAvailabilityChanged(QMultimedia::AvailabilityStatus availability);
	void handleMetaDataAvailableChanged(bool available);
	void handleMetaDataChanged();
	void handleMetaDataChanged(const QString & key, const QVariant & value);
	void handleNotifyIntervalChanged(int milliseconds);



private:
#if 0
	// Traces of old Qt4/Phonon:
	typedef struct {
		QWidget *widget;
		Phonon::VideoPlayer *player;
		QGraphicsProxyWidget *pWidget;
	} VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects; 
#endif
#ifdef ENABLE_MEDIA
	typedef struct {
		//QVideoWidget *widget; // would be easiest, but only with QtQuick2...
		QGraphicsVideoItem *videoItem;
		QMediaPlayer *player;
		qint64 duration;  //!< duration of video. This becomes available only after loading or at begin of playing! (?)
		QSize resolution; //!< stores resolution of video. This becomes available only after loading or at begin of playing!
	} VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects;
#endif
};

#endif // _STELVIDEOMGR_HPP_
