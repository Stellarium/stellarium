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

#include "Flight.hpp"
#include <QDebug>
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "Planes.hpp"
#include "FlightPainter.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>


static const double __f = 3.352779E-3; 		//!< Ellipsoid earth
static const double R_EARTH = 6378135; 		//!< Radius of the earth in m

Vec3d Flight::observerPos(R_EARTH,0,0);
double Flight::ECEFtoAzAl[3][3];
int Flight::numPaths;
int Flight::numVisible;
Flight::PathColour Flight::pathColour = Flight::SolidColour;
QFont Flight::labelFont;

double Flight::maxVertRate = 50;	//!< Max vertical rate, used for path colouring
double Flight::minVertRate = -50;	//!< Min vertical rate, used for path colouring
double Flight::maxVelocity = 500;	//!< Max ground speed, used for path colouring
double Flight::minVelocity = 0;  	//!< Min ground speed, used for path colouring
double Flight::velRange = Flight::maxVelocity - Flight::minVelocity;
double Flight::maxHeight = 20000;	//!< Max height, used for path colouring
double Flight::minHeight = 0;		//!< Min height, used for path colouring
double Flight::heightRange = Flight::maxHeight - Flight::minHeight;

std::vector<float> Flight::pathVert;
std::vector<float> Flight::pathCol;

Flight::Flight() : position(0, 0, 0)
{
    data = NULL;
    currentFrame = NULL;
    inTimeRange = false;
    flightSelected = false;
}

Flight::Flight(QString filename) : position(0, 0, 0)
{
    QFile file(filename);
    QStringList adsbData;
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            adsbData.push_back(file.readLine());
        }
    }
    data = new ADSBData(adsbData);
    currentFrame = NULL;
    inTimeRange = false;
    flightSelected = false;
    file.close();
}

Flight::Flight(QStringList &data) : position(0, 0, 0)
{
    this->data = new ADSBData(data);
    currentFrame = NULL;
    inTimeRange = false;
    flightSelected = false;
}

Flight::Flight(QList<ADSBFrame> &data, QString &modeS, QString &modeSHex, QString &callsign, QString &country) : position(0, 0, 0)
{
    this->data = new ADSBData(data, modeS, modeSHex, callsign, country);
    currentFrame = NULL;
    inTimeRange = false;
    flightSelected = false;
}

Flight::~Flight()
{
    delete data;
}

void Flight::appendData(QList<ADSBFrame> &newData)
{
    data->append(newData);
}

void Flight::appendSingle(ADSBFrame &frame)
{
   data->appendFrame(frame);
}

QString Flight::getInfoString(const StelCore *core, const StelObject::InfoStringGroup &flags) const
{
    if (!data->isInitialised()) {
        return "";
    }
    QString infoStr;
    QTextStream ss(&infoStr);
    if (flags & Name) {
        ss << "<h2>" << getEnglishName() << "</h2>";
        ss << "<br/>";
    }

    if (flags & CatalogNumber) {
        if (!data->getCallsign().isEmpty()) {
            ss << "Callsign: " << data->getCallsign() << "<br/>";
        }
        ss << "Mode S Address (hex): ";
        ss << data->getHexAddress() << "<br/><br/>";
    }

    /*
    if (flags & ObjectType) {
        ss << "Type: <b>plane</b><br/>";
    }
    */
    if (flags & Extra) {
        ss << "Type: <b>plane</b><br/>";
        ss << "Country: " << data->getCountry() << "<br/>";
        if (currentFrame) {
            ss << "On ground: " << (currentFrame->onGround ? "yes" : "no") << "<br/>";
            ss << "Position: lat " << StelUtils::radToDmsStr(currentFrame->latitude)
               << ", lon " << StelUtils::radToDmsStr(currentFrame->longitude) << "<br/>";
            ss << "Altitude: " << currentFrame->altitude << " m (" << currentFrame->altitude_feet << " ft)" << "<br/>";
            ss << "Distance: " << azAlPos.length() << " m" << "<br/>";
            double a;
            double b;
            StelUtils::rectToSphe(&a, &b, azAlPos);
            ss << "Azimuth / Elevation: " << StelUtils::radToDmsStr(M_PI - a) << " / " << StelUtils::radToDmsStr(b) << "<br/>";
            ss << "Over ground speed: " << currentFrame->ground_speed << " m/s (" << currentFrame->ground_speed_knots << " knots)" << "<br/>";
            ss << "Vertical rate: " << currentFrame->vertical_rate << " m/s (" << currentFrame->vertical_rate_ft_min << " ft/min)" << "<br/>";
            ss << "Ground track: " << currentFrame->ground_track << " deg" << "<br/>";
            //ss << "visible " << numVisible << " paths " << numPaths;
        } else {
            ss << "No data for this time.<br/>";
        }
    }

    postProcessInfoString(infoStr, flags);
    return infoStr;
}

