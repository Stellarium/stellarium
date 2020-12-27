/*
 * Copyright (C) 2012 Alexander Wolf
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

#ifndef PULSAR_HPP
#define PULSAR_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelFader.hpp"

class StelPainter;

//! @class Pulsar
//! A Pulsar object represents one pulsar on the sky.
//! Details about the Pulsars are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup pulsars

class Pulsar : public StelObject
{
	friend class Pulsars;
public:
	static const QString PULSAR_TYPE;

	//! @param id The official designation for a pulsar, e.g. "PSR J1919+21"
	Pulsar(const QVariantMap& map);
	~Pulsar();

	//! Get a QVariantMap which describes the pulsar. Could be used to create a duplicate.
	// TODO: Add proper documentation of these fields!
	//! - designation
	//! - parallax
	//! - bperiod
	//! - frequency
	//! - pfrequency
	//! - pderivative
	//! - dmeasure
	//! - eccentricity
	//! - RA
	//! - DE
	//! - period
	//! - w50
	//! - s400
	//! - s600
	//! - s1400
	//! - distance
	//! - glitch
	//! - notes
	QVariantMap getMap(void) const;

	//! Get the type of object
	virtual QString getType(void) const
	{
		return PULSAR_TYPE;
	}

	virtual QString getID(void) const
	{
		return designation;
	}

	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getMap(), except for designation, RA and DE fields.
	virtual QVariantMap getInfoMap(const StelCore *core) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
	//! Get the visual magnitude of pulsar
	virtual float getVMagnitude(const StelCore* core) const;
	virtual float getVMagnitudeWithExtinction(const StelCore *core) const;
	//! Get the angular size of pulsar
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of pulsar
	virtual QString getNameI18n(void) const;
	//! Get the english name of pulsar
	virtual QString getEnglishName(void) const;
	//! Get the designation of pulsar
	QString getDesignation(void) const { return designation; }

	void update(double deltaTime);

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position	

	static StelTextureSP hintTexture;
	static StelTextureSP markerTexture;
	static bool distributionMode;
	static bool glitchFlag;
	static bool filteredMode;
	static float filterValue;
	static Vec3f markerColor;
	static Vec3f glitchColor;

	void draw(StelCore* core, StelPainter *painter);

	//! Variables for description of properties of pulsars
	QString designation;	//! The designation of the pulsar (J2000 pulsar name)
	QString pulsarName;	//! The proper name of the pulsar
	double RA;		//! J2000 right ascension
	double DE;		//! J2000 declination
	float parallax;		//! Annual parallax (mas)
	double period;		//! Barycentric period of the pulsar (s)
	double frequency;	//! Barycentric rotation frequency (Hz)
	double pfrequency;	//! Time derivative of barycentric rotation frequency (s^-2)
	double pderivative;	//! Time derivative of barcycentric period (dimensionless)
	double dmeasure;	//! Dispersion measure (cm-3 pc)
	double bperiod;		//! Binary period of pulsar (days)
	double eccentricity;	//! Eccentricity	
	float w50;		//! Profile width at 50% of peak in ms
	float s400;		//! Time averaged flux density at 400MHz in mJy
	float s600;		//! Time averaged flux density at 600MHz in mJy
	float s1400;		//! Time averaged flux density at 1400MHz in mJy
	float distance;		//! Distance based on electron density model in kpc
	int glitch;		//! Number of glitches
	QString notes;		//! Notes to pulsar (Type of pulsar)

	LinearFader labelsFader;

	//! Calculate and get spin down energy loss rate (ergs/s)
	//! @param p0 - barycentric period of the pulsar (s)
	//! @param p1 - time derivative of barcycentric period (dimensionless)
	double getEdot(double p0, double p1) const;

	//! Calculate and get barycentric period derivative
	//! @param p0 - barycentric period of the pulsar (s)
	//! @param f1 - time derivative of barcycentric period (s^-2)
	double getP1(double p0, double f1) const;

	//! Get type of pulsar
	//! @param pcode - code of pulsar type
	QString getPulsarTypeInfoString(QString pcode) const;
};

#endif // PULSAR_HPP
