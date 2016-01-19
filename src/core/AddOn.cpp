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

AddOn::AddOn(const QString addOnId, const QVariantMap& map)
	: m_iAddOnId(addOnId)
	, m_eType(INVALID)
	, m_bIsValid(false)
	, m_eStatus(NotInstalled)
{
	m_sType = map.value("type").toString();
	m_eType = typeStringToEnum(m_sType);
	m_sTypeDisplayRole = typeToDisplayRole(m_eType);
	if (m_eType == INVALID)
	{
		qWarning() << "Add-On Catalog : Error! Add-on" << m_iAddOnId
			   << "does not have a valid type!";
		return;
	}

	m_sTitle = map.value("title").toString();
	m_sDescription = map.value("description").toString();
	m_dVersion = map.value("version").toDate();
	m_sFirstStel = map.value("first-stel").toString();
	m_sLastStel = map.value("last-stel").toString();
	m_sLicense = map.value("license").toString();
	m_sLicenseURL = map.value("license-url").toString();
	m_sDownloadURL = map.value("download-url").toString();
	m_sDownloadFilename = map.value("download-filename").toString();
	m_sDownloadSize = map.value("download-size").toString();
	m_sChecksum = map.value("checksum").toString();
	m_sThumbnail = map.value("thumbnail").toString();

	// specific fields in 'installed_addons.json'
	m_InstalledFiles = map.value("installed-files").toStringList();
	m_eStatus = (AddOn::Status) map.value("status").toInt();

	// early returns if the mandatory fields are not present
	if (m_sTitle.isEmpty() || m_sChecksum.isEmpty()
		|| m_sDownloadURL.isEmpty() || m_sDownloadFilename.isEmpty()
		|| m_sDownloadSize.isEmpty())
	{
		qWarning() << "Add-On Catalog : Error! Add-on" << m_iAddOnId
			   << "does not have all the required fields!";
		return;
	}

	if (m_eType == TEXTURE)
	{
		m_AllTextures = map.value("textures").toString().split(",").toSet().toList();
		// a texture must have "textures"
		if (m_AllTextures.isEmpty())
		{
			qWarning() << "Add-On Catalog : Error! Texture" << m_iAddOnId
				   << "does not have the field \"textures\"!";
			return;
		}
		for (int i=0; i < m_AllTextures.size(); i++) {
			m_AllTextures[i] = StelFileMgr::getUserDir() % "/textures/" % m_AllTextures[i];
		}
	}

	// checking compatibility
	if (!isCompatible(m_sFirstStel, m_sLastStel))
	{
		return;
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

bool AddOn::isCompatible(QString first, QString last)
{
	if (first.isEmpty() && last.isEmpty()) {
		return true;
	}

	QStringList c = StelUtils::getApplicationVersion().split(".");
	QStringList f = first.split(".");
	QStringList l = last.split(".");

	if (c.size() < 3 || f.size() < 3 || l.size() < 3) {
		return false; // invalid version
	}

	int currentVersion = QString(c.at(0) % "00" % c.at(1) % "0" % c.at(2)).toInt();
	int firstVersion = QString(f.at(0) % "00" % f.at(1) % "0" % f.at(2)).toInt();
	int lastVersion = QString(l.at(0) % "00" % l.at(1) % "0" % l.at(2)).toInt();

	if (currentVersion < firstVersion || currentVersion > lastVersion)
	{
		return false; // out of bounds
	}

	return true;
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

QString AddOn::getDownloadFilepath()
{
	return StelApp::getInstance().getStelAddOnMgr().getAddOnDir() % m_sDownloadFilename;
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
