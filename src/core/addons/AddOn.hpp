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
	AddOn(const QVariantMap &map);
	virtual ~AddOn();

private:
	typedef struct
	{
		QString name;
		QString email;
		QString url;
	} Authors;

	QString m_sType;
	QString m_sInstallID;
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

	bool m_bLoaded;
};

#endif // _ADDON_HPP_
