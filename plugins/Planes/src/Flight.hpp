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

#ifndef FLIGHT_HPP
#define FLIGHT_HPP


#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelLocation.hpp"
#include "ADS-B.hpp"
#include <vector>




//! @class Flight
//! Represents a series of ADS-B Datapoints that make up a flight event.
//! Handles information and rendering for flights.
//! Flights are managed by the FlightMgr.
//! @author Felix Zeltner
class Flight : public StelObject
{
public:

    //! @enum PathColour
    //! Determins how the flight paths are coloured
    enum PathColour
    {
        SolidColour = 0,		//!< Draw flight paths in solid colour
        EncodeHeight = 1,		//!< Draw flight paths coloured by height
        EncodeVelocity = 2		//!< Draw flight paths coloured by velocity
    };

    //! @enum PathDrawMode
    //! Determins for which flights paths are drawn
    enum PathDrawMode {
        NoPaths = 0,			//!< Draw no paths at all
        SelectedOnly = 1,		//!< Draw path for the selected flight only
        InViewOnly = 2,			//!< Draw paths for all planes in view
        AllPaths = 3			//!< Draw paths for all planes, regardless of screen position
    };

    Flight();
    Flight(QString filename);
    Flight(QStringList &data);
    Flight(QList<ADSBFrame> &data, QString &modeS, QString &modeSHex, QString &callsign, QString &country);
    ~Flight();


    void appendData(QList<ADSBFrame> &newData);
    void appendSingle(ADSBFrame &frame);
    void setInfo(QString &callsign, QString &country)
    {
        data->setInfo(callsign, country);
    }

    void appendSurfacePos(const double &jdate, const double altitude,
                          const double &groundSpeed, const double &track,
                          const double &lat, const double &lon)
    {
        data->appendSurfacePos(jdate, altitude, groundSpeed, track, lat, lon);
    }

    void appendAirbornePos(const double &jdate, const double &altitude,
                           const double &lat, const double &lon, const bool &onGround)
    {
        data->appendAirbornePos(jdate, altitude, lat, lon, onGround);
    }

    void appendAirborneVelocity(const double &jdate, const double &groundSpeed,
                                const double &track,
                                const double &verticalRate)
    {
        data->appendAirborneVelocity(jdate, groundSpeed, track, verticalRate);
    }

    QDateTime getLastUpdateTime() const
    {
        return data->getLastUpdateTime();
    }


    //! Get an HTML string to describe the flight.
    //! @param core A pointer to the StelCore.
    //! @param flags A set of flags indicating which info to include.
    virtual QString getInfoString(const StelCore *core, const InfoStringGroup &flags) const;


    //! Return the object's type. Type is the classname.
    virtual QString getType() const
    {
        return "Flight";
    }

    //! Returns the english name of this flight.
    //! This is either the callsign, or if that is empty, the hex ModeS address.
    virtual QString getEnglishName() const
    {
        return data->getCallsign().isEmpty() ? data->getHexAddress() : data->getCallsign();
    }

    //! Returns the translated name.
    //! This is the same as the english name, as they are only combinations
    //! of letters and numbers anyways.
    virtual QString getNameI18n() const
    {
        return getEnglishName();
    }

    //! Get the callsign of this flight.
    QString getCallsign() const
    {
        return data->getCallsign();
    }

    //! Get the ModeS address of this flight (in hex).
    QString getAddress() const
    {
        return data->getHexAddress();
    }

    QString getIntAddress() const
    {
        return data->getAddress();
    }

    QString getCountry() const
    {
        return data->getCountry();
    }

    virtual Vec3d getJ2000EquatorialPos(const StelCore *core) const;

    Vec3d getAzAl() const;

    virtual float getSelectPriority(const StelCore *core) const
    {
       return -11;
    }

    virtual double getAngularSize(const StelCore *core) const
    {
        return .1;
    }

    virtual float getVMagnitude(const StelCore *core) const
    {
        return 100;
    }

