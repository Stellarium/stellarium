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
//! However, support for multimedia content depends on the operating system, installed codecs, and completeness of the QtMultimedia system support,
//! so some features or video formats may not work for you (test video and re-code it if necessary).
//!
//! <h2>Linux notes</h2>
//! The listed functions have been tested and work on Ubuntu 15.04 with Qt5.4 with NVidia 9800M and Intel Core-i3/HD5500.
//! You need to install GStreamer plugins. Most critical seems to be gstreamer0.10-ffmpeg from
//! https://launchpad.net/~mc3man/+archive/ubuntu/gstffmpeg-keep,
//! then it plays
//! <ul>
//! <li>MP4 (h264)</li>
//! <li>Apple MOV(Sorenson)</li>
//! <li>WMV.</li>
//! <li>Some type of AVI failed</li>
//! </ul>
//!
//! <h2>Windows notes</h2>
//! According to https://wiki.qt.io/Qt_Multimedia, MinGW is limited to the decaying DirectShow platform plugin.
//! The WMF platform plugin requires Visual Studio, so building with MSVC should provide better result.
//! Some signals are not triggered under Windows, so we cannot use them, globally.
//! There is partial success with MP4 files on MinGW, but also these are rendered badly. Often just shows an error on Windows/MinGW:
//! DirectShowPlayerService::doRender: Unresolved error code 80040154
//! (number may differ, also seen: 80040228. Where is a list?)
//! The formats tested on Windows are: <ul>
//! <li>MP4 (h264; OK) </li>
//! <li>WMV (OK, but jumping to different locations via seekVideo() seems not to work properly) </li>
//! <li>MOV (mp4v codec, very jerky, basically unusable) </li>
//! <li>AVI (DIVX MP4 codec, same bad issues as MOV) </li>
//! <li>OGV (invalid media) </li>
//! <li>WEBM (invalid media) </li>
//! </ul>
//!
//! <h2>Mac OS X Notes</h2>
//! NOT TESTED ON A MAC! There is a critical difference (causing a crash!) between Win and Linux to either hide or not hide the player just after loading.
//! Please somebody find out Mac behaviour.
//!
//! QtMultimedia is a bit tricky to use: There seems to be no way to load a media file to analyze resolution or duration before starting its replay.
//! This means, configuring player frames either require absolute frame coordinates, or triggering necessary configuration steps only after replay has started.
//! We opted for the latter solution because it allows scaled but undistorted video frames which may also take current screen resolution into account.
//!
//! Under unclear circumstances we also have a pair of messages:
//! @code Failed to start video surface due to main thread blocked.
//! @code Failed to start video surface
//! and non-appearing video frame, this seems to be https://bugreports.qt.io/browse/QTBUG-39567.
//! This occurred on an Intel NUC5i3 with SSD, so loading the file should not be much of an issue.
//!
//! To help in debugging scripts, this module can be quite verbose in the logfile if Stellarium is called with the command-line argument "--verbose".

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
	//! Prepare replay at upper-left corner @param x/@param y in native resolution,
	//! decide whether to @param show (play) the video already, and set opacity @name alpha.
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
	//! @note This may not work if video has not been fully loaded. Better wait a second before proceeding after loadVideo(), and call seekVideo(., ., false); pauseVideo(.);
	void seekVideo(const QString& id, const qint64 ms, bool pause=false);

	//! move upper left corner of video @name id to @name x, @name y.
	//! If x or y are <1, this is relative to screen size!
	void setVideoXY(const QString& id, const float x, const float y, const bool relative=false);

	//! sets opacity
	//! @param alpha opacity for the video (0=transparent, ... 1=fully opaque).
	//! @note if alpha is 0, also @name showVideo(id, true) cannot show the video.
	//! @bug (as of Qt5.5) Note that with @param alpha =0 the video is invisible as expected, other values make it fully opaque. There is no semi-transparency possible.
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

	//! returns resolution (in pixels) of loaded video. Returned value may be invalid before video has been fully loaded.
	QSize getVideoResolution(const QString& id);
	//! returns native width (in pixels) of loaded video, or -1 in case of trouble.
	int getVideoWidth(const QString& id);
	//! returns native height (in pixels) of loaded video, or -1 in case of trouble.
	int getVideoHeight(const QString& id);

	//! set mute state of video player
	//! @param mute true to silence the video, false to hear audio.
	void muteVideo(const QString& id, bool muteVideo=true);
	//! set volume for video. Valid values are 0..100, values outside this range will be clamped.
	void setVideoVolume(const QString& id, int newVolume);
	//! return currently set volume (0..100) of media player, or -1 in case of some error.
	int getVideoVolume(const QString& id);

	//! returns whether video is currently playing.
	//! @param id name assigned during loadVideo().
	//! @note If video is not found, also returns @value false.
	bool isVideoPlaying(const QString& id);

private slots:
	// Slots to handle QMediaPlayer signals. Never call them yourself!
	// Some of them are useful to understand media handling and to get to crucial information like native resolution and duration during loading of media.
	// Most only give simple debug output...
	void handleAudioAvailableChanged(bool available);
	void handleBufferStatusChanged(int percentFilled);
	void handleDurationChanged(qint64 duration);
	void handleError(QMediaPlayer::Error error);
	void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
	void handleMutedChanged(bool muted);
	//void handlePositionChanged(qint64 position); // periodically notify where in the video we are. Could be used to update scale bars, not needed.
	void handleSeekableChanged(bool seekable);
	void handleStateChanged(QMediaPlayer::State state);
	void handleVideoAvailableChanged(bool videoAvailable);
	void handleVolumeChanged(int volume);

	// Slots to handle QMediaPlayer change signals inherited from QMediaObject:
	void handleAvailabilityChanged(bool available);
	void handleAvailabilityChanged(QMultimedia::AvailabilityStatus availability);
	//! @note This one works, while handleMetaDataChanged(key, value) is not called on Windows (QTBUG-42034)
	void handleMetaDataChanged();
	// This signal is not triggered on Windows, we must work around using handleMetaDataChanged()
	//void handleMetaDataChanged(const QString & key, const QVariant & value);



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
		bool keepVisible;          //!< true if you want to show the last frame after video has played. (Due to delays in signal/slot handling of mediaplayer status changes, we use update() to stop a few frames before end.)
		bool needResize;           //!< becomes true if resize is called before resolution becomes available.
		bool simplePlay;           //!< only play in one static size. true for playVideo(), false during playVideoPopout().
		LinearFader fader;         //!< Used during @name playVideoPopout() for nice transition effects.
		QSizeF  targetFrameSize;   //!< Video frame size, used during @name playVideo(), and final frame size during @name playVideoPopout().
		QPointF popupOrigin;       //!< Screen point where video appears to come out during @name playVideoPopout()
		QPointF popupTargetCenter; //!< Target frame position (center of target frame) used during @name playVideoPopout()
		int lastPos;               //!< This should not be required: We must track a bug in QtMultimedia where the QMediaPlayer is in playing state but does not progress the video position. In update() we try to let it run again.
	} VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects;
	bool verbose;                      //!< true to write many more log entries (useful for script debugging) Activate with command-line option "--verbose"
#endif
};

#endif // _STELVIDEOMGR_HPP_
