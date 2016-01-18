/*
 * Copyright (C) 2008 Matthew Gates
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

#include "StelAudioMgr.hpp"
#include "StelFileMgr.hpp"
#include <QDebug>
#include <QDir>

#ifdef ENABLE_MEDIA

#include <QMediaPlayer>

StelAudioMgr::StelAudioMgr()
{
}

StelAudioMgr::~StelAudioMgr()
{
	foreach(const QString& id, audioObjects.keys())
	{
		dropSound(id);
	}
}

void StelAudioMgr::loadSound(const QString& filename, const QString& id)
{
	if (audioObjects.contains(id))
	{
		qWarning() << "Audio object with ID" << id << "already exists, dropping it";
		dropSound(id);
	}

	QMediaPlayer* sound = new QMediaPlayer();
	QString path = QFileInfo(filename).absoluteFilePath();
	sound->setMedia(QMediaContent(QUrl::fromLocalFile(path)));
	audioObjects[id] = sound;
}

void StelAudioMgr::playSound(const QString& id)
{
	if (audioObjects.contains(id))
		if (audioObjects[id]!=NULL)
		{
			// if already playing, stop and play from the start
			if (audioObjects[id]->state()==QMediaPlayer::PlayingState)
				audioObjects[id]->stop();

			// otherwise just play it
			audioObjects[id]->play();
		}
}

void StelAudioMgr::pauseSound(const QString& id)
{
	if (audioObjects.contains(id))
		if (audioObjects[id]!=NULL)
			audioObjects[id]->pause();
}

void StelAudioMgr::stopSound(const QString& id)
{
	if (audioObjects.contains(id))
		if (audioObjects[id]!=NULL)
			audioObjects[id]->stop();
}

void StelAudioMgr::dropSound(const QString& id)
{
	if (!audioObjects.contains(id))
		return;
	if (audioObjects[id]!=NULL)
	{
		audioObjects[id]->stop();
		delete audioObjects[id];
		audioObjects.remove(id);
	}
}

#else 
void StelAudioMgr::loadSound(const QString& filename, const QString& id)
{
	qWarning() << "This build of Stellarium does not support sound - cannot load audio" << QDir::toNativeSeparators(filename) << id;
}
StelAudioMgr::StelAudioMgr() {}
StelAudioMgr::~StelAudioMgr() {;}
void StelAudioMgr::playSound(const QString&) {;}
void StelAudioMgr::pauseSound(const QString&) {;}
void StelAudioMgr::stopSound(const QString&) {;}
void StelAudioMgr::dropSound(const QString&) {;}
#endif
