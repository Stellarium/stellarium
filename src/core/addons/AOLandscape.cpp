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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>

#include "AOLandscape.hpp"

AOLandscape::AOLandscape(StelAddOnDAO* pStelAddOnDAO, QString thumbnailDir)
	: m_sThumbnailDir(thumbnailDir)
	, m_pNetworkReply(NULL)
	, m_pStelAddOnDAO(pStelAddOnDAO)
	, m_pLandscapeMgr(GETSTELMODULE(LandscapeMgr))
	, m_sLandscapeInstallDir(StelFileMgr::getUserDir() % "/landscapes/")
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sLandscapeInstallDir);
}

AOLandscape::~AOLandscape()
{
	delete m_pNetworkReply;
	m_pNetworkReply = NULL;
}

QStringList AOLandscape::checkInstalledAddOns() const
{
	return m_pLandscapeMgr->getUserLandscapeIDs();
}

bool AOLandscape::installFromFile(const QString& idInstall,
				  const QString& downloadFilepath) const
{
	Q_UNUSED(idInstall);

	// TODO: the method LandscapeMgr::installLandscapeFromArchive must be removed
	//       and this operation have to be done here.
	//       the LANDSCAPEID must be the same as idInstall (from db)
	QString landscapeId = m_pLandscapeMgr->installLandscapeFromArchive(downloadFilepath);
	if (landscapeId.isEmpty())
	{
		return false;
	}
	return true;
}

bool AOLandscape::uninstallAddOn(const QString& idInstall) const
{
	if(!m_pLandscapeMgr->removeLandscape(idInstall))
	{
		return false;
	}
	return true;
}

void AOLandscape::downloadThumbnails()
{
	m_thumbnails = m_pStelAddOnDAO->getThumbnails();
	if (m_thumbnails.isEmpty())
	{
		return;
	}

	QHashIterator<QString, QString> i(m_thumbnails); // <id_install, url>
	while (i.hasNext()) {
	    i.next();
	    if (!QFile(m_sThumbnailDir % i.key() % ".jpg").exists())
	    {
		    m_thumbnailQueue.append(i.value());
	    }
	}
	downloadNextThumbnail();
}

void AOLandscape::downloadNextThumbnail()
{
	if (m_thumbnailQueue.isEmpty()) {
	    return;
	}

	QUrl url(m_thumbnailQueue.first());
	m_pNetworkReply = StelApp::getInstance().getNetworkAccessManager()->get(QNetworkRequest(url));
	connect(m_pNetworkReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void AOLandscape::downloadFinished()
{
	if (m_thumbnailQueue.isEmpty() || m_pNetworkReply == NULL) {
	    return;
	}

	if (m_pNetworkReply->error() == QNetworkReply::NoError) {
	    QPixmap pixmap;
	    if (pixmap.loadFromData(m_pNetworkReply->readAll())) {
		    pixmap.save(m_sThumbnailDir % m_thumbnails.key(m_thumbnailQueue.first()), "jpg");
	    }
	}

	m_pNetworkReply->deleteLater();
	m_pNetworkReply = NULL;
	m_thumbnailQueue.removeFirst();
	downloadNextThumbnail();
}
