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
#include <QSize>
#include <QSizeF>
#include <QPoint>
#include <QPointF>
#include <QMediaContent>
#include <QMediaPlayer>
#include "StelModule.hpp"
#include "StelFader.hpp"

class QGraphicsVideoItem;

//! @class StelVideoMgr
//! A scriptable way to show videos embedded in the screen.
//! After experimental support with Qt4/Phonon library, this feature is back.
//! Videos can be scaled, paused, placed and relocated (shifted) on screen.
//! Setting opacity seems not to do much unless setting it to zero, the video is then simply invisible.
//! Therefore smooth fading in/out or setting a semitransparent overlay does not work, but there is now an intro/end animation available:
//! zooming out from a pixel position to a player frame position, and returning to that spot close to end of video playback.
//!
//! However, support for multimedia content depends on the operating system and completeness of the QtMultimedia system support,
//! some features may not work for you.
//!
//! The listed functions have been tested and work on Ubuntu 15.04 with Qt5.4 with NVidia 9800M and Intel Core-i3/HD5500.
//! You need to install GStreamer plugins. Most critical seems to be gstreamer0.10-ffmpeg from
//! https://launchpad.net/~mc3man/+archive/ubuntu/gstffmpeg-keep,
//! then it plays MP4 (h264), Apple MOV(Sorenson) and WMV. Some type of AVI failed.
//! Note on Windows version:
//! According to https://wiki.qt.io/Qt_Multimedia, MinGW is limited to the decaying DirectShow platform plugin.
//! The WMF platform plugin requires Visual Studio, so building with MSVC should provide better result.
//! Some signals are not triggered under Windows, so we cannot use them, globally.
//! There is partial success with MP4 files on MinGW, but also these are rendered badly. Often just shows an error on Windows/MinGW:
//! DirectShowPlayerService::doRender: Unresolved error code 80040154
//! (number may differ, also seen: 80040228. Where is a list?)
//! And after one such error, you may even have to reboot to get it running with the MP4 again (??)
//!
//! QtMultimedia is a bit tricky to use: There seems bo be no way to load a media file to analyze resolution or duration before starting its replay.
//! This means, configuring player frames either require absolute frame coordinates, or triggering necessary configuration steps only after replay has started.
//! We opted for the latter solution because it allows scaled but undistorted video frames which may also take current screen resolution into account.

class StelVideoMgr : public StelModule
{
	Q_OBJECT

public:
	StelVideoMgr();
	~StelVideoMgr();

public slots:

	// Methods from StelModule: only update() needed.
	virtual void init(){;} // Or do something?

	//! Update the module with respect to the time. This allows the special effects in @name playVideoPopout(), and may evaluate things like video resolution when they become available.
	//! @param deltaTime the time increment in second since last call.
	virtual void update(double deltaTime);

	//! load a video from @param filename, assign an @param id for it for later reference.
	//! If @param id is already in use, replace it.
	//! Prepare replay at upper-left corner @param x/@param y in natural resolution,
	//! decide whether to @param show the box already (paused at Frame 1), and set opacity @name alpha.
	//! If you want non-native resolution, load with @param show set to false, and use @name resizeVideo() and @name showVideo().
	//! @bug Note that with @param alpha =0 the video is invisible as expected, other values make it fully opaque. There is no semi-transparency possible.
	void loadVideo(const QString& filename, const QString& id, const float x, const float y, const bool show, const float alpha);
	//! play video from current position. If @param keepLastFrame is true, video pauses at last frame.
	void playVideo(const QString& id, const bool keepVisibleAtEnd=false);

	//! Play a video which has previously been loaded with loadVideo with a complex effect.
	//! The video appears to pop out from @param fromX/ @param fromY,
	//! grows within @param popupDuration to size @param finalSizeX/@param finalSizeY, and
	//! shrinks back towards @param fromX/@param fromY at the end during @param popdownDuration.
	//! @param id the identifier used when loadVideo was called
	//! @param fromX X position of starting point, counted from left of window. May be absolute (if >1) or relative (0<X<1)
	//! @param fromY Y position of starting point, counted from top of window. May be absolute (if >1) or relative (0<Y<1)
	//! @param atCenterX X position of center of final video frame, counted from left of window. May be absolute (if >1) or relative (0<X<1)
	//! @param atCenterY Y position of center of final video frame, counted from top of window. May be absolute (if >1) or relative (0<Y<1)
	//! @param finalSizeX X size of final video frame. May be absolute (if >1) or relative to window size (0<X<1). If -1, scale proportional from @param finalSizeY.
	//! @param finalSizeY Y size of final video frame. May be absolute (if >1) or relative to window size (0<Y<1). If -1, scale proportional from @param finalSizeX.
	//! @param popupDuration duration of growing (start) / shrinking (end) transitions (seconds)
	//! @param frozenInTransition true if video should be paused during growing/shrinking transition.
	void playVideoPopout(const QString& id, float fromX, float fromY, float atCenterX, float atCenterY, float finalSizeX, float finalSizeY, float popupDuration, bool frozenInTransition);

	//! Pause video, keep it visible on-screen.
	//! Sending @name playVideo() continues replay, sending seekVideo() can move to a different position.
	void pauseVideo(const QString& id);

	//! Stop playing, resets video and hides video output window
	void stopVideo(const QString& id);

	//! Unload video from memory.
	void dropVideo(const QString& id);

	//! Seek a position in video @param id. Pause the video playing if @param pause=true.
	void seekVideo(const QString& id, const qint64 ms, bool pause=false);

	//! move upper left corner of video @name id to @name x, @name y.
	//! If x or y are <1, this is relative to screen size!
	void setVideoXY(const QString& id, const float x, const float y, const bool relative=false);

