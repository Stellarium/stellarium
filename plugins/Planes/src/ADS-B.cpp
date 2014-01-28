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

#include "ADS-B.hpp"
#include "Flight.hpp"
#include "StelUtils.hpp"


static const double MAX_TIME_OVER_END = 1000 / (24 * 60 * 60 * 1000); // ms
static const double FEET_TO_METER = 0.3048;
static const double KNOTS_TO_MPS = 0.514444444;
static const double FT_MIN_TO_MPS = 0.00508;
static const double MAX_TIME_DIFF = 5.0 / (24 * 60 * 60); // 5 sec in jd

bool ADSBData::useInterp = true;


ADSBData::ADSBData(QStringList &data)
{
    initialised = false;
    lastIndex = 0;
    lastRequestedJd = 0;
    parseData(data);

    surfPosTime = 0;
    surfPosAlt = 0;
    surfPosSpd = 0;
    surfPosTrack = 0;
    surfPosLat = 0;
    surfPosLon = 0;

    airPosTime = 0;
    airPosAlt = 0;
    airPosLat = 0;
    airPosLon = 0;
    airPosOnGround = false;

    airVelTime = 0;
    airVelSpd = 0;
    airVelTrack = 0;
    airVelRate = 0;

    lastUpdate = QDateTime::currentDateTime();
}

ADSBData::ADSBData(QList<ADSBFrame> &data, QString &modeS, QString &modeSHex, QString &callsign, QString &country)
{
    this->data.append(data);
    this->modeSAddress = modeS;
    this->modeSAddressHex = modeSHex;
    this->callsign = callsign;
    this->country = country;
    lastIndex = 0;
    lastRequestedJd = 0;
    initialised = true;

    surfPosTime = 0;
    surfPosAlt = 0;
    surfPosSpd = 0;
    surfPosTrack = 0;
    surfPosLat = 0;
    surfPosLon = 0;

    airPosTime = 0;
    airPosAlt = 0;
    airPosLat = 0;
    airPosLon = 0;
    airPosOnGround = false;

    airVelTime = 0;
    airVelSpd = 0;
    airVelTrack = 0;
    airVelRate = 0;

    lastUpdate = QDateTime::currentDateTime();
}

ADSBData::~ADSBData()
{

}

ADSBFrame const *ADSBData::getData(double jd)
{
    if (data.size() == 0 || jd < data.first().time || jd > data.last().time) {
        return NULL;
    }

    // fwd search
    if (jd >= lastRequestedJd) {
        while (lastIndex < data.size() - 1) {
            // do interpolation
            if (useInterp && jd >= data.at(lastIndex).time && jd < data.at(lastIndex + 1).time) {
                lastRequestedJd = jd;
                interpolate(lastIndex, lastIndex + 1, jd);
                return &interpFrame;
            // use closest
            } else if(!useInterp && fabs(data.at(lastIndex).time - jd) < fabs(data.at(lastIndex + 1).time - jd)) {
                lastRequestedJd = jd;
                return &data.at(lastIndex);
            } else {
                lastIndex++;
            }
        }
        // didn't find
        if (fabs(data.at(lastIndex).time - jd) < MAX_TIME_OVER_END) {
            lastRequestedJd = jd;
            return &data.at(lastIndex);
        }
    } else {
        // bwd search
        while (lastIndex > 0) {
            // do interpolation
            if (useInterp && jd >= data.at(lastIndex).time && jd < data.at(lastIndex + 1).time) {
                lastRequestedJd = jd;
                interpolate(lastIndex, lastIndex + 1, jd);
                return &interpFrame;
            // use closest
            } else if (!useInterp && fabs(data.at(lastIndex).time - jd) < fabs(data.at(lastIndex - 1).time - jd)) {
                lastRequestedJd = jd;
                return &data.at(lastIndex);
            } else {
                lastIndex--;
            }
        }
        if (fabs(data.at(lastIndex).time - jd) < MAX_TIME_OVER_END) {
            lastRequestedJd = jd;
            return &data.at(lastIndex);
        }
    }
    //qDebug() << "didn't find " << lastIndex << " lastRequestedJd " << lastRequestedJd;
    lastRequestedJd = jd;
    return NULL;
}

bool ADSBData::append(QList<ADSBFrame> &newdata)
{
    if (newdata.size() == 0) {
        return false;
    }
    if (data.isEmpty()) {
        data.append(newdata);
        return true;
    }
    if (data.last().time == newdata.first().time) {
        data.pop_back();
        data.append(newdata);
        return true;
    }
    if (newdata.first().time > data.last().time) {
        data.append(newdata);
        return true;
    }
    return false;
}

bool ADSBData::appendFrame(ADSBFrame &frame)
{
    if (data.isEmpty()) {
        data.append(frame);
        return true;
    }
    if (data.last().time == frame.time) {
        return false;
    }
    if (frame.time > data.last().time) {
        data.append(frame);
        return true;
    }
    return false;
}

