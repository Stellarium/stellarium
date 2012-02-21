/*
 * Stellarium TelescopeControl Plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
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

#ifndef _TELESCOPE_CONTROL_GLOBALS_
#define _TELESCOPE_CONTROL_GLOBALS_

#include <QString>
#include <QStringList>

namespace TelescopeControlGlobals {
	const int MIN_SLOT_NUMBER = 1;
	const int SLOT_COUNT = 9;
	const int SLOT_NUMBER_LIMIT = MIN_SLOT_NUMBER + SLOT_COUNT;
	const int MAX_SLOT_NUMBER = SLOT_NUMBER_LIMIT - 1;

	const int BASE_TCP_PORT = 10000;
	#define DEFAULT_TCP_PORT_FOR_SLOT(X) (BASE_TCP_PORT + X)
	const int DEFAULT_TCP_PORT = DEFAULT_TCP_PORT_FOR_SLOT(MIN_SLOT_NUMBER);
	
	const int MAX_CIRCLE_COUNT = 10;
	
	#ifdef Q_OS_WIN32
	const QString TELESCOPE_SERVER_PATH = QString("/%1.exe");
	const QString SERIAL_PORT_PREFIX = QString("COM");
	#else
	const QString TELESCOPE_SERVER_PATH = QString("/%1");
	const QString SERIAL_PORT_PREFIX = QString("/dev/");
	#endif
	
	const int DEFAULT_DELAY = 500000; //Microseconds; == 0.5 seconds
	#define MICROSECONDS_FROM_SECONDS(X) (X * 1000000)
	#define SECONDS_FROM_MICROSECONDS(X) ((double) X / 1000000)
	
	enum ConnectionType {
		ConnectionNA = 0,
		ConnectionVirtual,
		ConnectionInternal,
		ConnectionLocal,
		ConnectionRemote,
		ConnectionCount
	};
	
	struct DeviceModel
	{
 		QString name;
		QString description;
		QString server;
		int defaultDelay;
		bool useExecutable;
	};
	
	
	#ifdef Q_OS_WIN32
	const QStringList SERIAL_PORT_NAMES = QString("COM1 COM2 COM3 COM4").split(' ', QString::SkipEmptyParts);
	#elif defined(Q_OS_MAC)
	const QStringList SERIAL_PORT_NAMES = QString("/dev/ ").split(' ', QString::SkipEmptyParts);
	#else
	const QStringList SERIAL_PORT_NAMES = QString("/dev/ttyS0 /dev/ttyS1 /dev/ttyS2 /dev/ttyS3 /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyUSB2 /dev/ttyUSB3").split(' ', QString::SkipEmptyParts);
	#endif
	
	//! List of the telescope servers that don't need external executables
	const QStringList EMBEDDED_TELESCOPE_SERVERS = QString("TelescopeServerDummy TelescopeServerLx200 TelescopeServerNexStar").split(' ', QString::SkipEmptyParts);
};

#endif //_TELESCOPE_CONTROL_GLOBALS_
