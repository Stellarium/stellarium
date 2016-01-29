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
	m_sDescription = map.value("description").toString();
	m_dVersion = map.value("version").toDate();
	m_supported = map.value("supported").toStringList();
	m_sLicense = map.value("license").toString();
	m_sLicenseURL = map.value("license-url").toString();
	m_sDownloadURL = map.value("download-url").toString();
	m_sDownloadFilename = map.value("download-filename").toString();
	m_fDownloadSize = map.value("download-size").toFloat();
	m_sChecksum = map.value("checksum").toString();
	m_sThumbnail = map.value("thumbnail").toString();

	// specific field for scripts
	m_sLanguage = map.value("language").toString();

	// specific fields in 'installed_addons.json'
	m_InstalledFiles = map.value("installed-files").toStringList();
	m_eStatus = (AddOn::Status) map.value("status").toInt();

	// early returns if the mandatory fields are not present
	if (m_sTitle.isEmpty() || m_sDescription.isEmpty() || m_dVersion.isNull() || m_sDownloadFilename.isEmpty())
	{
		qWarning() << "[Add-on] Error! Add-on" << m_sAddonId
			   << "does not have all the required fields!"
			   << "(title, description, version and download-filename)";
		return;
	}

	// checking compatibility
	if (!isSupported(m_supported))
	{
		qWarning() << "[Add-on] Error! Add-on" << m_sAddonId << "is not supported!";
		return;
	}

	m_sDownloadSize = fileSizeToString(m_fDownloadSize);

	if (m_eType == TEXTURE)
	{
		m_AllTextures = map.value("textures").toString().split(",").toSet().toList();
		// a texture must have "textures"
		if (m_AllTextures.isEmpty())
		{
			qWarning() << "[Add-on] Error! Texture" << m_sAddonId
				   << "does not have the field \"textures\"!";
			return;
		}
		for (int i=0; i < m_AllTextures.size(); i++) {
			m_AllTextures[i] = StelFileMgr::getUserDir() % "/textures/" % m_AllTextures[i];
		}
	}

	if (map.contains("authors"))
	{
		foreach (const QVariant& author, map.value("authors").toList())
		{
			QVariantMap authors = author.toMap();
			Authors a;
			a.name = authors.value("name").toString();
			a.email = authors.value("email").toString();
			a.url = authors.value("url").toString();
			m_authors.append(a);
		}
	}

	m_bIsValid = true;
}

AddOn::~AddOn()
{
}

bool AddOn::isSupported(QStringList supported)
{
	if (supported.isEmpty()) {
		return true;
	}

	QString current = StelUtils::getApplicationVersion();
	current.truncate(current.lastIndexOf("."));

	return supported.contains(current);
}

QString AddOn::getZipPath()
{
	return StelApp::getInstance().getStelAddOnMgr().getAddOnDir() % m_sDownloadFilename;
}

QString AddOn::getStatusString() {
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
	if (string == "landscape")
	{
		return LANDSCAPE;
	}
	else if (string == "language_stellarium")
	{
		return LANG_STELLARIUM;
	}
	else if (string == "language_skyculture")
	{
		return LANG_SKYCULTURE;
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
	else
	{
		return INVALID;
	}
}

QString AddOn::typeToDisplayRole(Type type) {
	if (type == LANDSCAPE)
	{
		return "Landscape";
	}
	else if (type == LANG_STELLARIUM)
	{
		return "Stellarium";
	}
	else if (type == LANG_SKYCULTURE)
	{
		return "Sky Culture";
	}
	else if (type == PLUGIN_CATALOG)
	{
		return "Plugin";
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
		return "Star";
	}
	else if (type == TEXTURE)
	{
		return "Texture";
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
