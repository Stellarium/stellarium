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

	bool isValid() { return m_bIsValid; }

	quint64 getAddOnId() { return m_iAddOnId; }
	QString getTitle() { return m_sTitle; }
	Type getType() { return m_eType; }
	QString getVersion() { return m_sVersion; }

private:
	typedef struct
	{
		QString name;
		QString email;
		QString url;
	} Authors;

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

	bool m_bIsValid;

	Type fromStringToType(QString string);
};

#endif // _ADDON_HPP_
