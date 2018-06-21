/*
 * Stellarium
 * Copyright (C) 2014 Alexander Wolf
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

#ifndef STELSCRIPTOUTPUT_HPP
#define STELSCRIPTOUTPUT_HPP

#include <QString>
#include <QFile>

//! @class StelScriptOutput
//! Class with only static members used to manage output for Stellarium scripts.
class StelScriptOutput
{
public:
	//! Create and initialize the log file.
	//! Prepend system information before any output.
	static void init(const QString& outputFilePath);

	//! Deinitialize the output file.
	//! Must be called after init() was called.
	static void deinit();

	//! Write the message plus a newline to the output file at $USERDIR/output.txt.
	//! @param msg message to write.
	static void writeLog(QString msg);

	//! Reset file, i.e., empty it. This may be useful to have repetitive output which may be read by other programs.
	static void reset(void);

	//! Save to new file, re-create output file.
	//! This allows reading of results on Windows, where otherwise reading programs cannot access files opened for writing by Stellarium.
	//! @param name new filename. If this is not an absolute path, it will be created in the same directory as output.txt
	//! @note For storing to absolute path names, set [scripts]/flag_script_allow_write_absolute_path=true.
	//! Normally you would call saveOutputAs(...), then reset().
	static void saveOutputAs(const QString& name);

private:
	static QFile outputFile;
	static QString outputText;
};

#endif // STELSCRIPTOUTPUT_HPP
