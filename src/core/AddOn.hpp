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

#ifndef _ADDON_HPP_
#define _ADDON_HPP_

#include <QDateTime>
#include <QObject>

class AddOn : public QObject
{
	Q_OBJECT
public:
	AddOn(const QString addonId, const QVariantMap& map);
	virtual ~AddOn();

	//! @enum Type
	//! Add-on type
	enum Type
	{
		ADDON_CATALOG,
		LANDSCAPE,
		LANG_STELLARIUM,
		LANG_SKYCULTURE,
		PLUGIN_CATALOG,
		SCRIPT,
		SKY_CULTURE,
		STAR_CATALOG,
		TEXTURE,
		INVALID
	};

	//! @enum AddOnStatus
	//! Used for keeping track of the download/update status
	enum Status
	{
		NotInstalled,
		PartiallyInstalled,
		FullyInstalled,
		Installing,
		Restart,
		Corrupted,
		InvalidFormat,
		InvalidDestination,
		UnableToWrite,
		UnableToRead,
		UnableToRemove,
		PartiallyRemoved,
		DownloadFailed
	};

	typedef struct
	{
		QString name;
		QString email;
		QString url;
	} Authors;

	bool isValid() { return m_bIsValid; }

	QString getAddOnId() { return m_sAddonId; }
	QString getTitle() { return m_sTitle; }
	Type getType() { return m_eType; }
	QString getTypeString() { return m_sType; }
	QString getTypeDisplayRole() { return m_sTypeDisplayRole; }
	int getVersion() { return m_iVersion; }
	QDate getDate() { return m_dDate; }
	QList<Authors> getAuthors() { return m_authors; }
	QString getDescription() { return m_sDescription; }
	QString getLicenseName() { return m_sLicense; }
	QString getLicenseURL() { return m_sLicenseURL; }
	QString getDownloadFilename() { return m_sDownloadFilename; }
	float getDownloadSize() { return m_fDownloadSize; }
	QString getDownloadSizeString() { return m_sDownloadSize; }
	QString getDownloadURL() { return m_sDownloadURL; }
	QString getChecksum() { return m_sChecksum; }

	//! Get path to the zip archive.
	//! @return path
	QString getZipPath() { return m_sZipPath; }

	Status getStatus() { return m_eStatus; }
	QString getStatusString();
	void setStatus(Status status) { m_eStatus = status; }
	QStringList getAllTextures() { return m_AllTextures; }
	void setInstalledFiles(QStringList installedFiles) { m_InstalledFiles = installedFiles; }
	QStringList getInstalledFiles() { return m_InstalledFiles; }

private:
	QString m_sAddonId;
	Type m_eType;
	QString m_sType;
	QString m_sTypeDisplayRole;
	QString m_sTitle;
	QString m_sDescription;
	int m_iVersion;
	QDate m_dDate;
	QStringList m_supported;
	QString m_sLanguage;
	QString m_sLastStel;
	QString m_sLicense;
	QString m_sLicenseURL;
	QString m_sDownloadURL;
	QString m_sDownloadFilename;
	float m_fDownloadSize;
	QString m_sDownloadSize;
	QString m_sChecksum;
	QList<Authors> m_authors;
	QStringList m_AllTextures;
	QStringList m_InstalledFiles;
	QString m_sZipPath;

	bool m_bIsValid;
	Status m_eStatus;

	Type typeStringToEnum(QString string);
	QString typeToDisplayRole(Type type);
	QString fileSizeToString(float bytes);
};

#endif // _ADDON_HPP_
