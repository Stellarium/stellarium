/*
 * Copyright (C) 2013 Alexander Wolf
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

#ifndef NOVA_HPP
#define NOVA_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTranslator.hpp"

class StelPainter;

//! @class Nova
//! A Nova object represents one nova on the sky.
//! Details about the novae are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup brightNovae

class Nova : public StelObject
{
	friend class Novae;
public:
	static const QString NOVA_TYPE;

	//! @param id The official designation for a nova, e.g. "........"
	Nova(const QVariantMap& map);
	~Nova() override;

	//! Get a QVariantMap which describes the nova.  Could be used to create a duplicate.
	QVariantMap getMap(void) const;

	QString getType(void) const override
	{
		return NOVA_TYPE;
	}

	QString getObjectType(void) const override
	{
		return N_("nova");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return getDesignation();
	}

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getMap().
	// TODO: Describe the entries!
	//! - designation
	//! - name
	//! - nova-type
	//! - max-magnitude
	//! - min-magnitude
	//! - peakJD
	//! - m2
	//! - m3
	//! - m6
	//! - m9
	//! - distance
	QVariantMap getInfoMap(const StelCore *core) const override;
	Vec3f getInfoColor(void) const override;
	Vec3d getJ2000EquatorialPos(const StelCore *core) const override;
	float getVMagnitude(const StelCore* core) const override;
	QString getNameI18n(void) const override;
	QString getEnglishName(void) const override;
	QString getDesignation(void) const;

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position

	void draw(StelCore* core, StelPainter* painter);

	// Nova
	QString designation;		//! The ID of the nova
	QString novaName;		//! Name of the nova
	QString novaType;		//! Type of the nova
	float maxMagnitude;		//! Maximal visual magnitude
	float minMagnitude;		//! Minimal visual magnitude
	double peakJD;			//! Julian Day of max. vis. mag.
	int m2;				//! Time to decline by 2mag from maximum
	int m3;				//! Time to decline by 3mag from maximum
	int m6;				//! Time to decline by 6mag from maximum
	int m9;				//! Time to decline by 9mag from maximum
	double RA;			//! R.A. for the nova
	double Dec;			//! Dec. for the nova
	double distance;		//! Distance to nova (10^3 ly)

	static bool syncShowLabels;

	QString getMaxBrightnessDate(const StelCore *core, const double JD) const;
};

#endif // NOVA_HPP
