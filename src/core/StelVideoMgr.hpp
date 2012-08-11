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

#ifndef _STELVIDEOMGR_HPP_
#define _STELVIDEOMGR_HPP_

#ifdef HAVE_QT_PHONON
#include <phonon/videoplayer.h>
#include <phonon/videowidget.h>
#include <phonon/mediaobject.h>
#endif

#include <QObject>
#include <QMap>
#include <QString>
#include <QGraphicsProxyWidget>

class StelVideoMgr : public QObject
{
	Q_OBJECT

public:
	StelVideoMgr();
	~StelVideoMgr();

public slots:
	void loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha);
	void playVideo(const QString& id);
	void pauseVideo(const QString& id);
	void stopVideo(const QString& id);
	void dropVideo(const QString& id);
	void seekVideo(const QString& id, qint64 ms);
	void setVideoXY(const QString& id, float x, float y);
	void setVideoAlpha(const QString& id, float alpha);
	void resizeVideo(const QString& id, float w, float h);
	void showVideo(const QString& id, bool show);

private:
#ifdef HAVE_QT_PHONON
	typedef struct {
		QWidget *widget;
		Phonon::VideoPlayer *player;
		QGraphicsProxyWidget *pWidget;
	} VideoPlayer;
	QMap<QString, VideoPlayer*> videoObjects; 
#endif

};

#endif // _STELVIDEOMGR_HPP_
