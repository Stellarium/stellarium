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

#ifndef BSDATASOURCE_HPP
#define BSDATASOURCE_HPP


#include "FlightDataSource.hpp"
#include "DatabaseWorker.hpp"

#include <QtNetwork>


//! @class BSDataSource
//! This class provides access to the data port of BaseStation
//! so it can be used as a source for flight data.
//! The captured data can optionally be written to a database.
class BSDataSource : public FlightDataSource
{
    Q_OBJECT
public:

    //! @enum MsgMsgType
    //! Encodes the types of message a line startin with "MSG" is
    enum MsgMsgType
    {
        NA = 0,
        IDMessage = 1,
        SurfacePositionMessage = 2,
        AirbornePositionMessage = 3,
        AirborneVelocityMessage = 4,
        SurveillanceAltMessage = 5,
        SurveillanceIDMessage = 6,
        AirToAirMessage = 7,
        AllCallReply = 8
    };

    //! Constructor
    BSDataSource();

    //! Implemented from FlightDataSource
    QList<FlightP> *getRelevantFlights();

    //! Implemented from FlightDataSource
    //! Removes old flights.
    void updateRelevantFlights(double jd, double rate);

    //! Prepare this datasource for use
    void init();

    //! Cleanup
    void deinit();

    //! Control whether old flights will be saved to the database
    //! before they are deleted.
    bool isDatabaseEnabled() const
    {
        return useDatabase;
    }

    bool isSocketEnabled() const;

    bool isDumpOldFlightsEnabled() const;

public slots:
    void connectClicked(QString host, quint16 port, DBCredentials creds);
    void disconnectSocket();
    void disconnectDatabase();
    void readData();
    void setDatabaseEnabled(bool enabled)
    {
        useDatabase = enabled;
    }
    void setSocketEnabled(bool value);
    void setDumpOldFlights(bool value);

signals:
    void bsStatus(QString status);
    void dbStatus(QString status);
    void dumpFlight(FlightP f);
    void connectDB(DBCredentials creds);
    void disconnectDB();
    void sigGetFlightList(double jd, double rate);
    void sigGetFlight(QString modeS, QString callsign, double startTime);
    void stopDB();

private slots:
    void error();
    void connected();
    void disconnected();
    void dbConnected(bool connected, QString error);
    void dbDisconnected(bool disconnected);
    void addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country);

    //! Receive a list of relevant flights from the database worker
    //! @param ids a list of IDs of the form modeS + callsign
    void setFlightList(QList<FlightID> ids);


private:
    void parseMsg(QByteArray &buf, int start, int end);

    QList<FlightP> relevantFlights;
    QHash<int, FlightP> flights;
    QHash<QString, FlightP> dbFlights;

    QTcpSocket socket;
    QByteArray readBuffer;
    bool useDatabase;
    bool useSocket;
    bool dumpOldFlights;
    bool isDbConnected;
    bool isSocketConnected;

    QThread *dbWorkerThread;
    DatabaseWorker *dbWorker;
};

#endif // BSDATASOURCE_HPP
