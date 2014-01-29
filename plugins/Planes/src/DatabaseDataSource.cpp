/*
 * Copyright (C) 2013 Felix Zeltner
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



#include "DatabaseDataSource.hpp"
#include "DatabaseWorker.hpp"

DatabaseDataSource::DatabaseDataSource() : FlightDataSource(10), worker(NULL), workerThread(NULL)
{
	lastUpdateJD = 0;
	lastUpdateRate = 0;
	initialised = false;
}

DatabaseDataSource::~DatabaseDataSource()
{

}

void DatabaseDataSource::updateRelevantFlights(double jd, double rate)
{
	lastUpdateJD = jd;
	lastUpdateRate = rate;
	emit flightListRequested(jd, rate);
}

void DatabaseDataSource::init()
{
	if (initialised)
	{
		return;
	}
	initialised = true;
	worker = new DatabaseWorker();
	workerThread = new QThread();
	worker->moveToThread(workerThread);

	this->connect(worker, SIGNAL(connected(bool,QString)), SLOT(setConnectResult(bool)));
	this->connect(worker, SIGNAL(disconnected(bool)), SLOT(setDisconnectResult(bool)));
	this->connect(worker, SIGNAL(flightQueried(QList<ADSBFrame>,QString,QString,QString,QString)),
				  SLOT(addFlight(QList<ADSBFrame>,QString,QString,QString,QString)));
	this->connect(worker, SIGNAL(flightListQueried(QList<FlightID>)), SLOT(setFlightList(QList<FlightID>)));

	worker->connect(this, SIGNAL(workerStopRequested()), SLOT(stop()));
	worker->connect(this, SIGNAL(connectRequested(DBCredentials)), SLOT(connectDB(DBCredentials)));
	worker->connect(this, SIGNAL(disconnectRequested()), SLOT(disconnectDB()));
	worker->connect(this, SIGNAL(flightRequested(QString,QString,double)), SLOT(getFlight(QString,QString, double)));
	worker->connect(this, SIGNAL(flightListRequested(double,double)), SLOT(getFlightList(double, double)));

	workerThread->connect(worker, SIGNAL(finished()), SLOT(quit()));
	workerThread->connect(worker, SIGNAL(finished()), SLOT(deleteLater()));
	worker->connect(worker, SIGNAL(finished()), SLOT(deleteLater()));

	workerThread->start();

	lastUpdateJD = 0;
	lastUpdateRate = 0;
}

void DatabaseDataSource::deinit()
{
	disconnectDB();
	flights.clear();
	emit workerStopRequested();
	initialised = false;
}

void DatabaseDataSource::connectDB(DBCredentials creds)
{
	qDebug() << "DatabaseDataSource::makeConnection()";
	emit connectRequested(creds);
}

void DatabaseDataSource::disconnectDB()
{
	emit disconnectRequested();
}

void DatabaseDataSource::addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country)
{
	qDebug() << "got flight " << modeS << " with " << data.size();
	if (data.size() == 0)
	{
		return;
	}
	QString key = modeS + callsign;
	if (flights.contains(key))
	{
		flights.value(key)->appendData(data);
	}
	else
	{
		flights.insert(key, FlightP(new Flight(data, modeS, modeSHex, callsign, country)));
	}
	// update the relevant flights list
	updateRelevantFlights();
}

void DatabaseDataSource::setFlightList(QList<FlightID> ids)
{
	QStringList updatedIDs;
	qDebug() << "Got flight list with " << ids.size() << " entries";
	foreach (FlightID id, ids)
	{
		if (flights.contains(id.key))
		{
			updatedIDs.append(id.key);
			//qDebug() << "requesting " << id.key << " from date " << flights.value(id.key)->getTimeEnd();
			emit flightRequested(id.mode_s, id.callsign, flights.value(id.key)->getTimeEnd());
		}
		else
		{
			qDebug() << "requesting new flight " << id.key << " from date " << -1;
			emit flightRequested(id.mode_s, id.callsign, -1);
		}
	}
	// Remove old flights
	QHash<QString, FlightP>::iterator i = flights.begin();
	while (i != flights.end())
	{
		bool removed = false;
		if (!updatedIDs.contains(i.key()))
		{
			if (lastUpdateRate > 0)
			{
				if (lastUpdateJD - i.value()->getTimeEnd() > 30 * lastUpdateRate)
				{
					i = flights.erase(i);
					removed = true;
				}
			}
			else
			{
				if (i.value()->getTimeStart() - lastUpdateJD > 30 * fabs(lastUpdateRate))
				{
					i = flights.erase(i);
					removed = true;
				}
			}
		}
		if (!removed)
		{
			++i;
		}
	}
}

void DatabaseDataSource::updateRelevantFlights()
{
	relevantFlights.clear();
	relevantFlights.append(flights.values());
}

void DatabaseDataSource::setConnectResult(bool res)
{
	qDebug() << "Connection established " << res;
	if (res)
	{
		emit statusChanged("Connected");
	}
	else
	{
		emit statusChanged("Connection failed");
	}
}

void DatabaseDataSource::setDisconnectResult(bool res)
{
	qDebug() << "Disconnected " << res;
	if (res)
	{
		emit statusChanged("Disconnected");
	}
	else
	{
		emit statusChanged("Disconnect failed");
	}
}
