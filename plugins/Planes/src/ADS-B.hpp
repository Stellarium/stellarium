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

#ifndef ADSB_HPP
#define ADSB_HPP

#include <QtCore>
#include "VecMath.hpp"


typedef struct adsb_frame_s {
    double time; //!< time of this data frame (julian day)
    bool onGround; //!< is the plane on the ground?
    double altitude; //!< altitude of the plane [m]
    double altitude_feet; //!< altitude of the plane [feet]
    double latitude; //!< latitude [rad]
    double longitude; //!< longitude [rad]
    double vertical_rate; //!< vertical rate [m/s]
    double vertical_rate_ft_min; //!< vertical rate [ft/min]
    double ground_speed; //!< ground speed [m/s]
    double ground_speed_knots; //!< ground speed [knots]
    double ground_track; //!< track angle [deg]
    Vec3d ecefPos;
} ADSBFrame;


class ADSBData {
public:
    ADSBData(QStringList &data);
    ADSBData(QList<ADSBFrame> &data, QString &modeS, QString &modeSHex, QString &callsign, QString &country);
    /*
    ADSBData(QString modeS, QString modeSHex, QString callsign, QString country, QList<ADSBData> data)
        : modeSAddress(modeS), modeSAddressHex(modeSHex), callsign(callsign), country(country), data(data), lastIndex(0), lastRequestedJd(0)
    {
        initialised = true;
    }
    */
    ~ADSBData();

    const ADSBFrame *getData(double jd);

    bool append(QList<ADSBFrame> &newdata);
    bool appendFrame(ADSBFrame &frame);

    void setInfo(QString &callsign, QString &country);

    void appendSurfacePos(const double &jdate, const double altitude,
                          const double &groundSpeed, const double &track,
                          const double &lat, const double &lon);
    void appendAirbornePos(const double &jdate, const double &altitude,
                           const double &lat, const double &lon, const double &onGround);
    void appendAirborneVelocity(const double &jdate, const double &groundSpeed,
                                const double &track,
                                const double &verticalRate);

    //! Returns the modeSAddress (Base10)
    QString getAddress() const
    {
        return modeSAddress;
    }

    //! Returns the modeSAddress (Hex)
    QString getHexAddress() const
    {
        return modeSAddressHex;
    }

    QString getCallsign() const
    {
        return callsign;
    }

    QString getCountry() const
    {
        return country;
    }

    bool isInitialised() const
    {
        return initialised;
    }

    QDateTime getLastUpdateTime() const
    {
        return lastUpdate;
    }

    bool timeInRange(double jd) const;

    double getTimeStart() const;

    double getTimeEnd() const;

    QList<ADSBFrame> *getAllData()
    {
        return &data;
    }

    int size() const
    {
        return data.size();
    }

    static QString ADSBFrameToString(ADSBFrame const *frame);
    static bool useInterp;
    //! This needs to be called after turning interpolation on or off
    //! to reset the conditions for the data search algorithm. Otherwise
    //! the algorithm might skip the correct datapoint and fail, at least
    //! while searching in forward direction.
    //! After resetting, data will be searched in fwd direction, until
    //! a match is found, after that, it operates normally again.
    void resetSearchAlgorithm();

    static bool parseLine(QString line, ADSBFrame &f, QString &modeS, QString &modeSHex, QString &callsign, QString &country);

    //! Takes a frame and calculates the metric values for altitude, vertical rate, speed
    //! @param frame the frame to process
    static void postProcessFrame(ADSBFrame *frame);

private:
    void parseData(QStringList &data);
    void interpolate(int start, int end, double time);
    void checkFrame();
    static double linearInterp(double a, double b, double pos);

    QString modeSAddress;
    QString modeSAddressHex;
    QString callsign;
    QString country;
    bool initialised;

    QList<ADSBFrame> data;
    double lastRequestedJd;
    int lastIndex;
    ADSBFrame interpFrame;


    double surfPosTime;
    double surfPosAlt;
    double surfPosSpd;
    double surfPosTrack;
    double surfPosLat;
    double surfPosLon;

    double airPosTime;
    double airPosAlt;
    double airPosLat;
    double airPosLon;
    bool airPosOnGround;

    double airVelTime;
    double airVelSpd;
    double airVelTrack;
    double airVelRate;

    QDateTime lastUpdate;
};



#endif // ADSB_HPP
