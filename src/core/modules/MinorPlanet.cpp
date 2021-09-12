/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau (some old code from the Planet class)
 * Copyright (C) 2010 Bogdan Marinov
 * Copyright (C) 2013-14 Georg Zotti (accuracy&speedup)
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
 
#include "MinorPlanet.hpp"
#include "Orbit.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObserver.hpp"

#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"
#include "RefractionExtinction.hpp"
#include "Orbit.hpp"

#include <QRegExp>
#include <QDebug>
#include <QElapsedTimer>

MinorPlanet::MinorPlanet(const QString& englishName,
			 double radius,
			 double oblateness,
			 Vec3f halocolor,
			 float albedo,
			 float roughness,
			 //float outgas_intensity,
			 //float outgas_falloff,
			 const QString& atexMapName,
			 const QString& anormalMapName,
			 const QString& aobjModelName,
			 posFuncType coordFunc,
			 KeplerOrbit* orbitPtr,
			 OsculatingFunctType *osculatingFunc,
			 bool acloseOrbit,
			 bool hidden,
			 const QString &pTypeStr)
	: Planet (englishName,
		  radius,
		  oblateness,
		  halocolor,
		  albedo,
		  roughness,
		  //0.f, // outgas_intensity,
		  //0.f, // outgas_falloff,
		  atexMapName,
		  anormalMapName,
		  aobjModelName,
		  coordFunc,
		  orbitPtr,
		  osculatingFunc,
		  acloseOrbit,
		  hidden,
		  false, //No atmosphere
		  true,  //Halo
		  pTypeStr),
	minorPlanetNumber(0),
	slopeParameter(-10.0f), // -10 == mark as uninitialized: used in getVMagnitude()
	nameIsProvisionalDesignation(false),
	properName(englishName),
	b_v(99.f),
	specT(""),
	specB("")
{
	//Try to handle an occasional naming conflict between a moon and asteroid. Conflicting names are also shown with appended *.
	if (englishName.endsWith('*'))
		properName = englishName.left(englishName.count() - 1);

	//Try to detect provisional designation	
	QString provisionalDesignation = renderProvisionalDesignationinHtml(englishName);
	if (!provisionalDesignation.isEmpty())
	{
		nameIsProvisionalDesignation = true;
		provisionalDesignationHtml = provisionalDesignation;
	}
}

MinorPlanet::~MinorPlanet()
{
	//Do nothing for the moment
}

void MinorPlanet::setSpectralType(QString sT, QString sB)
{
	specT = sT;
	specB = sB;
}

void MinorPlanet::setColorIndexBV(float bv)
{
	b_v = bv;
}

void MinorPlanet::setMinorPlanetNumber(int number)
{
	if (minorPlanetNumber)
		return;

	minorPlanetNumber = number;
}

void MinorPlanet::setAbsoluteMagnitudeAndSlope(const float magnitude, const float slope)
{
	if (slope < -1.0f || slope > 2.0f)
	{
		// G "should" be between 0 and 1, but may be somewhat outside.
		qDebug() << "MinorPlanet::setAbsoluteMagnitudeAndSlope(): Invalid slope parameter value (must be between -1 and 2, mostly [0..1])";
		return;
	}
	absoluteMagnitude = magnitude;
	slopeParameter = slope;
}

void MinorPlanet::setProvisionalDesignation(QString designation)
{
	//TODO: This feature has to be implemented better, anyway.
	if (!designation.isEmpty())
	{
		provisionalDesignationHtml = renderProvisionalDesignationinHtml(designation);
		nameIsProvisionalDesignation = false;
	}
}

QString MinorPlanet::getEnglishName() const
{
	return (minorPlanetNumber ? QString("(%1) %2").arg(minorPlanetNumber).arg(englishName) : englishName);
}

QString MinorPlanet::getNameI18n() const
{
	return (minorPlanetNumber ?  QString("(%1) %2").arg(minorPlanetNumber).arg(nameI18) : nameI18);
}

