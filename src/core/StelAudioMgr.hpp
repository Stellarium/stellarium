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

#ifndef STELAUDIOMGR_HPP
#define STELAUDIOMGR_HPP

#include <QObject>
#include <QMap>
#include <QString>

class QMediaPlayer;

class StelAudioMgr : public QObject
{
	Q_OBJECT

public:
	StelAudioMgr();
	~StelAudioMgr();

public slots:
	//! Load sound file to be accessed under ID.
	//! If id has been defined already, replace the old sound object.
	void loadSound(const QString& filename, const QString& id);
	//! Play sound with id previously loaded with loadSound. Prints warning to logfile if id not found.
	//! If sound is already playing, restart it. If sound had been paused previously, continue replay.
	void playSound(const QString& id);
	//! Pauses replay of sound id. Prints warning to logfile if id not found.
	void pauseSound(const QString& id);
	//! Stops replay of sound id. Prints warning to logfile if id not found.
	void stopSound(const QString& id);
	//! remove audio object from memory. Prints warning to logfile if id not found.
	void dropSound(const QString& id);
	//! report position (in ms) in running audio track id, or -1 if this is not possible.
	qint64 position(const QString& id);
	//! report duration (in ms) in running audio track id, 0 if unknown (before playback starts!), or -1 if this is not possible.
	//! @note duration may only be detected after playback has started! Call it a second after that.
	qint64 duration(const QString& id);

private:
	QMap<QString, QMediaPlayer*> audioObjects;
};

#endif // STELAUDIOMGR_HPP
