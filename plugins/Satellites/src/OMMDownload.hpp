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

#ifndef SATELLITES_OMMDOWNLOAD_HPP
#define SATELLITES_OMMDOWNLOAD_HPP

#include <QDir>
#include <QPair>
#include <QHash>
#include <QDebug>
#include <QTimer>
#include <QString>
#include <QObject>
#include <QSharedPointer>

class OMMDownload : public QObject
{
	Q_OBJECT

public:
	enum class sim_t
	{
		SIM_REMOTE = 0,
		SIM_LOCAL_TLE,
		SIM_LOCAL_XML,
		SIM_LOCAL_JSON,
		SIM_LOCAL_INLINE
	};
	
	OMMDownload();
	virtual ~OMMDownload();

	OMMDownload(sim_t s) : m_sim(s) {}

	virtual OMMDownload & setPluginDataDir(const QDir & d) {
		m_pluginDataDir = d;
		return *this;
	}

	virtual OMMDownload& clrURIS(void) {
		m_uriMap.clear();
		return *this;
	}

	virtual OMMDownload& setURIs(const QHash<QString, QString> & m) { 
		m_uriMap = m;
		return *this;
	}

	virtual OMMDownload& addURI(const QPair<QString,QString>& pair) { 
		m_uriMap.insert(pair.first, pair.second);
		return *this;
	}

	virtual OMMDownload& setSim(sim_t b) {
		m_sim = b;
		return *this;
	}
	virtual sim_t getSim() { return m_sim; }

	virtual void execute(void);

public slots:
	void slotSimulateEnd(void);

signals:
	void fileDownloadComplete(const QString& filename);
	void executeComplete(void);

private:

	virtual void executeSim(void);

	sim_t m_sim{sim_t::SIM_REMOTE};

	//! Used to time how long the download takes.
	//! In sim mode it's used to create a delay in simulation.
	QTimer* m_ptim{nullptr};

	//! The directory to store downloads in.
	QDir m_pluginDataDir;

	//! When a download is requested the current QHash key is placed here.
	QString m_current_key;
	//! When a download is requested the current QHash val is placed here.
	QString m_current_val;

	//! m_uriMap
	//! A key value map of Internet downloads to make.
	//! The key (dst) is the filename to store locally in m_pluginDataDir
	//! The value (src) is the URI to fetch from the Internet.
	QHash<QString, QString> m_uriMap;

	//! The currently downloading file
	QHash<QString, QString>::iterator m_current_itor;
};

#endif
