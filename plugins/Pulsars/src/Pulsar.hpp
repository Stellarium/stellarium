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

#ifndef _PULSAR_HPP_
#define _PULSAR_HPP_ 1

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelPainter.hpp"
#include "StelFader.hpp"

class StelPainter;

//! @class Pulsar
//! A Pulsar object represents one pulsar on the sky.
//! Details about the Pulsars are passed using a QVariant which contains
//! a map of data from the json file.

class Pulsar : public StelObject
{
	friend class Pulsars;
public:
	//! @enum pulsarTypeInfoGroup used as named bitfield flags as specifiers to
	//! filter results of getPulsarTypeInfoString. The precise definition of these should
	//! be documented in the getPulsarTypeInfoString documentation for the derived classes
	//! for all specifiers which are defined in that derivative.
	//! Description of types you can see here - http://cdsarc.u-strasbg.fr/viz-bin/Cat?VII/189
	enum pulsarTypeInfoGroup
	{
		C	= 0x00000001, //!< Globular cluster association
		S	= 0x00000002, //!< SNR association
		G	= 0x00000004, //!< Glitches in period
		B	= 0x00000010, //!< Binary or multiple pulsar
		M	= 0x00000020, //!< Millisecond pulsar
		R	= 0x00000040, //!< Recycled pulsar
		I	= 0x00000100, //!< Radio interpulse
		H	= 0x00000200, //!< Optical, X-ray or Gamma-ray pulsed emission (high energy)
		E	= 0x00000400  //!< Extragalactic (in MC) pulsar
	};

	//! @param id The official designation for a pulsar, e.g. "PSR J1919+21"
	Pulsar(const QVariantMap& map);
	~Pulsar();

	//! Get a QVariantMap which describes the pulsar. Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	//! Get the type of object
	virtual QString getType(void) const
	{
		return "Pulsar";
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
	//! Get the visual magnitude of pulsar
	virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;
	//! Get the angular size of pulsar
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of pulsar
	virtual QString getNameI18n(void) const
	{
		return designation;
	}
	//! Get the english name of pulsar
	virtual QString getEnglishName(void) const
	{
		return designation;
	}

	void update(double deltaTime);

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position	

	static StelTextureSP hintTexture;
	static StelTextureSP markerTexture;

	void draw(StelCore* core, StelPainter& painter);

	//! Variables for description of properties of pulsars
	QString designation;	//! The designation of the pulsar (J2000 pulsar name)
	float RA;		//! J2000 right ascension
	float DE;		//! J2000 declination
	float distance;		//! Adopted distance of pulsar in kpc
	double period;		//! Barycentric period in seconds
	int ntype;		//! Octal code for pulsar type
	float We;		//! Equivalent width of the integrated pulse profile in ms
	float w50;		//! Profile width at 50% of peak in ms
	float s400;		//! Time averaged flux density at 400MHz in mJy
	float s600;		//! Time averaged flux density at 600MHz in mJy
	float s1400;		//! Time averaged flux density at 1400MHz in mJy

	LinearFader labelsFader;

protected:
	//! Get type of pulsar from octal code
	//! @flags a set of flags with information types of pulsar.
	QString getPulsarTypeInfoString(const int flags) const;

};

#endif // _PULSAR_HPP_
