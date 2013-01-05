/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau (some old code from the Planet class)
 * Copyright (C) 2010 Bogdan Marinov
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

#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QRegExp>
#include <QDebug>

MinorPlanet::MinorPlanet(const QString& englishName,
						 int flagLighting,
						 double radius,
						 double oblateness,
						 Vec3f color,
						 float albedo,
						 const QString& atexMapName,
						 posFuncType coordFunc,
						 void* auserDataPtr,
						 OsculatingFunctType *osculatingFunc,
						 bool acloseOrbit,
						 bool hidden,
						 const QString& pType)
						: Planet (englishName,
								  flagLighting,
								  radius,
								  oblateness,
								  color,
								  albedo,
								  atexMapName,
								  coordFunc,
								  auserDataPtr,
								  osculatingFunc,
								  acloseOrbit,
								  hidden,
								  false, //No atmosphere
								  pType)
{
	texMapName = atexMapName;
	lastOrbitJD =0;
	deltaJD = StelCore::JD_SECOND;
	orbitCached = 0;
	closeOrbit = acloseOrbit;

	eclipticPos=Vec3d(0.,0.,0.);
	rotLocalToParent = Mat4d::identity();

	//MinorPlanet specific members
	minorPlanetNumber = 0;
	absoluteMagnitude = 0;
	slopeParameter = -1;//== uninitialized: used in getVMagnitude()	

	//TODO: Fix the name
	// - Detect numeric prefix and set number if any
	// - detect provisional designation
	// - create the HTML name
	//Try to detect number
	//TODO: Move this to the minor planet parse code in the plug-in?	
	/*
	QString name = englishName;
	QRegExp bracketedNumberPrefixPattern("^\\((\\d+)\\)\\s");
	QRegExp freeNumberPrefixPattern("^(\\d+)\\s[A-Za-z]{3,}");
	if (bracketedNumberPrefixPattern.indexIn(name) == 0)
	{
		QString numberString = bracketedNumberPrefixPattern.cap(1);
		bool ok = false;
		number = numberString.toInt(&ok);
		if (!ok)
			number = 0;

		//TODO: Handle a name consisting only of a number
		name.remove(0, numberString.length() + 3);
		htmlName = QString("(%1) ").arg(number);
	}
	else if (freeNumberPrefixPattern.indexIn(name) == 0)
	{
		QString numberString =freeNumberPrefixPattern.cap(1);
		bool ok = false;
		number = numberString.toInt(&ok);
		if (!ok)
			number = 0;

		//TODO: Handle a name consisting only of a number
		name.remove(0, numberString.length() + 3);
		htmlName = QString("(%1) ").arg(number);
	}*/

	//Try to detect a naming conflict
	if (englishName.endsWith('*'))
		properName = englishName.left(englishName.count() - 1);
	else
		properName = englishName;

	//Try to detect provisional designation
	nameIsProvisionalDesignation = false;
	QString provisionalDesignation = renderProvisionalDesignationinHtml(englishName);
	if (!provisionalDesignation.isEmpty())
	{
		nameIsProvisionalDesignation = true;
		provisionalDesignationHtml = provisionalDesignation;
	}

	nameI18 = englishName;

	flagLabels = true;
}

MinorPlanet::~MinorPlanet()
{
	//Do nothing for the moment
}

void MinorPlanet::setSemiMajorAxis(double value)
{
	semiMajorAxis = value;
}

void MinorPlanet::setMinorPlanetNumber(int number)
{
	if (minorPlanetNumber)
		return;

	minorPlanetNumber = number;
}

void MinorPlanet::setAbsoluteMagnitudeAndSlope(double magnitude, double slope)
{
	if (slope < 0 || slope > 1.0)
	{
		qDebug() << "MinorPlanet::setAbsoluteMagnitudeAndSlope(): Invalid slope parameter value (must be between 0 and 1)";
		return;
	}

	//TODO: More checks?
	//TODO: Make it set-once like the number?

	absoluteMagnitude = magnitude;
	slopeParameter = slope;
}

void MinorPlanet::setProvisionalDesignation(QString designation)
{
	//TODO: This feature has to be implemented better, anyway.
	provisionalDesignationHtml = renderProvisionalDesignationinHtml(designation);
}

