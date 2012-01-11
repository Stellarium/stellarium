/*
 * Stellarium
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

#ifndef _STELINIPARSER_HPP_
#define _STELINIPARSER_HPP_

#include <QSettings>

// QSettings only gives us a binary file handle when writing the
// ini file, so we must handle alternative end of line marks
// ourselves.
#ifdef Q_OS_WIN
	static const QString stelEndl="\r\n";
#else
	static const QString stelEndl="\n";
#endif


bool readStelIniFile(QIODevice &device, QSettings::SettingsMap &map);
bool writeStelIniFile(QIODevice &device, const QSettings::SettingsMap &map);

static const QSettings::Format StelIniFormat = QSettings::registerFormat("ini", readStelIniFile, writeStelIniFile);

#endif
