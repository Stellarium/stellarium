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
	for (const auto& id : audioObjects.keys())
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
	{
		if (audioObjects[id]!=Q_NULLPTR)
		{
			// if already playing, stop and play from the start
			if (audioObjects[id]->state()==QMediaPlayer::PlayingState)
				audioObjects[id]->stop();

			// otherwise just play it
			audioObjects[id]->play();
		}
		else
			qDebug() << "StelAudioMgr: Cannot play sound, " << id << "not correctly loaded.";
	}
	else
		qDebug() << "StelAudioMgr: Cannot play sound, " << id << "not loaded.";
}

void StelAudioMgr::pauseSound(const QString& id)
{
	if (audioObjects.contains(id))
	{
		if (audioObjects[id]!=Q_NULLPTR)
			audioObjects[id]->pause();
		else
			qDebug() << "StelAudioMgr: Cannot pause sound, " << id << "not correctly loaded.";
	}
	else
		qDebug() << "StelAudioMgr: Cannot pause sound, " << id << "not loaded.";
}

void StelAudioMgr::stopSound(const QString& id)
{
	if (audioObjects.contains(id))
	{
		if (audioObjects[id]!=Q_NULLPTR)
			audioObjects[id]->stop();
		else
			qDebug() << "StelAudioMgr: Cannot stop sound, " << id << "not correctly loaded.";
	}
	else
		qDebug() << "StelAudioMgr: Cannot stop sound, " << id << "not loaded.";
}

void StelAudioMgr::dropSound(const QString& id)
{
	if (!audioObjects.contains(id))
	{
		qDebug() << "StelAudioMgr: Cannot drop sound, " << id << "not loaded.";
		return;
	}
	if (audioObjects[id]!=Q_NULLPTR)
	{
		audioObjects[id]->stop();
		delete audioObjects[id];
		audioObjects.remove(id);
	}
}


qint64 StelAudioMgr::position(const QString& id)
{
	if (!audioObjects.contains(id))
	{
		qDebug() << "StelAudioMgr: Cannot report position for sound, " << id << "not loaded.";
		return(-1);
	}
	if (audioObjects[id]!=Q_NULLPTR)
	{
		return audioObjects[id]->position();
	}
	return (-1);
}

qint64 StelAudioMgr::duration(const QString& id)
{
	if (!audioObjects.contains(id))
	{
		qDebug() << "StelAudioMgr: Cannot report duration for sound, " << id << "not loaded.";
		return(-1);
	}
	if (audioObjects[id]!=Q_NULLPTR)
	{
		return audioObjects[id]->duration();
	}
	return (-1);
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
qint64 StelAudioMgr::position(const QString&){return -1;}
qint64 StelAudioMgr::duration(const QString&){return -1;}
#endif
