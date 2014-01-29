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


//! @struct ADSBFrame
//! Represents an ADS-B Datapoint
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
    Vec3d ecefPos; //!< position in the ecef frame
} ADSBFrame;


//! @class ADSBData
//! This class manages the ADS-B Data pointsfor a flight.
//! It is responsible for looking up and interpolating between data points
//! for a given time.
class ADSBData {
public:
    //! Constructor that parses a BaseStation Recording file that has
    //! already been split into separate lines.
    //! @param data the file linewise file contents
    ADSBData(QStringList &data);

    //! Construct an ADSBData object from already parsed data.
    //! @param data a list of parsed ADSBFrame structs
    //! @param modeS the mode S address
    //! @param modeSHex the hex mode S address
    //! @param callsign the callsign
    //! @param country the country
    ADSBData(QList<ADSBFrame> &data, QString &modeS, QString &modeSHex, QString &callsign, QString &country);
    ~ADSBData();

    //! Query data for the given point in time.
    //! Uses interpolation between closest points if interpolation is enabled.
    //! @param jd the jdate to get the data for
    //! @returns an ADSBFrame if found or NULL
    const ADSBFrame *getData(double jd);

    //! Append a list of ADSBFrame structs to this object.
    //! Checks whether the data fits at the end by comparing time fields
    //! @param newdata the list of new ADSBFrame structs
    //! @returns true if the data was appended successfully
    bool append(QList<ADSBFrame> &newdata);

    //! Appends a single ADSBFrame to this object.
    //! Checks if the frame is newer than the latest frame first.
    //! @param frame the frame
    //! @returns true if the frame was appended.
    bool appendFrame(ADSBFrame &frame);

    //! Set the callsign and country
    void setInfo(QString &callsign, QString &country);

    //! Append a surface position message.
    //! Performs checks if a frame can be built with this data, otherwise waits
    //! for more data
    void appendSurfacePos(const double &jdate, const double altitude,
                          const double &groundSpeed, const double &track,
                          const double &lat, const double &lon);

    //! Append an airborne position message.
    //! Performs checks if a frame can be built with this data, otherwise waits
    //! for more data
    void appendAirbornePos(const double &jdate, const double &altitude,
                           const double &lat, const double &lon, const double &onGround);
    //! Append an airborne velocity message.
    //! Performs checks if a frame can be built with this data, otherwise waits
    //! for more data
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

    //! Returns the callsign
    QString getCallsign() const
    {
        return callsign;
    }

    //! Returns the country
    QString getCountry() const
    {
        return country;
    }

    //! Returns true if this object is ready to be used
    bool isInitialised() const
    {
        return initialised;
    }

    //! Returns the last time data has been added to this frame
    //! if the data port is used as source.
    QDateTime getLastUpdateTime() const
    {
        return lastUpdate;
    }

    //! Checks if there is data for the given point in time
    //! @param jd the time
    //! @returns true if data is found
    bool timeInRange(double jd) const;

    //! Returns the earliest time this object has data for
    double getTimeStart() const;

    //! Returns the latest time this object has data for
    double getTimeEnd() const;

    //! Returns a pointer to the internal storage of ADSBFrames
    //! Used for path drawing
    QList<ADSBFrame> *getAllData()
    {
        return &data;
    }

    //! Returns the number of available data points
    int size() const
    {
        return data.size();
    }

    //! This needs to be called after turning interpolation on or off
    //! to reset the conditions for the data search algorithm. Otherwise
    //! the algorithm might skip the correct datapoint and fail, at least
    //! while searching in forward direction.
    //! After resetting, data will be searched in fwd direction, until
    //! a match is found, after that, it operates normally again.
    void resetSearchAlgorithm();

    ////////////////////////////////////////////////////////////////////////////
    // STATIC MEMBERS BELOW

    //! Convert a frame to a string
    //! Useful for debugging
    static QString ADSBFrameToString(ADSBFrame const *frame);

    static bool useInterp; //!< Controls whether interp is used or not

    //! Parse a line from a BaseStation Recording
    //! @param line the line
    //! @param f resulting ADSBFrame will be written to this
    //! @param modeS mode s address will be written to this
    //! @param modeSHex mode s hex address will be written to this
    //! @param callsign callsign will be written to this
    //! @param country country will be written to this
    //! @returns true if the line could be parsed successfully
    static bool parseLine(QString line, ADSBFrame &f, QString &modeS, QString &modeSHex, QString &callsign, QString &country);

    //! Takes a frame and calculates the metric values for altitude, vertical rate, speed
    //! @param frame the frame to process
    static void postProcessFrame(ADSBFrame *frame);

private:
    //! Parse a BaseStation Recording file linewise
    void parseData(QStringList &data);

    //! Interpolate a frame linearly between two values
    //! Operates on interpFrame
    //! @param start the index of the starting frame in the data list
    //! @param end the index of the ending frame in the data list
    //! @param time the requested jd
    void interpolate(int start, int end, double time);

    //! Checks whether enough data has been supplied from the
    //! BaseStation data port to build a frame
    void checkFrame();

    //! Linearly interpolate between two values
    //! @param a value 1
    //! @param b value 2
    //! @param pos the position between the two values (Range [0, 1])
    //! @returns the interpolated value
    static double linearInterp(double a, double b, double pos);

    QString modeSAddress; //!< Flight's mode s address
    QString modeSAddressHex; //!< Flight's hex encoded mode s address
    QString callsign; //!< Flight's callsign
    QString country; //!< Flight's country
    bool initialised; //!< Has the flight been initialised

    QList<ADSBFrame> data; //!< Internal data storage
    double lastRequestedJd; //!< Last time data was requested for with the getData() method
    int lastIndex; //!< Last position a frame was found
    ADSBFrame interpFrame; //!< Frame that holds interpolated data to return to Flight object

    //!@{
    //! Values for incoming parsed message data
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
    //!@}
};



#endif // ADSB_HPP