    //! Colour used to colour the info string
    virtual Vec3f getInfoColor() const
    {
        return Vec3f(0, 1, 0);
    }

    //! Checks if this plane is visible at the current time.
    //! Only takes into account the time range of available ADS-B data, not screen position
    //! @returns true if the flight has data for the current time.
    bool isVisible() const
    {
        return inTimeRange;
    }

    //! Returns true if jd is in the range of time this flight has data for.
    bool isTimeInRange(double jd) const;

    double getTimeStart() const
    {
        return data->getTimeStart();
    }

    double getTimeEnd() const
    {
        return data->getTimeEnd();
    }

    //! Returns true if the flight is selected
    bool isFlightSelected() const
    {
        return flightSelected;
    }

    //! Mark this flight as currently selected
    void setFlightSelected(bool selected)
    {
        flightSelected = selected;
    }


    //! Update the flight for the next frame.
    //! Updates position using the ADS-B Data and interpolation.
    void update(double currentTime);

    //! Draws the plane's position.
    void draw(StelCore *core, StelPainter &painter, Flight::PathDrawMode pathMode, bool drawLabel);

    //! Draws the plane's path.
    void drawPath(StelCore *core, StelPainter &painter);

    //! Reset the data search algorithm after interp toggle
    void resetDataSearch()
    {
        data->resetSearchAlgorithm();
    }


    //! Calculate a position in ECEF as x/y/z from a position in EFEC in lat/long/alt
    //! @param pos Vector with latitude in rad, longitude in rad and altitude in m
    //! @returns position in ECEF in m
    static Vec3d calcECEFPosition(const Vec3d &pos);

    //! Calculate a position in ECEF as x/y/z from a position in ECEF in lat/long/alt
    //! @overload calcECEFPosition(const Vec3d &pos)
    static Vec3d calcECEFPosition(double lat, double lon, double alt);

    //! Update the observer position when it has changed in Stellarium
    //! @param pos the new observer position
    static void updateObserverPos(StelLocation pos);

    //! Calculate the Azimut/Altitude position from a vector in ECEF
    //! @param v the vector in ECEF
    //! @returns the vector in Az/Al
    static Vec3d getAzAl(const Vec3d &v);

    //! Set the path colouring mode to mode.
    static void setPathColourMode(PathColour mode)
    {
        pathColour = mode;
    }

    //! Get the current path colouring mode
    static PathColour getPathColourMode()
    {
        return pathColour;
    }

    static int numVisible;
    static int numPaths;

    static double getMaxVertRate();
    static void setMaxVertRate(double value);

    static double getMinVertRate();
    static void setMinVertRate(double value);

    static double getMaxVelocity();
    static void setMaxVelocity(double value);

    static double getMinVelocity();
    static void setMinVelocity(double value);

    static double getMaxHeight();
    static void setMaxHeight(double value);

    static double getMinHeight();
    static void setMinHeight(double value);

    void writeToDb() const;

    int size() const
    {
        return data->size();
    }

private:
    static Vec3d observerPos;
    static double ECEFtoAzAl[3][3];
    static PathColour pathColour;
    static QFont labelFont;
    //!@{
    //! Values for colour coding paths
    static double maxVertRate;
    static double minVertRate;
    static double maxVelocity;
    static double minVelocity;
    static double velRange;
    static double maxHeight;
    static double minHeight;
    static double heightRange;
    //!@}
    static std::vector<float> pathVert;	//!< Buffer holding path points for rendering
    static std::vector<float> pathCol;	//!< Buffer holding path colours for rendering

    ADSBData *data;
    Vec3d azAlPos;

    bool flightSelected;
    ADSBFrame const *currentFrame;
    Vec3d position;
    bool inTimeRange;
};

typedef QSharedPointer<Flight> FlightP;
typedef QSharedPointer<QList<FlightP> > FlightListP;

Q_DECLARE_METATYPE(FlightP)

#endif // FLIGHT_HPP
