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



#ifndef DATABASEWORKER_HPP
#define DATABASEWORKER_HPP

#include <QObject>
#include "ADS-B.hpp"
#include "DBCredentials.hpp"
#include "DatabaseDataSource.hpp"

//! @class DatabaseWorker
//! This class handles the database communication for the DatabaseDataSource.
//! It is run asynchronously in its own thread. Communication is handled via
//! signals and slots.
class DatabaseWorker : public QObject
{
    Q_OBJECT
public:
    //! Create a new DatabaseWorker
    explicit DatabaseWorker(QObject *parent = 0);

signals:
    //! Returns the result of a getFlightList query
    void flightList(QList<FlightID> ids);

    //! Returns the result of a getFlight query
    void flight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country);

    //! Returns whether the connection was established or not
    void connected(bool connected, QString error);

    //! Returns whether the connection was disconnected or not
    void disconnected(bool disconnected);

    //! Emitted, when the worker is shut down
    void finished();

public slots:
    //! Open a connection to the specified database
    void connectDB(DBCredentials creds);

    //! Disconnect the database
    void disconnectDB();

    //! Get a list of flights that are relevant for the current time and time
    //! rate
    //! @param jd the current time
    //! @param speed the current time rate
    void getFlightList(double jd, double speed);

    //! Query database for flight with id and return values newer than startTime
    //! @param id the ID (hex address) of the flight
    //! @param startTime time from which on to query. if < 0, query everything
    void getFlight(QString modeS, QString callsign, double startTime);

    //! Writes a flight to the db
    void dumpFlight(FlightP f);

    //! Shut down this worker
    void stop();

private:
    bool dbConnected;

};

#endif // DATABASEWORKER_HPP
