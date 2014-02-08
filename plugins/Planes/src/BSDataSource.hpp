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
#include "DBCredentials.hpp"

#include <QtNetwork>


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

	//! Is a database connection used?
	bool isDatabaseEnabled() const;

	//! Is a TCP socket used?
	bool isSocketEnabled() const;

	//! Control whether old flights will be saved to the database
	//! before they are deleted.
	bool isDumpOldFlightsEnabled() const;

public slots:
	//! Prompted to connect to db and/or socket
	void connectDBBS(QString host, quint16 port, DBCredentials creds);

	//! Disconnect the socket
	void disconnectSocket();

	//! Disconnect the database
	void disconnectDatabase();

	//! Read available data from the TCP socket
	void readData();

	//! Turn on usage of database
	void setDatabaseEnabled(bool enabled);

	//! Turn on usage of TCP socket
	void setSocketEnabled(bool value);

	//! Turn on writing of old flights to database
	void setDumpOldFlights(bool value);

signals:
	//! Emitted to signal status changes of the socket to the GUI
	void bsStatusChanged(QString status);

	//! Emitted to signal status changes of the database to the GUI
	void dbStatusChanged(QString status);

	//! Emitted to request dumping a flight by the DatabaseWorker
	void dumpFlightRequested(FlightP f);

	//! Request connecting to database
	void connectDBRequested(DBCredentials creds);

	//! Request disconnecting from database
	void disconnectDBRequested();

	//! Request querying list of relevant flights by DatabaseWorker
	void flightListRequested(double jd, double rate);

	//! Request querying of flight by DatabaseWorker
	void flightRequested(QString modeS, QString callsign, double startTime);

	//! Stop using db, disconnect and exit
	void dbStopRequested();

private slots:
	//! Socket error occured
	void handleSocketError();

	//! Socket entered state "connected"
	void setSocketConnected();

	//! Socket entered state "disconnected"
	void setSocketDisconnected();

	//! Database connected
	//! @param connected true if connection was successful
	//! @param error last error message
	void setDBConnected(bool setSocketConnected, QString handleSocketError);

	//! Database disconnected
	void setDBDisconnected(bool setSocketDisconnected);

	//! Database returned result of sigGetFlight() request
	void addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country);

	//! Receive a list of relevant flights from the database worker
	//! @param ids a list of IDs of the form modeS + callsign
	void setFlightList(QList<FlightID> ids);


private:
	//! Parse a message received on the TCP socket
	//! @param buf the buffer containing the read data
	//! @param start start position of the message to parse
	//! @param end end position of the message to parse
	void parseMsg(QByteArray &buf, int start, int end);

	QList<FlightP> relevantFlights; //!< List of flights returned to FlightMgr
	QHash<int, FlightP> flights; //!< Hash of flights used to keep track of data received by socket
	QHash<QString, FlightP> dbFlights; //!< Hash of flights used to keep track of data received by database

	QTcpSocket socket; //!< TCP socket to receive data from BaseStation on
	QByteArray readBuffer; //!< Read buffer for the TCP data

	//!@{
	//! Internal config/state
	bool useDatabase;
	bool useSocket;
	bool dumpOldFlights;
	bool isDbConnected;
	bool isSocketConnected;
	//!@}

	QThread *dbWorkerThread; //!< DatabaseWorker thread to make database access asynchronous
	DatabaseWorker *dbWorker; //!< DatabaseWorker
};

#endif // BSDATASOURCE_HPP
