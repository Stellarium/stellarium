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

#include "BSDataSource.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "Planes.hpp"
#include "DatabaseWorker.hpp"

// Locations of fields in string
const int TYPE_POS = 0;
const int MSG_TYPE_POS = 1;
const int SESSION_ID_POS = 2;
const int AIRCRAFT_ID_POS = 3;
const int HEX_IDENT_POS = 4;
const int FLIGHT_ID_POS = 5;
const int DATE_MSG_GEN_POS = 6;
const int TIME_MSG_GEN_POS = 7;
const int DATE_MSG_LOGGED_POS = 8;
const int TIME_MSG_LOGGED_POS = 9;
const int CALLSIGN_POS = 10;
const int ALTITUDE_POS = 11;
const int GROUND_SPEED_POS = 12;
const int TRACK_POS = 13;
const int LAT_POS = 14;
const int LON_POS = 15;
const int VERTICAL_RATE_POS = 16;
const int SQUAWK_POS = 17;
const int ALERT_POS = 18;
const int EMERGENCY_POS = 19;
const int SPI_POS = 20;
const int ON_GROUND_POS = 21;

//#define BS_PARSE_DEBUG_OUTPUTS

#if 0
// Shorter waiting time for debugging
const qint64 MAX_TIME_NO_UPDATE = 1000 * 60 * .5; // .5 min
const double MAX_JD_DIFF = .5 / (24.0 * 60); // .5 min
#else
const qint64 MAX_TIME_NO_UPDATE = 1000 * 60 * 10; //!< time after which to assume no more data is gonna arrive on the TCP socket for a flight (10 min)
const double MAX_JD_DIFF = 10.0 / (24.0 * 60); //!< Remove database flights from memory if they are older than this (10 min)
#endif


BSDataSource::BSDataSource() : FlightDataSource(10)
{
	qRegisterMetaType<FlightP>();
	useDatabase = false;
	useSocket = false;
	this->connect(&socket, SIGNAL(readyRead()), SLOT(readData()));
	this->connect(&socket, SIGNAL(connected()), SLOT(setSocketConnected()));
	this->connect(&socket, SIGNAL(disconnected()), SLOT(setSocketDisconnected()));
	this->connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(handleSocketError()));
	isSocketConnected = false;
	isDbConnected = false;
	connectionAttemptInProgress = false;
	reconnectOnConnectionLoss = false;
	isUserDisconnect = false;
	dbWorker = NULL;
	dbWorkerThread = NULL;
	timer = new QTimer();
	this->connect(timer, SIGNAL(timeout()), SLOT(reconnect()));
}

BSDataSource::~BSDataSource()
{
	timer->stop();
	delete timer;
}

QList<FlightP> *BSDataSource::getRelevantFlights()
{
	relevantFlights.clear();
	relevantFlights.append(flights.values());
	relevantFlights.append(dbFlights.values());
	return &relevantFlights;
}

void BSDataSource::updateRelevantFlights(double jd, double rate)
{
	qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
	if (useDatabase)
	{
		emit flightListRequested(jd, rate);
	}
	// remove old flights from socket data
	int size = flights.size();
	QHash<int, FlightP>::iterator i = flights.begin();
	while (i != flights.end())
	{
		if ((rate > 0) && (i.value()->size() > 0) && ((jd - i.value()->getTimeEnd()) > 600 * rate) && currTime - i.value()->getLastUpdateTime().toMSecsSinceEpoch() > MAX_TIME_NO_UPDATE)
		{ // older than 10 minutes
			if (dumpOldFlights)
			{
				qDebug() << "Dumping " << i.value()->getNameI18n();
				emit dumpFlightRequested(i.value());
			}
			i = flights.erase(i);
		}
		else if (((!i.value()->isTimeInRange(jd) && fabs(jd - i.value()->getTimeStart()) > 600 * rate) || i.value()->size() == 0) && currTime - i.value()->getLastUpdateTime().toMSecsSinceEpoch() > MAX_TIME_NO_UPDATE)
		{
			// If we are far in the past, remove flights in the future that haven't gotten any updates in a while.
			// When we jump to that time, we can then query them from the db.
			if (i.value()->size() > 0 && dumpOldFlights)
			{
				emit dumpFlightRequested(i.value());
			}
			i = flights.erase(i);
		}
		else
		{
			++i;
		}
	}
	if (size - flights.size() > 0)
	{
		qDebug() << "deleted " << size - flights.size() << " of " << size << " old flights from live data";
	}
	// remove old flights from db flights
	size = dbFlights.size();
	QHash<QString, FlightP>::iterator j = dbFlights.begin();
	while (j != dbFlights.end())
	{
		if (!j.value()->isTimeInRange(jd) && (jd - j.value()->getTimeEnd() > MAX_JD_DIFF || j.value()->getTimeStart() - jd > MAX_JD_DIFF))
		{
			j = dbFlights.erase(j);
		}
		else
		{
			++j;
		}
	}
	if (size - dbFlights.size() > 0)
	{
		qDebug() << "deleted " << size - dbFlights.size() << " of " << size << " old flights from db flights";
	}
}

