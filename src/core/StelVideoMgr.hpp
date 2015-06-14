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
class QMediaPlayer;
class QGraphicsVideoItem;

class StelVideoMgr : public QObject
{
	Q_OBJECT

public:
	StelVideoMgr();
	~StelVideoMgr();

public slots:
	void loadVideo(const QString& filename, const QString& id, const float x, const float y, const bool show, const float alpha);
	void playVideo(const QString& id);
	void pauseVideo(const QString& id);
	void stopVideo(const QString& id);
	void dropVideo(const QString& id);
	void seekVideo(const QString& id, const qint64 ms);
	//! move lower left corner of video @name id to @name x, @name y.
	void setVideoXY(const QString& id, const float x, const float y);
	//! sets opacity
	//! @param alpha opacity for the video.
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
		//QGraphicsProxyWidget *pWidget; // No longer required?
	} VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects;
#endif
};

#endif // _STELVIDEOMGR_HPP_
