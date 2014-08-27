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

#ifndef _AOLANDSCAPE_HPP_
#define _AOLANDSCAPE_HPP_

#include "LandscapeMgr.hpp"
#include "StelAddOn.hpp"

class AOLandscape : public StelAddOn
{
	Q_OBJECT
public:
	AOLandscape(StelAddOnDAO* pStelAddOnDAO, QString thumbnailDir);
	virtual ~AOLandscape();

	// check landscapes which are already installed.
	virtual QStringList checkInstalledAddOns() const;

	// install landscape from a zip file.
	virtual bool installFromFile(const QString& idInstall,
				     const QString& downloadedFilepath,
				     const QStringList& selectedFiles) const;

	// uninstall landscape
	virtual bool uninstallAddOn(const QString& idInstall) const;

	// download thumbnails
	void downloadThumbnails();

private slots:
	void downloadFinished();

private:
	QString m_sThumbnailDir;
	QNetworkReply* m_pNetworkReply;
	QHash<QString, QString> m_thumbnails;
	QStringList m_thumbnailQueue;

	StelAddOnDAO* m_pStelAddOnDAO;
	LandscapeMgr* m_pLandscapeMgr;
	const QString m_sLandscapeInstallDir;

	void downloadNextThumbnail();
};

#endif // _AOLANDSCAPE_HPP_
