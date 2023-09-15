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
#include "StelObject.hpp"
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
		double start;              //! Initial solar longitude (J2000) of activity
		double finish;             //! Last solar longitude (J2000) of activity
		double peak;               //! Peak solar longitude (J2000) of activity
		int disttype;              //! Distribution type (0 for Gauss, 1 for Lorentz)
		float b1;                  //! B slope before peak
		float b2;                  //! B slope after peak
	};

	//! Constructor
	//! @param map QVariantMap containing all the data about a Meteor Shower.
	MeteorShower(MeteorShowersMgr* mgr, const QVariantMap& map);

	//! Destructor
	~MeteorShower() Q_DECL_OVERRIDE;

	//! Update
	//! @param deltaTime the time increment in seconds since the last call.
	void update(StelCore *core, double deltaTime);

	//! Draw
	void draw(StelCore *core);

	//! Checks if we have generic data for a given date
	//! @param solLong the Solar longitude (J2000)
	//! @return Activity
	Activity hasGenericShower(double solLong, bool &found) const;

	//! Checks if we have confirmed data for a given date
	//! @param solLong the Solar longitude (J2000)
	//! @return Activity
	Activity hasConfirmedShower(double solLong, bool &found) const;

	//! Checks if this meteor shower is being displayed or not
	//! @return true if it's being displayed
	bool enabled() const;

	//! Gets the meteor shower id
	//! //! @return designation
	QString getDesignation() const;

	//! Gets the current meteor shower status
	//! @return status
	Status getStatus() { return m_status; }

	//! Gets the current ZHR
	//! @return ZHR
	int getZHR() { return m_activity.zhr; }

	//
	// Methods defined in StelObject class
	//
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;

	//! Return a map like StelObject, but with a few extra tags:
	// TODO: Describe the fields!
	//! - status
	//! - id
	//! - type (translated string "meteor shower")
	//! - speed (km/s)
	//! - pop-idx (population index)
	//! - parent
	//! - zhr-max (information string)
	virtual QVariantMap getInfoMap(const StelCore *core) const Q_DECL_OVERRIDE;
	virtual QString getType(void) const Q_DECL_OVERRIDE { return METEORSHOWER_TYPE; }
	virtual QString getObjectType(void) const Q_DECL_OVERRIDE { return N_("meteor shower"); }
	virtual QString getObjectTypeI18n(void) const Q_DECL_OVERRIDE { return q_(getObjectType()); }
	virtual QString getID(void) const Q_DECL_OVERRIDE { return m_showerID; }
	virtual QString getEnglishName(void) const Q_DECL_OVERRIDE { return m_designation.trimmed(); }
	virtual QString getNameI18n(void) const Q_DECL_OVERRIDE	{ return q_(m_designation.trimmed()); }
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const Q_DECL_OVERRIDE { return m_position; }
	virtual float getSelectPriority(const StelCore*) const Q_DECL_OVERRIDE { return -4.0; }
	virtual Vec3f getInfoColor(void) const Q_DECL_OVERRIDE;

	//! @return approximate Julian day calculated from solar longitude (J2000)
	static double JDfromSolarLongitude(double solarLong, int year);

private:
	MeteorShowersMgr* m_mgr;           //! MeteorShowersMgr instance
	Status m_status;                   //! Meteor shower status

	// data from catalog
	QString m_showerID;                //! The ID of the meteor shower
	QString m_designation;             //! The designation of the meteor shower
	QString m_IAUNumber;               //! IAU Number of the meteor shower
	QList<Activity> m_activities;      //! Activity list
	int m_speed;                       //! Speed of meteors
	float m_rAlphaPeak;                //! R.A. for radiant of meteor shower on the peak day
	float m_rDeltaPeak;                //! Dec. for radiant of meteor shower on the peak day
	float m_driftAlpha;                //! Drift of R.A. for each degree of solar longitude from peak
	float m_driftDelta;                //! Drift of Dec. for each degree of solar longitude from peak
	QString m_parentObj;               //! Parent object for meteor shower
	float m_pidx;                      //! The population index
	QList<Meteor::ColorPair> m_colors; //! <colorName, 0-100>

	//current information
	Vec3d m_position;                  //! Cartesian equatorial position
	double m_radiantAlpha;             //! Current R.A. for radiant of meteor shower
	double m_radiantDelta;             //! Current Dec. for radiant of meteor shower
	Activity m_activity;               //! Current activity

	QList<MeteorObj*> m_activeMeteors; //! List with all the active meteors

	double eclJ2000;                   //! Ecliptic on J2000.0 epoch

	//! Draws the radiant
	void drawRadiant(StelCore* core);

	//! Draws all active meteors
	void drawMeteors(StelCore* core);

	//! Calculates the ZHR using two types of distribution function
	int calculateZHR(StelCore* core);
};

#endif /* METEORSHOWER_HPP */
