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
	AddOn(const QString addOnId, const QVariantMap& map);
	virtual ~AddOn();

	//! @enum Type
	//! Add-on type
	enum Type
	{
		Landscape,
		Language_Stellarium,
		Language_SkyCulture,
		Plugin_Catalog,
		Script,
		Sky_Culture,
		Star_Catalog,
		Texture,
		INVALID
	};

	//! @enum categories
	enum Category {
		CATALOG,
		LANDSCAPE,
		LANGUAGEPACK,
		SCRIPT,
		STARLORE,
		TEXTURE,
		INVALID_CATEGORY
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

	QString getAddOnId() { return m_iAddOnId; }
	QString getTitle() { return m_sTitle; }
	Type getType() { return m_eType; }
	QString getTypeString();
	Category getCategory() { return m_eCategory; }
	QDate getVersion() { return m_dVersion; }
	QList<Authors> getAuthors() { return m_authors; }
	QString getDescription() { return m_sDescription; }
	QString getLicenseName() { return m_sLicense; }
	QString getLicenseURL() { return m_sLicenseURL; }
	QString getDownloadFilename() { return m_sDownloadFilename; }
	QString getDownloadFilepath();
	float getDownloadSize() { return m_sDownloadSize.toFloat() * 1000; }
	QString getDownloadURL() { return m_sDownloadURL; }
	QString getThumbnail() { return m_sThumbnail; }
	QString getChecksum() { return m_sChecksum; }

	Status getStatus() { return m_eStatus; }
	QString getStatusString();
	void setStatus(Status status) { m_eStatus = status; }
	QStringList getAllTextures() { return m_AllTextures; }
	void setInstalledFiles(QStringList installedFiles) { m_InstalledFiles = installedFiles; }
	QStringList getInstalledFiles() { return m_InstalledFiles; }

private:
	QString m_iAddOnId;
	Type m_eType;
	QString m_sTitle;
	QString m_sDescription;
	QDate m_dVersion;
	QString m_sFirstStel;
	QString m_sLastStel;
	QString m_sLicense;
	QString m_sLicenseURL;
	QString m_sDownloadURL;
	QString m_sDownloadFilename;
	QString m_sDownloadSize;
	QString m_sChecksum;
	QString m_sThumbnail;
	QList<Authors> m_authors;
	QStringList m_AllTextures;
	QStringList m_InstalledFiles;

	bool m_bIsValid;
	Category m_eCategory;
	Status m_eStatus;

	bool isCompatible(QString first, QString last);
	Category getCategoryFromType(Type type);
	Type fromStringToType(QString string);
};

#endif // _ADDON_HPP_
