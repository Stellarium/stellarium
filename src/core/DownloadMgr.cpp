/*
 * Stellarium
 * Copyright (C) 2016 Marcos Cardinot
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

#include <QDebug>
#include <QDir>
#include <QTimer>

#include "DownloadMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"

DownloadMgr::DownloadMgr(StelAddOnMgr* mgr)
	: m_pMgr(mgr)
	, m_networkRequest(QNetworkRequest())
	, m_networkReply(NULL)
	, m_downloadingAddOn(NULL)
	, m_currentDownloadFile(NULL)
{
	// setting the network request
	m_networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, false);
	m_networkRequest.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	m_networkRequest.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	// set user agent as "Stellarium/$version$ ($platform$)"
	m_networkRequest.setRawHeader("User-Agent", QString("Stellarium/%1 (%2)")
				      .arg(StelUtils::getApplicationVersion())
				      .arg(StelUtils::getOperatingSystemInfo())
				      .toLatin1());

	connect(this, SIGNAL(updateTableViews()), m_pMgr, SIGNAL(updateTableViews()));

	// every 5 min, check if it's time to update the catalog of add-ons
	QTimer* timer = new QTimer(this);
	timer->setInterval(300000);
	timer->setInterval(300);
	connect(timer, SIGNAL(timeout()), this, SLOT(checkInterval()));
	timer->start();
}

DownloadMgr::~DownloadMgr()
{
}

void DownloadMgr::download(AddOn *addon)
{
	m_downloadQueue.append(addon);
	downloadNextAddOn();
}

bool DownloadMgr::isDownloading(AddOn *addon)
{
	return m_downloadQueue.contains(addon);
}

void DownloadMgr::downloadNextAddOn()
{
	if (m_downloadingAddOn != NULL || m_downloadQueue.isEmpty())
	{
		return;
	}

	m_downloadingAddOn = m_downloadQueue.first();
	m_currentDownloadFile = new QFile(m_downloadingAddOn->getZipPath());
	if (!m_currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "[Add-on] cannot open a writable file:"
			   << QDir::toNativeSeparators(m_downloadingAddOn->getZipPath());
		cancelAllDownloads();
		return;
	}

	m_networkRequest.setUrl(m_downloadingAddOn->getDownloadURL());
	m_networkReply = StelApp::getInstance().getNetworkAccessManager()->get(m_networkRequest);
	m_networkReply->setReadBufferSize(1024*1024*2);
	connect(m_networkReply, SIGNAL(readyRead()), this, SLOT(newDataAvailable()));
	connect(m_networkReply, SIGNAL(finished()), this, SLOT(downloadFinished()));

	m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, m_downloadingAddOn->getDownloadSize());
	m_progressBar->setFormat(QString("%1: %p%").arg(m_downloadingAddOn->getDownloadFilename()));
}

void DownloadMgr::newDataAvailable()
{
	float size = m_networkReply->bytesAvailable();
	m_progressBar->setValue((float) m_progressBar->getValue() + (float) size);
	m_currentDownloadFile->write(m_networkReply->read(size));
}

void DownloadMgr::downloadFinished()
{
	if (m_networkReply->error() == QNetworkReply::NoError)
	{
		const QVariant& redirect = m_networkReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
		if (!redirect.isNull())
		{
			// We got a redirection, we need to follow
			m_currentDownloadFile->reset();
			m_networkReply->deleteLater();
			QNetworkRequest req(redirect.toUrl());
			req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
			req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
			m_networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
			m_networkReply->setReadBufferSize(1024*1024*2);
			connect(m_networkReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
			connect(m_networkReply, SIGNAL(finished()), this, SLOT(downloadAddOnFinished()));
			return;
		}
		AddOn* addon = m_downloadingAddOn;
		finishCurrentDownload();
		m_pMgr->installAddOn(addon, false);
	}
	else
	{
		qWarning() << "[Add-on] unable to download file!" << m_networkReply->url();
		qWarning() << "[Add-on] download error:" << m_networkReply->errorString();
		m_downloadingAddOn->setStatus(AddOn::DownloadFailed);
		finishCurrentDownload();
	}

	downloadNextAddOn();
}

void DownloadMgr::finishCurrentDownload()
{
	if (m_currentDownloadFile)
	{
		m_currentDownloadFile->close();
		m_currentDownloadFile->deleteLater();
		m_currentDownloadFile = NULL;
	}

	if (m_networkReply)
	{
		m_networkReply->deleteLater();
		m_networkReply = NULL;
	}

	if (m_progressBar)
	{
		m_progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(m_progressBar);
		m_progressBar = NULL;
	}

	m_downloadQueue.removeOne(m_downloadingAddOn);
	m_downloadingAddOn = NULL;
}

void DownloadMgr::cancelAllDownloads()
{
	qDebug() << "[Add-on] canceling all downloads!";
	finishCurrentDownload();
	m_downloadQueue.clear();
	emit(updateTableViews());
}

void DownloadMgr::checkInterval()
{
	QDateTime lastUpdate = m_pMgr->getLastUpdate();
	QDateTime nextUpdate;
	switch (m_pMgr->getUpdateFrequency()) {
		case StelAddOnMgr::NEVER:
		case StelAddOnMgr::ON_STARTUP:
			return;
		case StelAddOnMgr::EVERY_DAY:
			nextUpdate = lastUpdate.addDays(1);
			break;
		case StelAddOnMgr::EVERY_THREE_DAYS:
			nextUpdate = lastUpdate.addDays(3);
			break;
		case StelAddOnMgr::EVERY_WEEK:
			nextUpdate = lastUpdate.addDays(7);
			break;
		default:
			qWarning() << "[Add-on] Error! Invalid update frequency!";
			return;
	}

	// check if it's time to update the catalog of add-ons
	if (QDateTime::currentDateTime() >= nextUpdate)
	{
		updateCatalog();
	}
}

void DownloadMgr::updateCatalog()
{
	// TO DO
}

/*
void AddOnDialog::downloadFinished(QNetworkReply* reply)
{
	if (m_progressBar)
	{
		m_progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(m_progressBar);
		m_progressBar = NULL;
	}

	QByteArray result(reply->readAll());
	if (reply->error() != QNetworkReply::NoError || result.isEmpty())
	{
		qWarning() << "[Add-on] unable to download file!" << reply->url();
		qWarning() << "[Add-on] download error:" << reply->errorString();
		ui->txtLastUpdate->setText(q_("Database update failed!"));
		return;
	}

	QFile file(StelApp::getInstance().getStelAddOnMgr().getAddonJsonPath());
	file.remove(); // overwrite
	if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		qWarning() << "[Add-on] unable to open a temporary file!";
		ui->txtLastUpdate->setText(q_("Database update failed!"));
		return;
	}

	file.write(result);
	file.close();

	StelApp::getInstance().getStelAddOnMgr().reloadCatalogues();
	StelApp::getInstance().getStelAddOnMgr().setLastUpdate(QDateTime::currentDateTime());
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	populateTables();
	ui->btnUpdate->setEnabled(true);
}
*/
