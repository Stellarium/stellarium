/*
 * Copyright (C) 2011, 2018 Alexander Wolf
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

#ifndef QUASAR_HPP
#define QUASAR_HPP

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

//! @class Quasar
//! A Quasar object represents one Quasar on the sky.
//! Details about the Quasars are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup quasars

class Quasar : public StelObject
{
	friend class Quasars;
public:
	static const QString QUASAR_TYPE;

	//! @param id The official designation for a quasar, e.g. "RXS J00066+4342"
	Quasar(const QVariantMap& map);
	~Quasar() override;

	//! Get a QVariantMap which describes the Quasar.  Could be used to create a duplicate.
	//! - designation
	//! - Vmag
	//! - Amag
	//! - bV
	//! - RA
	//! - DE
	//! - z
	//! - f6
	//! - f20
	//! - sclass
	QVariantMap getMap(void) const;

	QString getType(void) const override
	{
		return QUASAR_TYPE;
	}

	QString getObjectType(void) const override
	{
		return N_("quasar");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return designation;
	}

	float getSelectPriority(const StelCore *core) const override;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getMap().
	// TODO: Describe the fields.
	//! - amag
	//! - bV
	//! - redshift
	QVariantMap getInfoMap(const StelCore *core) const override;
	Vec3f getInfoColor(void) const override;
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	float getVMagnitude(const StelCore* core) const override;
	QString getNameI18n(void) const override
	{
		return designation;
	}
	QString getEnglishName(void) const override
	{
		return designation;
	}

private:
	bool initialized;
	float shiftVisibility;

	Vec3d XYZ;                         // holds J2000 position

	static StelTextureSP hintTexture;
	static StelTextureSP markerTexture;
	static bool distributionMode;
	static bool useMarkers;
	static Vec3f markerColor;

	void draw(StelCore* core, StelPainter& painter);
	//! Calculate a color of quasar
	//! @param b_v value of B-V color index
	unsigned char BvToColorIndex(float b_v);

	// Quasar
	QString designation;		//! The ID of the quasar
	float VMagnitude;		//! Visual magnitude
	float AMagnitude;		//! Absolute magnitude
	float bV;			//! B-V color index
	double qRA;			//! R.A. J2000 for the quasar
	double qDE;			//! Dec. J2000 for the quasar
	float redshift;			//! Distance to quasar (redshift)
	float f6;			//! Radio flux density around 5GHz (6cm)
	float f20;			//! Radio flux density around 1.4GHz (21cm)
	QString sclass;			//! Spectrum classification
};

#endif // QUASAR_HPP
