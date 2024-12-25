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

#ifndef SUPERNOVA_HPP
#define SUPERNOVA_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelTranslator.hpp"

class StelPainter;

//! @class Supernova
//! A Supernova object represents one supernova on the sky.
//! Details about the supernovas are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup historicalSupernovae

class Supernova : public StelObject
{
	friend class Supernovae;
public:
	static const QString SUPERNOVA_TYPE;

	//! @param id The official designation for a supernova, e.g. "SN 1054A"
	Supernova(const QVariantMap& map);
	~Supernova() override;

	//! Get a QVariantMap which describes the supernova.  Could be used to create a duplicate.
	//! - designation
	//! - sntype
	//! - maxMagnitude
	//! - peakJD
	//! - snra
	//! - snde
	//! - note
	//! - distance
	QVariantMap getMap(void) const;

	QString getType(void) const override
	{
		return SUPERNOVA_TYPE;
	}

	QString getObjectType(void) const override
	{
		return N_("supernova");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return designation;
	}

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getMap().
	//! - sntype
	//! - max-magnitude
	//! - peakJD
	//! - note
	//! - distance
	QVariantMap getInfoMap(const StelCore *core) const override;
	Vec3f getInfoColor(void) const override;
	Vec3d getJ2000EquatorialPos(const StelCore *core) const override;
	float getVMagnitude(const StelCore* core) const override;
	QString getNameI18n(void) const override;
	QString getEnglishName(void) const override;

protected:
	QString getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals=1, const float& magOffset=0.f) const override;

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position

	static StelTextureSP hintTexture;

	void draw(StelCore* core, StelPainter& painter);

	// Supernova
	QString designation;               //! The ID of the supernova
	QString sntype;			   //! Type of the supernova
	double maxMagnitude;		   //! Maximal visual magnitude
	double peakJD;			   //! Julian Day of max. vis. mag.
	double snra;			   //! R.A. for the supernova
	double snde;			   //! Dec. for the supernova
	QString note;			   //! Notes for the supernova
	double distance;		   //! Distance to supernova (10^3 ly)

	static bool syncShowLabels;

	QString getMaxBrightnessDate(const StelCore *core, const double JD) const;
};

#endif // SUPERNOVA_HPP