QString MinorPlanet::getInfoStringName(const StelCore *core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core) Q_UNUSED(flags)
	QString str;
	QTextStream oss(&str);

	oss << "<h2>";
	if (nameIsProvisionalDesignation)
	{
		if (minorPlanetNumber)
			oss << QString("(%1) ").arg(minorPlanetNumber);
		oss << provisionalDesignationHtml;
	}
	else
		oss << getNameI18n();  // UI translation can differ from sky translation
	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(1);
	if (sphereScale != 1.)
		oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
	oss << "</h2>";
	if (!nameIsProvisionalDesignation && !provisionalDesignationHtml.isEmpty())
	{
		oss << QString(q_("Provisional designation: %1")).arg(provisionalDesignationHtml);
		oss << "<br>";
	}
	return str;
}

QString MinorPlanet::getInfoStringExtraMag(const StelCore *core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core)
	if (flags&Extra && b_v<99.f)
		return QString("%1: <b>%2</b><br/>").arg(q_("Color Index (B-V)"), QString::number(b_v, 'f', 2));
	else
		return "";
}

QString MinorPlanet::getInfoStringExtra(const StelCore *core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core)
	QString str;
	QTextStream oss(&str);
	if (flags&Extra)
	{
		if (!specT.isEmpty())
		{
			// TRANSLATORS: Tholen spectral taxonomic classification of asteroids
			oss << QString("%1: %2<br/>").arg(q_("Tholen spectral type"), specT);
		}

		if (!specB.isEmpty())
		{
			// TRANSLATORS: SMASSII spectral taxonomic classification of asteroids
			oss << QString("%1: %2<br/>").arg(q_("SMASSII spectral type"), specB);
		}
	}
	return str;
}

double MinorPlanet::getSiderealPeriod() const
{
	return static_cast<KeplerOrbit*>(orbitPtr)->calculateSiderealPeriod();
}

float MinorPlanet::getVMagnitude(const StelCore* core) const
{
	//If the H-G system is not used, use the default radius/albedo mechanism
	if (slopeParameter < -9.99f) // G can be somewhat <0! Set to -10 to mark invalid.
	{
		return Planet::getVMagnitude(core);
	}

	//Calculate phase angle
	//(Code copied from Planet::getVMagnitude())
	//(this is actually vector subtraction + the cosine theorem :))
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const float observerRq = static_cast<float>(observerHelioPos.lengthSquared());
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const float planetRq = static_cast<float>(planetHelioPos.lengthSquared());
	const float observerPlanetRq = static_cast<float>((observerHelioPos - planetHelioPos).lengthSquared());
	const float cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0f*std::sqrt(observerPlanetRq*planetRq));
	const float phaseAngle = std::acos(cos_chi);

	//Calculate reduced magnitude (magnitude without the influence of distance)
	//Source of the formulae: http://www.britastro.org/asteroids/dymock4.pdf
	// Same model as in Explanatory Supplement 2013, p.423
	const float tanPhaseAngleHalf=std::tan(phaseAngle*0.5f);
	const float phi1 = std::exp(-3.33f * std::pow(tanPhaseAngleHalf, 0.63f));
	const float phi2 = std::exp(-1.87f * std::pow(tanPhaseAngleHalf, 1.22f));
	float reducedMagnitude = absoluteMagnitude - 2.5f * std::log10( (1.0f - slopeParameter) * phi1 + slopeParameter * phi2 );

	//Calculate apparent magnitude
	float apparentMagnitude = reducedMagnitude + 5.0f * std::log10(std::sqrt(planetRq * observerPlanetRq));

	return apparentMagnitude;
}

void MinorPlanet::translateName(const StelTranslator &translator)
{
	nameI18 = translator.qtranslate(properName, "minor planet");
	if (englishName.endsWith('*'))
	{
		nameI18.append('*');
	}
}

QString MinorPlanet::renderProvisionalDesignationinHtml(QString plainTextName)
{
	QRegExp provisionalDesignationPattern("^(\\d{4}\\s[A-Z]{2})(\\d*)$");
	if (provisionalDesignationPattern.indexIn(plainTextName) == 0)
	{
		QString main = provisionalDesignationPattern.cap(1);
		QString suffix = provisionalDesignationPattern.cap(2);
		if (!suffix.isEmpty())
		{
			return (QString("%1<sub>%2</sub>").arg(main).arg(suffix));
		}
		else
		{
			return main;
		}
	}
	else
	{
		return QString();
	}
}

