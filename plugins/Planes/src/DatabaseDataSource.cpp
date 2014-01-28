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
    emit sigGetFlightList(jd, rate);
}

void DatabaseDataSource::init()
{
    if (initialised) {
        return;
    }
    initialised = true;
    worker = new DatabaseWorker();
    workerThread = new QThread();
    worker->moveToThread(workerThread);

    this->connect(worker, SIGNAL(connected(bool)), SLOT(connectResult(bool)));
    this->connect(worker, SIGNAL(disconnected(bool)), SLOT(disconnectResult(bool)));
    this->connect(worker, SIGNAL(flight(QList<ADSBFrame>,QString,QString,QString,QString)),
                  SLOT(addFlight(QList<ADSBFrame>,QString,QString,QString,QString)));
    this->connect(worker, SIGNAL(flightList(QList<FlightID>)), SLOT(setFlightList(QList<FlightID>)));

    worker->connect(this, SIGNAL(stopWorker()), SLOT(stop()));
    worker->connect(this, SIGNAL(sigConnect(DBCredentials)), SLOT(connectDB(DBCredentials)));
    worker->connect(this, SIGNAL(sigDisconnect()), SLOT(disconnectDB()));
    worker->connect(this, SIGNAL(sigGetFlight(QString,QString,double)), SLOT(getFlight(QString,QString, double)));
    worker->connect(this, SIGNAL(sigGetFlightList(double,double)), SLOT(getFlightList(double, double)));

    workerThread->connect(worker, SIGNAL(finished()), SLOT(quit()));
    workerThread->connect(worker, SIGNAL(finished()), SLOT(deleteLater()));
    worker->connect(worker, SIGNAL(finished()), SLOT(deleteLater()));

    workerThread->start();

    lastUpdateJD = 0;
    lastUpdateRate = 0;
}

void DatabaseDataSource::deinit()
{
    makeDisconnect();
    flights.clear();
    emit stopWorker();
    initialised = false;
}

void DatabaseDataSource::makeConnection(DBCredentials creds)
{
    qDebug() << "DatabaseDataSource::makeConnection()";
    emit sigConnect(creds);
}

void DatabaseDataSource::makeDisconnect()
{
    emit sigDisconnect();
}

void DatabaseDataSource::addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country)
{
    qDebug() << "got flight " << modeS << " with " << data.size();
    if (data.size() == 0) {
        return;
    }
    QString key = modeS + callsign;
    if (flights.contains(key)) {
        flights.value(key)->appendData(data);
    } else {
        flights.insert(key, FlightP(new Flight(data, modeS, modeSHex, callsign, country)));
    }
    // update the relevant flights list
    updateRelevantFlights();
}

void DatabaseDataSource::setFlightList(QList<FlightID> ids)
{
    QStringList updatedIDs;
    qDebug() << "Got flight list with " << ids.size() << " entries";
    foreach (FlightID id, ids) {
        if (flights.contains(id.key)) {
            updatedIDs.append(id.key);
            //qDebug() << "requesting " << id.key << " from date " << flights.value(id.key)->getTimeEnd();
            emit sigGetFlight(id.mode_s, id.callsign, flights.value(id.key)->getTimeEnd());
        } else {
            qDebug() << "requesting new flight " << id.key << " from date " << -1;
            emit sigGetFlight(id.mode_s, id.callsign, -1);
        }
    }
    // Remove old flights
    QHash<QString, FlightP>::iterator i = flights.begin();
    while (i != flights.end()) {
        bool removed = false;
        if (!updatedIDs.contains(i.key())) {
            if (lastUpdateRate > 0) {
                if (lastUpdateJD - i.value()->getTimeEnd() > 30 * lastUpdateRate) {
                    i = flights.erase(i);
                    removed = true;
                }
            } else {
                if (i.value()->getTimeStart() - lastUpdateJD > 30 * fabs(lastUpdateRate)) {
                    i = flights.erase(i);
                    removed = true;
                }
            }
        }
        if (!removed) {
            ++i;
        }
    }
}

void DatabaseDataSource::updateRelevantFlights()
{
   relevantFlights.clear();
   relevantFlights.append(flights.values());
}

void DatabaseDataSource::connectResult(bool res)
{
   qDebug() << "Connection established " << res;
   if (res) {
       emit dbStatus("Connected");
   } else {
       emit dbStatus("Connection failed");
   }
}

void DatabaseDataSource::disconnectResult(bool res)
{
   qDebug() << "Disconnected " << res;
   if (res) {
       emit dbStatus("Disconnected");
   } else {
       emit dbStatus("Disconnect failed");
   }
}
