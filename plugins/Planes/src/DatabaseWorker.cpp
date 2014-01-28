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



#include "DatabaseWorker.hpp"
#include <QtSql>
#include <QSqlError>

DatabaseWorker::DatabaseWorker(QObject *parent) :
    QObject(parent)
{
    dbConnected = false;
}

void DatabaseWorker::connectDB(DBCredentials creds)
{
    qDebug() << "DatabaseWorker::connectDB()";
    QSqlDatabase oldDb = QSqlDatabase::database();
    if (oldDb.isValid()) {
        oldDb.close();
    }
    QSqlDatabase db = QSqlDatabase::addDatabase(creds.type);
    /*
    if (!db.isValid()) {
        emit connected(false);
        qDebug() << creds.type << " is not available or could not be loaded.";
        qDebug() << db.lastError().text();
        qDebug() << "Available drivers: " << db.drivers();
        return;
    }
    */
    db.setHostName(creds.host);
    db.setPort(creds.port);
    db.setUserName(creds.user);
    db.setPassword(creds.pass);
    db.setDatabaseName(creds.name);
    dbConnected = db.open();
    emit connected(dbConnected, db.lastError().text());
    qDebug() << "Database connected " << dbConnected << " last error: " << db.lastError();
    if (creds.type == "QSQLITE") {
        QStringList tables = db.tables();
        if (!tables.contains("flights") || !tables.contains("adsb")) {
            qDebug() << "creating tables";
            QSqlQuery qry;
            qry.prepare("CREATE TABLE IF NOT EXISTS `flights` ("
                        "  `mode_s` int(10) NOT NULL,"
                        "  `callsign` varchar(45) NOT NULL,"
                        "  `mode_s_hex` varchar(45) DEFAULT NULL,"
                        "  `country` varchar(45) DEFAULT NULL,"
                        " CONSTRAINT `pk` PRIMARY KEY (`mode_s`, `callsign`)"
                        ");");
            qry.exec();
            if (qry.lastError().type() != QSqlError::NoError) {
                qDebug() << qry.lastError().text();
            } else {
                qDebug() << "success query 1";
            }
            qry.prepare("CREATE TABLE IF NOT EXISTS `adsb` ("
                        "  `jdate` double NOT NULL,"
                        "  `flights_mode_s` int(10) NOT NULL,"
                        "  `flights_callsign` varchar(45) NOT NULL,"
                        "  `on_ground` tinyint(3) DEFAULT NULL,"
                        "  `altitude_feet` int(11) DEFAULT NULL,"
                        "  `latitude` double DEFAULT NULL,"
                        "  `longitude` double DEFAULT NULL,"
                        "  `vertical_rate_ft_min` int(11) DEFAULT NULL,"
                        "  `ground_speed_knots` double DEFAULT NULL,"
                        "  `ground_track` double DEFAULT NULL,"
                        "  CONSTRAINT `mode_s` FOREIGN KEY (`flights_mode_s`) REFERENCES `flights` (`mode_s`) ON DELETE NO ACTION ON UPDATE NO ACTION,"
                        "  CONSTRAINT `callsign` FOREIGN KEY (`flights_callsign`) REFERENCES `flights` (`callsign`) ON DELETE NO ACTION ON UPDATE NO ACTION"
                        ");");
            qry.exec();
            if (qry.lastError().type() != QSqlError::NoError) {
                qDebug() << qry.lastError().text();
            } else {
                qDebug() << "success query 2";
            }
        }
    }
}

void DatabaseWorker::disconnectDB()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid()) {
        db.close();
    }
    dbConnected = false;
    emit disconnected(true);
}

