/*
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

#ifndef _SUPERNOVA_HPP_
#define _SUPERNOVA_HPP_ 1

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelFader.hpp"
#include "StelProjectorType.hpp"


//! @class Supernova
//! A Supernova object represents one supernova on the sky.
//! Details about the supernovas are passed using a QVariant which contains
//! a map of data from the json file.

class Supernova : public StelObject
{
	friend class Supernovae;
public:
	//! @param id The official designation for a supernova, e.g. "SN 1054A"
	Supernova(const QVariantMap& map);
	~Supernova();

	//! Get a QVariantMap which describes the supernova.  Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	virtual QString getType(void) const
	{
		return "Supernova";
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
	virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;
	virtual double getAngularSize(const StelCore* core) const;
	virtual QString getNameI18n(void) const
	{
		return designation;
	}
	virtual QString getEnglishName(void) const
	{
		return designation;
	}

	void update(double deltaTime);

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position

	void draw(StelCore* core, class StelRenderer* renderer, StelProjectorP projector);

	// Supernova
	QString designation;               //! The ID of the supernova
	QString sntype;			   //! Type of the supernova
	float maxMagnitude;		   //! Maximal visual magnitude
	double peakJD;			   //! Julian Day of max. vis. mag.
	double snra;			   //! R.A. for the supernova
	double snde;			   //! Dec. for the supernova
	QString note;			   //! Notes for the supernova
	double distance;		   //! Distance to supernova (10^3 ly)

	LinearFader labelsFader;
};

#endif // _SUPERNOVA_HPP_
