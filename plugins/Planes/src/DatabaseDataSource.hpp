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



#ifndef DATABASEDATASOURCE_HPP
#define DATABASEDATASOURCE_HPP

#include "FlightDataSource.hpp"
#include "DBCredentials.hpp"
#include <QtCore>

//! @struct FlightID
//! Combines mode s address and callsign into key
//! Can be passed on signal/slot connections
typedef struct flight_id_s
{
    QString mode_s;
    QString callsign;
    QString key;
} FlightID;

Q_DECLARE_METATYPE(QList<FlightID>)
Q_DECLARE_METATYPE(QList<ADSBFrame>)

class DatabaseWorker;

//! @class DatabaseDataSource
//! Unused now, functionality rolled into BSDataSource
//! @see BSDataSource
class DatabaseDataSource : public FlightDataSource
{
    Q_OBJECT
public:
    DatabaseDataSource();
    ~DatabaseDataSource();

    QList<FlightP> *getRelevantFlights()
    {
        return &relevantFlights;
    }

    void updateRelevantFlights(double jd, double rate);
    void init();
    void deinit();

signals:
    void sigConnect(DBCredentials creds);
    void sigDisconnect();
    void sigGetFlightList(double jd, double rate);
    void sigGetFlight(QString modeS, QString callsign, double startTime);
    void dbStatus(QString status);
    void stopWorker();

public slots:
    void makeConnection(DBCredentials creds);
    void makeDisconnect();
    void addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country);

    //! Receive a list of relevant flights from the database worker
    //! @param ids a list of IDs of the form modeS + callsign
    void setFlightList(QList<FlightID> ids);
    void updateRelevantFlights();
    void connectResult(bool res);
    void disconnectResult(bool res);

private:
    QHash<QString, FlightP> flights;
    QList<FlightP> relevantFlights;
    DatabaseWorker *worker;
    QThread *workerThread;
    double lastUpdateJD;
    double lastUpdateRate;

    bool initialised;
};

#endif // DATABASEDATASOURCE_HPP