Vec3d Flight::getJ2000EquatorialPos(const StelCore *core) const
{
    return core->altAzToJ2000(getAzAl(), StelCore::RefractionOff);
}

Vec3d Flight::getAzAl() const
{
    return getAzAl(position);
}

bool Flight::isTimeInRange(double jd) const
{
    return data->timeInRange(jd);
}

void Flight::update(double currentTime)
{
    currentFrame = data->getData(currentTime);
    if (currentFrame) {
        inTimeRange = true;
        position = calcECEFPosition(Vec3d(currentFrame->latitude, currentFrame->longitude, currentFrame->altitude));
        azAlPos = getAzAl();
        //qDebug() << "position " << position.toString();
        //qDebug() << ADSBData::ADSBFrameToString(currentFrame);
    } else {
        inTimeRange = false;
    }
}

void Flight::draw(StelCore *core, StelPainter &painter, Flight::PathDrawMode pathMode, bool drawLabel)
{
    if (!inTimeRange) {
        return;
    }

    if ((pathMode == Flight::SelectedOnly && flightSelected) || (pathMode == Flight::AllPaths)) {
        drawPath(core, painter);
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);

    Vec3d xy;
    if (prj->project(azAlPos, xy) && prj->checkInViewport(xy)) {
        if (pathMode == Flight::InViewOnly) {
            drawPath(core, painter);
        }
        numVisible++;
        painter.drawSprite2dMode(xy[0], xy[1], 11);
        /*
        Vec4f col = painter.getColor();
        painter.setColor(1,0,0,1);
        painter.drawPoint2d(xy[0], xy[1]);
        painter.setColor(col[0], col[1], col[2], col[3]);
        */
        if (drawLabel) {
            painter.drawText(xy[0], xy[1], getEnglishName(), 0, 10, 10, false);
        }
    }
}

//! Clamp val between min and max inclusive
double clamp(double min, double val, double max)
{
    if (val <= min) {
        return min;
    }
    if (val >= max) {
        return max;
    }
    return val;
}


void Flight::drawPath(StelCore *core, StelPainter &painter)
{
    if (!inTimeRange) {
        return;
    }
    //TODO eventually do this all in a shader
    numPaths++;
    //glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
    Vec3d onscreen; // projected coords
    painter.enableClientStates(true, false, true);
    QList<ADSBFrame> *pdata = data->getAllData();
    if (pathVert.size() < pdata->size() * 2) {
        pathVert.resize(pdata->size() * 2);
        pathCol.resize(pdata->size() * 4);
    }
    for (int i = 0; i < pdata->size(); ++i) {
        prj->project(getAzAl(pdata->at(i).ecefPos), onscreen);
        pathVert[i * 2] = onscreen[0];
        pathVert[i * 2 + 1] = onscreen[1];
        if (pathColour == SolidColour) {
            pathCol[i * 4] = .2 + .8 * (float)(i) / pdata->size();
            pathCol[i * 4 + 1] = .2 + .8 * (float)(i) / pdata->size();
            pathCol[i * 4 + 2] = .2 + .8 * (float)(i) / pdata->size();
        } else if (pathColour == EncodeHeight) {
            pathCol[i * 4] = clamp(minHeight, pdata->at(i).altitude, maxHeight) / heightRange;
            pathCol[i * 4 + 1] = 0;
            pathCol[i * 4 + 2] = 0;
        } else if (pathColour == EncodeVelocity) {
            pathCol[i * 4] = clamp(minVelocity, pdata->at(i).ground_speed, maxVelocity) / velRange;
            pathCol[i * 4 + 1] = clamp(minVertRate, pdata->at(i).vertical_rate, 0) / minVertRate;
            pathCol[i * 4 + 2] = clamp(0, pdata->at(i).vertical_rate, maxVertRate) / maxVertRate;
        } else {
            pathCol[i * 4] = 1;
            pathCol[i * 4 + 1] = 1;
            pathCol[i * 4 + 2] = 1;
        }
        // Alpha determined by fading
        pathCol[i * 4 + 3] = painter.getColor()[3];
    }
    painter.setVertexPointer(2, GL_FLOAT, pathVert.data());
    painter.setColorPointer(4, GL_FLOAT, pathCol.data());
    painter.drawFromArray(StelPainter::LineStrip, pdata->size(), 0, false);
    painter.enableClientStates(false);
}

