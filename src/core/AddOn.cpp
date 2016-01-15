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
	m_eType = fromStringToType(map.value("type").toString());
	if (m_eType == INVALID)
	{
		qWarning() << "Add-On Catalog : Error! Add-on" << m_iAddOnId
			   << "does not have a valid type!";
		return;
	}

	m_eCategory = getCategoryFromType(m_eType);
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

	if (m_eType == Texture)
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

AddOn::Type AddOn::fromStringToType(QString string)
{
	if (string == "landscape")
	{
		return Landscape;
	}
	else if (string == "language_stellarium")
	{
		return Language_Stellarium;
	}
	else if (string == "language_skyculture")
	{
		return Language_SkyCulture;
	}
	else if (string == "plugin_catalog")
	{
		return Plugin_Catalog;
	}
	else if (string == "script")
	{
		return Script;
	}
	else if (string == "sky_culture")
	{
		return Sky_Culture;
	}
	else if (string == "star_catalog")
	{
		return Star_Catalog;
	}
	else if (string == "texture")
	{
		return Texture;
	}
	else
	{
		return INVALID;
	}
}

AddOn::Category AddOn::getCategoryFromType(Type type)
{
	switch (type) {
		case Landscape:
			return LANDSCAPE;
		case Language_Stellarium:
		case Language_SkyCulture:
			return LANGUAGEPACK;
		case Plugin_Catalog:
		case Star_Catalog:
			return CATALOG;
		case Script:
			return SCRIPT;
		case Sky_Culture:
			return STARLORE;
		case Texture:
			return TEXTURE;
		default:
			return INVALID_CATEGORY;
	}
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

QString AddOn::getTypeString() {
	switch (m_eType) {
		case Landscape:
			return "Landscape";
		case Language_Stellarium:
			return "Stellarium";
		case Language_SkyCulture:
			return "Sky Culture";
		case Plugin_Catalog:
			return "Plugin";
		case Star_Catalog:
			return "Star";
		case Script:
			return "Script";
		case Sky_Culture:
			return "Sky Culture";
		case Texture:
			return "Texture";
		default:
			return "Invalid";
	}
}

QString AddOn::getDownloadFilepath()
{
	return StelApp::getInstance().getStelAddOnMgr().getAddOnDir() % m_sDownloadFilename;
}
