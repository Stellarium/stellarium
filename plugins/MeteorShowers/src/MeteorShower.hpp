/*
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
#define _METEORSHOWER_HPP_ 1

#include <QVariantMap>
#include <QString>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelPainter.hpp"
#include "StelFader.hpp"
#include "StelTranslator.hpp"

class StelPainter;

//! @class MeteorShower
//! A MeteorShower object represents one meteor shower on the sky.
//! Details about the meteor showers are passed using a QVariant which contains
//! a map of data from the json file.

class MeteorShower : public StelObject
{
	friend class MeteorShowers;

public:
	//! @param id The official ID designation for a meteor shower, e.g. "LYR"
	MeteorShower(const QVariantMap& map);
	~MeteorShower();

	//! Get a QVariantMap which describes the meteor shower. Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

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
		return q_(designation);
	}
	virtual QString getEnglishName(void) const
	{
		return designation;
	}
    QString getDesignation(void) const;
	void update(double deltaTime);

private:
	Vec3d XYZ;                      // Cartesian equatorial position
	Vec3d XY;                       // Store temporary 2D position

    static StelTextureSP radiantTexture;          //white
    static StelTextureSP radiantActiveTexture;    //yellow
    static StelTextureSP radiantActiveTexture2;   //blue

    LinearFader labelsFader;

    typedef struct
    {
        QString year;		   //! Value of year for actual data
        int zhr;			   //! ZHR of shower
        QString variable;      //! value of variable for ZHR
        QString start;		   //! First day for activity
        QString finish;		   //! Latest day for activity
        QString peak;		   //! Day with maximum for activity
    } activityData;

    bool initialized;
    QString showerID;		        //! The ID of the meteor shower
    QString designation;            //! The designation of the meteor shower
    QList<activityData> activity;	//! List of activity
    int speed;                      //! Speed of meteors
    double radiantAlpha;            //! R.A. for radiant of meteor shower
    double radiantDelta;            //! Dec. for radiant of meteor shower
    double driftAlpha;		   //! Drift of R.A.
    double driftDelta;		   //! Drift of Dec.
    QString parentObj;		   //! Parent object for meteor shower
    float pidx;			       //! The population index
    float slong;			   //! Solar longitude

	void draw(StelPainter &painter);

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

    //! Check if the radiant is active for the current sky date
    //! @return if is active, return 1 to real data OR 2 to generic data
    int checkActiveDate() const;

    //! Check if the JSON file has real data to a given year
    //! @param yyyy year to check
    //! @return index of the year or 0 to generic data
    int checkYear(QString yyyy) const;
};

#endif // _METEORSHOWER_HPP_
