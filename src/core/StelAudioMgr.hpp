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

#ifndef _STELAUDIOMGR_HPP_
#define _STELAUDIOMGR_HPP_

#ifdef HAVE_QT_PHONON
#include <phonon/mediaobject.h>
#endif

#include <QObject>
#include <QMap>
#include <QString>

class StelAudioMgr : public QObject
{
	Q_OBJECT

public:
	StelAudioMgr();
	~StelAudioMgr();

public slots:
	void loadSound(const QString& filename, const QString& id);
	void playSound(const QString& id);
	void pauseSound(const QString& id);
	void stopSound(const QString& id);
	void dropSound(const QString& id);

private:
#ifdef HAVE_QT_PHONON
	QMap<QString, Phonon::MediaObject*> audioObjects;
#endif

};

#endif // _STELAUDIOMGR_HPP_
