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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "AudioMgr.hpp"
#include <QDebug>

AudioMgr::AudioMgr()
{
}

#ifdef HAVE_QT_PHONON
AudioMgr::~AudioMgr()
{
	foreach(QString id, audioObjects.keys())
	{
		if (audioObjects[id] != NULL)
		{
			delete audioObjects[id];
			audioObjects[id] = NULL;
		}
	}
}

void AudioMgr::loadSound(const QString& filename, const QString& id)
{
	if (audioObjects.contains(id))
	{
		qWarning() << "Audio object with ID" << id << "already exists, dropping it";
		dropSound(id);
	}

	Phonon::MediaObject* sound = Phonon::createPlayer(Phonon::MusicCategory, Phonon::MediaSource(filename));
	audioObjects[id] = sound;
}

void AudioMgr::playSound(const QString& id)
{
	if (audioObjects.contains(id))
		if (audioObjects[id]!=NULL)
			audioObjects[id]->play();
}

void AudioMgr::pauseSound(const QString& id)
{
	if (audioObjects.contains(id))
		if (audioObjects[id]!=NULL)
			audioObjects[id]->pause();
}

void AudioMgr::stopSound(const QString& id)
{
	if (audioObjects.contains(id))
		if (audioObjects[id]!=NULL)
			audioObjects[id]->stop();
}

void AudioMgr::dropSound(const QString& id)
{
	if (!audioObjects.contains(id))
		return;
	if (audioObjects[id]!=NULL)
	{
		delete audioObjects[id];
		audioObjects.remove(id);
	}
}

#else  // HAVE_QT_PHONON
void AudioMgr::loadSound(const QString& filename, const QString& id)
{
	qWarning() << "This build of Stellarium does not support sound - cannot load audio" << filename << id;
}
AudioMgr::~AudioMgr() {;}
void AudioMgr::playSound(const QString& id) {;}
void AudioMgr::pauseSound(const QString& id) {;}
void AudioMgr::stopSound(const QString& id) {;}
void AudioMgr::dropSound(const QString& id) {;}
#endif // HAVE_QT_PHONON