void BSDataSource::init()
{

}

void BSDataSource::deinit()
{
	disconnectSocket();
	if (dumpOldFlights)
	{
		qDebug() << "Writing all remaining unsaved data to database...";
		foreach (FlightP f, flights.values())
		{
			// Application exits before this is even executed, so we have to do it synchronously instead
			//emit dumpFlightRequested(f);
			f->writeToDb();
		}
		qDebug() << "Done.";
	}
	relevantFlights.clear();
	flights.clear();
	dbFlights.clear();
	emit disconnectDBRequested();
	emit dbStopRequested();
	isDbConnected = false;
	isSocketConnected = false;
}

bool BSDataSource::isDatabaseEnabled() const
{
	return useDatabase;
}

void BSDataSource::connectDBBS(QString host, quint16 port, DBCredentials creds)
{
	if (useSocket && !isSocketConnected)
	{
		qDebug() << "Connecting socket";
		emit bsStatusChanged("Connecting...");
		socket.connectToHost(host, port);
	}
	if (useDatabase && !isDbConnected && !connectionAttemptInProgress)
	{
		emit dbStatusChanged("Connecting...");
		connectionAttemptInProgress = true;
		qDebug() << "Connecting database";
		dbWorker = new DatabaseWorker();
		dbWorkerThread = new QThread();
		dbWorker->moveToThread(dbWorkerThread);

		this->connect(dbWorker, SIGNAL(connected(bool,QString)), SLOT(setDBConnected(bool, QString)));
		this->connect(dbWorker, SIGNAL(disconnected(bool)), SLOT(setDBDisconnected(bool)));
		this->connect(dbWorker, SIGNAL(flightQueried(QList<ADSBFrame>,QString,QString,QString,QString)),
					  SLOT(addFlight(QList<ADSBFrame>,QString,QString,QString,QString)));
		this->connect(dbWorker, SIGNAL(flightListQueried(QList<FlightID>)), SLOT(setFlightList(QList<FlightID>)));

		dbWorker->connect(this, SIGNAL(dbStopRequested()), SLOT(stop()));
		dbWorker->connect(this, SIGNAL(connectDBRequested(DBCredentials)), SLOT(connectDB(DBCredentials)));
		dbWorker->connect(this, SIGNAL(disconnectDBRequested()), SLOT(disconnectDB()));
		dbWorker->connect(this, SIGNAL(flightRequested(QString,QString,double)), SLOT(getFlight(QString,QString,double)));
		dbWorker->connect(this, SIGNAL(flightListRequested(double,double)), SLOT(getFlightList(double, double)));
		dbWorker->connect(this, SIGNAL(dumpFlightRequested(FlightP)), SLOT(dumpFlight(FlightP)));

		dbWorkerThread->connect(dbWorker, SIGNAL(finished()), SLOT(quit()));
		dbWorkerThread->connect(dbWorker, SIGNAL(finished()), SLOT(deleteLater()));
		dbWorker->connect(dbWorker, SIGNAL(finished()), SLOT(deleteLater()));
		this->connect(dbWorkerThread, SIGNAL(destroyed(QObject*)), SLOT(setWorkerStopped()));

		dbWorkerThread->start();
		emit connectDBRequested(creds);
	}
}

void BSDataSource::disconnectSocket()
{
	isUserDisconnect = true;
	socket.disconnectFromHost();
}

void BSDataSource::disconnectDatabase()
{
	emit disconnectDBRequested();
	emit dbStopRequested();
}

void BSDataSource::readData()
{
#ifdef BS_PARSE_DEBUG_OUTPUTS
	qDebug() << "bytes available " << socket.bytesAvailable();
#endif
	QByteArray currentReadBuffer(readBuffer);
	currentReadBuffer.append(socket.readAll());
	int splitPos = 0;
	int startPos = 0;
	do
	{
		splitPos = currentReadBuffer.indexOf('\n', startPos);
		if (splitPos >= 0)
		{
			parseMsg(currentReadBuffer, startPos, splitPos);
			startPos = splitPos + 1; // keep pos of last find for next search
		}
	}
	while (splitPos >= 0);
	readBuffer.clear();
	readBuffer = currentReadBuffer.mid(startPos);
}

void BSDataSource::setDatabaseEnabled(bool enabled)
{
	useDatabase = enabled;
}

void BSDataSource::handleSocketError()
{
	qDebug() << socket.errorString();
	emit bsStatusChanged(socket.errorString());
}

