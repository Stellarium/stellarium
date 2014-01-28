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
#include "Planes.hpp"

const int typePos = 0;
const int msgTypePos = 1;
const int sessionIdPos = 2;
const int aircraftIdPos = 3;
const int hexIdentPos = 4;
const int flightIdPos = 5;
const int dateMsgGenPos = 6;
const int timeMsgGenPos = 7;
const int dateMsgLoggedPos = 8;
const int timeMsgLoggedPos = 9;
const int callsignPos = 10;
const int altitudePos = 11;
const int groundSpeedPos = 12;
const int trackPos = 13;
const int latPos = 14;
const int lonPos = 15;
const int verticalRatePos = 16;
const int squawkPos = 17;
const int alertPos = 18;
const int emergencyPos = 19;
const int spiPos = 20;
const int onGroundPos = 21;

//#define BS_PARSE_DEBUG_OUTPUTS

#if 0
// Shorter waiting time for debugging
const qint64 MAX_TIME_NO_UPDATE = 1000 * 60 * .5; // .5 min
const double MAX_JD_DIFF = .5 / (24.0 * 60); // .5 min
#else
const qint64 MAX_TIME_NO_UPDATE = 1000 * 60 * 10; // 10 min
const double MAX_JD_DIFF = 10.0 / (24.0 * 60); // 10 min
#endif


BSDataSource::BSDataSource() : FlightDataSource(10)
{
    qRegisterMetaType<FlightP>();
    useDatabase = false;
    useSocket = false;
    this->connect(&socket, SIGNAL(readyRead()), SLOT(readData()));
    this->connect(&socket, SIGNAL(connected()), SLOT(connected()));
    this->connect(&socket, SIGNAL(disconnected()), SLOT(disconnected()));
    this->connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(error()));
    isSocketConnected = false;
    isDbConnected = false;
    dbWorker = NULL;
    dbWorkerThread = NULL;
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
    if (useDatabase) {
        emit sigGetFlightList(jd, rate);
    }
    // remove old flights from socket data
    int size = flights.size();
    QHash<int, FlightP>::iterator i = flights.begin();
    while (i != flights.end()) {
        if ((rate > 0) && (i.value()->size() > 0) && ((jd - i.value()->getTimeEnd()) > 600 * rate) && currTime - i.value()->getLastUpdateTime().toMSecsSinceEpoch() > MAX_TIME_NO_UPDATE) { // older than 10 minutes
            if (dumpOldFlights) {
                qDebug() << "Dumping " << i.value()->getNameI18n();
                emit dumpFlight(i.value());
            }
            i = flights.erase(i);
        } else if (((!i.value()->isTimeInRange(jd) && fabs(jd - i.value()->getTimeStart()) > 600 * rate) || i.value()->size() == 0) && currTime - i.value()->getLastUpdateTime().toMSecsSinceEpoch() > MAX_TIME_NO_UPDATE) {
            // If we are far in the past, remove flights in the future that haven't gotten any updates in a while.
            // When we jump to that time, we can then query them from the db.
            if (i.value()->size() > 0 && dumpOldFlights) {
                emit dumpFlight(i.value());
            }
            i = flights.erase(i);
        } else {
            ++i;
        }
    }
    if (size - flights.size() > 0) {
        qDebug() << "deleted " << size - flights.size() << " of " << size << " old flights from live data";
    }
    // remove old flights from db flights
    size = dbFlights.size();
    QHash<QString, FlightP>::iterator j = dbFlights.begin();
    while (j != dbFlights.end()) {
        if (!j.value()->isTimeInRange(jd) && (jd - j.value()->getTimeEnd() > MAX_JD_DIFF || j.value()->getTimeStart() - jd > MAX_JD_DIFF)) {
            j = dbFlights.erase(j);
        } else {
            ++j;
        }
    }
    if (size - dbFlights.size() > 0) {
        qDebug() << "deleted " << size - dbFlights.size() << " of " << size << " old flights from db flights";
    }
}

void BSDataSource::init()
{

}

void BSDataSource::deinit()
{
    disconnectSocket();
    if (dumpOldFlights) {
        qDebug() << "Writing all remaining unsaved data to database...";
        foreach (FlightP f, flights.values()) {
            // Application exits before this is even executed, so we have to do it synchronously instead
            //emit dumpFlight(f);
            f->writeToDb();
        }
        qDebug() << "Done.";
    }
    relevantFlights.clear();
    flights.clear();
    dbFlights.clear();
    emit disconnectDB();
    emit stopDB();
    isDbConnected = false;
    isSocketConnected = false;
}

