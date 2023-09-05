/*
 * Copyright (C) 2023 Alexander Wolf
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

#ifndef MISSINGSTAR_HPP
#define MISSINGSTAR_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelFader.hpp"
#include "StelTranslator.hpp"

class StelPainter;

//! @class MissingStar
//! A Missing Star object represents one missing star on the sky.
//! Details about the missing stars are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup missingStars

class MissingStar : public StelObject
{
	friend class MissingStars;
public:
	static const QString MISSINGSTAR_TYPE;

	//! @param id The official designation for a missing star
	MissingStar(const QVariantMap& map);
	~MissingStar() Q_DECL_OVERRIDE;

	//! Get a QVariantMap which describes the missing star. Could be used to create a duplicate.
	//! - designation
	//! - RA
	//! - DEC
	//! - pmRA
	//! - pmDEC
	//! - bMag
	//! - vMag
	//! - parallax
	QVariantMap getMap(void) const;

	virtual QString getType(void) const Q_DECL_OVERRIDE
	{
		return MISSINGSTAR_TYPE;
	}

	virtual QString getObjectType(void) const Q_DECL_OVERRIDE
	{
		return N_("star");
	}
	virtual QString getObjectTypeI18n(void) const Q_DECL_OVERRIDE
	{
		return q_(getObjectType());
	}

	virtual QString getID(void) const Q_DECL_OVERRIDE
	{
		return designation;
	}

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getMap().
	//virtual QVariantMap getInfoMap(const StelCore *core) const Q_DECL_OVERRIDE;
	virtual Vec3f getInfoColor(void) const Q_DECL_OVERRIDE;
	virtual Vec3d getJ2000EquatorialPos(const StelCore *core) const Q_DECL_OVERRIDE;
	virtual float getVMagnitude(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual QString getNameI18n(void) const Q_DECL_OVERRIDE;
	virtual QString getEnglishName(void) const Q_DECL_OVERRIDE;

	void update(double deltaTime);

//protected:
//	virtual QString getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals=1) const Q_DECL_OVERRIDE;

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position

	static StelTextureSP hintTexture;

	void draw(StelCore* core, StelPainter& painter);

	// Missing Star
	QString designation;               //! The ID of the missing star
	double RA;			   //! R.A. for the missing star
	double DEC;			   //! Dec. for the missing star
	float pmRA;			   //! proper motion in R.A. for the missing star (mas/yr)
	float pmDEC;			   //! proper motion in Dec. for the missing star (mas/yr)
	float bMag;			   //! B magnitude for the missing star
	float vMag;			   //! V magnitude for the missing star
	int colorIndex;

	LinearFader labelsFader;
};

#endif // MISSINGSTAR_HPP