Vec3d Flight::calcECEFPosition(double lat, double lon, double alt)
{
    return calcECEFPosition(Vec3d(lat, lon, alt));
}

void Flight::updateObserverPos(StelLocation pos)
{
    observerPos = calcECEFPosition(Vec3d(pos.latitude * M_PI / 180.0, pos.longitude * M_PI / 180.0, pos.altitude));
    double sla = sin(pos.latitude * M_PI / 180.0);
    double cla = cos(pos.latitude * M_PI / 180.0);
    double slo = sin(pos.longitude * M_PI / 180.0);
    double clo = cos(pos.longitude * M_PI / 180.0);
    ECEFtoAzAl[0][0] = sla * clo;
    ECEFtoAzAl[0][1] = sla * slo;
    ECEFtoAzAl[0][2] = -cla;
    ECEFtoAzAl[1][0] = -slo;
    ECEFtoAzAl[1][1] = clo;
    ECEFtoAzAl[1][2] = 0;
    ECEFtoAzAl[2][0] = cla * clo;
    ECEFtoAzAl[2][1] = cla * slo;
    ECEFtoAzAl[2][2] = sla;

}

Vec3d Flight::getAzAl(const Vec3d &v)
{
    Vec3d toPoint = v - observerPos;
    Vec3d ret;
    // South
    ret[0] = ECEFtoAzAl[0][0] * toPoint[0]
            + ECEFtoAzAl[0][1] * toPoint[1]
            + ECEFtoAzAl[0][2] * toPoint[2];
    // East
    ret[1] = ECEFtoAzAl[1][0] * toPoint[0]
            + ECEFtoAzAl[1][1] * toPoint[1]
            + ECEFtoAzAl[1][2] * toPoint[2];
    // Local Vertical
    ret[2] = ECEFtoAzAl[2][0] * toPoint[0]
            + ECEFtoAzAl[2][1] * toPoint[1]
            + ECEFtoAzAl[2][2] * toPoint[2];
    return ret;
}
double Flight::getMaxVertRate()
{
    return maxVertRate;
}

void Flight::setMaxVertRate(double value)
{
    maxVertRate = value;
}
double Flight::getMinVertRate()
{
    return minVertRate;
}

void Flight::setMinVertRate(double value)
{
    minVertRate = value;
}
double Flight::getMaxVelocity()
{
    return maxVelocity;
}

void Flight::setMaxVelocity(double value)
{
    maxVelocity = value;
}
double Flight::getMinVelocity()
{
    return minVelocity;
}

void Flight::setMinVelocity(double value)
{
    minVelocity = value;
}
double Flight::getMaxHeight()
{
    return maxHeight;
}

void Flight::setMaxHeight(double value)
{
    maxHeight = value;
}
double Flight::getMinHeight()
{
    return minHeight;
}

void Flight::setMinHeight(double value)
{
    minHeight = value;
}