void BSDataSource::connectClicked(QString host, quint16 port, DBCredentials creds)
{
    if (useSocket && !isSocketConnected) {
        qDebug() << "Connecting socket";
        socket.connectToHost(host, port);
    }
    if (useDatabase && !isDbConnected) {
        qDebug() << "Connecting database";
        dbWorker = new DatabaseWorker();
        dbWorkerThread = new QThread();
        dbWorker->moveToThread(dbWorkerThread);

        this->connect(dbWorker, SIGNAL(connected(bool, QString)), SLOT(dbConnected(bool, QString)));
        this->connect(dbWorker, SIGNAL(disconnected(bool)), SLOT(dbDisconnected(bool)));
        dbWorker->connect(this, SIGNAL(dumpFlight(FlightP)), SLOT(dumpFlight(FlightP)));
        this->connect(dbWorker, SIGNAL(flight(QList<ADSBFrame>,QString,QString,QString,QString)),
                      SLOT(addFlight(QList<ADSBFrame>,QString,QString,QString,QString)));
        this->connect(dbWorker, SIGNAL(flightList(QList<FlightID>)), SLOT(setFlightList(QList<FlightID>)));

        dbWorker->connect(this, SIGNAL(stopDB()), SLOT(stop()));
        dbWorker->connect(this, SIGNAL(connectDB(DBCredentials)), SLOT(connectDB(DBCredentials)));
        dbWorker->connect(this, SIGNAL(disconnectDB()), SLOT(disconnectDB()));
        dbWorker->connect(this, SIGNAL(sigGetFlight(QString,QString,double)), SLOT(getFlight(QString,QString,double)));
        dbWorker->connect(this, SIGNAL(sigGetFlightList(double,double)), SLOT(getFlightList(double, double)));

        dbWorkerThread->connect(dbWorker, SIGNAL(finished()), SLOT(quit()));
        dbWorkerThread->connect(dbWorker, SIGNAL(finished()), SLOT(deleteLater()));
        dbWorker->connect(dbWorker, SIGNAL(finished()), SLOT(deleteLater()));

        dbWorkerThread->start();
        emit connectDB(creds);
    }
}

void BSDataSource::disconnectSocket()
{
    socket.disconnectFromHost();
}

void BSDataSource::disconnectDatabase()
{
    emit disconnectDB();
    emit stopDB();
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
    do {
        splitPos = currentReadBuffer.indexOf('\n', startPos);
        if (splitPos >= 0) {
            parseMsg(currentReadBuffer, startPos, splitPos);
            startPos = splitPos + 1; // keep pos of last find for next search
        }
    } while (splitPos >= 0);
    readBuffer.clear();
    readBuffer = currentReadBuffer.mid(startPos);
}

void BSDataSource::error()
{
    qDebug() << socket.errorString();
    emit bsStatus(socket.errorString());
}

void BSDataSource::connected()
{
    isSocketConnected = true;
    emit bsStatus("Connected");
}

void BSDataSource::disconnected()
{
    isSocketConnected = false;
    emit bsStatus("Disconnected");
}

void BSDataSource::dbConnected(bool connected, QString error)
{
    isDbConnected = connected;
    if (connected) {
        emit dbStatus("Connected");
    } else {
        emit dbStatus(QString("Connection failed: ") + error);
    }
}

void BSDataSource::dbDisconnected(bool disconnected)
{
    isDbConnected = !disconnected;
    if (disconnected) {
        emit dbStatus("Disconnected");
    } else {
        emit dbStatus("Disconnect failed");
    }
}

void BSDataSource::addFlight(QList<ADSBFrame> data, QString modeS, QString modeSHex, QString callsign, QString country)
{
    //qDebug() << "got flight" << data.size() << modeS << modeSHex << callsign << country;
    if (data.size() == 0) {
        return;
    }
    QString key = modeS + callsign;
    if (dbFlights.contains(key)) {
        dbFlights.value(key)->appendData(data);
    } else {
        dbFlights.insert(key, FlightP(new Flight(data, modeS, modeSHex, callsign, country)));
    }
}

void BSDataSource::setFlightList(QList<FlightID> ids)
{
    qDebug() << "Got flight list with " << ids.size() << " entries";
    foreach(FlightID id, ids) {
        foreach(FlightP f, flights.values()) {
            if (f->getIntAddress() + f->getCallsign() == id.key) {
                // Skip flights that we have live data for
                return;
            }
        }
        if (dbFlights.contains(id.key)) {
            //qDebug() << "requesting " << id.key << " from date " << flights.value(id.key)->getTimeEnd();
            emit sigGetFlight(id.mode_s, id.callsign, dbFlights.value(id.key)->getTimeEnd());
        } else {
            qDebug() << "requesting new flight " << id.key << " from date " << -1;
            emit sigGetFlight(id.mode_s, id.callsign, -1);
        }
    }
}