void BSDataSource::setSocketConnected()
{
	isSocketConnected = true;
	timer->stop();
	emit bsStatusChanged(q_("Connected"));
}

void BSDataSource::setSocketDisconnected()
{
	isSocketConnected = false;
	if(!isUserDisconnect && reconnectOnConnectionLoss)
	{
		emit bsStatusChanged(q_("Attempting to reconnect..."));
		timer->setInterval(60 * 1000);
		timer->start();
	} else {
		emit bsStatusChanged(q_("Disconnected"));
		isUserDisconnect = false;
	}
}

void BSDataSource::setDBConnected(bool connected, QString error)
{
	isDbConnected = connected;
	if (connected)
	{
		// Allow new connection attempts
		connectionAttemptInProgress = false;
		emit dbStatusChanged(q_("Connected"));
	}
	else
	{
		// TRANSLATORS: Connecting to the database failed because: <error>
		emit dbStatusChanged(QString(q_("Connection failed: ")) + error);
	}
}

void BSDataSource::setDBDisconnected(bool disconnected)
{
	isDbConnected = !disconnected;
	if (disconnected)
	{
		emit dbStatusChanged(q_("Disconnected"));
	}
	else
	{
		emit dbStatusChanged(q_("Disconnect failed"));
	}
}

void BSDataSource::addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country)
{
	//qDebug() << "got flight" << data.size() << modeS << modeSHex << callsign << country;
	if (data.size() == 0)
	{
		return;
	}
	QString key = modeS + callsign;
	if (dbFlights.contains(key))
	{
		dbFlights.value(key)->appendData(data);
	}
	else
	{
		dbFlights.insert(key, FlightP(new Flight(data, modeS, modeSHex, callsign, country)));
	}
}

void BSDataSource::setFlightList(QList<FlightID> ids)
{
	qDebug() << "Got flight list with " << ids.size() << " entries";
	foreach(FlightID id, ids)
	{
		foreach(FlightP f, flights.values())
		{
			if (f->getIntAddress() + f->getCallsign() == id.key)
			{
				// Skip flights that we have live data for
				return;
			}
		}
		if (dbFlights.contains(id.key))
		{
			//qDebug() << "requesting " << id.key << " from date " << flights.value(id.key)->getTimeEnd();
			emit flightRequested(id.mode_s, id.callsign, dbFlights.value(id.key)->getTimeEnd());
		}
		else
		{
			qDebug() << "requesting new flight " << id.key << " from date " << -1;
			emit flightRequested(id.mode_s, id.callsign, -1);
		}
	}
}

void BSDataSource::setWorkerStopped()
{
	// wait for worker to be destroyed until we attempt to connect again
	// otherwise  bad things happen
	connectionAttemptInProgress = false;
	isDbConnected = false;
}

void BSDataSource::reconnect()
{
	Planes *planes = GETSTELMODULE(Planes);
	connectDBBS(planes->getBSHost(), planes->getBSPort(), planes->getDBCreds());
}

