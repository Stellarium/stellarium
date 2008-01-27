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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _STELINIPARSER_HPP_
#define _STELINIPARSER_HPP_

#include <QSettings>

bool readStelIniFile(QIODevice &device, QSettings::SettingsMap &map);
bool writeStelIniFile(QIODevice &device, const QSettings::SettingsMap &map);

static const QSettings::Format StelIniFormat = QSettings::registerFormat("ini", readStelIniFile, writeStelIniFile);

#endif
