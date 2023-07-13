/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
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

#include "OMMDownload.hpp"

OMMDownload::OMMDownload() :
	m_sim(sim_t::SIM_REMOTE),
	m_ptim(new QTimer(this)),
	m_nam(new QNetworkAccessManager(this))
{
	connect(m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(slotDownloadComplete(QNetworkReply*)));
}

OMMDownload::~OMMDownload()
{}

OMMDownload::OMMDownload(sim_t s) :
	m_sim(s),
	m_ptim(new QTimer(this))
{}

void OMMDownload::execute(void)
{
	if (m_sim == sim_t::SIM_REMOTE) {
		executeRemote();
	}
	else {
		// Simulators.
		switch(m_sim) {
			default:
				executeSim();
				break;
		}
	}
}

void OMMDownload::executeRemote(void)
{
	if (!m_req_list.isEmpty()) {
		m_curr_req = m_req_list.takeFirst();
		if(!m_curr_req.isNull()) {
			m_nam->get(*(m_curr_req.data()));
		}
	}
	else {
		emit executeComplete();
	}
}

void OMMDownload::slotDownloadComplete(QNetworkReply* p_reply) 
{
	emit fileDownloadComplete(p_reply);
	executeRemote();
}

void OMMDownload::executeSim(void)
{
	if (m_req_list.isEmpty()) {
		//qDebug() << "Emitting 'executeComplete' signal";
		emit executeComplete();
	}
	else {
		m_curr_req = m_req_list.takeFirst();
		m_ptim->setInterval(20);
		connect(m_ptim, &QTimer::timeout, this, &OMMDownload::slotSimulateEnd);
		m_ptim->start();
	}
}

static char testdata[] = "TESTDATA";

void OMMDownload::slotSimulateEnd(void)
{
	m_ptim->stop();
	char* p_test = testdata;
	QNetworkReply* p = reinterpret_cast<QNetworkReply*>(p_test);
	emit fileDownloadComplete(p);
	executeSim();
}


#if 0
void OMMDownload::handleDownloadedData(QNetworkReply* p_reply)
{
	QByteArray data = p_reply->readAll();
	qDebug() << "URL: " << p_reply->url().toString(QUrl::RemoveUserInfo);
	QString path;

	//path.append(m_pluginDataDir.dirName());
	//path.append(QDir::separator());
	path.append(m_current_key);
	qDebug() << path;

	QFile * tmpFile = new QFile(path, this);
	if (tmpFile->exists()) {
		tmpFile->remove();
	}
	if (tmpFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
		tmpFile->write(data);
		tmpFile->close();
	}
}
#endif