void ADSBData::setInfo(QString &callsign, QString &country)
{
    this->callsign = callsign;
    this->country = country;
}

void ADSBData::appendSurfacePos(const double &jdate, const double altitude, const double &groundSpeed, const double &track, const double &lat, const double &lon)
{
    surfPosTime = jdate;
    surfPosAlt = altitude;
    surfPosSpd = groundSpeed;
    surfPosTrack = track;
    surfPosLat = lat;
    surfPosLon = lon;

    checkFrame();
}

void ADSBData::appendAirbornePos(const double &jdate, const double &altitude, const double &lat, const double &lon, const double &onGround)
{
    airPosTime = jdate;
    airPosAlt = altitude;
    airPosLat = lat;
    airPosLon = lon;
    airPosOnGround = onGround;

    checkFrame();
}

void ADSBData::appendAirborneVelocity(const double &jdate, const double &groundSpeed, const double &track, const double &verticalRate)
{
    airVelTime = jdate;
    airVelSpd = groundSpeed;
    airVelTrack = track;
    airVelRate = verticalRate;

    checkFrame();
}

bool ADSBData::timeInRange(double jd) const
{
    if (data.size() == 0) {
        return false;
    }
    return (jd >= data.first().time && jd <= data.last().time);
}

double ADSBData::getTimeStart() const
{
    if (data.isEmpty()) {
        return 0;
    }
    return data.first().time;
}

double ADSBData::getTimeEnd() const
{
    if (data.isEmpty()) {
        return 0;
    }
   return data.last().time;
}

QString ADSBData::ADSBFrameToString(const ADSBFrame *f)
{
    QString s;
    QTextStream ss(&s);
    ss << "time " << f->time;
    ss << " lat " << f->latitude << " lon " << f->longitude << " alt " << f->altitude;
    ss << " spd " << f->ground_speed << " gnd track " << f->ground_track;
    ss << " vert rate " << f->vertical_rate << " on ground " << (f->onGround ? "true" : "false");
    return s;
}

void ADSBData::resetSearchAlgorithm()
{
    // Fwd search until first match
    lastIndex = 0;
    lastRequestedJd = 0;
}

bool ADSBData::parseLine(QString line, ADSBFrame &f, QString &modeS, QString &modeSHex, QString &callsign, QString &country)
{
    //line.replace("\"", "");
    QStringList seg = line.split("\",\"");
    if (seg.size() != 17) {
        qDebug() << "Invalid line encountered";
        return false;
    }
    modeS = seg.at(2);
    modeSHex = seg.at(3);
    // Attempt to find callsign, even if it's missing in first entry
    callsign = seg.at(4);
    if (modeS.isEmpty() || callsign.isEmpty()) {
        // Cannot distinguish flight events without having both of these
        return false;
    }
    // Attempt to find country, even if it's missing in first entry
    country = seg.at(5);
    QDateTime t = QDateTime::fromString(QString("%1 %2").arg(seg.at(0).right(seg.at(0).size() - 1), seg.at(1)), "yyyy/MM/dd hh:mm:ss.zzz");
    t.setTimeSpec(Qt::LocalTime);
    f.time = StelUtils::qDateTimeToJd(t.toUTC());
    f.onGround = (seg.at(6) == "1");
    f.altitude_feet = seg.at(7).toDouble();
    f.altitude = f.altitude_feet * FEET_TO_METER;
    f.latitude = seg.at(9).toDouble() * M_PI / 180.0;
    f.longitude = seg.at(10).toDouble() * M_PI / 180.0;
    f.vertical_rate_ft_min = seg.at(11).toDouble();
    f.vertical_rate = f.vertical_rate_ft_min * FT_MIN_TO_MPS;
    f.ground_speed_knots = seg.at(13).toDouble();
    f.ground_speed = f.ground_speed_knots * KNOTS_TO_MPS;
    f.ground_track = seg.at(14).toDouble();
    f.ecefPos = Flight::calcECEFPosition(f.latitude, f.longitude, f.altitude);
    //qDebug() << ADSBFrameToString(&f);
    return true;
}

void ADSBData::postProcessFrame(ADSBFrame *frame)
{
    frame->altitude = frame->altitude_feet * FEET_TO_METER;
    frame->vertical_rate = frame->vertical_rate_ft_min * FT_MIN_TO_MPS;
    frame->ground_speed = frame->ground_speed_knots * KNOTS_TO_MPS;
    frame->ecefPos = Flight::calcECEFPosition(frame->latitude, frame->longitude, frame->altitude);
}

