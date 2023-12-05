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

/*
 * When writing Unit Tests one problem stands out and that is the inability
 * of creating Mock Class Types from Q classes that derive from QObject and
 * which emit signals.
 * This is most notable for the QNetworkAccessManager that would actually make
 * network calls when Unit Tests run. This class is used to perform any network
 * access requests. It has a buit in simulator and can be configured to make
 * mock network requests and emit fake signals that can be captured in Unit Tests
 * using the QSignalSpy system.
 * This allows the use of Dependancy Injection to setup Unit Tests that are self
 * contained and make no network calls during the Unit Test run at build time.
 */

#ifndef SATELLITES_OMMDOWNLOAD_HPP
#define SATELLITES_OMMDOWNLOAD_HPP

#include <QList>
#include <QDebug>
#include <QTimer>
#include <QObject>
#include <QSharedPointer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

//! @class OMMDownload
//! Auxiliary class for the %Satellites plugin.
//! @author Andy Kirkham
//! @ingroup satellites
//! 
//! Used to abstract the QNetworkAccessManager from the
//! main classes as that particular class makes writing
//! unit tests almost impossible. This class wrapps the
//! QNAM and provides a simulatot that can be enabled
//! for unit tests to hook into using QSignalSpy without
//! any network calls being made.
class OMMDownload : public QObject
{
	Q_OBJECT

public:
	typedef QSharedPointer<QNetworkRequest> ReqShPtr;

	union Response {
		void* m_fake_reply;
		QNetworkReply* m_reply;
	};
	typedef QSharedPointer<Response> ResponseShPtr;
	
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

	OMMDownload(sim_t s);

	virtual OMMDownload& addReqShPtr(const ReqShPtr req) {
		m_req_list.append(req);
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
	void slotDownloadComplete(QNetworkReply *);

signals:
	void fileDownloadComplete(QNetworkReply*);
	void fileDownloadFailed(QNetworkReply*);
	void executeComplete(void);

private:
	//! executeSim()
	//! Used to begin a simulated run of network requests
	virtual void executeSim(void);

	//! executeRemote()
	//!	Used to begin a requested run of network requests.
	virtual void executeRemote(void);

	QNetworkAccessManager *m_nam{nullptr};

	sim_t m_sim{sim_t::SIM_REMOTE};

	//! Used to time how long the download takes.
	//! In sim mode it's used to create a delay in simulation.
	QTimer* m_ptim{nullptr};

	//! The current request in operation.
	ReqShPtr m_curr_req{};

	//! The list of requests waiting.
	QList<ReqShPtr> m_req_list{};
};

#endif
