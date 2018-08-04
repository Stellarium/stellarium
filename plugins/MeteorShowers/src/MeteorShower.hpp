/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#ifndef METEORSHOWER_HPP
#define METEORSHOWER_HPP

#include "MeteorObj.hpp"
#include "MeteorShowersMgr.hpp"
#include "StelFader.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelTextureTypes.hpp"
#include "StelTranslator.hpp"

//! @class MeteorShower
//! A MeteorShower object represents one meteor shower on the sky.
//! Details about the meteor showers are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup meteorShowers

class MeteorShower : public StelObject
{
public:
	static const QString METEORSHOWER_TYPE;

	//! @enum Meteor Shower status.
	enum Status {
		INVALID,          // not initialized properly
		UNDEFINED,        // it's loaded but with 'activity' undefined
		INACTIVE,         // inactive radiant
		ACTIVE_CONFIRMED, // active radiant - confirmed data
		ACTIVE_GENERIC    // active radiant - generic data
	};

	//! @struct Activity
	struct Activity
	{
		int year;                  //! The catalog year (0 for generic)
		int zhr;                   //! The ZHR on peak
		QList<int> variable;       //! The ZHR range when it's variable
		QDate start;               //! Initial date of activity
		QDate finish;              //! Last date of activity
		QDate peak;                //! Peak activity
	};

	//! Constructor
	//! @param map QVariantMap containing all the data about a Meteor Shower.
	MeteorShower(MeteorShowersMgr* mgr, const QVariantMap& map);

	//! Destructor
	~MeteorShower();

	//! Update
	//! @param deltaTime the time increment in seconds since the last call.
	void update(StelCore *core, double deltaTime);

	//! Draw
	void draw(StelCore *core);

	//! Checks if we have generic data for a given date
	//! @param date QDate
	//! @return Activity
	Activity hasGenericShower(QDate date, bool &found) const;

	//! Checks if we have confirmed data for a given date
	//! @param date QDate
	//! @return Activity
	Activity hasConfirmedShower(QDate date, bool &found) const;

	//! Checks if this meteor shower is being displayed or not
	//! @return true if it's being displayed
	bool enabled() const;

	//! Gets the meteor shower id
	//! //! @return designation
	QString getDesignation() const;

	//! Gets the current meteor shower status
	//! @return status
	Status getStatus() { return m_status; }

	//! Gets the peak
	//! @return peak
	QDate getPeak() { return m_activity.peak; }

	//! Gets the current ZHR
	//! @return ZHR
	int getZHR() { return m_activity.zhr; }

	//
	// Methods defined in StelObject class
	//
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;

	//! Return a map like StelObject, but with a few extra tags:
	// TODO: Describe the fields!
	//! - status
	//! - id
	//! - type (translated string "meteor shower")
	//! - speed (km/s)
	//! - pop-idx (population index)
	//! - parent
	//! - zhr-max (information string)
	virtual QVariantMap getInfoMap(const StelCore *core) const;
	virtual QString getType(void) const { return METEORSHOWER_TYPE; }
	virtual QString getID(void) const { return m_showerID; }
	virtual QString getEnglishName(void) const { return m_designation.trimmed(); }
	virtual QString getNameI18n(void) const	{ return q_(m_designation.trimmed()); }
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const { return m_position; }
	virtual float getSelectPriority(const StelCore*) const { return -4.0; }
	virtual Vec3f getInfoColor(void) const;
	virtual double getAngularSize(const StelCore*) const { return 0.001; }

private:
	MeteorShowersMgr* m_mgr;           //! MeteorShowersMgr instance
	Status m_status;                   //! Meteor shower status

	// data from catalog
	QString m_showerID;                //! The ID of the meteor shower
	QString m_designation;             //! The designation of the meteor shower
	QList<Activity> m_activities;      //! Activity list
	int m_speed;                       //! Speed of meteors
	float m_rAlphaPeak;                //! R.A. for radiant of meteor shower on the peak day
	float m_rDeltaPeak;                //! Dec. for radiant of meteor shower on the peak day
	float m_driftAlpha;                //! Drift of R.A. for each day from peak
	float m_driftDelta;                //! Drift of Dec. for each day from peak
	QString m_parentObj;               //! Parent object for meteor shower
	float m_pidx;                      //! The population index
	QList<Meteor::ColorPair> m_colors; //! <colorName, 0-100>

	//current information
	Vec3d m_position;                  //! Cartesian equatorial position
	double m_radiantAlpha;             //! Current R.A. for radiant of meteor shower
	double m_radiantDelta;             //! Current Dec. for radiant of meteor shower
	Activity m_activity;               //! Current activity

	QList<MeteorObj*> m_activeMeteors; //! List with all the active meteors

	//! Draws the radiant
	void drawRadiant(StelCore* core);

	//! Draws all active meteors
	void drawMeteors(StelCore* core);

	//! Calculates the ZHR using normal distribution
	//! @param current julian day
	int calculateZHR(const double& currentJD);

	//! Gets the mean solar longitude for a specified date (approximate formula)
	//! @param date QDate
	//! @return solar longitude in degree
	static QString getSolarLongitude(QDate date);
};

#endif /* METEORSHOWER_HPP */
