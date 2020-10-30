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

//! @class EphemerisMgr
//! The EphemerisMgr controls the visualization of ephemeris
class EphemerisMgr : public StelModule
{
	Q_OBJECT

	Q_PROPERTY(bool ephemerisMarkersDisplayed	READ getFlagEphemerisMarkers		WRITE setFlagEphemerisMarkers		NOTIFY ephemerisMarkersChanged)
	Q_PROPERTY(bool ephemerisHorizontalCoordinates	READ getFlagEphemerisHorizontalCoordinates	WRITE setFlagEphemerisHorizontalCoordinates	NOTIFY ephemerisHorizontalCoordinatesChanged)
	Q_PROPERTY(bool ephemerisDatesDisplayed		READ getFlagEphemerisDates		WRITE setFlagEphemerisDates		NOTIFY ephemerisDatesChanged)
	Q_PROPERTY(bool ephemerisMagnitudesDisplayed	READ getFlagEphemerisMagnitudes		WRITE setFlagEphemerisMagnitudes	NOTIFY ephemerisMagnitudesChanged)
	Q_PROPERTY(bool ephemerisLineDisplayed		READ getFlagEphemerisLine		WRITE setFlagEphemerisLine		NOTIFY ephemerisLineChanged)
	Q_PROPERTY(int ephemerisLineThickness		READ getEphemerisLineThickness		WRITE setEphemerisLineThickness		NOTIFY ephemerisLineThicknessChanged)
	Q_PROPERTY(bool ephemerisSkippedData		READ getFlagEphemerisSkipData		WRITE setFlagEphemerisSkipData		NOTIFY ephemerisSkipDataChanged)
	Q_PROPERTY(bool ephemerisSkippedMarkers		READ getFlagEphemerisSkipMarkers	WRITE setFlagEphemerisSkipMarkers	NOTIFY ephemerisSkipMarkersChanged)
	Q_PROPERTY(int ephemerisDataStep		READ getEphemerisDataStep		WRITE setEphemerisDataStep		NOTIFY ephemerisDataStepChanged)
	Q_PROPERTY(int ephemerisDataLimit		READ getEphemerisDataLimit		WRITE setEphemerisDataLimit		NOTIFY ephemerisDataLimitChanged)
	Q_PROPERTY(bool ephemerisSmartDates		READ getFlagEphemerisSmartDates		WRITE setFlagEphemerisSmartDates	NOTIFY ephemerisSmartDatesChanged)
	Q_PROPERTY(bool ephemerisScaleMarkersDisplayed	READ getFlagEphemerisScaleMarkers	WRITE setFlagEphemerisScaleMarkers	NOTIFY ephemerisScaleMarkersChanged)

	Q_PROPERTY(Vec3f ephemerisGenericMarkerColor	READ getEphemerisGenericMarkerColor	WRITE setEphemerisGenericMarkerColor	NOTIFY ephemerisGenericMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisSecondaryMarkerColor	READ getEphemerisSecondaryMarkerColor	WRITE setEphemerisSecondaryMarkerColor	NOTIFY ephemerisSecondaryMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisSelectedMarkerColor	READ getEphemerisSelectedMarkerColor	WRITE setEphemerisSelectedMarkerColor	NOTIFY ephemerisSelectedMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisMercuryMarkerColor	READ getEphemerisMercuryMarkerColor	WRITE setEphemerisMercuryMarkerColor	NOTIFY ephemerisMercuryMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisVenusMarkerColor	READ getEphemerisVenusMarkerColor	WRITE setEphemerisVenusMarkerColor	NOTIFY ephemerisVenusMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisMarsMarkerColor	READ getEphemerisMarsMarkerColor	WRITE setEphemerisMarsMarkerColor	NOTIFY ephemerisMarsMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisJupiterMarkerColor	READ getEphemerisJupiterMarkerColor	WRITE setEphemerisJupiterMarkerColor	NOTIFY ephemerisJupiterMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisSaturnMarkerColor	READ getEphemerisSaturnMarkerColor	WRITE setEphemerisSaturnMarkerColor	NOTIFY ephemerisSaturnMarkerColorChanged)

public:
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
	void ephemerisMarkersChanged(bool b);
	void ephemerisHorizontalCoordinatesChanged(bool b);
	void ephemerisDatesChanged(bool b);
	void ephemerisMagnitudesChanged(bool b);
	void ephemerisLineChanged(bool b);
	void ephemerisLineThicknessChanged(int v);
	void ephemerisSkipDataChanged(bool b);
	void ephemerisSkipMarkersChanged(bool b);
	void ephemerisDataStepChanged(int s);
	void ephemerisDataLimitChanged(int s);
	void ephemerisSmartDatesChanged(bool b);
	void ephemerisScaleMarkersChanged(bool b);

	void ephemerisGenericMarkerColorChanged(const Vec3f & color) const;
	void ephemerisSecondaryMarkerColorChanged(const Vec3f & color) const;
	void ephemerisSelectedMarkerColorChanged(const Vec3f & color) const;
	void ephemerisMercuryMarkerColorChanged(const Vec3f & color) const;
	void ephemerisVenusMarkerColorChanged(const Vec3f & color) const;
	void ephemerisMarsMarkerColorChanged(const Vec3f & color) const;
	void ephemerisJupiterMarkerColorChanged(const Vec3f & color) const;
	void ephemerisSaturnMarkerColorChanged(const Vec3f & color) const;

	void requestEphemerisVisualization();

private slots:

