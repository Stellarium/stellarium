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

	//! @enum pulsarSurveyInfoGroup used as named bitfield flags as specifiers to
	//! filter results of getPulsarSurveyInfoString. The precise definition of these should
	//! be documented in the getPulsarSurveyInfoString documentation for the derived classes
	//! for all specifiers which are defined in that derivative.
	//! Description of surveys you can see here - http://cdsarc.u-strasbg.fr/viz-bin/Cat?VII/189
	enum pulsarSurveyInfoGroup
	{
		Molonglo1	= 0x00000001, //!< Molonglo 1, 408 MHz
		JodrellBank1	= 0x00000002, //!< Jodrell Bank 1, 408 MHz
		Arecibo1	= 0x00000004, //!< Arecibo 1, 430 MHz
		Molonglo2	= 0x00000010, //!< Molonglo 2, 408 MHz
		GreenBank1	= 0x00000020, //!< Green Bank 1, 400 MHz
		GreenBank2	= 0x00000040, //!< Green Bank 2, 400 MHz
		JodrellBank2	= 0x00000100, //!< Jodrell Bank 2, 1400 MHz
		GreenBank3	= 0x00000200, //!< Green Bank 3, 400 MHz
		Arecibo2	= 0x00000400, //!< Arecibo 2, 430 MHz
		Parkes1		= 0x00001000, //!< Parkes 1, 1520 MHz
		Arecibo3	= 0x00002000, //!< Arecibo 3, 430 MHz
		Parkes70	= 0x00004000, //!< Parkes 70, 436 MHz
		Arecibo4a	= 0x00010000, //!< Arecibo 4a, 430 MHz
		Arecibo4b	= 0x00020000, //!< Arecibo 4b, 430 MHz
		Arecibo4c	= 0x00040000, //!< Arecibo 4c, 430 MHz
		Arecibo4d	= 0x00100000, //!< Arecibo 4d, 430 MHz
		Arecibo4e	= 0x00200000, //!< Arecibo 4e, 430 MHz
		Arecibo4f	= 0x00400000, //!< Arecibo 4f, 430 MHz
		GreenBank4	= 0x01000000  //!< Green Bank 4, 370 MHz
	};

	//! @param id The official designation for a pulsar, e.g. "PSR 1919+21"
	Pulsar(const QVariantMap& map);
	~Pulsar();

	//! Get a QVariantMap which describes the pulsar. Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

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

	static StelTextureSP hintTexture;
	static StelTextureSP markerTexture;

	void draw(StelCore* core, StelPainter& painter);

	// Pulsar
	QString designation;	//! The designation of the pulsar (J2000 pulsar name)
	float RA;		//! J2000 right ascension
	float DE;		//! J2000 declination
	float distance;		//! Adopted distance of pulsar in kpc
	double period;		//! Barycentric period in seconds
	int ntype;		//! Octal code for pulsar type
	int survey;		//! Octal code for survey

	LinearFader labelsFader;

protected:

	QString getPulsarTypeInfoString(const int flags) const;

	QString getPulsarSurveyInfoString(const int flags) const;

};

#endif // _PULSAR_HPP_
