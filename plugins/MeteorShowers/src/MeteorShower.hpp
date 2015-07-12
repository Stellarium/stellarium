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

#ifndef _METEORSHOWER_HPP_
#define _METEORSHOWER_HPP_

#include <QDateTime>

#include "MeteorObj.hpp"
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
	//! @enum Meteor Shower status.
	enum Status {
		INVALID,         // not initialized properly
		INACTIVE,        // inactive radiant
		ACTIVE_REAL,     // active radiant - real data
		ACTIVE_GENERIC   // active radiant - generic data
	};

	//! Constructor
	//! @param map QVariantMap containing all the data about a Meteor Shower.
	MeteorShower(const QVariantMap& map);

	//! Destructor
	~MeteorShower();

	//! Update
	void update(double deltaTime);

	//! Draw
	void draw(StelCore *core);

	//! Update value of current information(zhr, variable, stat, finish and peak)
	//! @param current sky QDateTime
	void updateCurrentData(QDateTime skyDate);

	bool enabled() { return m_enabled; }

	//! Gets a QVariantMap which describes the meteor shower.
	//! Could be used to create a duplicate.
	QVariantMap getMap();

	//! Gets the meteor shower id
	//! //! @return designation
	QString getDesignation() const;

	//! Gets the current meteor shower status
	//! @return status
	Status getStatus() { return m_status; }

	//! Gets the peak
	//! @return peak
	QDateTime getPeak() { return m_peak; }

	//! Gets the current ZHR
	//! @return ZHR
	int getZHR() { return m_zhr; }

	//
	// Methods defined in StelObject class
	//
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual QString getType(void) const { return "MeteorShower"; }
	virtual QString getEnglishName(void) const { return m_designation.trimmed(); }
	virtual QString getNameI18n(void) const	{ return q_(m_designation.trimmed()); }
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const { return m_position; }
	virtual float getSelectPriority(const StelCore* core) const { return -4.0; }
	virtual Vec3f getInfoColor(void) const;
	virtual double getAngularSize(const StelCore* core) const { return 0.001; }

private:
	typedef struct
	{
		QString year;              //! Value of year for actual data
		int zhr;                   //! The ZHR on peak
		QString variable;          //! The ZHR range when it's variable
		QString start;             //! First day for activity
		QString finish;            //! Latest day for activity
		QString peak;              //! Day with maximum for activity
	} Activity;

	Status m_status;                 //! Meteor shower status
	bool m_enabled;                    //! True if the meteor shower is being displayed

	// data from catalog
	QString m_showerID;                //! The ID of the meteor shower
	QString m_designation;             //! The designation of the meteor shower
	QList<Activity> m_activity;    //! Activity list
	int m_speed;                       //! Speed of meteors
	float m_rAlphaPeak;                //! R.A. for radiant of meteor shower on the peak day
	float m_rDeltaPeak;                //! Dec. for radiant of meteor shower on the peak day
	float m_driftAlpha;                //! Drift of R.A.
	float m_driftDelta;                //! Drift of Dec.
	QString m_parentObj;               //! Parent object for meteor shower
	float m_pidx;                      //! The population index
	QList<Meteor::colorPair> m_colors; //! <colorName, 0-100>

	//current information
	Vec3d m_position;                  //! Cartesian equatorial position
	double m_radiantAlpha;             //! Current R.A. for radiant of meteor shower
	double m_radiantDelta;             //! Current Dec. for radiant of meteor shower
	int m_zhr;                         //! ZHR of shower
	QString m_variable;                //! value of variable for ZHR
	QDateTime m_start;                 //! First day for activity
	QDateTime m_finish;                //! Latest day for activity
	QDateTime m_peak;                  //! Day with maximum for activity

	QList<MeteorObj*> m_activeMeteors; //! List with all the active meteors

	//! Calculate value of ZHR using normal distribution
	//! @param zhr
	//! @param variable
	//! @param start
	//! @param finish
	//! @param peak
	int calculateZHR(int zhr, QString variable, QDateTime start, QDateTime finish, QDateTime peak);

	//! Get the current sky QDateTime
	//! @return Current QDateTime of sky
	QDateTime getSkyQDateTime(StelCore *core) const;

	//! Check if the JSON file has real data to a given year
	//! @param yyyy year to check
	//! @return index of the year or 0 to generic data
	int searchRealData(QString yyyy) const;

	//! Get the solar longitude for a specified date
	//! @param QDT QDateTime
	//! @return solar longitude in degree
	float getSolarLongitude(QDateTime QDT) const;
};

#endif /*_METEORSHOWER_HPP_*/
