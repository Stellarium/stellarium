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

#include "StelCore.hpp"
#include "StelTranslator.hpp"
#include "StelObject.hpp"
#include "StelTextureTypes.hpp"

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
	~Pulsar() Q_DECL_OVERRIDE;

	//! Get a QVariantMap which describes the pulsar. Could be used to create a duplicate.
	//! - designation: pulsar name based on J2000 coordinates
	//! - bdesignation: pulsar name based on B1950 coordinates
	//! - parallax: annual parallax (mas)
	//! - bperiod: binary period of pulsar (days)
	//! - frequency: barycentric rotation frequency (Hz)
	//! - pfrequency: time derivative of barycentric rotation frequency (s^-2)
	//! - pderivative: time derivative of barcycentric period (dimensionless)
	//! - dmeasure: dispersion measure (pc/cm^3)
	//! - eccentricity
	//! - RA: right ascension (J2000) (hh:mm:ss.s)
	//! - DE: declination (J2000) (+dd:mm:ss)
	//! - pmRA: proper motion in the right ascension direction (mas/yr)
	//! - pmDE: proper motion in declination (mas/yr)
	//! - period: barycentric period of the pulsar (s)
	//! - w50: width of pulse at 50% of peak (ms). Note, pulse widths are a function of both observing frequency and observational time resolution, so quoted widths are indicative only.
	//! - s400: mean flux density at 400 MHz (mJy)
	//! - s600: mean flux density at 1400 MHz (mJy)
	//! - s1400: mean flux density at 2000 MHz (mJy)
	//! - distance: best estimate of the pulsar distance using the YMW16 DM-based distance as default (kpc)
	//! - glitch: number of glitches
	//! - notes: pulsar types
	QVariantMap getMap(void) const;

	//! Get the type of object
	virtual QString getType(void) const Q_DECL_OVERRIDE
	{
		return PULSAR_TYPE;
	}

	//! Get the type of object
	virtual QString getObjectType(void) const Q_DECL_OVERRIDE
	{
		return (glitch==0) ? N_("pulsar") : N_("pulsar with glitches");
	}
	virtual QString getObjectTypeI18n(void) const Q_DECL_OVERRIDE
	{
		return q_(getObjectType());
	}

	virtual QString getID(void) const Q_DECL_OVERRIDE
	{
		return designation;
	}

	virtual float getSelectPriority(const StelCore* core) const Q_DECL_OVERRIDE;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getMap(), except for designation, RA and DE fields.
	virtual QVariantMap getInfoMap(const StelCore *core) const Q_DECL_OVERRIDE;
	virtual Vec3f getInfoColor(void) const Q_DECL_OVERRIDE;
	virtual Vec3d getJ2000EquatorialPos(const StelCore* core) const Q_DECL_OVERRIDE;
	//! Get the visual magnitude of pulsar
	virtual float getVMagnitude(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual float getVMagnitudeWithExtinction(const StelCore *core) const;
	//! Get the localized name of pulsar
	virtual QString getNameI18n(void) const Q_DECL_OVERRIDE;
	//! Get the english name of pulsar
	virtual QString getEnglishName(void) const Q_DECL_OVERRIDE;
	//! Get the designation of pulsar (based on J2000 coordinates)
	QString getDesignation(void) const { return designation; }
	//! Get the designation of pulsar (based on B1950 coordinates)
	QString getBDesignation(void) const { return bdesignation; }

private:
	bool initialized;

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
	QString designation;	//! The designation of the pulsar (based on J2000 coordinates)
	QString bdesignation;	//! The designation of the pulsar (based on B1550 coordinates)
	QString pulsarName;	//! The proper name of the pulsar
	double RA;		//! J2000 right ascension
	double DE;		//! J2000 declination
	double pmRA;		//! proper motion by right ascension (mas/yr)
	double pmDE;		//! proper motion by declination (mas/yr)
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
