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

#include <QtDebug>
#include <QVariantMap>

#include "AddOn.hpp"

AddOn::AddOn(const qint64 addOnId, const QVariantMap& map)
	: m_iAddOnId(addOnId)
	, m_eType(INVALID)
	, m_bLoaded(false)
{
	m_eType = fromStringToType(map.value("type").toString());
	m_sInstallId = map.value("install-id").toString();
	m_sTitle = map.value("title").toString();
	m_sDescription = map.value("description").toString();
	m_sVersion = map.value("version").toString();
	m_sFirstStel = map.value("first-stel").toString();
	m_sLastStel = map.value("last-stel").toString();
	m_sLicense = map.value("license").toString();
	m_sLicenseURL = map.value("license-url").toString();
	m_sDownloadURL = map.value("download-url").toString();
	m_sDownloadFilename = map.value("download-filename").toString();
	m_sDownloadSize = map.value("download-size").toString();
	m_sInstalledSize = map.value("installed-size").toString();
	m_sChecksum = map.value("checksum").toString();
	m_sThumbnail = map.value("thumbnail").toString();

	if (m_eType == INVALID)
	{
		qWarning() << "Add-On Catalog : Error! Add-on" << m_iAddOnId
			   << "does not have a valid type!";
		return;
	}

	// early returns if the mandatory fields are not present
	if (m_sInstallId.isEmpty() || m_sTitle.isEmpty() || m_sDownloadURL.isEmpty()
		|| m_sDownloadFilename.isEmpty() || m_sDownloadSize.isEmpty() || m_sChecksum.isEmpty())
	{
		qWarning() << "Add-On Catalog : Error! Add-on" << m_iAddOnId
			   << "does not have all the required fields!";
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

	m_bLoaded = true;
}

AddOn::~AddOn()
{
}

AddOn::Type AddOn::fromStringToType(QString string)
{
	if (string == "landscape")
	{
		return Landscape;
	}
	else if (string == "language_pack")
	{
		return Language_Pack;
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