void Flight::writeToDb() const
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery qry;
    //speed up sqlite considerably
    if (db.driverName() == "QSQLITE") {
        qry.exec("PRAGMA synchronous=OFF");
        qry.exec("PRAGMA journal_mode=MEMORY");
        qry.exec("PRAGMA temp_store=MEMORY");
    }
    qry.prepare("INSERT INTO flights (mode_s, callsign, mode_s_hex, country) VALUES (:mode_s, :callsign, :mode_s_hex, :country);");
    QString mode_s = data->getAddress();
    QString callsign = data->getCallsign();
    qry.bindValue(":mode_s", mode_s);
    qry.bindValue(":callsign", callsign);
    qry.bindValue(":mode_s_hex", data->getHexAddress());
    qry.bindValue(":country", data->getCountry());
    qry.exec();
    if (qry.lastError().type() != QSqlError::NoError) {
        qDebug() << qry.lastError().text();
    }
//#define BULKQUERY
#ifdef BULKQUERY
    QVariantList jdates;
    QVariantList mode_ss;
    QVariantList callsigns;
    QVariantList onGrounds;
    QVariantList altitudes;
    QVariantList latitudes;
    QVariantList longitudes;
    QVariantList verticalRates;
    QVariantList groundSpeeds;
    QVariantList groundTracks;
#endif
    qry.prepare("INSERT INTO adsb (jdate, flights_mode_s, flights_callsign, on_ground, altitude_feet, latitude, longitude, vertical_rate_ft_min, ground_speed_knots, ground_track) "
                "VALUES (:jdate, :flights_mode_s, :flights_callsign, :on_ground, :altitude_feet, :latitude, :longitude, :vertical_rate_ft_min, :ground_speed_knots, :ground_track)");
#ifndef BULKQUERY
    foreach (ADSBFrame f, *data->getAllData()) {
        qry.bindValue(":jdate", f.time);
        qry.bindValue(":flights_mode_s", mode_s);
        qry.bindValue(":flights_callsign", callsign);
        qry.bindValue(":on_ground", f.onGround);
        qry.bindValue(":altitude_feet", f.altitude_feet);
        qry.bindValue(":latitude", f.latitude);
        qry.bindValue(":longitude", f.longitude);
        qry.bindValue(":vertical_rate_ft_min", f.vertical_rate_ft_min);
        qry.bindValue(":ground_speed_knots", f.ground_speed_knots);
        qry.bindValue(":ground_track", f.ground_track);
        qry.exec();
        if (qry.lastError().type() != QSqlError::NoError) {
            qDebug() << qry.lastError().text();
        }
    }
#else
        jdates << f.time;
        mode_ss << mode_s;
        callsigns << callsign;
        onGrounds << f.onGround;
        altitudes << f.altitude_feet;
        latitudes << f.latitude;
        longitudes << f.longitude;
        verticalRates << f.vertical_rate_ft_min;
        groundSpeeds << f.ground_speed_knots;
        groundTracks << f.ground_track;
    }
    qry.bindValue(":jdate", jdates);
    qry.bindValue(":flights_mode_s",mode_ss);
    qry.bindValue(":flights_callsign", callsigns);
    qry.bindValue(":on_ground",onGrounds);
    qry.bindValue(":altitude_feet", altitudes);
    qry.bindValue(":latitude", latitudes);
    qry.bindValue(":longitude", longitudes);
    qry.bindValue(":vertical_rate_ft_min", verticalRates);
    qry.bindValue(":ground_speed_knots", groundSpeeds);
    qry.bindValue(":ground_track", groundTracks);
    qry.exec();
    if (qry.lastError().type() != QSqlError::NoError) {
        qDebug() << qry.lastError().text();
    }
#endif
#undef BULKQUERY
    db.commit();
}

Vec3d Flight::calcECEFPosition(const Vec3d &pos)
{
    double r;
    double c;
    double sq;
    double sinlat = sin(pos[0]);
    Vec3d res;

    c = 1 / sqrt(1 + __f * (__f - 2) * (sinlat * sinlat));
    sq = (1 - __f) * (1 - __f) * c;
    r = (R_EARTH * c + pos[2]) * cos(pos[0]);
    res[0] = r * cos(pos[1]);
    res[1] = r * sin(pos[1]);
    res[2] = (R_EARTH * sq + pos[2]) * sinlat;
    return res;
}
