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
	AddOn(const qint64 addOnId, const QVariantMap &map);
	virtual ~AddOn();

	//! @enum Type
	//! Add-on type
	enum Type
	{
		Landscape,
		Language_Pack,
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
		Corrupted,
		InvalidFormat,
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

	quint64 getAddOnId() { return m_iAddOnId; }
	QString getTitle() { return m_sTitle; }
	Type getType() { return m_eType; }
	Category getCategory() { return m_eCategory; }
	QString getVersion() { return m_sVersion; }
	QList<Authors> getAuthors() { return m_authors; }
	QString getDescription() { return m_sDescription; }
	QString getLicenseName() { return m_sLicense; }
	QString getLicenseURL() { return m_sLicenseURL; }
	QString getDownloadFilename() { return m_sDownloadFilename; }
	QString getDownloadFilepath();
	quint64 getDownloadSize() { return m_sDownloadSize.toLongLong(); }
	QString getDownloadURL() { return m_sDownloadURL; }
	QString getThumbnail() { return m_sThumbnail; }
	QString getInstallId() { return m_sInstallId; }
	QString getChecksum() { return m_sChecksum; }
	QString getDate() { return m_dateTime.toString("dd MMM yyyy - hh:mm:ss"); }

	Status getStatus() { return m_eStatus; }
	QString getStatusString();
	void setStatus(Status status) { m_eStatus = status; }
	void setTextureStatus(QString name, bool installed);
	QSet<QString> getAllTextures() { return m_AllTextures; }
	QSet<QString> getInstalledTextures() { return m_InstalledTextures; }

private:
	qint64 m_iAddOnId;
	Type m_eType;
	QString m_sInstallId;
	QString m_sTitle;
	QString m_sDescription;
	QString m_sVersion;
	QString m_sFirstStel;
	QString m_sLastStel;
	QString m_sLicense;
	QString m_sLicenseURL;
	QString m_sDownloadURL;
	QString m_sDownloadFilename;
	QString m_sDownloadSize;
	QString m_sInstalledSize;
	QString m_sChecksum;
	QString m_sThumbnail;
	QList<Authors> m_authors;
	QDateTime m_dateTime;
	QSet<QString> m_AllTextures;
	QSet<QString> m_InstalledTextures;
	QHash<QString, int> m_textures; // <texture_name, installed>

	bool m_bIsValid;
	Category m_eCategory;
	Status m_eStatus;

	Category getCategoryFromType(Type type);
	Type fromStringToType(QString string);
};

#endif // _ADDON_HPP_
