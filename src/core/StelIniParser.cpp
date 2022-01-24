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

#include "StelIniParser.hpp"
#include "StelUtils.hpp"

#include <QSettings>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <QIODevice>
#include <QRegularExpression>

bool readStelIniFile(QIODevice &device, QSettings::SettingsMap &map)
{
	// Is this the right conversion?
	const QString& data = QString::fromUtf8(device.readAll().constData());

	// Split by a RE which should match any platform's line breaking rules
	QRegularExpression matchLbr("[\\n\\r]+");
	#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
	const QStringList& lines = data.split(matchLbr, Qt::SkipEmptyParts);
	#else
	const QStringList& lines = data.split(matchLbr, QString::SkipEmptyParts);
	#endif

	QString currentSection = "";
	QRegularExpression sectionRe("^\\[(.+)\\]$");
	QRegularExpression keyRe("^([^=]+)\\s*=\\s*(.+)$");
	QRegularExpression cleanComment("[#;].*$");
	QRegularExpression initialWhiteSpace("^\\s+");
	QRegularExpression appendedWhitespace("\\s+$");

	for(int i=0; i<lines.size(); i++)
	{
		QString l = lines.at(i);
		l.replace(cleanComment, "");		// clean comments
		l.replace(initialWhiteSpace, "");	// clean whitespace
		l.replace(appendedWhitespace, "");

		QRegularExpressionMatch sectionMatch=sectionRe.match(l);
		QRegularExpressionMatch keyMatch=keyRe.match(l);
		// If it's a section marker set the section variable
		if (sectionMatch.hasMatch())
			currentSection = sectionMatch.captured(1);

		// Otherwise only process if it matches an re which looks like: key = value
		else if (keyMatch.hasMatch())
		{
			// Let REs do the work for us extracting the key and value
			// and cleaning them up by removing whitespace
			QString k = keyMatch.captured(1);
			QString v = keyMatch.captured(2);
			v.replace(initialWhiteSpace, "");
			k.replace(appendedWhitespace, "");

			// keys with no section should have no leading /, so only
			// add it when there is a valid section.
			if (currentSection != "")
				k = currentSection + "/" + k;

			// Set the map item.
			map[k] = QVariant(v);
		}
	}
	return true;
}

bool writeStelIniFile(QIODevice &device, const QSettings::SettingsMap &map)
{
	// QSettings only gives us a binary file handle when writing the
	// ini file, so we must handle alternative end of line marks
	// ourselves.
	const QString stelEndl = StelUtils::getEndLineChar();

	int maxKeyWidth = 30;
	QRegularExpression reKeyXt("^([^/]+)/(.+)$");  // for extracting keys/values

	// first go over map and find longest key length
	for (auto &key : map.keys())
	{
		QRegularExpressionMatch match=reKeyXt.match(key);
		if (match.hasMatch())
			key = match.captured(2);
		if (key.size() > maxKeyWidth) maxKeyWidth = key.size();
	}

	// OK, this time actually write to the file - first non-section values
	QString outputLine;
	for (auto &key : map.keys())
	{
		QRegularExpressionMatch match=reKeyXt.match(key);
		if (!match.hasMatch())
		{
			// this is for those keys without a section
			outputLine = QString("%1").arg(key,0-maxKeyWidth) + " = " + map[key].toString() + stelEndl;
			device.write(outputLine.toUtf8());
		}
	}

	// Now those values with sections.
	QString currentSection("");
	for (auto &key : map.keys())
	{
		QRegularExpressionMatch match=reKeyXt.match(key);
		if (match.hasMatch())
		{
			QString section = match.captured(1); QString sectionKey = match.captured(2);

			// detect new sections and write section headers in file
			if (section != currentSection)
			{
				currentSection = section;

				outputLine = stelEndl + "[" + currentSection + "]" + stelEndl;
				device.write(outputLine.toUtf8());
			}
			outputLine = QString("%1").arg(sectionKey,0-maxKeyWidth) + " = " + map[key].toString() + stelEndl;
			device.write(outputLine.toUtf8());
		}
	}
	return true;
}

