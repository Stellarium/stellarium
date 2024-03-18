/*
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef STELLOCATIONMGR_P_HPP
#define STELLOCATIONMGR_P_HPP

//! @file
//! StelLocationMgr private implementation details
//! Just contains GPS lookup helpers for now

#include "StelLocationMgr.hpp"
#include <QDebug>

// Abstract dummy class. Must be available even for non-GPS builds.
class GPSLookupHelper : public QObject
{
	Q_OBJECT
protected:
	GPSLookupHelper(QObject* parent) : QObject(parent)
	{
	}
public:
	//! True if the helper is ready to be used (query() can be called)
	virtual bool isReady() = 0;
	//! Performs a GPS query.
	//! Either the queryFinished() or the queryError() signal
	//! will be called after this call (or within it)
public slots:
	virtual void query() = 0;
	//! Activate a series of continuous queries. Those will call queryFinished()
	//! every interval milliseconds or queryError().
	//! The handler for queryError() should call setPeriodicQuery(0) to stop it.
	virtual void setPeriodicQuery(int interval) = 0;
signals:
	//! Emitted when the query finished successfully
	//! @param loc The location
	void queryFinished(const StelLocation& loc);
	//! Emitted when the query is aborted with an error message
	void queryError(const QString& msg);
};

#ifdef ENABLE_GPS
#ifdef ENABLE_LIBGPS
#include <libgpsmm.h>
#include <QTimer>

class LibGPSLookupHelper : public GPSLookupHelper
{
	Q_OBJECT
public:
	LibGPSLookupHelper(QObject * parent);
	~LibGPSLookupHelper() override;

	bool isReady() override;
public slots:
	void query() override;
	void setPeriodicQuery(int interval) override;
private:
	bool ready;
	gpsmm* gps_rec;
	QTimer timer;
};
#endif //ENABLE_LIBGPS

#include <QNmeaPositionInfoSource>
#include <QSerialPort>
#include <QSerialPortInfo>
class NMEALookupHelper : public GPSLookupHelper
{
	Q_OBJECT
public:
	NMEALookupHelper(QObject* parent);
	~NMEALookupHelper() override;
	bool isReady() override
	{
		//if (nmea) qDebug() << "NMEALookupHelper::isReady(): Last Error was:" << nmea->error();
		return nmea && nmea->device();
	}
public slots:
	void query() override;
	void setPeriodicQuery(int interval) override;
private slots:
	void nmeaUpdated(const QGeoPositionInfo &update);
	void nmeaError(QGeoPositionInfoSource::Error error);
	#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	void nmeaTimeout();
	#endif
private:
	QSerialPort* serial;
	QNmeaPositionInfoSource* nmea;
};

#endif //ENABLE_GPS

#endif // STELLOCATIONMGR_P_HPP