QString MinorPlanet::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	//Mostly copied from Planet::getInfoString():

	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>";
		if (minorPlanetNumber)
			oss << QString("(%1) ").arg(minorPlanetNumber);
		if (nameIsProvisionalDesignation)
			oss << provisionalDesignationHtml;
		else
			oss << q_(properName);  // UI translation can differ from sky translation
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
		if (!nameIsProvisionalDesignation && !provisionalDesignationHtml.isEmpty())
		{
			oss << QString(q_("Provisional designation: %1")).arg(provisionalDesignationHtml);
			oss << "<br>";
		}
	}

	if (flags&Extra1)
	{
		if (pType.length()>0)
			oss << q_("Type: <b>%1</b>").arg(q_(pType)) << "<br />";
	}

	if (flags&Magnitude)
	{
	    if (core->getSkyDrawer()->getFlagHasAtmosphere())
		oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
										QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
	    else
		oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core, false), 0, 'f', 2) << "<br>";

	}

	if (flags&AbsoluteMagnitude)
	{
		//TODO: Make sure absolute magnitude is a sane value
		//If the H-G system is not used, use the default radius/albedo mechanism
		if (slopeParameter < 0)
		{
			oss << q_("Absolute Magnitude: %1").arg(getVMagnitude(core, false) - 5. * (std::log10(getJ2000EquatorialPos(core).length()*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";
		}
		else
		{
			oss << q_("Absolute Magnitude: %1").arg(absoluteMagnitude, 0, 'f', 2) << "<br>";
		}
	}

	oss << getPositionInfoString(core, flags);

	if (flags&Distance)
	{
		double distanceAu = getJ2000EquatorialPos(core).length();
		if (distanceAu < 0.1)
		{
			double distanceKm = AU * distanceAu;
			// xgettext:no-c-format
			oss << QString(q_("Distance: %1AU (%2 km)"))
			       .arg(distanceAu, 0, 'f', 8)
			       .arg(distanceKm, 0, 'f', 0);
		}
		else
		{
			// xgettext:no-c-format
			oss << q_("Distance: %1AU").arg(distanceAu, 0, 'f', 8);
		}
		oss << "<br>";
	}

	if (flags&Size)
		oss << q_("Apparent diameter: %1").arg(StelUtils::radToDmsStr(2.*getAngularSize(core)*M_PI/180., true)) << "<br>";

	// If semi-major axis not zero then calculate and display orbital period for asteroid in days
	double siderealPeriod = getSiderealPeriod();
	if ((flags&Extra1) && (siderealPeriod>0))
	{
		// TRANSLATORS: Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
		oss << q_("Sidereal period: %1 days (%2 a)").arg(QString::number(siderealPeriod, 'f', 2)).arg(QString::number(siderealPeriod/365.25, 'f', 3)) << "<br>";
	}

	//This doesn't work, even if setOpenExternalLinks(true) is used in InfoPanel
	/*
	if (flags&Extra1)
		oss << QString("<br><a href=\"http://ssd.jpl.nasa.gov/sbdb.cgi?sstr=%1\">JPL Small-Body Database Browser</a>").arg( (minorPlanetNumber) ? QString::number(minorPlanetNumber) : englishName );
	*/

	postProcessInfoString(str, flags);

	return str;
}

double MinorPlanet::getSiderealPeriod() const
{
	double period;
	if (semiMajorAxis>0)
		period = StelUtils::calculateSiderealPeriod(semiMajorAxis);
	else
		period = 0;

	return period;
}

float MinorPlanet::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction)
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	//If the H-G system is not used, use the default radius/albedo mechanism
	if (slopeParameter < 0)
	{
		return Planet::getVMagnitude(core, withExtinction);
	}

	//Calculate phase angle
	//(Code copied from Planet::getVMagnitude())
	//(LOL, this is actually vector subtraction + the cosine theorem :))
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const double observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	double phaseAngle = std::acos(cos_chi);

	//Calculate reduced magnitude (magnitude without the influence of distance)
	//Source of the formulae: http://www.britastro.org/asteroids/dymock4.pdf
	const double phi1 = std::exp(-3.33 * std::pow(std::tan(phaseAngle/2), 0.63));
	const double phi2 = std::exp(-1.87 * std::pow(std::tan(phaseAngle/2), 1.22));
	double reducedMagnitude = absoluteMagnitude - 2.5 * std::log10( (1 - slopeParameter) * phi1 + slopeParameter * phi2 );

	//Calculate apparent magnitude
	//TODO: See if you can "collapse" some calculations
	double apparentMagnitude = reducedMagnitude + 5 * std::log10(std::sqrt(planetRq * observerPlanetRq));

	return apparentMagnitude+extinctionMag;
}

void MinorPlanet::translateName(StelTranslator &translator)
{
	nameI18 = translator.qtranslate(properName);
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
		//qDebug() << "renderProvisionalDesignationinHtml():" << plainTextName
		//         << "is not a provisional designation in plain text.";
		return QString();
	}
}