void DatabaseWorker::getFlightList(double jd, double speed)
{
    if (!dbConnected) {
        return;
    }
    // Query here
    QSqlQuery qry;
    QList<FlightID> ids;
    qry.prepare("SELECT DISTINCT adsb.flights_mode_s, adsb.flights_callsign FROM adsb WHERE adsb.jdate >= :minjdate AND adsb.jdate <= :maxjdate");
    if (speed > 0) {
        qry.bindValue(":minjdate", jd);
        qry.bindValue(":maxjdate", jd + 10 * speed);
    } else {
        qry.bindValue(":maxjdate", jd);
        qry.bindValue(":minjdate", jd + 10 * speed);
    }
    qry.exec();
    if (qry.lastError().type() != QSqlError::NoError) {
        qDebug() << qry.lastError().text();
    }
    while (qry.next()) {
        FlightID f;
        f.mode_s = qry.value(0).toString();
        f.callsign = qry.value(1).toString();
        f.key = f.mode_s + f.callsign;
        ids.append(f);
    }
    if (ids.size() > 0) {
        emit flightList(ids);
    }
    // emit flightlist
}

void DatabaseWorker::getFlight(QString modeS, QString callsign, double startTime)
{
    if (!dbConnected) {
        return;
    }
    //qDebug() << "Querying " << modeS << " " << callsign;
    // Query flight
    QSqlQuery qry;
    qry.prepare("SELECT flights.mode_s_hex, flights.country FROM flights WHERE flights.mode_s = :mode_s AND flights.callsign = :callsign");
    qry.bindValue(":mode_s", modeS);
    qry.bindValue(":callsign", callsign);
    qry.exec();
    if (qry.lastError().type() != QSqlError::NoError) {
        qDebug() << qry.lastError().text();
    }
    if (qry.next()) {
		QString country;
		QString modeSHex;
        QList<ADSBFrame> data;
        modeSHex = qry.value(0).toString();
        country = qry.value(1).toString();
        if (startTime > 0) {
            //						0					1						2						3					4
            qry.prepare("SELECT adsb.jdate, adsb.on_ground, adsb.altitude_feet, adsb.latitude, adsb.longitude, "
                        //		5									6								7
                        "adsb.vertical_rate_ft_min, adsb.ground_speed_knots, adsb.ground_track FROM adsb "
                        "WHERE adsb.flights_mode_s = :mode_s AND adsb.flights_callsign = :callsign "
                        "AND adsb.jdate > :start_time ORDER BY adsb.jdate ASC");
            qry.bindValue(":start_time", startTime);
        } else {
            //						0					1						2						3					4
            qry.prepare("SELECT adsb.jdate, adsb.on_ground, adsb.altitude_feet, adsb.latitude, adsb.longitude, "
                        //		5									6								7
                        "adsb.vertical_rate_ft_min, adsb.ground_speed_knots, adsb.ground_track FROM adsb "
                        "WHERE adsb.flights_mode_s = :mode_s AND adsb.flights_callsign = :callsign ORDER BY adsb.jdate ASC");
        }
        qry.bindValue(":mode_s", modeS);
        qry.bindValue(":callsign", callsign);
        qry.exec();
        if (qry.lastError().type() != QSqlError::NoError) {
            qDebug() << qry.lastError().text();
        }
        while(qry.next()) {
            ADSBFrame f;
            f.time = qry.value(0).toDouble();
            f.onGround = qry.value(1).toBool();
            f.altitude_feet = qry.value(2).toDouble();
            f.latitude = qry.value(3).toDouble();
            f.longitude = qry.value(4).toDouble();
            f.vertical_rate_ft_min = qry.value(5).toDouble();
            f.ground_speed_knots = qry.value(6).toDouble();
            f.ground_track = qry.value(7).toDouble();
            ADSBData::postProcessFrame(&f);
            data.append(f);
        }
        // Sometimes we only get the same frame every time due to rounding errors with >
        if (data.size() > 1) {
            emit flight(data, modeS, modeSHex, callsign, country);
        }
    }
}

void DatabaseWorker::dumpFlight(FlightP f)
{
   f->writeToDb();
}

void DatabaseWorker::stop()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid()) {
        db.close();
    }
    dbConnected = false;
    emit finished();
}
