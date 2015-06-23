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
//! After experimental support with Qt4/Phonon library, this feature is back, and better than before.
//! Videos can be scaled, paused, placed and relocated (shifted) on screen.
//! Setting opacity seems not to do much unless setting it to zero, the video is then simply invisible.
//! Therefore smooth fading in/out does not work, but still there is an intro/end animation available:
//! growing from a pixel position to the player position, and returning to that spot close to end of video playback.
//! TODO: Implement this animation ;-)
//!
//! However, support for multimedia content depends on the operating system and multimedia system support,
//! some features may not work for you. The listed functions have been tested and work on Ubuntu 15.04 with Qt5.4.
//! You need to install GStreamer plugins. Most critical seems to be gstreamer0.10-ffmpeg from
//! https://launchpad.net/~mc3man/+archive/ubuntu/gstffmpeg-keep,
//! then it plays MP4 (h264), Apple MOV(Sorenson) and WMV. Some type of AVI failed.
//! Note on Windows version:
//! According to https://wiki.qt.io/Qt_Multimedia, MinGW is limited to the decaying DirectShow platform plugin.
//! The WMF platform plugin requires Visual Studio.
//! There is partial success with MP4 files, but also these are rendered badly. Often just shows an error on Windows/MinGW:
//! DirectShowPlayerService::doRender: Unresolved error code 80040154
//! (number may differ, also seen: 80040228. Where is a list?)
//! And after one such error, you may even have to reboot to get it running with the MP4 again (??)

class StelVideoMgr : public StelModule
{
	Q_OBJECT

public:
	StelVideoMgr();
	~StelVideoMgr();

public slots:

    // Methods from StelModule: only update() needed.
    virtual void init(){;} // Do something?

    //! Update the module with respect to the time. This allows the special effects in @name playVideoPopout().
    //! @param deltaTime the time increment in second since last call.
    virtual void update(double deltaTime);


    //! load a video from @param filename, assign an @param id for it for later reference.
    //! If @param id is already in use, replace it.
    //! Prepare replay at upper-left corner @param x/@param y in natural resolution,
    //! decide whether to @param show the box already (paused at Frame 1), and set opacity @name alpha.
    //! If you want non-native resolution, load with @param show set to false, and use @name resizeVideo() and @name showVideo().
	void loadVideo(const QString& filename, const QString& id, const float x, const float y, const bool show, const float alpha);
	//! play video from current position. If @param keepLastFrame is true, video pauses at last frame.
	void playVideo(const QString& id, bool keepLastFrame=false);

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
    //! @param alpha opacity for the video (0=transparent, 1=fully visible).
    //! @note if alpha is 0, also @name showVideo(id, true) cannot show the video.
    //! @note This seems not to work on Linux (GStreamer0.10 backend) and Windows (DirectShow backend).
    //!       The only visible effect is visibility=(alpha>0)
    //! @todo Find a way to fade/mix video with its background, i.e. make this work.
	void setVideoAlpha(const QString& id, const float alpha);
	//! set video size to width @name w and height @name h.
    //! Use @value w=-1 or @value h=-1 for autoscale from the other dimension,
    //! or use @value w=h=-1 for native size of video.
    //! if @value w or @value h are <=1, the size is relative to screen dimensions.
    //! This should be the preferred use, because it allows the development of device-independent scripts.
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


    // Slots to handle QMediaPlayer signals. Never call them yourelf!
	// These might hopefully be useful to understand media handling and how to get to crucial information like native resolution during loading of media.
	// We start by simple debug output...
	void handleAudioAvailableChanged(bool available);
	void handleBufferStatusChanged(int percentFilled);
	void handleCurrentMediaChanged(const QMediaContent & media);
	void handleDurationChanged(qint64 duration);
	void handleError(QMediaPlayer::Error error);
    // This should never be called in practice!
	void handleMediaChanged(const QMediaContent & media);
	void handleMediaStatusChanged(QMediaPlayer::MediaStatus status); // debug-log messages
	void handleMutedChanged(bool muted);
	//void handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration);
    //void handlePlaybackRateChanged(qreal rate);
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
        QSize resolution; //!< stores resolution of video. This becomes available only after loading or at begin of playing, so we must apply a trick with signals and slots to set it.
        bool playNativeResolution; //!< store whether you want to play the video in native resolution. (Needed to set size after detection of resolution.)
        bool keepVisible; //!< true if you want to show the last frame after video has played. (Due to delays in signal/slot handling, this does not work properly.)
        LinearFader fader; //!< Used during @name playVideoPopout() for nice transition effects.
        QSizeF  popupFrameSize;   //!< Target frame size, used during @name playVideoPopout()
        QPointF popupOrigin;      //!< Screen point where video appears to come out during @name playVideoPopout()
        QPointF popupTargetCenter; //!< Target frame position (center of target frame) used during @name playVideoPopout()
    } VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects;
#endif
};

#endif // _STELVIDEOMGR_HPP_
