/*
 * Stellarium
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
 
#include "Comet.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QRegExp>
#include <QDebug>

Comet::Comet(const QString& englishName,
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

	//Comet specific members
	absoluteMagnitude = 0;
	slopeParameter = -1;//== uninitialized: used in getVMagnitude()

	//TODO: Name processing?

	nameI18 = englishName;

	flagLabels = true;
}

Comet::~Comet()
{
	//Do nothing for the moment
}

void Comet::setAbsoluteMagnitudeAndSlope(double magnitude, double slope)
{
	if (slope < 0 || slope > 20.0)
	{
		qDebug() << "Comet::setAbsoluteMagnitudeAndSlope(): Invalid slope parameter value (must be between 0 and 20)";
		return;
	}

	//TODO: More checks?
	//TODO: Make it set-once like the number?

	absoluteMagnitude = magnitude;
	slopeParameter = slope;
}

QString Comet::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	//Mostly copied from Planet::getInfoString():
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>";
		oss << q_(englishName);  // UI translation can differ from sky translation
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
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
		//If the two parameter magnitude system is not use, don't display this
		//value. (Using radius/albedo doesn't make any sense for comets.)
		if (slopeParameter >= 0)
			oss << q_("Absolute Magnitude: %1").arg(absoluteMagnitude, 0, 'f', 2) << "<br>";
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

	/*
	if (flags&Size)
		oss << q_("Apparent diameter: %1").arg(StelUtils::radToDmsStr(2.*getAngularSize(core)*M_PI/180., true));
	*/

	// If semi-major axis not zero then calculate and display orbital period for comet in days
	double siderealPeriod = getSiderealPeriod();
	if ((flags&Extra1) && (siderealPeriod>0))
	{
		// TRANSLATORS: Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
		oss << q_("Sidereal period: %1 days (%2 a)").arg(QString::number(siderealPeriod, 'f', 2)).arg(QString::number(siderealPeriod/365.25, 'f', 3)) << "<br>";
	}

	postProcessInfoString(str, flags);

	return str;
}

void Comet::setSemiMajorAxis(double value)
{
	semiMajorAxis = value;
}

double Comet::getSiderealPeriod() const
{
	double period;
	if (semiMajorAxis>0)
		period = StelUtils::calculateSiderealPeriod(semiMajorAxis);
	else
		period = 0;

	return period;
}

float Comet::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	//If the two parameter system is not used,
	//use the default radius/albedo mechanism
	if (slopeParameter < 0)
	{
		return Planet::getVMagnitude(core, withExtinction);
	}

	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	//Calculate distances
	const Vec3d& observerHeliocentricPosition = core->getObserverHeliocentricEclipticPos();
	const Vec3d& cometHeliocentricPosition = getHeliocentricEclipticPos();
	const double cometSunDistance = std::sqrt(cometHeliocentricPosition.lengthSquared());
	const double observerCometDistance = std::sqrt((observerHeliocentricPosition - cometHeliocentricPosition).lengthSquared());

	//Calculate apparent magnitude
	//Sources: http://www.clearskyinstitute.com/xephem/help/xephem.html#mozTocId564354
	//(XEphem manual, section 7.1.2.3 "Magnitude models"), also
	//http://www.ayton.id.au/gary/Science/Astronomy/Ast_comets.htm#Comet%20facts:
	double apparentMagnitude = absoluteMagnitude + 5 * std::log10(observerCometDistance) + 2.5 * slopeParameter * std::log10(cometSunDistance);

	return apparentMagnitude + extinctionMag;
}