	//! sets opacity
	//! @param alpha opacity for the video (0=transparent, ... 1=fully opaque).
	//! @note if alpha is 0, also @name showVideo(id, true) cannot show the video.
	//! @bug (as of Qt5.4) Note that with @param alpha =0 the video is invisible as expected, other values make it fully opaque. There is no semi-transparency possible.
	void setVideoAlpha(const QString& id, const float alpha);

	//! set video size to width @param w and height @param h.
	//! Use @param w=-1 or @param h=-1 for autoscale from the other dimension,
	//! or use @param w=h=-1 for native size of video.
	//! if @param w or @param h are <=1, the size is relative to screen dimensions.
	//! This should be the preferred use, because it allows the development of device-independent scripts.
	//! When native resolution is unknown at the time you call this, it will be evaluated at the next update() after resolution becomes known.
	//! Autoscaling uses viewport dimensions at the time of calling, so resizing the Stellarium window will not resize the video frame during replay.
	void resizeVideo(const QString& id, float w = -1.0f, float h = -1.0f);

	//! show or hide video player
	//! @param id name given during loadVideo()
	//! @param show @value true to show, @value false to hide
	void showVideo(const QString& id, const bool show);

	//! returns duration (in milliseconds) of loaded video. This may return valid result only after @name playVideo() or @name pauseVideo() have been called.
	//! Returns -1 if video has not been analyzed yet. (loaded, but not started).
	qint64 getVideoDuration(const QString& id);

	//! returns current position (in milliseconds) of loaded video. This may return valid result only after @name playVideo() or @name pauseVideo() have been called.
	//! Returns -1 if video has not been analyzed yet. (loaded, but not started).
	qint64 getVideoPosition(const QString& id);

	//! returns resolution (in pixels) of loaded video. Returned value may be invalid before video is playing.
	QSize getVideoResolution(const QString& id);
	//! returns native width (in pixels) of loaded video, or -1 in case of trouble.
	int getVideoWidth(const QString& id);
	//! returns native height (in pixels) of loaded video, or -1 in case of trouble.
	int getVideoHeight(const QString& id);
	//! set mute state of video player
	//! @param mute true to silence the video, false to hear audio.
	void muteVideo(const QString& id, bool muteVideo=true);
	//! set volume for video
	void setVideoVolume(const QString& id, int newVolume);
	//! return currently set volume of media player, or -1 in case of some error.
	int getVideoVolume(const QString& id);
	//! returns whether video is currently playing.
	//! @param id name assigned during loadVideo().
	//! @note If video is not found, also returns @value false.
	bool isVideoPlaying(const QString& id);


	// Slots to handle QMediaPlayer signals. Never call them yourself!
	// These might hopefully be useful to understand media handling and how to get to crucial information like native resolution and duration during loading of media.
	// We start by simple debug output...
	void handleAudioAvailableChanged(bool available);
	void handleBufferStatusChanged(int percentFilled);
	void handleDurationChanged(qint64 duration);
	void handleError(QMediaPlayer::Error error);
	//void handleCurrentMediaChanged(const QMediaContent & media);
	//void handleMediaChanged(const QMediaContent & media);
	void handleMediaStatusChanged(QMediaPlayer::MediaStatus status); // debug-log messages
	void handleMutedChanged(bool muted);
	//void handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration);
	//void handlePlaybackRateChanged(qreal rate);
	//void handlePositionChanged(qint64 position); // periodically notify where in the video we are. Could be used to update scale bars, not needed.
	void handleSeekableChanged(bool seekable);
	void handleStateChanged(QMediaPlayer::State state);
	void handleVideoAvailableChanged(bool videoAvailable);
	void handleVolumeChanged(int volume);
	// Slots to handle QMediaPlayer change signals inherited from QMediaObject
	void handleAvailabilityChanged(bool available);
	void handleAvailabilityChanged(QMultimedia::AvailabilityStatus availability);
	// This seems not necessary.
	//void handleMetaDataAvailableChanged(bool available);
	//! @note This one works instead of handleMetaDataChanged(key, value) (QTBUG-42034)
	void handleMetaDataChanged();
	// This signal is not triggered on Windows, we must work around using handleMetaDataChanged()
	//void handleMetaDataChanged(const QString & key, const QVariant & value);
	//void handleNotifyIntervalChanged(int milliseconds); // interval for positionchange messages. Not needed.



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
		qint64 duration;           //!< duration of video. This becomes available only after loading or at begin of playing! (?)
		QSize resolution;          //!< stores resolution of video. This becomes available only after loading or at begin of playing, so we must apply a trick with signals and slots to set it.
		bool playNativeResolution; //!< store whether you want to play the video in native resolution. (Needed to set size after detection of resolution.)
		bool keepVisible;          //!< true if you want to show the last frame after video has played. (Due to delays in signal/slot handling of mediaplayer status changes, we use update() to stop a few frames before end.)
		bool needResize;           //!< becomes true if resize is called before resolution becomes available.
		bool simplePlay;           //!< only play in static size. true for playVideo(), false during popupPlay().
		LinearFader fader;         //!< Used during @name playVideoPopout() for nice transition effects.
		QSizeF  targetFrameSize;   //!< Video frame size, used during @name playVideo(), and final frame size during @name playVideoPopout().
		QPointF popupOrigin;       //!< Screen point where video appears to come out during @name playVideoPopout()
		QPointF popupTargetCenter; //!< Target frame position (center of target frame) used during @name playVideoPopout()
	} VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects;
#endif
};

#endif // _STELVIDEOMGR_HPP_
