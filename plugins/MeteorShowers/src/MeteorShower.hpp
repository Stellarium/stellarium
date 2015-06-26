/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013 Marcos Cardinot
 * Copyright (C) 2011 Alexander Wolf
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
	friend class MeteorShowers;

public:
	enum RadiantStatus {
		INACTIVE,        // inactive radiant.
		ACTIVE_REAL,     // active radiant - real data.
		ACTIVE_GENERIC   // active radiant - generic data.
	};

	//! @param id The official ID designation for a meteor shower, e.g. "LYR"
	MeteorShower(const QVariantMap& map);
	~MeteorShower();

	//! Get a QVariantMap which describes the meteor shower. Could be used to
	//! create a duplicate.
	QVariantMap getMap();

	virtual QString getType(void) const
	{
		return "MeteorShower";
	}
	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
	virtual double getAngularSize(const StelCore* core) const;
	virtual QString getNameI18n(void) const
	{
		return q_(designation.trimmed());
	}
	virtual QString getEnglishName(void) const
	{
		return designation.trimmed();
	}
	QString getDesignation(void) const;
	void update(double deltaTime);
	static bool showLabels;

	//! Get current activity status of MS
	//! @return 0:inactive 1:activeRealData 2:activeGenericData
	int getStatus() { return status; }

	//! Get peak
	//! @return peak
	QDateTime getPeak()
	{
		return peak;
	}

	//! Get zhr
	//! @return ZHR
	int getZHR()
	{
		return zhr;
	}

private:
	Vec3d XYZ;                      //! Cartesian equatorial position
	Vec3d XY;                       //! Store temporary 2D position

	static StelTextureSP radiantTexture;
	static bool radiantMarkerEnabled;
	static bool showActiveRadiantsOnly;

	typedef struct
	{
		QString year;		   //! Value of year for actual data
		int zhr;		   //! ZHR of shower
		QString variable;	   //! value of variable for ZHR
		QString start;		   //! First day for activity
		QString finish;		   //! Latest day for activity
		QString peak;		   //! Day with maximum for activity
	} activityData;

	bool initialized;
	bool active;

	QList<MeteorObj*> activeMeteors; //! List of active meteors

	QString showerID;		//! The ID of the meteor shower
	QString designation;            //! The designation of the meteor shower
	QList<activityData> activity;	//! List of activity
	int speed;                      //! Speed of meteors
	float rAlphaPeak;               //! R.A. for radiant of meteor shower on the peak day
	float rDeltaPeak;               //! Dec. for radiant of meteor shower on the peak day
	float driftAlpha;		//! Drift of R.A.
	float driftDelta;		//! Drift of Dec.
	QString parentObj;		//! Parent object for meteor shower
	float pidx;			//! The population index
	QList<Meteor::colorPair> colors;//! <colorName, 0-100>

	//current information
	double radiantAlpha;            //! Current R.A. for radiant of meteor shower
	double radiantDelta;            //! Current Dec. for radiant of meteor shower
	int zhr;			//! ZHR of shower
	QString variable;		//! value of variable for ZHR
	QDateTime start;		//! First day for activity
	QDateTime finish;		//! Latest day for activity
	QDateTime peak;			//! Day with maximum for activity

	int status;		        //! Check if the radiant is active for the current sky date
					//! 0=inactive; 1=realData 2=genericData

	void draw(StelCore *core);

	//! Calculate value of ZHR using normal distribution
	//! @param zhr
	//! @param variable
	//! @param start
	//! @param finish
	//! @param peak
	int calculateZHR(int zhr, QString variable, QDateTime start, QDateTime finish, QDateTime peak);

	//! Get a date string from JSON file and parse it for display in info corner
	//! @param jsondate A string from JSON file
	QString getDateFromJSON(QString jsondate) const;

	//! Get a day from JSON file and parse it for display in info corner
	//! @param jsondate A string from JSON file
	QString getDayFromJSON(QString jsondate) const;

	//! Get a month string from JSON file and parse it for display in info corner
	//! @param jsondate A string from JSON file
	int getMonthFromJSON(QString jsondate) const;

	//! Get a month string from JSON file and parse it for display in info corner
	//! @param jsondate A string from JSON file
	QString getMonthNameFromJSON(QString jsondate) const;

	//! Get a month name from month number
	//! @param jsondate A string from JSON file
	QString getMonthName(int number) const;

	//! Get the current sky QDateTime
	//! @return Current QDateTime of sky
	QDateTime getSkyQDateTime() const;

	//! Update value of current information(zhr, variable, stat, finish and peak)
	//! @param current sky QDateTime
	void updateCurrentData(QDateTime skyDate);

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
