/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#ifndef STELLOGGER_HPP
#define STELLOGGER_HPP

#include <QString>
#include <QFile>

//! @class StelLogger
//! Class wit only static members used to manage logging for Stellarium.
//! The debugLogHandler() method allow to defined it as a standard Qt messages handler
//! which is then used by qDebug, qWarning and qFatal.
class StelLogger
{
public:
	//! Create and initialize the log file.
	//! Prepend system information before any debugging output.
	static void init(const QString& logFilePath);

	//! Deinitialize the log file.
	//! Must be called after init() was called.
	static void deinit();

	//! Handler for qDebug() and friends. Writes message to log file at $USERDIR/log.txt and echoes to stderr.
	static void debugLogHandler(QtMsgType, const char*);

	//! Return a copy of text of the log file.
	static const QString& getLog() {return log;}

	static QString getLogFileName() {return logFile.fileName();}

	//! Write the message plus a newline to the log file at $USERDIR/log.txt.
	//! @param msg message to write.
	//! If you call this function the message will be only in the log file,
	//! not on the console like with qDebug().
	static void writeLog(QString msg);

private:
	static QFile logFile;
	static QString log;
};

#endif // STELLOGGER_HPP
