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
#include "StelTranslator.hpp"
#include "ADS-B.hpp"
#include <vector>

class Planes;

//! @class Flight
//! Represents a series of ADS-B Datapoints that make up a flight event.
//! Handles information and rendering for flights.
//! Flights are managed by the FlightMgr.
//! @author Felix Zeltner
class Flight : public StelObject
{
	friend Planes;
	//Required for Q_FLAG macro, this requires this header to be MOC'ed
	Q_GADGET
public:

	//! @enum PathColorMode
	//! Determines how the flight paths are colored
	enum PathColorMode
	{
		SolidColor    = 0,  //!< Draw flight paths in solid color
		EncodeHeight   = 1,  //!< Draw flight paths colored by height
		EncodeVelocity = 2   //!< Draw flight paths colored by velocity
	};
	Q_ENUM(PathColorMode)

	//! @enum PathDrawMode
	//! Determines for which flights paths are drawn
	enum PathDrawMode
	{
		NoPaths      = 0,    //!< Draw no paths at all
		SelectedOnly = 1,    //!< Draw path for the selected flight only
		InViewOnly   = 2,    //!< Draw paths for all planes in view
		AllPaths     = 3     //!< Draw paths for all planes, regardless of screen position
	};
	Q_ENUM(PathDrawMode)

	//! Default constructor1
	Flight();

	//! Construct a Flight object by parsing a BaseStation Recording
	//! @param data the BaseStation Recording data (linewise)
	Flight(QStringList &data);

	//! Construct a Flight object by passing all the data already parsed
	Flight(QList<ADSBFrame> &data, QString &modeS, QString &modeSHex, QString &callsign, QString &country);
	~Flight();


	//! Append a list of ADSBFrames to the underlying ADSBData object
	void appendData(QList<ADSBFrame> &newData);

	//! Append a signle frame to the underlying ADSBData object
	void appendSingle(ADSBFrame &frame);

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
						   const double &lat, const double &lon, const bool &onGround);

	//! Append an airborne velocity message.
	//! Performs checks if a frame can be built with this data, otherwise waits
	//! for more data
	void appendAirborneVelocity(const double &jdate, const double &groundSpeed,
								const double &track,
								const double &verticalRate);

	//! Last time a frame has been parsed from messages by ADSBData
	QDateTime getLastUpdateTime() const;


	//! Get an HTML string to describe the flight.
	//! @param core A pointer to the StelCore.
	//! @param flags A set of flags indicating which info to include.
	QString getInfoString(const StelCore *core, const InfoStringGroup &flags) const override;


	//! Return the object's type. Type is the classname.
	QString getType() const override
	{
		return QStringLiteral("Flight");
	}
	//! Return object's type. It should be English lowercase name of the type of the object.
	virtual QString getObjectType() const override
	{
		return N_("plane");
	}
	//! Return object's type. It should be translated lowercase name of the type of the object.
	virtual QString getObjectTypeI18n() const override
	{
		return q_(getObjectType());
	}

	//! Returns the english name of this flight.
	//! This is either the callsign, or if that is empty, the hex ModeS address.
	QString getEnglishName() const override;

	//! Returns the translated name.
	//! This is the same as the english name, as they are only combinations
	//! of letters and numbers anyways.
	QString getNameI18n() const override;

	//! An object may have multiple IDs (different catalog numbers, etc). StelObjectMgr::searchByID()
	//! should search through all ID variants, but this method only returns one of them.
	virtual QString getID() const override
	{
		return getCallsign();
	}

	//! Get the callsign of this flight.
	QString getCallsign() const;

	//! Get the ModeS address of this flight (in hex).
	QString getAddress() const;

	//! Get ModeS address in raw form
	QString getIntAddress() const;

	//! Get the country
	QString getCountry() const;

	//! Get the J2000Equatorial Position
	Vec3d getJ2000EquatorialPos(const StelCore *core) const override;

	//! Get azimuth / elevation position, depends on observer
	Vec3d getAzAl() const;

	//! Selection priority for Flight objects when user clicks
	float getSelectPriority(const StelCore *core) const override
	{
		return -11;
	}

	//! Angular size of Flight objects
	virtual double getAngularRadius(const StelCore *core) const override
	{
		return .05;
	}

	//! Magnitude of Flight objects
	float getVMagnitude(const StelCore *core) const override
	{
		return 100;
	}

	//! Color used to color the info string
	Vec3f getInfoColor() const override
	{
		return infoColor;
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

	//! Get time datapoints start at
	double getTimeStart() const;

	//! Get time datapoints end at
	double getTimeEnd() const;

	//! Returns true if the flight is selected
	bool isFlightSelected() const
	{
		return flightSelected;
	}

	//! Mark this flight as currently selected.
	//! Used for path drawing checks.
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

	//! Set the path coloring mode to mode.
	static void setPathColorMode(PathColorMode mode)
	{
		pathColorMode = mode;
	}

	//! Get the current path coloring mode
	static PathColorMode getPathColorMode()
	{
		return pathColorMode;
	}
	//! Get the current path draw mode
	static PathDrawMode getPathDrawMode()
	{
		return pathDrawMode;
	}

	static int numVisible; //!< Debugging count of planes visible currently
	static int numPaths; //!< Debugging count of paths drawn currently


	//! Write this flight to the database.
	void writeToDb() const;

	//! Return number of available data points
	int size() const;

	//! The color used for drawing info text and icons
	static Vec3f getFlightInfoColor() {return infoColor;}


private:
	static Vec3d observerPos; //!< The position of the observer, used to calculate Az/Al pos
	static double ECEFtoAzAl[3][3]; //!< Matrix to transform from ECEF to Az/Al
	static PathColorMode pathColorMode; //!< Path coloring mode
	static PathDrawMode pathDrawMode;     //!< Path drawing mode

	static QFont labelFont; //!< Font for rendering labels
	static Vec3f infoColor; //!< Color for rendering infoString
	//!@{
	//! Values for color coding paths
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
	static std::vector<float> pathCol;	//!< Buffer holding path colors for rendering

	ADSBData *data; //!< Holds the actual ADS-B Data
	Vec3d azAlPos; //!< Current position in Az/Al frame

	bool flightSelected; //!< Flight currently selected
	ADSBFrame const *currentFrame; //!< Last frame returned by data
	Vec3d position; //!< ECEF position of the flight
	bool inTimeRange; //!< Have data currently
};

//! @typedef FlightP smartpointer to a Flight object
typedef QSharedPointer<Flight> FlightP;
//! @typedef FlightListP smartpointer to a list of Flight objects
typedef QSharedPointer<QList<FlightP> > FlightListP;

//! Let FlightP be passed via signals and slots
Q_DECLARE_METATYPE(FlightP)

#endif // FLIGHT_HPP
