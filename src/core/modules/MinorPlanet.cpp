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

#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"
#include "RefractionExtinction.hpp"
#include "Orbit.hpp"

#include <QRegExp>
#include <QDebug>

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

QString MinorPlanet::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	//Mostly copied from Planet::getInfoString():

	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));	
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	Q_UNUSED(az_app);

	if (flags&Name)
	{
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
	}

	if (flags&ObjectType && getPlanetType()!=isUNDEFINED)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_(getPlanetTypeString())) << "<br />";
	}

	oss << getMagnitudeInfoString(core, flags, alt_app, 1);

	if (flags&AbsoluteMagnitude)
	{
		// GZ Huh? "Absolute Magnitude" for solar system objects has nothing to do with 10pc standard distance!
		//    I have deactivated this for now.
		// TODO: Apparently, the first solution makes "Absolute" magnitudes for 10pc distance. This is not the definition for Abs.Mag. for Minor Bodies!
		// Absolute magnitude H for Minor Planets is its magnitude when in solar radius 1AU, at distance 1AU, with phase angle 0.
		// Hint from Wikipedia:https://en.wikipedia.org/wiki/Absolute_magnitude#Solar_System_bodies_.28H.29: subtract 31.57 from the value...
		// If the H-G system is not used, use the default radius/albedo mechanism
		//if (slopeParameter < -1.0f)
		//{
		//     double distanceAu = getJ2000EquatorialPos(core).length();
		//	oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core) - 5. * (std::log10(distanceAu*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";
		//}
		//else
		//{
			oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(absoluteMagnitude, 0, 'f', 2) << "<br>";
		//}
	}

	if (flags&Extra && b_v<99.f)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(b_v, 'f', 2)) << "<br />";

	oss << getCommonInfoString(core, flags);

	if (flags&Distance)
	{
		double hdistanceAu = getHeliocentricEclipticPos().length();
		double hdistanceKm = AU * hdistanceAu;
		// TRANSLATORS: Unit of measure for distance - astronomical unit
		QString au = qc_("AU", "distance, astronomical unit");
		// TRANSLATORS: Unit of measure for distance - kilometers
		QString km = qc_("km", "distance");
		QString distAU, distKM;
		if (hdistanceAu < 0.1)
		{
			distAU = QString::number(hdistanceAu, 'f', 6);
			distKM = QString::number(hdistanceKm, 'f', 3);
		}
		else
		{
			distAU = QString::number(hdistanceAu, 'f', 3);
			distKM = QString::number(hdistanceKm / 1.0e6, 'f', 3);
			// TRANSLATORS: Unit of measure for distance - milliones kilometers
			km = qc_("M km", "distance");
		}
		oss << QString("%1: %2 %3 (%4 %5)").arg(q_("Distance from Sun"), distAU, au, distKM, km) << "<br />";

		double distanceAu = getJ2000EquatorialPos(core).length();
		double distanceKm = AU * distanceAu;
		if (distanceAu < 0.1)
		{
			distAU = QString::number(distanceAu, 'f', 6);
			distKM = QString::number(distanceKm, 'f', 3);
			// TRANSLATORS: Unit of measure for distance - kilometers
			km = qc_("km", "distance");
		}
		else
		{
			distAU = QString::number(distanceAu, 'f', 3);
			distKM = QString::number(distanceKm / 1.0e6, 'f', 3);
			// TRANSLATORS: Unit of measure for distance - millions kilometers
			km = qc_("M km", "distance");
		}
		oss << QString("%1: %2 %3 (%4 %5)").arg(q_("Distance"), distAU, au, distKM, km) << "<br />";
	}

	if (flags&Velocity)
	{
		// TRANSLATORS: Unit of measure for speed - kilometers per second
		QString kms = qc_("km/s", "speed");

		Vec3d orbitalVel=getEclipticVelocity();
		double orbVel=orbitalVel.length();
		if (orbVel>0.)
		{ // AU/d * km/AU /24
			double orbVelKms=orbVel* AU/86400.;
			oss << QString("%1: %2 %3").arg(q_("Orbital velocity")).arg(orbVelKms, 0, 'f', 3).arg(kms) << "<br />";
			double helioVel=getHeliocentricEclipticVelocity().length(); // just in case we have asteroid moons!
			if (!fuzzyEquals(helioVel, orbVel))
				oss << QString("%1: %2 %3").arg(q_("Heliocentric velocity")).arg(helioVel* AU/86400., 0, 'f', 3).arg(kms) << "<br />";
		}
		if (qAbs(re.period)>0.)
		{
			double eqRotVel = 2.0*M_PI*(AU*getEquatorialRadius())/(getSiderealDay()*86400.0);
			oss << QString("%1: %2 %3").arg(q_("Equatorial rotation velocity")).arg(qAbs(eqRotVel), 0, 'f', 3).arg(kms) << "<br />";
		}
	}

	if (flags&ProperMotion)
	{
		Vec3d equPos=getEquinoxEquatorialPos(core);
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,equPos);
		StelCore* core1 = StelApp::getInstance().getCore(); // we need non-const reference here.
		const double currentJD=core1->getJD();
		core1->setJD(currentJD-StelCore::JD_HOUR);
		core1->update(0);
		Vec3d equPosPrev=getEquinoxEquatorialPos(core1);
		double dec_equPrev, ra_equPrev;
		StelUtils::rectToSphe(&ra_equPrev,&dec_equPrev,equPosPrev);
		core1->setJD(currentJD);
		core1->update(0);
		const double deltaEq=equPos.angle(equPosPrev);
		double pa=atan2(ra_equ-ra_equPrev, dec_equ-dec_equPrev); // position angle: From North counterclockwise!
		if (pa<0) pa += 2.*M_PI;
		oss << QString("%1: %2 %3 %4%5").arg(q_("Hourly motion"), StelUtils::radToDmsStr(deltaEq), qc_("towards", "into the direction of"), QString::number(pa*M_180_PI, 'f', 1), QChar(0x00B0)) << "<br/>";
		oss << QString("%1: d&alpha;=%2 d&delta;=%3").arg(q_("Hourly motion"), StelUtils::radToDmsStr(ra_equ-ra_equPrev), StelUtils::radToDmsStr(dec_equ-dec_equPrev)) << "<br/>";
	}

	double angularSize = 2.*getAngularSize(core)*M_PI/180.;
	if (flags&Size && angularSize>=4.8e-8)
	{
		QString sizeStr = "";
		if (sphereScale!=1.) // We must give correct diameters even if upscaling (e.g. Moon)
		{
			QString sizeOrig, sizeScaled;
			if (withDecimalDegree)
			{
				sizeOrig   = StelUtils::radToDecDegStr(angularSize / sphereScale, 5, false, true);
				sizeScaled = StelUtils::radToDecDegStr(angularSize, 5, false, true);
			}
			else
			{
				sizeOrig   = StelUtils::radToDmsPStr(angularSize / sphereScale, 2);
				sizeScaled = StelUtils::radToDmsPStr(angularSize, 2);
			}

			sizeStr = QString("%1, %2: %3").arg(sizeOrig, q_("scaled up to"), sizeScaled);
		}
		else
		{
			if (withDecimalDegree)
				sizeStr = StelUtils::radToDecDegStr(angularSize, 5, false, true);
			else
				sizeStr = StelUtils::radToDmsPStr(angularSize, 2);
		}

		oss << QString("%1: %2").arg(q_("Apparent diameter"), sizeStr) << "<br />";
	}

	if (flags&Size)
	{
		// Many asteroides has irregular shape
		oss << QString("%1: %2 %3").arg(q_("Diameter"), QString::number(AU * getEquatorialRadius() * 2.0, 'f', 1) , qc_("km", "distance")) << "<br />";
	}

	// If semi-major axis not zero then calculate and display orbital period for asteroid in days
	double siderealPeriod = getSiderealPeriod();
	if (flags&Extra)
	{
		if (!specT.isEmpty())
		{
			// TRANSLATORS: Tholen spectral taxonomic classification of asteroids
			oss << QString("%1: %2").arg(q_("Tholen spectral type"), specT) << "<br />";
		}

		if (!specB.isEmpty())
		{
			// TRANSLATORS: SMASSII spectral taxonomic classification of asteroids
			oss << QString("%1: %2").arg(q_("SMASSII spectral type"), specB) << "<br />";
		}

		QString days = qc_("days", "duration");
		if (siderealPeriod>0)
		{
			// Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
			oss << QString("%1: %2 %3 (%4 a)").arg(q_("Sidereal period"), QString::number(siderealPeriod, 'f', 2), days, QString::number(siderealPeriod/365.25, 'f', 3)) << "<br />";
		}

		double siderealPeriodCurrentPlanet = core->getCurrentPlanet()->getSiderealPeriod();
		if (siderealPeriodCurrentPlanet > 0.0 && siderealPeriod > 0.0 && core->getCurrentPlanet()->getPlanetType()==Planet::isPlanet && getPlanetType()!=Planet::isArtificial && getPlanetType()!=Planet::isStar && getPlanetType()!=Planet::isMoon)
		{
			double sp = qAbs(1/(1/siderealPeriodCurrentPlanet - 1/siderealPeriod));
			// Synodic period for solar system bodies in days and in Julian years (symbol: a)
			oss << QString("%1: %2 %3 (%4 a)").arg(q_("Synodic period")).arg(QString::number(sp, 'f', 2)).arg(days).arg(QString::number(sp/365.25, 'f', 3)) << "<br />";
		}

		const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
		const double elongation = getElongation(observerHelioPos);

		QString pha, elo;
		if (withDecimalDegree)
		{
			pha = StelUtils::radToDecDegStr(getPhaseAngle(observerHelioPos),4,false,true);
			elo = StelUtils::radToDecDegStr(elongation,4,false,true);
		}
		else
		{
			pha = StelUtils::radToDmsStr(getPhaseAngle(observerHelioPos), true);
			elo = StelUtils::radToDmsStr(elongation, true);
		}

		oss << QString("%1: %2").arg(q_("Phase angle"), pha) << "<br />";
		oss << QString("%1: %2").arg(q_("Elongation"), elo) << "<br />";
	}

	postProcessInfoString(str, flags);

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

