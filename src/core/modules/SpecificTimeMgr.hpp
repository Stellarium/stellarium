/*
 * Copyright (C) 2022 Alexander Wolf
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

#ifndef SPECIFICTIMEMGR_HPP
#define SPECIFICTIMEMGR_HPP

#include "SolarSystem.hpp"
#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"

#include <QList>

class StelPainter;
class QSettings;

class SpecificTimeMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(double twilightAltitude READ getTwilightAltitude WRITE setTwilightAltitude NOTIFY twilightAltitudeChanged)

public:
	SpecificTimeMgr();
	virtual ~SpecificTimeMgr() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	//! Get twilight altitude [degrees]
	double getTwilightAltitude() const {return twilightAltitude;}

public slots:
	//! Set twilight altitude [degrees]
	void setTwilightAltitude(double alt);


	//! Set simulation time to the time of next transit of selected object
	void nextTransit();
	//! Set simulation time to the time of previous transit of selected object
	void previousTransit();
	//! Set simulation time to the time of today's transit of selected object
	void todayTransit();

	//! Set simulation time to the time of next rising of selected object (if applicable)
	//! @note for circumpolar objects, this sets to time of lower culmination
	//! @note for permanently invisible objects, this sets to time of transit
	void nextRising();
	//! Set simulation time to the time of previous rising of selected object (if applicable)
	//! @note for circumpolar objects, this sets to time of lower culmination
	//! @note for permanently invisible objects, this sets to time of transit
	void previousRising();
	//! Set simulation time to the time of today's rising of selected object (if applicable)
	//! @note for circumpolar objects, this sets to time of lower culmination
	//! @note for permanently invisible objects, this sets to time of transit
	void todayRising();

	//! Set simulation time to the time of next setting of selected object (if applicable)
	//! @note for circumpolar objects, this sets to time of lower culmination
	//! @note for permanently invisible objects, this sets to time of transit
	void nextSetting();
	//! Set simulation time to the time of previous setting of selected object (if applicable)
	//! @note for circumpolar objects, this sets to time of lower culmination
	//! @note for permanently invisible objects, this sets to time of transit
	void previousSetting();
	//! Set simulation time to the time of today's setting of selected object (if applicable)
	//! @note for circumpolar objects, this sets to time of lower culmination
	//! @note for permanently invisible objects, this sets to time of transit
	void todaySetting();

	//! Set simulation time to this day's morning when Sun reaches twilightAltitude
	void todayMorningTwilight();
	//! Set simulation time to this day's evening when Sun reaches twilightAltitude
	void todayEveningTwilight();
	//! Set simulation time to the previous day's morning when Sun reaches twilightAltitude
	void previousMorningTwilight();
	//! Set simulation time to the previous day's evening when Sun reaches twilightAltitude
	void previousEveningTwilight();
	//! Set simulation time to the next day's morning when Sun reaches twilightAltitude
	void nextMorningTwilight();
	//! Set simulation time to the next day's evening when Sun reaches twilightAltitude
	void nextEveningTwilight();

	//! Set simulation time to this day's morning when selected object reaches current altitude
	void todayMorningAtAltitude();
	//! Set simulation time to the next morning when selected object reaches current altitude
	void nextMorningAtAltitude();
	//! Set simulation time to the previous morning when selected object reaches current altitude
	void previousMorningAtAltitude();
	//! Set simulation time to this day's evening when selected object reaches current altitude
	void todayEveningAtAltitude();
	//! Set simulation time to the next evening when selected object reaches current altitude
	void nextEveningAtAltitude();
	//! Set simulation time to the previous evening when selected object reaches current altitude
	void previousEveningAtAltitude();

signals:
	//! Signal that the configurable twilight altitude for the sun has changed.
	void twilightAltitudeChanged(double alt);

private:
	QSettings* conf;
	StelCore* core;
	StelObjectMgr* objMgr;
	PlanetP sun;

	//! configurable altitude for the sun for "goto next twilight" actions
	double twilightAltitude;
};

#endif /* SPECIFICTIMEMGR_HPP */
