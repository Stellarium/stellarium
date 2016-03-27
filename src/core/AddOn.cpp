/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#include <QStringBuilder>
#include <QtDebug>
#include <QDir>
#include <QVariantMap>

#include "AddOn.hpp"
#include "StelAddOnMgr.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"

AddOn::AddOn(const QString addonId, const QVariantMap& map)
	: m_sAddonId(addonId)
	, m_eType(INVALID)
	, m_bIsValid(false)
	, m_eStatus(NotInstalled)
{
	m_sType = map.value("type").toString();
	m_eType = typeStringToEnum(m_sType);
	m_sTypeDisplayRole = typeToDisplayRole(m_eType);
	if (m_eType == INVALID)
	{
		qWarning() << "[Add-on] Error! Add-on" << m_sAddonId
			   << "does not have a valid type!";
		return;
	}

	m_sTitle = map.value("title").toString();
	m_dDate = map.value("date").toDate();
	m_sDescription = map.value("description").toString();
	m_lSupported = map.value("supported").toStringList();
	m_sLicense = map.value("license").toString();
	m_sLicenseURL = map.value("license-url").toString();

	m_sChecksum = map.value("checksum").toString();
	m_sDownloadFilename = map.value("download-filename").toString();
	m_fDownloadSize = map.value("download-size").toFloat();
	m_sDownloadURL = map.value("download-url").toString();

	m_iVersion = map.value("version").toInt();

	// early returns if the mandatory fields are not present
	if (m_sTitle.isEmpty() || m_sDescription.isEmpty() || m_dDate.isNull() || m_sDownloadFilename.isEmpty())
	{
		qWarning() << "[Add-on] Error! Add-on" << m_sAddonId
			   << "does not have all the required fields!"
			   << "(title, description, date and download-filename)";
		return;
	}

	// checking compatibility
	if (!m_lSupported.isEmpty() && !m_lSupported.contains(StelUtils::getApplicationSeries()))
	{
		qWarning() << "[Add-on] Error! Add-on" << m_sAddonId << "is not supported!";
		return;
	}

	m_sDownloadSize = fileSizeToString(m_fDownloadSize);

	if (m_eType == SCRIPT)
	{
		m_sLanguage = map.value("language").toString();
	}

	if (map.contains("authors"))
	{
		QVariantList list = map.value("authors").toList();
		foreach (const QVariant& author, list)
		{
			QVariantMap authors = author.toMap();
			Authors a;
			a.name = authors.value("name").toString();
			a.email = authors.value("email").toString();
			a.url = authors.value("url").toString();
			m_lAuthors.append(a);
		}
	}

	m_sZipPath = StelFileMgr::getAddonDir() % QDir::separator() % m_sDownloadFilename;

	// specific fields in 'installed_addons.json'
	m_lInstalledFiles = map.value("installed-files").toStringList();
	m_eStatus = (AddOn::Status) map.value("status").toInt();

	m_bIsValid = true;
}

AddOn::~AddOn()
{
}

QVariantMap AddOn::getMap()
{
	QVariantMap map;
	map.insert("type", m_sType);
	map.insert("title", m_sTitle);
	map.insert("date", m_dDate.toString("yyyy.MM.dd"));
	map.insert("description", m_sDescription);
	map.insert("supported", m_lSupported);
	map.insert("license", m_sLicense);
	map.insert("license-url", m_sLicenseURL);

	map.insert("checksum", m_sChecksum);
	map.insert("download-filename", m_sDownloadFilename);
	map.insert("download-size", m_fDownloadSize);
	map.insert("download-url", m_sDownloadURL);

	map.insert("version", m_iVersion);

	QVariantList authors;
	foreach (AddOn::Authors a, m_lAuthors)
	{
		QVariantMap author;
		author.insert("name", a.name);
		author.insert("email", a.email);
		author.insert("url", a.url);
		authors.append(author);
	}
	map.insert("authors", authors);

	if (m_eType == SCRIPT)
	{
		m_sLanguage = map.value("language").toString();
	}

	if (!m_lInstalledFiles.isEmpty())
	{
		map.insert("installed-files", m_lInstalledFiles);
	}
	map.insert("status", m_eStatus);

	return map;
}

QString AddOn::getStatusString()
{
	switch (m_eStatus)
	{
		case PartiallyInstalled:
			return "Partially Installed";
		case FullyInstalled:
			return "Installed";
		case Installing:
			return "Installing";
		case Restart:
			return "Restart";
		case Corrupted:
			return "Corrupted";
		case InvalidFormat:
			return "Invalid format";
		case InvalidDestination:
			return "Invalid destination";
		case UnableToWrite:
			return "Unable to write";
		case UnableToRead:
			return "Unable to read";
		case UnableToRemove:
			return "Unable to remove";
		case PartiallyRemoved:
			return "Partially removed";
		case DownloadFailed:
			return "Download failed";
		default:
			return "Not installed";
	}
}

AddOn::Type AddOn::typeStringToEnum(QString string)
{
	if (string == "addon_catalog")
	{
		return ADDON_CATALOG;
	}
	else if (string == "landscape")
	{
		return LANDSCAPE;
	}
	else if (string == "plugin_catalog")
	{
		return PLUGIN_CATALOG;
	}
	else if (string == "script")
	{
		return SCRIPT;
	}
	else if (string == "sky_culture")
	{
		return SKY_CULTURE;
	}
	else if (string == "star_catalog")
	{
		return STAR_CATALOG;
	}
	else if (string == "texture")
	{
		return TEXTURE;
	}
	else if (string == "translation")
	{
		return TRANSLATION;
	}
	else
	{
		return INVALID;
	}
}

QString AddOn::typeToDisplayRole(Type type) {
	if (type == ADDON_CATALOG)
	{
		return "Add-on Catalog";
	}
	else if (type == LANDSCAPE)
	{
		return "Landscape";
	}
	else if (type == PLUGIN_CATALOG)
	{
		return "Plugin Catalog";
	}
	else if (type == SCRIPT)
	{
		return "Script";
	}
	else if (type == SKY_CULTURE)
	{
		return "Sky Culture";
	}
	else if (type == STAR_CATALOG)
	{
		return "Star Catalog";
	}
	else if (type == TEXTURE)
	{
		return "Texture";
	}
	else if (type == TRANSLATION)
	{
		return "Translation";
	}
	else
	{
		return "Invalid";
	}
}

QString AddOn::fileSizeToString(float bytes)
{
	QStringList list;
	list << "KB" << "MB" << "GB" << "TB";
	QStringListIterator i(list);
	QString unit("bytes");
	while(bytes >= 1000.0 && i.hasNext())
	{
	    unit = i.next();
	    bytes /= 1000.0;
	}
	return QString::number(bytes,'f',2) % " " % unit;
}
