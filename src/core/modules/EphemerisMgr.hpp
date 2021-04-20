/*
 * Stellarium
 * Copyright (C) 2020 Alexander Wolf
 * Copyright (C) 2020 Georg Zotti
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

#ifndef EPHEMERISMGR_HPP
#define EPHEMERISMGR_HPP

#include "StelModule.hpp"
#include "StelTextureTypes.hpp"
#include "VecMath.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"

typedef QPair<QString, int> EphemerisSIPair;

//! @class EphemerisMgr
//! The EphemerisMgr controls the visualization of ephemeris
class EphemerisMgr : public StelModule
{
	Q_OBJECT

	Q_PROPERTY(bool markersDisplayed	READ getFlagMarkers		WRITE setFlagMarkers		NOTIFY markersDisplayedChanged)
	Q_PROPERTY(bool horizontalCoordinates	READ getFlagHorizontalCoordinates	WRITE setFlagHorizontalCoordinates	NOTIFY horizontalCoordinatesChanged)
	Q_PROPERTY(bool datesDisplayed		READ getFlagDates		WRITE setFlagDates		NOTIFY datesDisplayedChanged)
	Q_PROPERTY(bool magnitudesDisplayed	READ getFlagMagnitudes		WRITE setFlagMagnitudes		NOTIFY magnitudesDisplayedChanged)
	Q_PROPERTY(bool lineDisplayed		READ getFlagLine		WRITE setFlagLine		NOTIFY lineDisplayedChanged)
	Q_PROPERTY(int lineThickness		READ getLineThickness		WRITE setLineThickness		NOTIFY lineThicknessChanged)
	Q_PROPERTY(bool skippedData		READ getFlagSkipData		WRITE setFlagSkipData		NOTIFY skipDataChanged)
	Q_PROPERTY(bool skippedMarkers		READ getFlagSkipMarkers		WRITE setFlagSkipMarkers	NOTIFY skipMarkersChanged)
	Q_PROPERTY(int dataStep			READ getDataStep		WRITE setDataStep		NOTIFY dataStepChanged)
	Q_PROPERTY(int dataLimit		READ getDataLimit		WRITE setDataLimit		NOTIFY dataLimitChanged)
	Q_PROPERTY(bool smartDates		READ getFlagSmartDates		WRITE setFlagSmartDates		NOTIFY smartDatesChanged)
	Q_PROPERTY(bool scaleMarkersDisplayed	READ getFlagScaleMarkers	WRITE setFlagScaleMarkers	NOTIFY scaleMarkersChanged)

	Q_PROPERTY(Vec3f genericMarkerColor	READ getGenericMarkerColor	WRITE setGenericMarkerColor	NOTIFY genericMarkerColorChanged)
	Q_PROPERTY(Vec3f secondaryMarkerColor	READ getSecondaryMarkerColor	WRITE setSecondaryMarkerColor	NOTIFY secondaryMarkerColorChanged)
	Q_PROPERTY(Vec3f selectedMarkerColor	READ getSelectedMarkerColor	WRITE setSelectedMarkerColor	NOTIFY selectedMarkerColorChanged)
	Q_PROPERTY(Vec3f mercuryMarkerColor	READ getMercuryMarkerColor	WRITE setMercuryMarkerColor	NOTIFY mercuryMarkerColorChanged)
	Q_PROPERTY(Vec3f venusMarkerColor	READ getVenusMarkerColor	WRITE setVenusMarkerColor	NOTIFY venusMarkerColorChanged)
	Q_PROPERTY(Vec3f marsMarkerColor	READ getMarsMarkerColor		WRITE setMarsMarkerColor	NOTIFY marsMarkerColorChanged)
	Q_PROPERTY(Vec3f jupiterMarkerColor	READ getJupiterMarkerColor	WRITE setJupiterMarkerColor	NOTIFY jupiterMarkerColorChanged)
	Q_PROPERTY(Vec3f saturnMarkerColor	READ getSaturnMarkerColor	WRITE setSaturnMarkerColor	NOTIFY saturnMarkerColorChanged)

	Q_ENUMS(EphemerisTimeStep)

public:

	enum EphemerisTimeStep
	{
		EtsCustomInterval		= 0,
		Ets10Minutes			= 1,
		Ets30Minutes			= 2,
		Ets1Hour				= 3,
		Ets6Hours				= 4,
		Ets12Hours			= 5,
		Ets1SolarDay			= 6,
		Ets5SolarDays			= 7,
		Ets10SolarDays			= 8,
		Ets15SolarDays			= 9,
		Ets30SolarDays			= 10,
		Ets60SolarDays			= 11,
		Ets1JulianDay			= 12,
		Ets5JulianDays			= 13,
		Ets10JulianDays		= 14,
		Ets15JulianDays		= 15,
		Ets30JulianDays		= 16,
		Ets60JulianDays		= 17,
		Ets1SiderealDay		= 18,
		Ets5SiderealDays		= 19,
		Ets10SiderealDays		= 20,
		Ets15SiderealDays		= 21,
		Ets30SiderealDays		= 22,
		Ets60SiderealDays		= 23,
		Ets100SolarDays		= 24,
		Ets100SiderealDays		= 25,
		Ets100JulianDays		= 26,
		Ets1SiderealYear		= 27,
		Ets1JulianYear			= 28,
		Ets1GaussianYear		= 29,
		Ets1SynodicMonth		= 30,
		Ets1DraconicMonth		= 31,
		Ets1MeanTropicalMonth	= 32,
		Ets1AnomalisticMonth	= 33,
		Ets1AnomalisticYear		= 34,
		Ets1Saros				= 35,
		Ets500SiderealDays		= 36	,
		Ets500SolarDays		= 37,
		Ets1Minute			= 38
	};


	EphemerisMgr();
	virtual ~EphemerisMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the EphemerisMgr.
	virtual void init();

	//! Get the module ID, returns "EphemerisMgr".
	virtual QString getModuleID() const {return "EphemerisMgr";}

	//! Draw the markers, lines and text.
	virtual void draw(StelCore* core);

	//! Update time-dependent features.
	//! Used to control fading when turning on and off the markes, lines and text.
	virtual void update(double deltaTime);

	//! Used to determine the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

signals:
	void markersDisplayedChanged(bool b);
	void horizontalCoordinatesChanged(bool b);
	void datesDisplayedChanged(bool b);
	void magnitudesDisplayedChanged(bool b);
	void lineDisplayedChanged(bool b);
	void lineThicknessChanged(int v);
	void skipDataChanged(bool b);
	void skipMarkersChanged(bool b);
	void dataStepChanged(int s);
	void dataLimitChanged(int s);
	void smartDatesChanged(bool b);
	void scaleMarkersChanged(bool b);

	void genericMarkerColorChanged(const Vec3f & color) const;
	void secondaryMarkerColorChanged(const Vec3f & color) const;
	void selectedMarkerColorChanged(const Vec3f & color) const;
	void mercuryMarkerColorChanged(const Vec3f & color) const;
	void venusMarkerColorChanged(const Vec3f & color) const;
	void marsMarkerColorChanged(const Vec3f & color) const;
	void jupiterMarkerColorChanged(const Vec3f & color) const;
	void saturnMarkerColorChanged(const Vec3f & color) const;

	void requestVisualization();

private slots:

	//! Set flag which enabled the showing of ephemeris markers or not
	void setFlagMarkers(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris markers or not
	bool getFlagMarkers() const;

	//! Set flag which enabled the showing of ephemeris line between markers or not
	void setFlagLine(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris line between markers or not
	bool getFlagLine() const;

	//! Set the thickness of ephemeris line
	void setLineThickness(int v);
	//! Get the thickness of ephemeris line
	int getLineThickness() const;

	//! Set flag which enabled the showing of ephemeris markers in horizontal coordinates or not
	void setFlagHorizontalCoordinates(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris markers in horizontal coordinates or not
	bool getFlagHorizontalCoordinates() const;

	//! Set flag which enable the showing the date near ephemeris markers or not
	void setFlagDates(bool b);
	//! Get the current value of the flag which enable the showing the date near ephemeris markers or not
	bool getFlagDates() const;

	//! Set flag which enable the showing the magnitude near ephemeris markers or not
	void setFlagMagnitudes(bool b);
	//! Get the current value of the flag which enable the showing the magnitude near ephemeris markers or not
	bool getFlagMagnitudes() const;

	//! Set flag which allow skipping dates near ephemeris markers
	void setFlagSkipData(bool b);
	//! Get the current value of the flag which allow skipping dates near ephemeris markers
	bool getFlagSkipData() const;

	//! Set flag which allow skipping the ephemeris markers without dates
	void setFlagSkipMarkers(bool b);
	//! Get the current value of the flag which allow skipping the ephemeris markers without dates
	bool getFlagSkipMarkers() const;

	//! Set flag which allow using smart format for dates near ephemeris markers
	void setFlagSmartDates(bool b);
	//! Get the current value of the flag which allow using smart format for dates near ephemeris markers
	bool getFlagSmartDates() const;

	//! Set flag which allow scaling the ephemeris markers
	void setFlagScaleMarkers(bool b);
	//! Get the current value of the flag which allow scaling the ephemeris markers
	bool getFlagScaleMarkers() const;

	//! Set the step of skip for date of ephemeris markers (and markers if it enabled)
	void setDataStep(int step);
	//! Get the step of skip for date of ephemeris markers
	int getDataStep() const;

	//! Set the limit for data: we computed ephemeris for 1, 2 or 5 celestial bodies
	void setDataLimit(int limit);
	//! Get the limit of the data (how many celestial bodies was in computing of ephemeris)
	int getDataLimit() const;

	void setGenericMarkerColor(const Vec3f& c);
	Vec3f getGenericMarkerColor(void) const;

	void setSecondaryMarkerColor(const Vec3f& c);
	Vec3f getSecondaryMarkerColor(void) const;

	void setSelectedMarkerColor(const Vec3f& c);
	Vec3f getSelectedMarkerColor(void) const;

	void setMercuryMarkerColor(const Vec3f& c);
	Vec3f getMercuryMarkerColor(void) const;

	void setVenusMarkerColor(const Vec3f& c);
	Vec3f getVenusMarkerColor(void) const;

	void setMarsMarkerColor(const Vec3f& c);
	Vec3f getMarsMarkerColor(void) const;

	void setJupiterMarkerColor(const Vec3f& c);
	Vec3f getJupiterMarkerColor(void) const;

	void setSaturnMarkerColor(const Vec3f& c);
	Vec3f getSaturnMarkerColor(void) const;

	//! Taking the JD dates for each ephemeride and preparation the human readable dates according to the settings for dates
	void fillDates();

private:
	QSettings* conf;

	//! Draw a nice markers for ephemeris of objects.
	void drawMarkers(StelPainter* painter);

	//! Draw a line, who connected markers for ephemeris of objects.
	void drawLine(StelPainter* painter);

	Vec3f getMarkerColor(int index) const;

	StelTextureSP texGenericMarker;
	StelTextureSP texCometMarker;

	bool markersDisplayed;
	bool datesDisplayed;
	bool magnitudesDisplayed;
	bool horizontalCoordinates;
	bool lineDisplayed;
	int lineThickness;
	bool skipDataDisplayed;
	bool skipMarkersDisplayed;
	int dataStep;				// How many days skip for dates near ephemeris markers (and the markers if it enabled)
	int dataLimit;				// Number of celestial bodies in ephemeris data (how many celestial bodies was in computing of ephemeris)
	bool smartDatesDisplayed;
	bool scaleMarkersDisplayed;
	Vec3f genericMarkerColor;
	Vec3f secondaryMarkerColor;
	Vec3f selectedMarkerColor;
	Vec3f mercuryMarkerColor;
	Vec3f venusMarkerColor;
	Vec3f marsMarkerColor;
	Vec3f jupiterMarkerColor;
	Vec3f saturnMarkerColor;

};

#endif // EPHEMERISMGR_HPP
