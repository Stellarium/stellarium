/*
 * Stellarium
 * 
 * Copyright (C) 2022 Worachate Boonplod
 * Copyright (C) 2022 Georg Zotti
 * Copyright (C) 2024 Ruslan Kabatsayev
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef SOLARECLIPSE_HPP
#define SOLARECLIPSE_HPP

#include <deque>
#include <vector>
#include <variant>
#include "Planet.hpp"

class StelCore;
class StelLocaleMgr;

struct EclipseBesselElements
{
	double x;   //!< x coordinate of the shadow axis in the fundamental plane (in units of equatorial Earth radius)
	double y;   //!< y coordinate of the shadow axis in the fundamental plane (in units of equatorial Earth radius)
	double d;   //!< declination of the shadow axis direction in the celestial sphere (in radians)
	double mu;  //!< Greenwich hour angle of the shadow axis direction in the celestial sphere (in degrees)
	double tf1; //!< tangent of the angle of the penumbral shadow cone with the shadow axis
	double tf2; //!< tangent of the angle of the umbral shadow cone with the shadow axis
	double L1;  //!< radius of the penumbral shadow on the fundamental plane (in units of equatorial Earth radius)
	double L2;  //!< radius of the umbral shadow on the fundamental plane (in units of equatorial Earth radius)
};

//! Calculate Besselian elements of solar eclipse
EclipseBesselElements calcSolarEclipseBessel();

struct EclipseBesselParameters
{
	double xdot;  //!< rate of change of X in Earth radii per second
	double ydot;  //!< rate of change of Y in Earth radii per second
	double ddot;  //!< rate of change of d in radians per second
	double mudot; //!< rate of change of mu in radians per second
	double ldot;  //!< rate of change of L1 (for penumbra) or L2 (for umbra) in Earth radii per second
	double etadot;
	double bdot;
	double cdot;
	EclipseBesselElements elems;
};

// Compute parameters from Besselian elements
EclipseBesselParameters calcBesselParameters(bool penumbra);

// Calculate solar eclipse data at given time
void calcSolarEclipseData(double JD, double &dRatio, double &latDeg, double &lngDeg, double &altitude,
                          double &pathWidth, double &duration, double &magnitude);

class SolarEclipseComputer
{
public:
	struct EclipseMapData
	{
		enum class EclipseType
		{
			Undefined,
			Total,
			Annular,
			Hybrid,
		};
		struct GeoPoint
		{
			double longitude;
			double latitude;

			GeoPoint() = default;
			GeoPoint(double lon, double lat)
				: longitude(lon), latitude(lat)
			{
			}
		};
		struct GeoTimePoint
		{
			double JD = -1;
			double longitude;
			double latitude;

			GeoTimePoint() = default;
			GeoTimePoint(double JD, double lon, double lat)
				: JD(JD), longitude(lon), latitude(lat)
			{
			}
		};
		struct UmbraOutline
		{
			std::vector<GeoPoint> curve;
			double JD;
			EclipseType eclipseType = EclipseType::Undefined;
		};
		GeoTimePoint greatestEclipse;
		GeoTimePoint firstContactWithEarth; // AKA P1
		GeoTimePoint lastContactWithEarth;  // AKA P4
		GeoTimePoint centralEclipseStart;   // AKA C1
		GeoTimePoint centralEclipseEnd;     // AKA C2

		// Generally these lines are supposed to represent the north and south limits of
		// penumbra. But in practice they are computed in smaller segments, so there'll
		// usually be more than two.
		std::vector<std::vector<GeoTimePoint>> penumbraLimits;

		// The curves in arrays are split into two lines by the computation algorithm
		struct TwoLimits
		{
			std::vector<GeoPoint> p12curve;
			std::vector<GeoPoint> p34curve;
		};
		struct SingleLimit
		{
			std::vector<GeoPoint> curve;
		};
		std::variant<SingleLimit,TwoLimits> riseSetLimits[2];

		// These curves appear to be split generally in multiple sections
		std::vector<std::deque<GeoTimePoint>> maxEclipseAtRiseSet;

		std::vector<GeoPoint> centerLine;
		std::vector<UmbraOutline> umbraOutlines;
		std::vector<std::vector<GeoTimePoint>> umbraLimits;

		EclipseType eclipseType;
	};

	struct GeoPoint
	{
		double latitude;
		double longitude;
		GeoPoint() = default;
		GeoPoint(double latitude, double longitude)
			: latitude(latitude), longitude(longitude)
		{
		}
	};

	SolarEclipseComputer(StelCore* core, StelLocaleMgr* localeMgr);
	EclipseMapData generateEclipseMap(const double JDMid) const;

	//! Geographic coordinates of extreme contact
	GeoPoint getContactCoordinates(double x, double y, double d, double mu) const;
	//! Geographic coordinates where solar eclipse begins/ends at sunrise/sunset
	GeoPoint getRiseSetLineCoordinates(bool first, double x, double y, double d, double L, double mu) const;
	//! Geographic coordinates where maximum solar eclipse occurs at sunrise/sunset
	GeoPoint getMaximumEclipseAtRiseSet(bool first, double JD) const;
	//! Geographic coordinates of shadow outline
	GeoPoint getShadowOutlineCoordinates(double angle, double x, double y, double d, double L, double tf,double mu) const;
	//! Iteration to calculate minimum distance from Besselian elements
	double getJDofMinimumDistance(double JD) const;
	//! Iteration to calculate JD of solar eclipse contacts
	double getJDofContact(double JD, bool beginning, bool penumbral, bool external, bool outerContact) const;
	//! Iteration to calculate contact times of solar eclipse
	double getDeltaTimeOfContact(double JD, bool beginning, bool penumbra, bool external, bool outerContact) const;

	void generateKML(const EclipseMapData& data, const QString& dateString, QTextStream& stream) const;
	bool generatePNGMap(const EclipseMapData& data, const QString& filePath) const;

private:
	//! Check whether both northern and southern penumbra limits exist for a given eclipsee
	//! @param JDMid time of greatest eclipse
	bool bothPenumbraLimitsPresent(double JDMid) const;

	void computeNSLimitsOfShadow(double JDP1, double JDP4, bool penumbra,
	                             std::vector<std::vector<EclipseMapData::GeoTimePoint>>& limits) const;

private:
	StelCore* core = nullptr;
	StelLocaleMgr* localeMgr = nullptr;
};

#endif