	//! Set flag which enabled the showing of ephemeris markers or not
	void setFlagEphemerisMarkers(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris markers or not
	bool getFlagEphemerisMarkers() const;

	//! Set flag which enabled the showing of ephemeris line between markers or not
	void setFlagEphemerisLine(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris line between markers or not
	bool getFlagEphemerisLine() const;

	//! Set the thickness of ephemeris line
	void setEphemerisLineThickness(int v);
	//! Get the thickness of ephemeris line
	int getEphemerisLineThickness() const;

	//! Set flag which enabled the showing of ephemeris markers in horizontal coordinates or not
	void setFlagEphemerisHorizontalCoordinates(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris markers in horizontal coordinates or not
	bool getFlagEphemerisHorizontalCoordinates() const;

	//! Set flag which enable the showing the date near ephemeris markers or not
	void setFlagEphemerisDates(bool b);
	//! Get the current value of the flag which enable the showing the date near ephemeris markers or not
	bool getFlagEphemerisDates() const;

	//! Set flag which enable the showing the magnitude near ephemeris markers or not
	void setFlagEphemerisMagnitudes(bool b);
	//! Get the current value of the flag which enable the showing the magnitude near ephemeris markers or not
	bool getFlagEphemerisMagnitudes() const;

	//! Set flag which allow skipping dates near ephemeris markers
	void setFlagEphemerisSkipData(bool b);
	//! Get the current value of the flag which allow skipping dates near ephemeris markers
	bool getFlagEphemerisSkipData() const;

	//! Set flag which allow skipping the ephemeris markers without dates
	void setFlagEphemerisSkipMarkers(bool b);
	//! Get the current value of the flag which allow skipping the ephemeris markers without dates
	bool getFlagEphemerisSkipMarkers() const;

	//! Set flag which allow using smart format for dates near ephemeris markers
	void setFlagEphemerisSmartDates(bool b);
	//! Get the current value of the flag which allow using smart format for dates near ephemeris markers
	bool getFlagEphemerisSmartDates() const;

	//! Set flag which allow scaling the ephemeris markers
	void setFlagEphemerisScaleMarkers(bool b);
	//! Get the current value of the flag which allow scaling the ephemeris markers
	bool getFlagEphemerisScaleMarkers() const;

	//! Set the step of skip for date of ephemeris markers (and markers if it enabled)
	void setEphemerisDataStep(int step);
	//! Get the step of skip for date of ephemeris markers
	int getEphemerisDataStep() const;

	//! Set the limit for data: we computed ephemeris for 1, 2 or 5 celestial bodies
	void setEphemerisDataLimit(int limit);
	//! Get the limit of the data (how many celestial bodies was in computing of ephemeris)
	int getEphemerisDataLimit() const;

	void setEphemerisGenericMarkerColor(const Vec3f& c);
	Vec3f getEphemerisGenericMarkerColor(void) const;

	void setEphemerisSecondaryMarkerColor(const Vec3f& c);
	Vec3f getEphemerisSecondaryMarkerColor(void) const;

	void setEphemerisSelectedMarkerColor(const Vec3f& c);
	Vec3f getEphemerisSelectedMarkerColor(void) const;

	void setEphemerisMercuryMarkerColor(const Vec3f& c);
	Vec3f getEphemerisMercuryMarkerColor(void) const;

	void setEphemerisVenusMarkerColor(const Vec3f& c);
	Vec3f getEphemerisVenusMarkerColor(void) const;

	void setEphemerisMarsMarkerColor(const Vec3f& c);
	Vec3f getEphemerisMarsMarkerColor(void) const;

	void setEphemerisJupiterMarkerColor(const Vec3f& c);
	Vec3f getEphemerisJupiterMarkerColor(void) const;

	void setEphemerisSaturnMarkerColor(const Vec3f& c);
	Vec3f getEphemerisSaturnMarkerColor(void) const;

	//! Taking the JD dates for each ephemeride and preparation the human readable dates according to the settings for dates
	void fillEphemerisDates();

private:
	QSettings* conf;

	//! Draw a nice markers for ephemeris of objects.
	void drawEphemerisMarkers(const StelCore* core);

	//! Draw a line, who connected markers for ephemeris of objects.
	void drawEphemerisLine(const StelCore* core);

	Vec3f getEphemerisMarkerColor(int index) const;

	StelTextureSP texEphemerisMarker;
	StelTextureSP texEphemerisCometMarker;

	bool ephemerisMarkersDisplayed;
	bool ephemerisDatesDisplayed;
	bool ephemerisMagnitudesDisplayed;
	bool ephemerisHorizontalCoordinates;
	bool ephemerisLineDisplayed;
	int ephemerisLineThickness;
	bool ephemerisSkipDataDisplayed;
	bool ephemerisSkipMarkersDisplayed;
	int ephemerisDataStep;				// How many days skip for dates near ephemeris markers (and the markers if it enabled)
	int ephemerisDataLimit;				// Number of celestial bodies in ephemeris data (how many celestial bodies was in computing of ephemeris)
	bool ephemerisSmartDatesDisplayed;
	bool ephemerisScaleMarkersDisplayed;
	Vec3f ephemerisGenericMarkerColor;
	Vec3f ephemerisSecondaryMarkerColor;
	Vec3f ephemerisSelectedMarkerColor;
	Vec3f ephemerisMercuryMarkerColor;
	Vec3f ephemerisVenusMarkerColor;
	Vec3f ephemerisMarsMarkerColor;
	Vec3f ephemerisJupiterMarkerColor;
	Vec3f ephemerisSaturnMarkerColor;

};

#endif // EPHEMERISMGR_HPP