void BSDataSource::parseMsg(QByteArray &buf, int start, int end)
{
    static QList<ADSBFrame> emptyFrameList;
    static QString n_a = "N/A";
    QList<QByteArray> line = buf.mid(start, end - start).split(',');
    if (line.size() < 2) {
        if (start != end) { // don't be quite so verbose
            qDebug() << "invalid line start " << start << " end " << end << " " << buf.mid(start, end - start);
        }
        return;
    }
#ifdef BS_PARSE_DEBUG_OUTPUTS
    qDebug() << "start " << start << " end " << end << " " << buf.mid(start, end - start);
#endif
    bool ok = false;
    if (line.size() <= flightIdPos) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
        qDebug() << "malformatted line";
#endif
        return;
    }
    int flightId = line.at(flightIdPos).toInt(&ok);
    if (!ok) {
        qDebug() << "Failed to parse flightID: " << line;
        return;
    }
    if (line.at(typePos) == "AIR") {
        // NEW AIRCRAFT MESSAGE
#ifdef BS_PARSE_DEBUG_OUTPUTS
        qDebug() << "AIR msg";
#endif
        if (line.size() != 10) {
            qDebug() << "invalid msg length";
            return;
        }
    } else if (line.at(typePos) == "ID") {
        // ID set or changed MESSAGE
#ifdef BS_PARSE_DEBUG_OUTPUTS
        qDebug() << "ID msg";
#endif
        if (line.size() != 11) {
            qDebug() << "invalid msg length";
            return;
        }
        if (flights.contains(flightId)) {
            return;
        } else {
            if (line.size() <= callsignPos || line.at(callsignPos).isEmpty() || line.at(hexIdentPos).isEmpty()) {
                qDebug() << "parse error, string empty";
                return;
            }
            QString callsign = QString(line.at(callsignPos));
            QString hexid = QString(line.at(hexIdentPos));
            QString id = QString::number(hexid.toInt(&ok, 16));
            flights.insert(flightId, FlightP(new Flight(emptyFrameList, id, hexid, callsign, n_a)));
        }
    } else if (line.at(typePos) == "MSG") {
        // MSG message
#ifdef BS_PARSE_DEBUG_OUTPUTS
        qDebug() << "MSG msg";
#endif
        if (line.size() != 22) {
            qDebug() << "invalid msg length";
            return;
        }
        int msgType = line.at(msgTypePos).toInt();
        if (msgType != IDMessage && !flights.contains(flightId)) {
            // Wait for ID before collecting data
            return;
        }
        QDateTime t = QDateTime::fromString(QString("%1 %2").arg(QString(line.at(dateMsgGenPos)), QString(line.at(timeMsgGenPos))), "yyyy/MM/dd hh:mm:ss.zzz");
        t.setTimeSpec(Qt::LocalTime);
        double jdate = StelUtils::qDateTimeToJd(t.toUTC());
        if (msgType == IDMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "IDMessage";
#endif
            // IDMessage: Callsign
            if (flights.contains(flightId)) {
                return;
            } else {
                if (line.at(callsignPos).isEmpty() || line.at(hexIdentPos).isEmpty()) {
                    qDebug() << "parse error, string empty";
                    return;
                }
                QString callsign = QString(line.at(callsignPos));
                QString hexid = QString(line.at(hexIdentPos));
                QString id = QString::number(hexid.toInt(&ok, 16));
                flights.insert(flightId, FlightP(new Flight(emptyFrameList, id, hexid, callsign, n_a)));
            }

        } else if (msgType == SurfacePositionMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "SurfacePos";
#endif
            // SurfacePositionMessage: Altitude, GroundSpeed, Track, Lat, Long
            flights.value(flightId)->appendSurfacePos(jdate, line.at(altitudePos).toDouble(),
                                                      line.at(groundSpeedPos).toDouble(),
                                                      line.at(trackPos).toDouble(),
                                                      line.at(latPos).toDouble(),
                                                      line.at(lonPos).toDouble());
        } else if (msgType == AirbornePositionMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "AirbornePos";
#endif
            // AirbornePositionMessage: Altitude, Lat, Long, Alert, Emergency, SPI
            flights.value(flightId)->appendAirbornePos(jdate, line.at(altitudePos).toDouble(),
                                                       line.at(latPos).toDouble(),
                                                       line.at(lonPos).toDouble(),
                                                       line.at(onGroundPos).toInt() != 0);
        } else if (msgType == AirborneVelocityMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "AirborneVel";
#endif
            // AirborneVelocityMessage: GroundSpeed, Track, VerticalRate
            flights.value(flightId)->appendAirborneVelocity(jdate, line.at(groundSpeedPos).toDouble(),
                                                            line.at(trackPos).toDouble(),
                                                            line.at(verticalRatePos).toDouble());
        } else if (msgType == SurveillanceAltMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "SurveilAlt";
#endif
            // SurveillanceAltMessage: Altitude, Alert, SPI

        } else if (msgType == SurveillanceIDMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "SurveilId";
#endif
            // SurveillanceIDMessage: Altitude, Squawk, Alert, Emergency, SPI

        } else if (msgType == AirToAirMessage) {
#ifdef BS_PARSE_DEBUG_OUTPUTS
            qDebug() << "AirToAir";
#endif
            // AirToAirMessage: Altitude

        } else if (msgType == AllCallReply) {
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

void BSDataSource::setDumpOldFlights(bool value)
{
    dumpOldFlights = value;
}

bool BSDataSource::isSocketEnabled() const
{
    return useSocket;
}

void BSDataSource::setSocketEnabled(bool value)
{
    useSocket = value;
}

