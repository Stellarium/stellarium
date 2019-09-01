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

#include <QDir>
#include <QDebug>
#include <QSettings>
#include "StelScriptOutput.hpp"
#include "StelApp.hpp"

// Init static variables.
QFile StelScriptOutput::outputFile;
QString StelScriptOutput::outputText;

void StelScriptOutput::init(const QString& outputFilePath)
{
	outputFile.setFileName(outputFilePath);
	if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text | QIODevice::Unbuffered))
		qDebug() << "ERROR: Cannot open file" << outputFilePath;
}

void StelScriptOutput::deinit()
{
	outputFile.close();
}

void StelScriptOutput::writeLog(QString msg)
{
	msg += "\n";
	outputFile.write(qPrintable(msg), msg.size());
	outputText += msg;
}

void StelScriptOutput::reset(void)
{
	outputFile.resize(0);
	outputFile.reset();
	outputText.clear();
}

void StelScriptOutput::saveOutputAs(const QString &name)
{
	QFile asFile;
	QFileInfo outputInfo(outputFile);
	QDir dir=outputInfo.dir(); // will hold complete dirname
	QFileInfo newFileNameInfo(name);

	bool okToSaveToAbsolutePath=StelApp::getInstance().getSettings()->value("scripts/flag_script_allow_write_absolute_path", false).toBool();

	if (!okToSaveToAbsolutePath && (newFileNameInfo.isAbsolute()))
	{
		qWarning() << "SCRIPTING CONFIGURATION ISSUE: You are trying to save to an absolute pathname.";
		qWarning() << "  To enable this, edit config.ini and set [scripts]/flag_script_allow_write_absolute_path=true";
		asFile.setFileName(dir.absolutePath() + "/" + newFileNameInfo.fileName());
		qWarning() << "  Storing to " << asFile.fileName() << " instead";
	}
	else if (okToSaveToAbsolutePath && (newFileNameInfo.isAbsolute()))
	{
		asFile.setFileName(name);
	}
	else
	{
		asFile.setFileName(dir.absolutePath() + "/" + name);
	}

	if (!asFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text | QIODevice::Unbuffered))
	{
		qDebug() << "ERROR: Cannot open file" << asFile.fileName();
		return;
	}
	qDebug() << "saving copy of output.txt to " << asFile.fileName();
	asFile.write(qPrintable(outputText), outputText.size());
	asFile.close();
}