void ADSBData::parseData(QStringList &data)
{
    //qDebug() << "Begin parsing data";
    if (data.size() >= 1) {
        QString line(data.at(0));
        QStringList seg = line.replace("\"", "").split(",");
        modeSAddress = seg.at(2);
        modeSAddressHex = seg.at(3);
        callsign = seg.at(4);
        country = seg.at(5);
        initialised = true;
    }
    foreach (QString line, data) {
        line.replace("\"", "");
        QStringList seg = line.split(",");
        if (seg.size() != 17) {
            qDebug() << "Invalid line encountered";
            continue;
        }
        if (callsign.isEmpty()) {
            // Attempt to find callsign, even if it's missing in first entry
            callsign = seg.at(4);
        }
        if (country.isEmpty()) {
            // Attempt to find country, even if it's missing in first entry
            country = seg.at(5);
        }
        ADSBFrame f;
        QDateTime t = QDateTime::fromString(QString("%1 %2").arg(seg.at(0), seg.at(1)), "yyyy/MM/dd hh:mm:ss.zzz");
        t.setTimeSpec(Qt::LocalTime);
        f.time = StelUtils::qDateTimeToJd(t.toUTC());
        f.onGround = (seg.at(6) == "1");
        f.altitude_feet = seg.at(7).toDouble();
        f.altitude = f.altitude_feet * FEET_TO_METER;
        f.latitude = seg.at(9).toDouble() * M_PI / 180.0;
        f.longitude = seg.at(10).toDouble() * M_PI / 180.0;
        f.vertical_rate_ft_min = seg.at(11).toDouble();
        f.vertical_rate = f.vertical_rate_ft_min * FT_MIN_TO_MPS;
        f.ground_speed_knots = seg.at(13).toDouble();
        f.ground_speed = f.ground_speed_knots * KNOTS_TO_MPS;
        f.ground_track = seg.at(14).toDouble();
        f.ecefPos = Flight::calcECEFPosition(f.latitude, f.longitude, f.altitude);
        //qDebug() << ADSBFrameToString(&f);
        this->data.push_back(f);
    }
    qDebug() << "End parsing data for flight" << modeSAddressHex << " (Callsign " << callsign << "): " << this->data.size() << " data data points.";
}

void ADSBData::interpolate(int start, int end, double time)
{
    if (start < 0 || start >= data.size()) {
        qDebug() << "Index start out of range: " << start;
        return;
    }
    if (end < 0 || end >= data.size()) {
        qDebug() << "Index end out of range: " << end;
        return;
    }
    if (end <= start) {
        qDebug() << "End must be bigger than start. start: " << start << " end: " << end;
        return;
    }
    double pos = (time - data.at(start).time) / (data.at(end).time - data.at(start).time);
    interpFrame.time = time;
    interpFrame.onGround = data.at(start).onGround;
    interpFrame.altitude_feet = linearInterp(data.at(start).altitude_feet, data.at(end).altitude_feet, pos);
    interpFrame.altitude = interpFrame.altitude_feet * FEET_TO_METER;
    interpFrame.latitude = linearInterp(data.at(start).latitude, data.at(end).latitude, pos);
    interpFrame.longitude = linearInterp(data.at(start).longitude, data.at(end).longitude, pos);
    interpFrame.vertical_rate_ft_min = linearInterp(data.at(start).vertical_rate_ft_min, data.at(end).vertical_rate_ft_min, pos);
    interpFrame.vertical_rate = interpFrame.vertical_rate_ft_min * FT_MIN_TO_MPS;
    interpFrame.ground_speed_knots = linearInterp(data.at(start).ground_speed_knots, data.at(end).ground_speed_knots, pos);
    interpFrame.ground_speed = interpFrame.ground_speed_knots * KNOTS_TO_MPS;
    interpFrame.ground_track = linearInterp(data.at(start).ground_track, data.at(end).ground_track, pos);
    if (interpFrame.ground_track < 0) {
        interpFrame.ground_track += 360;
    }
}

void ADSBData::checkFrame()
{
    if (airVelTime == 0) {
        // missing data
        return;
    }
    if (airPosTime > 0) {
        if (fabs(airPosTime - airVelTime) < MAX_TIME_DIFF) {
            ADSBFrame f;
            f.time = (airVelTime > airPosTime) ? airVelTime : airPosTime;
            f.onGround = airPosOnGround;
            f.altitude_feet = airPosAlt;
            f.latitude = airPosLat * M_PI / 180.0;
            f.longitude = airPosLon * M_PI / 180.0;
            f.vertical_rate_ft_min = airVelRate;
            f.ground_speed_knots = airVelSpd;
            f.ground_track = airVelTrack;
            postProcessFrame(&f);
            appendFrame(f);
            lastUpdate = QDateTime::currentDateTime();
            if (airVelTime > airPosTime) {
                airVelTime = 0;
            } else {
                airPosTime = 0;
            }
        }
    }
    /*
    if (surfPosTime > 0) {
        if (fabs(surfPosTimes - airVelTime) < MAX_TIME_DIFF) {
            ADSBFrame f;
            f.time = (airVelTime > surfPosTime) ? airVelTime : surfPosTime;
            airVelTime = 0;
            surfPosTime = 0;
        }
    }
    */
}

double ADSBData::linearInterp(double a, double b, double pos)
{
    return (a + pos * (b - a));
}