void BSDataSource::parseMsg(QByteArray &buf, int start, int end)
{
	static QList<ADSBFrame> emptyFrameList;
	// TRANSLATORS: No data available
	static QString n_a = q_("N/A");
	QList<QByteArray> line = buf.mid(start, end - start).split(',');
	if (line.size() < 2)
	{
		if (start != end)
		{ // don't be quite so verbose, don't warn on empty lines
			qDebug() << "invalid line start " << start << " end " << end << " " << buf.mid(start, end - start);
		}
		return;
	}
#ifdef BS_PARSE_DEBUG_OUTPUTS
	qDebug() << "start " << start << " end " << end << " " << buf.mid(start, end - start);
#endif
	bool ok = false;
	if (line.size() <= FLIGHT_ID_POS)
	{
#ifdef BS_PARSE_DEBUG_OUTPUTS
		qDebug() << "malformatted line";
#endif
		return;
	}
	int flightId = line.at(FLIGHT_ID_POS).toInt(&ok);
	if (!ok)
	{
		qDebug() << "Failed to parse flightID: " << line;
		return;
	}
	if (line.at(TYPE_POS) == "AIR")
	{
		// NEW AIRCRAFT MESSAGE
#ifdef BS_PARSE_DEBUG_OUTPUTS
		qDebug() << "AIR msg";
#endif
		if (line.size() != 10)
		{
			qDebug() << "invalid msg length";
			return;
		}
	}
	else if (line.at(TYPE_POS) == "ID")
	{
		// ID set or changed MESSAGE
#ifdef BS_PARSE_DEBUG_OUTPUTS
		qDebug() << "ID msg";
#endif
		if (line.size() != 11)
		{
			qDebug() << "invalid msg length";
			return;
		}
		if (flights.contains(flightId))
		{
			return;
		}
		else
		{
			if (line.size() <= CALLSIGN_POS || line.at(CALLSIGN_POS).isEmpty() || line.at(HEX_IDENT_POS).isEmpty())
			{
				qDebug() << "parse error, string empty";
				return;
			}
			QString callsign = QString(line.at(CALLSIGN_POS));
			QString hexid = QString(line.at(HEX_IDENT_POS));
			QString id = QString::number(hexid.toInt(&ok, 16));
			flights.insert(flightId, FlightP(new Flight(emptyFrameList, id, hexid, callsign, n_a)));
		}
	}
	else if (line.at(TYPE_POS) == QStringLiteral("MSG"))
	{
		// MSG message
#ifdef BS_PARSE_DEBUG_OUTPUTS
		qDebug() << "MSG msg";
#endif
		if (line.size() != 22)
		{
			qDebug() << "invalid msg length";
			return;
		}
		int msgType = line.at(MSG_TYPE_POS).toInt();
		if (msgType != IDMessage && !flights.contains(flightId))
		{
			// Wait for ID before collecting data
			return;
		}
		QDateTime t = QDateTime::fromString(QString(QStringLiteral("%1 %2")).arg(QString(line.at(DATE_MSG_GEN_POS)), QString(line.at(TIME_MSG_GEN_POS))), QStringLiteral("yyyy/MM/dd hh:mm:ss.zzz"));
		t.setTimeSpec(Qt::LocalTime);
		double jdate = StelUtils::qDateTimeToJd(t.toUTC());
		if (msgType == IDMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "IDMessage";
#endif
			// IDMessage: Callsign
			if (flights.contains(flightId))
			{
				return;
			}
			else
			{
				if (line.at(CALLSIGN_POS).isEmpty() || line.at(HEX_IDENT_POS).isEmpty())
				{
					qDebug() << "parse error, string empty";
					return;
				}
				QString callsign = QString(line.at(CALLSIGN_POS));
				QString hexid = QString(line.at(HEX_IDENT_POS));
				QString id = QString::number(hexid.toInt(&ok, 16));
				flights.insert(flightId, FlightP(new Flight(emptyFrameList, id, hexid, callsign, n_a)));
			}

		}
		else if (msgType == SurfacePositionMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "SurfacePos";
#endif
			// SurfacePositionMessage: Altitude, GroundSpeed, Track, Lat, Long
			flights.value(flightId)->appendSurfacePos(jdate, line.at(ALTITUDE_POS).toDouble(),
													  line.at(GROUND_SPEED_POS).toDouble(),
													  line.at(TRACK_POS).toDouble(),
													  line.at(LAT_POS).toDouble(),
													  line.at(LON_POS).toDouble());
		}
		else if (msgType == AirbornePositionMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "AirbornePos";
#endif
			// AirbornePositionMessage: Altitude, Lat, Long, Alert, Emergency, SPI
			flights.value(flightId)->appendAirbornePos(jdate, line.at(ALTITUDE_POS).toDouble(),
													   line.at(LAT_POS).toDouble(),
													   line.at(LON_POS).toDouble(),
													   line.at(ON_GROUND_POS).toInt() != 0);
		}
		else if (msgType == AirborneVelocityMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "AirborneVel";
#endif
			// AirborneVelocityMessage: GroundSpeed, Track, VerticalRate
			flights.value(flightId)->appendAirborneVelocity(jdate, line.at(GROUND_SPEED_POS).toDouble(),
															line.at(TRACK_POS).toDouble(),
															line.at(VERTICAL_RATE_POS).toDouble());
		}
		else if (msgType == SurveillanceAltMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "SurveilAlt";
#endif
			// SurveillanceAltMessage: Altitude, Alert, SPI

		}
		else if (msgType == SurveillanceIDMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "SurveilId";
#endif
			// SurveillanceIDMessage: Altitude, Squawk, Alert, Emergency, SPI

		}
		else if (msgType == AirToAirMessage)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "AirToAir";
#endif
			// AirToAirMessage: Altitude

		}
		else if (msgType == AllCallReply)
		{
#ifdef BS_PARSE_DEBUG_OUTPUTS
			qDebug() << "AllCallReply";
#endif
			// AllCallReply: None at the moment (need to do a further check).

		}
	}

}

bool BSDataSource::isDumpOldFlightsEnabled() const
{
	return dumpOldFlights;
}

bool BSDataSource::isReconnectOnConnectionLossEnabled() const
{
	return reconnectOnConnectionLoss;
}

void BSDataSource::setDumpOldFlights(bool value)
{
	dumpOldFlights = value;
}

void BSDataSource::setReconnectOnConnectionLoss(bool enabled)
{
	reconnectOnConnectionLoss = enabled;
}

bool BSDataSource::isSocketEnabled() const
{
	return useSocket;
}

void BSDataSource::setSocketEnabled(bool value)
{
	useSocket = value;
}

