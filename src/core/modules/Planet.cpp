/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelTexture.hpp"
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"

#include "StelProjector.hpp"
#include "sidereal_time.h"
#include "StelTextureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "StelOpenGL.hpp"

#include <iomanip>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QVarLengthArray>
#include <QOpenGLContext>
#include <QOpenGLShader>

Vec3f Planet::labelColor = Vec3f(0.4,0.4,0.8);
Vec3f Planet::orbitColor = Vec3f(1,0.6,1);
StelTextureSP Planet::hintCircleTex;
StelTextureSP Planet::texEarthShadow;

QOpenGLShaderProgram* Planet::planetShaderProgram=NULL;
Planet::PlanetShaderVars Planet::planetShaderVars;
QOpenGLShaderProgram* Planet::ringPlanetShaderProgram=NULL;
Planet::RingPlanetShaderVars Planet::ringPlanetShaderVars;
QOpenGLShaderProgram* Planet::moonShaderProgram=NULL;
Planet::MoonShaderVars Planet::moonShaderVars;
	
Planet::Planet(const QString& englishName,
	       int flagLighting,
	       double radius,
	       double oblateness,
	       Vec3f color,
	       float albedo,
	       const QString& atexMapName,
	       const QString& anormalMapName,
	       posFuncType coordFunc,
	       void* auserDataPtr,
	       OsculatingFunctType *osculatingFunc,
	       bool acloseOrbit,
	       bool hidden,
	       bool hasAtmosphere,
	       bool hasHalo,
	       const QString& pType)
	: englishName(englishName),
	  flagLighting(flagLighting),
	  radius(radius),
	  oneMinusOblateness(1.0-oblateness),
	  color(color),
	  albedo(albedo),
	  axisRotation(0.),
	  rings(NULL),
	  sphereScale(1.f),
	  lastJD(J2000),
	  coordFunc(coordFunc),
	  userDataPtr(auserDataPtr),
	  osculatingFunc(osculatingFunc),
	  parent(NULL),
	  hidden(hidden),
	  atmosphere(hasAtmosphere),
	  halo(hasHalo),
	  pType(pType)
{
	texMapName = atexMapName;
	normalMapName = anormalMapName;
	lastOrbitJD =0;
	deltaJD = StelCore::JD_SECOND;
	orbitCached = 0;
	closeOrbit = acloseOrbit;
	deltaOrbitJD = 0;
	distance = 0;

	eclipticPos=Vec3d(0.,0.,0.);
	rotLocalToParent = Mat4d::identity();
	texMap = StelApp::getInstance().getTextureManager().createTextureThread(StelFileMgr::getInstallationDir()+"/textures/"+texMapName, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
	normalMap = StelApp::getInstance().getTextureManager().createTextureThread(StelFileMgr::getInstallationDir()+"/textures/"+normalMapName, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

	nameI18 = englishName;
	if (englishName!="Pluto")
	{
		deltaJD = 0.001*StelCore::JD_SECOND;
	}
	flagLabels = true;
}

Planet::~Planet()
{
	if (rings)
		delete rings;
}

void Planet::translateName(const StelTranslator& trans)
{
	nameI18 = trans.qtranslate(englishName);
}

// Return the information string "ready to print" :)
QString Planet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << q_(englishName);  // UI translation can differ from sky translation
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
	}

	if (flags&ObjectType)
	{
		if (pType.length()>0)
			oss << q_("Type: <b>%1</b>").arg(q_(pType)) << "<br />";
	}

	if (flags&Magnitude)
	{
		if (core->getSkyDrawer()->getFlagHasAtmosphere())
		    oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core), 'f', 2),
										    QString::number(getVMagnitudeWithExtinction(core), 'f', 2)) << "<br>";
		else
		    oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core), 0, 'f', 2) << "<br>";
	}
	if (flags&AbsoluteMagnitude)
		oss << q_("Absolute Magnitude: %1").arg(getVMagnitude(core)-5.*(std::log10(getJ2000EquatorialPos(core).length()*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if (flags&Extra)
	{
		static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
		double ecl= ssystem->getEarth()->getRotObliquity(core->getJDay());
		if (core->getCurrentLocation().planetName=="Earth")
			oss << q_("Obliquity (of date, for Earth): %1").arg(StelUtils::radToDmsStr(ecl, true)) << "<br>";
		//if (englishName!="Sun")
		//	oss << q_("Obliquity (of date): %1").arg(StelUtils::radToDmsStr(getRotObliquity(core->getJDay()), true)) << "<br>";
	}

	if (flags&Distance)
	{
		double distanceAu = getJ2000EquatorialPos(core).length();
		double distanceKm = AU * distanceAu;
		if (distanceAu < 0.1)
		{
			// xgettext:no-c-format
			oss << QString(q_("Distance: %1AU (%2 km)"))
				   .arg(distanceAu, 0, 'f', 6)
				   .arg(distanceKm, 0, 'f', 3);
		}
		else
		{
			// xgettext:no-c-format
			oss << QString(q_("Distance: %1AU (%2 Mio km)"))
				   .arg(distanceAu, 0, 'f', 3)
				   .arg(distanceKm / 1.0e6, 0, 'f', 3);
		}
		oss << "<br>";
	}

	if (flags&Size)
	{
		double angularSize = 2.*getAngularSize(core)*M_PI/180.;
		if (rings)
		{
			double withoutRings = 2.*getSpheroidAngularSize(core)*M_PI/180.;
			oss << q_("Apparent diameter: %1, with rings: %2")
			       .arg(StelUtils::radToDmsStr(withoutRings, true),
			            StelUtils::radToDmsStr(angularSize, true));
		}
		else
		{
			oss << q_("Apparent diameter: %1").arg(StelUtils::radToDmsStr(angularSize, true));
		}
		oss << "<br>";
	}

	double siderealPeriod = getSiderealPeriod();
	double siderealDay = getSiderealDay();
	if (flags&Extra)
	{
		if (siderealPeriod>0)
		{
			// TRANSLATORS: Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
			oss << q_("Sidereal period: %1 days (%2 a)").arg(QString::number(siderealPeriod, 'f', 2)).arg(QString::number(siderealPeriod/365.25, 'f', 3)) << "<br>";
			if (std::abs(siderealDay)>0)
			{
				oss << q_("Sidereal day: %1").arg(StelUtils::hoursToHmsStr(std::abs(siderealDay*24))) << "<br>";
				oss << q_("Mean solar day: %1").arg(StelUtils::hoursToHmsStr(std::abs(getMeanSolarDay()*24))) << "<br>";
			}
		}
		if (englishName.compare("Sun")!=0)
		{
			const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
			oss << QString(q_("Phase Angle: %1")).arg(StelUtils::radToDmsStr(getPhaseAngle(observerHelioPos))) << "<br>";
			oss << QString(q_("Elongation: %1")).arg(StelUtils::radToDmsStr(getElongation(observerHelioPos))) << "<br>";
			oss << QString(q_("Phase: %1")).arg(getPhase(observerHelioPos), 0, 'f', 2) << "<br>";
			oss << QString(q_("Illuminated: %1%")).arg(getPhase(observerHelioPos) * 100, 0, 'f', 1) << "<br>";
		}
	}

	postProcessInfoString(str, flags);

	return str;
}

//! Get sky label (sky translation)
QString Planet::getSkyLabel(const StelCore*) const
{
	QString str;
	QTextStream oss(&str);
	oss.setRealNumberPrecision(2);
	oss << nameI18;

	if (sphereScale != 1.f)
	{
		oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
	}
	return str;
}

float Planet::getSelectPriority(const StelCore* core) const
{
	if( ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getFlagHints() )
	{
	// easy to select, especially pluto
		return getVMagnitudeWithExtinction(core)-15.f;
	}
	else
	{
		return getVMagnitudeWithExtinction(core) - 8.f;
	}
}

Vec3f Planet::getInfoColor(void) const
{
	return ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getLabelsColor();
}


double Planet::getCloseViewFov(const StelCore* core) const
{
	return std::atan(radius*sphereScale*2.f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
}

double Planet::getSatellitesFov(const StelCore* core) const
{
	// TODO: calculate from satellite orbits rather than hard code
	if (englishName=="Jupiter") return std::atan(0.005f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	if (englishName=="Saturn") return std::atan(0.005f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	if (englishName=="Mars") return std::atan(0.0001f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	if (englishName=="Uranus") return std::atan(0.002f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	return -1.;
}

double Planet::getParentSatellitesFov(const StelCore* core) const
{
	if (parent && parent->parent) return parent->getSatellitesFov(core);
	return -1.0;
}

// Set the rotational elements of the planet body.
void Planet::setRotationElements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate, double _siderealPeriod )
{
	re.period = _period;
	re.offset = _offset;
	re.epoch = _epoch;
	re.obliquity = _obliquity;
	re.ascendingNode = _ascendingNode;
	re.precessionRate = _precessionRate;
	re.siderealPeriod = _siderealPeriod;  // used for drawing orbit lines

	deltaOrbitJD = re.siderealPeriod/ORBIT_SEGMENTS;
}

Vec3d Planet::getJ2000EquatorialPos(const StelCore *core) const
{
	return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(getHeliocentricEclipticPos() - core->getObserverHeliocentricEclipticPos());
}

// Compute the position in the parent Planet coordinate system
// Actually call the provided function to compute the ecliptical position
void Planet::computePositionWithoutOrbits(const double dateJD)
{
	if (fabs(lastJD-dateJD)>deltaJD)
	{
		coordFunc(dateJD, eclipticPos, userDataPtr);
		lastJD = dateJD;
	}
}

double Planet::getRotObliquity(double JDay) const
{
	// JDay=2451545.0 for J2000.0
	if (englishName=="Earth")
		return get_mean_ecliptical_obliquity(JDay) *M_PI/180.0;
	else
		return re.obliquity;
}


bool willCastShadow(const Planet* thisPlanet, const Planet* p)
{
	Vec3d thisPos = thisPlanet->getHeliocentricEclipticPos();
	Vec3d planetPos = p->getHeliocentricEclipticPos();
	
	// If the planet p is farther from the sun than this planet, it can't cast shadow on it.
	if (planetPos.lengthSquared()>thisPos.lengthSquared())
		return false;
	
	Vec3d ppVector = planetPos;
	ppVector.normalize();
	
	double shadowDistance = ppVector * thisPos;
	static const double sunRadius = 696000./AU;
	double d = planetPos.length() / (p->getRadius()/sunRadius+1);
	double penumbraRadius = (shadowDistance-d)/d*sunRadius;
	
	double penumbraCenterToThisPlanetCenterDistance = (ppVector*shadowDistance-thisPos).length();
	
	if (penumbraCenterToThisPlanetCenterDistance<penumbraRadius+thisPlanet->getRadius())
		return true;
	return false;
}

QVector<const Planet*> Planet::getCandidatesForShadow() const
{
	QVector<const Planet*> res;
	const SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
	const Planet* sun = ssystem->getSun().data();
	if (this==sun || (parent.data()==sun && satellites.empty()))
		return res;
	
	foreach (const PlanetP& planet, satellites)
	{
		if (willCastShadow(this, planet.data()))
			res.append(planet.data());
	}
	if (willCastShadow(this, parent.data()))
		res.append(parent.data());
	
	return res;
}

void Planet::computePosition(const double dateJD)
{

	if (orbitFader.getInterstate()>0.000001 && deltaOrbitJD > 0 && (fabs(lastOrbitJD-dateJD)>deltaOrbitJD || !orbitCached))
	{
		double calc_date;
		// int delta_points = (int)(0.5 + (date - lastOrbitJD)/date_increment);
		int delta_points;

		if( dateJD > lastOrbitJD )
		{
			delta_points = (int)(0.5 + (dateJD - lastOrbitJD)/deltaOrbitJD);
		}
		else
		{
			delta_points = (int)(-0.5 + (dateJD - lastOrbitJD)/deltaOrbitJD);
		}
		double new_date = lastOrbitJD + delta_points*deltaOrbitJD;

		// qDebug( "Updating orbit coordinates for %s (delta %f) (%d points)\n", name.c_str(), deltaOrbitJD, delta_points);

		if( delta_points > 0 && delta_points < ORBIT_SEGMENTS && orbitCached)
		{

			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				if(d + delta_points >= ORBIT_SEGMENTS )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*deltaOrbitJD;

					// date increments between points will not be completely constant though
					computeTransMatrix(calc_date);
					if (osculatingFunc)
					{
						(*osculatingFunc)(dateJD,calc_date,eclipticPos);
					}
					else
					{
						coordFunc(calc_date, eclipticPos, userDataPtr);
					}
					orbitP[d] = eclipticPos;
					orbit[d] = getHeliocentricEclipticPos();
				}
				else
				{
					orbitP[d] = orbitP[d+delta_points];
					orbit[d] = getHeliocentricPos(orbitP[d]);
				}
			}

			lastOrbitJD = new_date;
		}
		else if( delta_points < 0 && abs(delta_points) < ORBIT_SEGMENTS  && orbitCached)
		{

			for( int d=ORBIT_SEGMENTS-1; d>=0; d-- )
			{
				if(d + delta_points < 0 )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*deltaOrbitJD;

					computeTransMatrix(calc_date);
					if (osculatingFunc) {
						(*osculatingFunc)(dateJD,calc_date,eclipticPos);
					}
					else
					{
						coordFunc(calc_date, eclipticPos, userDataPtr);
					}
					orbitP[d] = eclipticPos;
					orbit[d] = getHeliocentricEclipticPos();
				}
				else
				{
					orbitP[d] = orbitP[d+delta_points];
					orbit[d] = getHeliocentricPos(orbitP[d]);
				}
			}

			lastOrbitJD = new_date;

		}
		else if( delta_points || !orbitCached)
		{

			// update all points (less efficient)
			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				calc_date = dateJD + (d-ORBIT_SEGMENTS/2)*deltaOrbitJD;
				computeTransMatrix(calc_date);
				if (osculatingFunc)
				{
					(*osculatingFunc)(dateJD,calc_date,eclipticPos);
				}
				else
				{
					coordFunc(calc_date, eclipticPos, userDataPtr);
				}
				orbitP[d] = eclipticPos;
				orbit[d] = getHeliocentricEclipticPos();
			}

			lastOrbitJD = dateJD;
			if (!osculatingFunc) orbitCached = 1;
		}


		// calculate actual Planet position
		coordFunc(dateJD, eclipticPos, userDataPtr);

		lastJD = dateJD;

	}
	else if (fabs(lastJD-dateJD)>deltaJD)
	{
		// calculate actual Planet position
		coordFunc(dateJD, eclipticPos, userDataPtr);
		// XXX: do we need to do that even when the orbit is not visible?
		for( int d=0; d<ORBIT_SEGMENTS; d++ )
			orbit[d]=getHeliocentricPos(orbitP[d]);
		lastJD = dateJD;
	}

}

// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate
void Planet::computeTransMatrix(double jd)
{
	axisRotation = getSiderealTime(jd);

	// Special case - heliocentric coordinates are on ecliptic,
	// not solar equator...
	if (parent)
	{
		rotLocalToParent = Mat4d::zrotation(re.ascendingNode - re.precessionRate*(jd-re.epoch)) * Mat4d::xrotation(re.obliquity);
	}
}

Mat4d Planet::getRotEquatorialToVsop87(void) const
{
	Mat4d rval = rotLocalToParent;
	if (parent)
	{
		for (PlanetP p=parent;p->parent;p=p->parent)
			rval = p->rotLocalToParent * rval;
	}
	return rval;
}

void Planet::setRotEquatorialToVsop87(const Mat4d &m)
{
	Mat4d a = Mat4d::identity();
	if (parent)
	{
		for (PlanetP p=parent;p->parent;p=p->parent)
			a = p->rotLocalToParent * a;
	}
	rotLocalToParent = a.transpose() * m;
}


// Compute the z rotation to use from equatorial to geographic coordinates
double Planet::getSiderealTime(double jd) const
{
	if (englishName=="Earth")
	{
		return get_apparent_sidereal_time(jd);
	}

	double t = jd - re.epoch;
	double rotations = t / (double) re.period;
	double wholeRotations = floor(rotations);
	double remainder = rotations - wholeRotations;

// TODO: This block need rewrite
	if (englishName=="Jupiter")
	{
		// use semi-empirical coefficient for GRS drift
		return remainder * 360. + re.offset - 0.2483 * std::abs(jd - 2456172);
	}
	else
		return remainder * 360. + re.offset;
}

double Planet::getMeanSolarDay() const
{
	double msd = 0.;

	if (englishName=="Sun")
		return msd;

	double sday = getSiderealDay();	
	double coeff = std::abs(sday/getSiderealPeriod());
	float sign = 1;
	// planets with retrograde rotation
	if (englishName=="Venus" || englishName=="Uranus" || englishName=="Pluto")
		sign = -1;

	if (pType.contains("moon"))
	{
		// duration of mean solar day on moon are same as synodic month on this moon
		double a = parent->getSiderealPeriod()/sday;
		msd = sday*(a/(a-1));
	}
	else
		msd = sign*sday/(1 - sign*coeff);

	return msd;
}

// Get the Planet position in the parent Planet ecliptic coordinate in AU
Vec3d Planet::getEclipticPos() const
{
	return eclipticPos;
}

// Return the heliocentric ecliptical position (Vsop87)
Vec3d Planet::getHeliocentricEclipticPos() const
{
	Vec3d pos = eclipticPos;
	PlanetP pp = parent;
	if (pp)
	{
		while (pp->parent)
		{
			pos += pp->eclipticPos;
			pp = pp->parent;
		}
	}
	return pos;
}

// Return heliocentric coordinate of p
Vec3d Planet::getHeliocentricPos(Vec3d p) const
{
	// Note: using shared copies is too slow here.  So we use direct access
	// instead.
	Vec3d pos = p;
	const Planet* pp = parent.data();
	if (pp)
	{
		while (pp->parent.data())
		{
			pos += pp->eclipticPos;
			pp = pp->parent.data();
		}
	}
	return pos;
}

void Planet::setHeliocentricEclipticPos(const Vec3d &pos)
{
	eclipticPos = pos;
	PlanetP p = parent;
	if (p)
	{
		while (p->parent)
		{
			eclipticPos -= p->eclipticPos;
			p = p->parent;
		}
	}
}

// Compute the distance to the given position in heliocentric coordinate (in AU)
// This is called by SolarSystem::draw()
double Planet::computeDistance(const Vec3d& obsHelioPos)
{
	distance = (obsHelioPos-getHeliocentricEclipticPos()).length();
	// GZ: improve fps by juggling updates for asteroids. They must be fast if close to observer, but can be slow if further away.
	if (pType == "asteroid")
			deltaJD=distance*StelCore::JD_SECOND;
	return distance;
}

// Get the phase angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getPhaseAngle(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	return std::acos((observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq)));
}

// Get the planet phase for an observer at pos obsPos in heliocentric coordinates (in AU)
float Planet::getPhase(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	return 0.5f * std::abs(1.f + cos_chi);
}

// Get the elongation angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getElongation(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	return std::acos((observerPlanetRq  + observerRq - planetRq)/(2.0*sqrt(observerPlanetRq*observerRq)));
}

// Computation of the visual magnitude (V band) of the planet.
float Planet::getVMagnitude(const StelCore* core) const
{
	if (parent == 0)
	{
		// sun, compute the apparent magnitude for the absolute mag (4.83) and observer's distance
		const double distParsec = std::sqrt(core->getObserverHeliocentricEclipticPos().lengthSquared())*AU/PARSEC;
		return 4.83 + 5.*(std::log10(distParsec)-1.);
	}

	// Compute the angular phase
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const double observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*std::sqrt(observerPlanetRq*planetRq));
	const double phase = std::acos(cos_chi);

	double shadowFactor = 1.;
	// Check if the satellite is inside the inner shadow of the parent planet:
	if (parent->parent != 0)
	{
		const Vec3d& parentHeliopos = parent->getHeliocentricEclipticPos();
		const double parent_Rq = parentHeliopos.lengthSquared();
		const double pos_times_parent_pos = planetHelioPos * parentHeliopos;
		if (pos_times_parent_pos > parent_Rq)
		{
			// The satellite is farther away from the sun than the parent planet.
			const double sun_radius = parent->parent->radius;
			const double sun_minus_parent_radius = sun_radius - parent->radius;
			const double quot = pos_times_parent_pos/parent_Rq;

			// Compute d = distance from satellite center to border of inner shadow.
			// d>0 means inside the shadow cone.
			double d = sun_radius - sun_minus_parent_radius*quot - std::sqrt((1.-sun_minus_parent_radius/sqrt(parent_Rq)) * (planetRq-pos_times_parent_pos*quot));
			if (d>=radius)
			{
				// The satellite is totally inside the inner shadow.
				shadowFactor = 1e-9;
			}
			else if (d>-radius)
			{
				// The satellite is partly inside the inner shadow,
				// compute a fantasy value for the magnitude:
				d /= radius;
				shadowFactor = (0.5 - (std::asin(d)+d*std::sqrt(1.0-d*d))/M_PI);
			}
		}
	}

	// Use empirical formulae for main planets when seen from earth
	if (core->getCurrentLocation().planetName=="Earth")
	{
		const double phaseDeg=phase*180./M_PI;
		const double d = 5. * log10(sqrt(observerPlanetRq*planetRq));
		//double f1 = phaseDeg/100.;

		/*
		// Algorithm provided by Pere Planesas (Observatorio Astronomico Nacional)
		if (englishName=="Mercury")
		{
			if ( phaseDeg > 150. ) f1 = 1.5;
			return -0.36 + d + 3.8*f1 - 2.73*f1*f1 + 2*f1*f1*f1;
		}
		if (englishName=="Venus")
			return -4.29 + d + 0.09*f1 + 2.39*f1*f1 - 0.65*f1*f1*f1;
		if (englishName=="Mars")
			return -1.52 + d + 0.016*phaseDeg;
		if (englishName=="Jupiter")
			return -9.25 + d + 0.005*phaseDeg;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
			// double rings = -2.6*sinx + 1.25*sinx*sinx;
			return -8.88 + d + 0.044*phaseDeg;// + rings;
		}
		if (englishName=="Uranus")
			return -7.19 + d + 0.0028*phaseDeg;
		if (englishName=="Neptune")
			return -6.87 + d;
		if (englishName=="Pluto")
			return -1.01 + d + 0.041*phaseDeg;
		*/
		// GZ: I prefer the values given by Meeus, Astronomical Algorithms (1992).
		// There are two solutions:
		// (1) G. Mueller, based on visual observations 1877-91. [Expl.Suppl.1961]
		// (2) Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes.
		// The structure is almost identical, just the numbers are different!
		// I activate (1) for now, because we want to simulate the eye's impression. (Esp. Venus!)
		// (1)
		if (englishName=="Mercury")
		    {
			double ph50=phaseDeg-50.0;
			return 1.16 + d + 0.02838*ph50 + 0.0001023*ph50*ph50;
		    }
		if (englishName=="Venus")
			return -4.0 + d + 0.01322*phaseDeg + 0.0000004247*phaseDeg*phaseDeg*phaseDeg;
		if (englishName=="Mars")
			return -1.3 + d + 0.01486*phaseDeg;
		if (englishName=="Jupiter")
			return -8.93 + d;
		if (englishName=="Saturn")
		{
			// add rings computation
			// GZ: implemented from Meeus, Astr.Alg.1992
			const double jd=core->getJDay();
			const double T=(jd-2451545.0)/36525.0;
			const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
			const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
			SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
			const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
			double lambda=atan2(saturnEarth[1], saturnEarth[0]);
			double beta=atan2(saturnEarth[2], sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
			const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
			double rings = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
			return -8.68 + d + 0.044*phaseDeg + rings;
		}
		if (englishName=="Uranus")
			return -6.85 + d;
		if (englishName=="Neptune")
			return -7.05 + d;
		if (englishName=="Pluto")
			return -1.0 + d;
		/*
		// (2)
		if (englishName=="Mercury")
			return 0.42 + d + .038*phaseDeg - 0.000273*phaseDeg*phaseDeg + 0.000002*phaseDeg*phaseDeg*phaseDeg;
		if (englishName=="Venus")
			return -4.40 + d + 0.0009*phaseDeg + 0.000239*phaseDeg*phaseDeg - 0.00000065*phaseDeg*phaseDeg*phaseDeg;
		if (englishName=="Mars")
			return -1.52 + d + 0.016*phaseDeg;
		if (englishName=="Jupiter")
			return -9.40 + d + 0.005*phaseDeg;
		if (englishName=="Saturn")
		{
			// add rings computation
			// GZ: implemented from Meeus, Astr.Alg.1992
			const double jd=core->getJDay();
			const double T=(jd-2451545.0)/36525.0;
			const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
			const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
			static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
			const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
			double lambda=atan2(saturnEarth[1], saturnEarth[0]);
			double beta=atan2(saturnEarth[2], sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
			const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
			double rings = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
			return -8.88 + d + 0.044*phaseDeg + rings;
		}
		if (englishName=="Uranus")
			return -7.19f + d;
		if (englishName=="Neptune")
			return -6.87f + d;
		if (englishName=="Pluto")
			return -1.00f + d;
	*/
	// TODO: decide which set of formulae is best?
	}

	// This formula seems to give wrong results
	const double p = (1.0 - phase/M_PI) * cos_chi + std::sqrt(1.0 - cos_chi*cos_chi) / M_PI;
	double F = 2.0 * albedo * radius * radius * p / (3.0*observerPlanetRq*planetRq) * shadowFactor;
	return -26.73 - 2.5 * std::log10(F);
}

double Planet::getAngularSize(const StelCore* core) const
{
	double rad = radius;
	if (rings)
		rad = rings->getSize();
	return std::atan2(rad*sphereScale,getJ2000EquatorialPos(core).length()) * 180./M_PI;
}


double Planet::getSpheroidAngularSize(const StelCore* core) const
{
	return std::atan2(radius*sphereScale,getJ2000EquatorialPos(core).length()) * 180./M_PI;
}

// Draw the Planet and all the related infos : name, circle etc..
void Planet::draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont)
{
	if (hidden)
		return;
	// GZ: Try to improve speed for minor planets: test if visible at all.
	// For a full catalog of NEAs (11000 objects), with this and resetting deltaJD according to distance, rendering time went 4.5fps->12fps.	
	// AW: Apply this rule to asteroids only
	if (((getVMagnitude(core)-1.0f) > core->getSkyDrawer()->getLimitMagnitude()) && pType=="asteroid")
	{
		return;
	}

	Mat4d mat = Mat4d::translation(eclipticPos) * rotLocalToParent;
	PlanetP p = parent;
	while (p && p->parent)
	{
		mat = Mat4d::translation(p->eclipticPos) * mat * p->rotLocalToParent;
		p = p->parent;
	}

	// This removed totally the Planet shaking bug!!!
	StelProjector::ModelViewTranformP transfo = core->getHeliocentricEclipticModelViewTransform();
	transfo->combine(mat);
	if (getEnglishName() == core->getCurrentLocation().planetName)
	{
		// Draw the rings if we are located on a planet with rings, but not the planet itself.
		if (rings)
		{
			draw3dModel(core, transfo, 1024, true);
		}
		return;
	}

	// Compute the 2D position and check if in the screen
	const StelProjectorP prj = core->getProjection(transfo);
	float screenSz = getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();
	float viewport_left = prj->getViewportPosX();
	float viewport_bottom = prj->getViewportPosY();
	if (prj->project(Vec3d(0), screenPos)
	    && screenPos[1]>viewport_bottom - screenSz && screenPos[1] < viewport_bottom + prj->getViewportHeight()+screenSz
	    && screenPos[0]>viewport_left - screenSz && screenPos[0] < viewport_left + prj->getViewportWidth() + screenSz)
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlapping (e.g. for Jupiter's satellites)
		float ang_dist = 300.f*atan(getEclipticPos().length()/getEquinoxEquatorialPos(core).length())/core->getMovementMgr()->getCurrentFov();
		if (ang_dist==0.f)
			ang_dist = 1.f; // if ang_dist == 0, the Planet is sun..

		// by putting here, only draw orbit if Planet is visible for clarity
		drawOrbit(core);  // TODO - fade in here also...

		if (flagLabels && ang_dist>0.25 && maxMagLabels>getVMagnitude(core))
		{
			labelsFader=true;
		}
		else
		{
			labelsFader=false;
		}
		drawHints(core, planetNameFont);

		draw3dModel(core,transfo,screenSz);
	}
	return;
}

class StelPainterLight
{
public:
	Vec3f position;
	Vec3f diffuse;
	Vec3f ambient;
};
static StelPainterLight light;

void Planet::PlanetShaderVars::initLocations(QOpenGLShaderProgram* p)
{
	GL(projectionMatrix = p->uniformLocation("projectionMatrix"));
	GL(texCoord = p->attributeLocation("texCoord"));
	GL(unprojectedVertex = p->attributeLocation("unprojectedVertex"));
	GL(vertex = p->attributeLocation("vertex"));
	GL(texture = p->uniformLocation("tex"));
	GL(lightDirection = p->uniformLocation("lightDirection"));
	GL(eyeDirection = p->uniformLocation("eyeDirection"));
	GL(diffuseLight = p->uniformLocation("diffuseLight"));
	GL(ambientLight = p->uniformLocation("ambientLight"));
	GL(shadowCount = p->uniformLocation("shadowCount"));
	GL(shadowData = p->uniformLocation("shadowData"));
	GL(sunInfo = p->uniformLocation("sunInfo"));
}

void Planet::initShader()
{
	qWarning() << "Intializing planets GL shaders... ";
	
	const char *vsrc =
		"attribute highp vec3 vertex;\n"
		"attribute highp vec3 unprojectedVertex;\n"
		"attribute mediump vec2 texCoord;\n"
		"uniform highp mat4 projectionMatrix;\n"
		"uniform highp vec3 lightDirection;\n"
		"uniform highp vec3 eyeDirection;\n"
		"varying mediump vec2 texc;\n"
		"varying highp vec3 P;\n"
		"#ifdef IS_MOON\n"
		"    varying highp vec3 normalX;\n"
		"    varying highp vec3 normalY;\n"
		"    varying highp vec3 normalZ;\n"
		"#else\n"
		"    varying mediump float lum_;\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = projectionMatrix * vec4(vertex, 1.);\n"
		"    texc = texCoord;\n"
		"    highp vec3 normal = normalize(unprojectedVertex);\n"
		"#ifdef IS_MOON\n"
		"    normalX = normalize(cross(vec3(0,0,1), normal));\n"
		"    normalY = normalize(cross(normal, normalX));\n"
		"    normalZ = normal;\n"
		"#else\n"
		"    mediump float c = dot(lightDirection, normal);\n"
		"    lum_ = clamp(c, 0.0, 1.0);\n"
		"#endif\n"
		"\n"
		"    P = unprojectedVertex;\n"
		"}\n"
		"\n";
	
	const char *fsrc =
		"varying mediump vec2 texc;\n"
		"uniform sampler2D tex;\n"
		"uniform mediump vec3 ambientLight;\n"
		"uniform mediump vec3 diffuseLight;\n"
		"uniform highp vec4 sunInfo;\n"
		"varying highp vec3 P;\n"
		"\n"
		"uniform int shadowCount;\n"
		"uniform highp mat4 shadowData;\n"
		"\n"
		"#ifdef RINGS_SUPPORT\n"
		"uniform bool ring;\n"
		"uniform highp float outerRadius;\n"
		"uniform highp float innerRadius;\n"
		"uniform sampler2D ringS;\n"
		"uniform bool isRing;\n"
		"#endif\n"
		"\n"
		"#ifdef IS_MOON\n"
		"uniform sampler2D earthShadow;\n"
		"uniform sampler2D normalMap;\n"
		"uniform highp vec3 lightDirection;\n"
		"uniform highp vec3 eyeDirection;\n"
		"varying highp vec3 normalX;\n"
		"varying highp vec3 normalY;\n"
		"varying highp vec3 normalZ;\n"
		"#else\n"
		"varying mediump float lum_;\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    mediump float final_illumination = 1.0;\n"
		"#ifdef IS_MOON\n"
		"    mediump float lum = 1.;\n"
		"#else\n"
		"    mediump float lum = lum_;\n"
		"#endif\n"
		"#ifdef RINGS_SUPPORT\n"
		"    if(isRing)"
		"        lum=1.0;\n"
		"#endif\n"
		"    if(lum > 0.0)\n"
		"    {\n"
		"        highp vec3 sunPosition = sunInfo.xyz;\n"
		"#ifdef RINGS_SUPPORT\n"
		"        if(ring && !isRing)\n"
		"        {\n"
		"            highp vec3 ray = normalize(sunPosition);\n"
		"            highp float u = - P.z / ray.z;\n"
		"            if(u > 0.0 && u < 1e10)\n"
		"            {\n"
		"                mediump float ring_radius = length(P + u * ray);\n"
		"                if(ring_radius > innerRadius && ring_radius < outerRadius)\n"
		"                {\n"
		"                    ring_radius = (ring_radius - innerRadius) / (outerRadius - innerRadius);\n"
		"                    lowp float ringAlpha = texture2D(ringS, vec2(ring_radius, 0.5)).w;\n"
		"                    final_illumination = 1.0 - ringAlpha;\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"#endif\n"
		"\n"
		"        highp float sunRadius = sunInfo.w;\n"
		"        highp float L = length(sunPosition - P);\n"
		"        highp float R = asin(sunRadius / L);\n"
		"        for (int i = 0; i < shadowCount; ++i)\n"
		"        {\n"
		"            highp vec3 satellitePosition = shadowData[i].xyz;\n"
		"            highp float satelliteRadius = shadowData[i].w;\n"
		"            highp float l = length(satellitePosition - P);\n"
		"            highp float r = asin(satelliteRadius / l);\n"
		"            highp float d = acos(min(1.0, dot(normalize(sunPosition - P), normalize(satellitePosition - P))));\n"
		"\n"
		"            mediump float illumination = 1.0;\n"
		"            if(d >= R + r)\n"
		"            {\n"
		"                // distance too far\n"
		"                illumination = 1.0;\n"
		"            }\n"
		"            else if(r >= R + d)\n"
		"            {\n"
		"                // umbra\n"
		"#ifdef IS_MOON\n"
		"                illumination = d / (r - R) * 0.6;\n"
		"#else\n"
		"                illumination = 0.0;\n"
		"#endif\n"
		"            }\n"
		"            else if(d + r <= R)\n"
		"            {\n"
		"                // penumbra completely inside\n"
		"                illumination = 1.0 - r * r / (R * R);\n"
		"            }\n"
		"            else\n"
		"            {\n"
		"                // penumbra partially inside\n"
		"#ifdef IS_MOON\n"
		"                illumination = ((d - abs(R-r)) / (R + r - abs(R-r))) * 0.4 + 0.6;\n"
		"#else\n"
		"                mediump float x = (R * R + d * d - r * r) / (2.0 * d);\n"
		"                mediump float alpha = acos(x / R);\n"
		"                mediump float beta = acos((d - x) / r);\n"
		"                mediump float AR = R * R * (alpha - 0.5 * sin(2.0 * alpha));\n"
		"                mediump float Ar = r * r * (beta - 0.5 * sin(2.0 * beta));\n"
		"                mediump float AS = R * R * 2.0 * 1.57079633;\n"
		"                illumination = 1.0 - (AR + Ar) / AS;\n"
		"#endif\n"
		"            }\n"
		"\n"
		"            final_illumination = min(illumination, final_illumination);\n"
		"        }\n"
		"    }\n"
		"\n"
		"#ifdef IS_MOON\n"
		"    mediump vec3 normal = texture2D(normalMap, texc).rgb-vec3(0.5, 0.5, 0);\n"
		"    normal = normalize(normalX*normal.x+normalY*normal.y+normalZ*normal.z);\n"
		"    // normal now contains the real surface normal taking normal map into account\n"
		"    // Use an Oren-Nayar model for rough surfaces\n"
		"    // Ref: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Oren-Nayar\n"
		"    highp float cosAngleLightNormal = dot(normal, lightDirection);\n"
		"    highp float cosAngleEyeNormal = dot(normal, eyeDirection);\n"
		"    mediump float angleLightNormal = acos(cosAngleLightNormal);\n"
		"    mediump float angleEyeNormal = acos(cosAngleEyeNormal);\n"
		"    mediump float alpha = max(angleEyeNormal, angleLightNormal);\n"
		"    mediump float beta = min(angleEyeNormal, angleLightNormal);\n"
		"    mediump float gamma = dot(eyeDirection - normal * cosAngleEyeNormal, lightDirection - normal * cosAngleLightNormal);\n"
		"    mediump float roughness = 1.;\n"
		"    mediump float roughnessSquared = roughness * roughness;\n"
		"    mediump float A = 1.0 - 0.5 * (roughnessSquared / (roughnessSquared + 0.57));\n"
		"    mediump float B = 0.45 * (roughnessSquared / (roughnessSquared + 0.09));\n"
		"    mediump float C = sin(alpha) * tan(beta);\n"
		"    lum = max(0.0, cosAngleLightNormal) * (A + B * max(0.0, gamma) * C) * 2.;\n"
		"#endif\n"
		"    mediump vec4 litColor = vec4(lum * final_illumination * diffuseLight + ambientLight, 1.0);\n"
		"#ifdef IS_MOON\n"
		"    if(final_illumination < 0.99)\n"
		"    {\n"
		"        lowp vec4 shadowColor = texture2D(earthShadow, vec2(final_illumination, 0.5));\n"
		"        gl_FragColor = mix(texture2D(tex, texc) * litColor, shadowColor, shadowColor.a);\n"
		"    }\n"
		"    else\n"
		"#endif\n"
		"    {\n"
		"        gl_FragColor = texture2D(tex, texc) * litColor;\n"
		"    }\n"
		"}\n";
	
	// Default planet shader program
	QOpenGLShader vshader(QOpenGLShader::Vertex);
	vshader.compileSourceCode(vsrc);
	if (!vshader.log().isEmpty()) { qWarning() << "Planet: Warnings while compiling vshader: " << vshader.log(); }
	
	QOpenGLShader fshader(QOpenGLShader::Fragment);
	fshader.compileSourceCode(fsrc);
	if (!fshader.log().isEmpty()) { qWarning() << "Planet: Warnings while compiling fshader: " << fshader.log(); }

	planetShaderProgram = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	planetShaderProgram->addShader(&vshader);
	planetShaderProgram->addShader(&fshader);
	GL(StelPainter::linkProg(planetShaderProgram, "planetShaderProgram"));
	GL(planetShaderProgram->bind());
	planetShaderVars.initLocations(planetShaderProgram);
	GL(planetShaderProgram->release());
	
	// Planet with ring shader program
	QByteArray arr = "#define RINGS_SUPPORT\n\n";
	arr+=fsrc;
	QOpenGLShader ringFragmentShader(QOpenGLShader::Fragment);
	ringFragmentShader.compileSourceCode(arr.constData());
	if (!ringFragmentShader.log().isEmpty()) { qWarning() << "Planet: Warnings while compiling ringFragmentShader: " << ringFragmentShader.log(); }
	
	ringPlanetShaderProgram = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	ringPlanetShaderProgram->addShader(&vshader);
	ringPlanetShaderProgram->addShader(&ringFragmentShader);
	GL(StelPainter::linkProg(ringPlanetShaderProgram, "ringPlanetShaderProgram"));
	GL(ringPlanetShaderProgram->bind());
	ringPlanetShaderVars.initLocations(ringPlanetShaderProgram);
	GL(ringPlanetShaderVars.isRing = ringPlanetShaderProgram->uniformLocation("isRing"));
	GL(ringPlanetShaderVars.ring = ringPlanetShaderProgram->uniformLocation("ring"));
	GL(ringPlanetShaderVars.outerRadius = ringPlanetShaderProgram->uniformLocation("outerRadius"));
	GL(ringPlanetShaderVars.innerRadius = ringPlanetShaderProgram->uniformLocation("innerRadius"));
	GL(ringPlanetShaderVars.ringS = ringPlanetShaderProgram->uniformLocation("ringS"));
	GL(ringPlanetShaderProgram->release());
	
	// Moon shader program
	arr = "#define IS_MOON\n\n";
	arr+=vsrc;
	QOpenGLShader moonVertexShader(QOpenGLShader::Vertex);
	moonVertexShader.compileSourceCode(arr.constData());
	if (!moonVertexShader.log().isEmpty()) { qWarning() << "Planet: Warnings while compiling moonVertexShader: " << moonVertexShader.log(); }
	
	arr = "#define IS_MOON\n\n";
	arr+=fsrc;
	QOpenGLShader moonFragmentShader(QOpenGLShader::Fragment);
	moonFragmentShader.compileSourceCode(arr.constData());
	if (!moonFragmentShader.log().isEmpty()) { qWarning() << "Planet: Warnings while compiling moonFragmentShader: " << moonFragmentShader.log(); }
	
	moonShaderProgram = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	moonShaderProgram->addShader(&moonVertexShader);
	moonShaderProgram->addShader(&moonFragmentShader);
	GL(StelPainter::linkProg(moonShaderProgram, "moonPlanetShaderProgram"));
	GL(moonShaderProgram->bind());
	moonShaderVars.initLocations(moonShaderProgram);
	GL(moonShaderVars.earthShadow = moonShaderProgram->uniformLocation("earthShadow"));
	GL(moonShaderVars.normalMap = moonShaderProgram->uniformLocation("normalMap"));
	GL(moonShaderProgram->release());
}

void Planet::deinitShader()
{
	delete planetShaderProgram;
	planetShaderProgram = NULL;
}

void Planet::draw3dModel(StelCore* core, StelProjector::ModelViewTranformP transfo, float screenSz, bool drawOnlyRing)
{
	// This is the main method drawing a planet 3d model
	// Some work has to be done on this method to make the rendering nicer
	SolarSystem* ssm = GETSTELMODULE(SolarSystem);

	if (screenSz>1.)
	{
		StelProjector::ModelViewTranformP transfo2 = transfo->clone();
		transfo2->combine(Mat4d::zrotation(M_PI/180*(axisRotation + 90.)));
		StelPainter* sPainter = new StelPainter(core->getProjection(transfo2));
		
		if (flagLighting)
		{
			// Set the main source of light to be the sun
			Vec3d sunPos(0);
			core->getHeliocentricEclipticModelViewTransform()->forward(sunPos);
			light.position=Vec4f(sunPos[0],sunPos[1],sunPos[2],1.f);

			// Set the light parameters taking sun as the light source
			light.diffuse = Vec4f(1.f,1.f,1.f);
			light.ambient = Vec4f(0.02f,0.02f,0.02f);

			if (this==ssm->getMoon())
			{
				// Special case for the Moon (maybe better use 1.5,1.5,1.5,1.0 ?)
				light.diffuse = Vec4f(1.6f,1.6f,1.6f,1.f);
			}
		}
		else
		{
			sPainter->setColor(albedo,albedo,albedo);
		}

		if (rings)
		{
			// The planet has rings, we need to use depth buffer and adjust the clipping planes to avoid 
			// reaching the maximum resolution of the depth buffer
			const double dist = getEquinoxEquatorialPos(core).length();
			double z_near = 0.9*(dist - rings->getSize());
			double z_far  = 1.1*(dist + rings->getSize());
			if (z_near < 0.0) z_near = 0.0;
			double n,f;
			core->getClippingPlanes(&n,&f); // Save clipping planes
			core->setClippingPlanes(z_near,z_far);

			drawSphere(sPainter, screenSz, drawOnlyRing);
			
			core->setClippingPlanes(n,f);  // Restore old clipping planes
		}
		else
		{
			// Normal planet
			drawSphere(sPainter, screenSz);
		}
		delete sPainter;
		sPainter=NULL;
	}

	// Draw the halo if it enabled in the ssystem.ini file (+ special case for backward compatible for the Sun)
	if (hasHalo() || this==ssm->getSun())
	{
		// Prepare openGL lighting parameters according to luminance
		float surfArcMin2 = getSpheroidAngularSize(core)*60;
		surfArcMin2 = surfArcMin2*surfArcMin2*M_PI; // the total illuminated area in arcmin^2

		StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
		Vec3d tmp = getJ2000EquatorialPos(core);
		core->getSkyDrawer()->postDrawSky3dModel(&sPainter, Vec3f(tmp[0], tmp[1], tmp[2]), surfArcMin2, getVMagnitudeWithExtinction(core), color);
	}
}

struct Planet3DModel
{
	QVector<float> vertexArr;
	QVector<float> texCoordArr;
	QVector<unsigned short> indiceArr;
};

void sSphere(Planet3DModel* model, const float radius, const float oneMinusOblateness, const int slices, const int stacks)
{
	model->indiceArr.resize(0);
	model->vertexArr.resize(0);
	model->texCoordArr.resize(0);
	
	GLfloat x, y, z;
	GLfloat s=0.f, t=1.f;
	GLint i, j;

	const float* cos_sin_rho = StelUtils::ComputeCosSinRho(stacks);
	const float* cos_sin_theta =  StelUtils::ComputeCosSinTheta(slices);
	
	const float* cos_sin_rho_p;
	const float *cos_sin_theta_p;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
	// If the texture is flipped, we iterate the coordinates backward.
	const GLfloat ds = 1.f / slices;
	const GLfloat dt = 1.f / stacks; // from inside texture is reversed

	// draw intermediate  as quad strips
	for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks; ++i,cos_sin_rho_p+=2)
	{
		s = 0.f;
		for (j = 0,cos_sin_theta_p = cos_sin_theta; j<=slices;++j,cos_sin_theta_p+=2)
		{
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
			z = cos_sin_rho_p[0];
			model->texCoordArr << s << t;
			model->vertexArr << x * radius << y * radius << z * oneMinusOblateness * radius;
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
			z = cos_sin_rho_p[2];
			model->texCoordArr << s << t - dt;
			model->vertexArr << x * radius << y * radius << z * oneMinusOblateness * radius;
			s += ds;
		}
		unsigned int offset = i*(slices+1)*2;
		for (j = 2;j<slices*2+2;j+=2)
		{
			model->indiceArr << offset+j-2 << offset+j-1 << offset+j;
			model->indiceArr << offset+j << offset+j-1 << offset+j+1;
		}
		t -= dt;
	}
}

struct Ring3DModel
{
	QVector<float> vertexArr;
	QVector<float> texCoordArr;
	QVector<unsigned short> indiceArr;
};


void sRing(Ring3DModel* model, const float rMin, const float rMax, int slices, const int stacks)
{
	float x,y;
	
	const float dr = (rMax-rMin) / stacks;
	const float* cos_sin_theta = StelUtils::ComputeCosSinTheta(slices);
	const float* cos_sin_theta_p;

	model->vertexArr.resize(0);
	model->texCoordArr.resize(0);
	model->indiceArr.resize(0);

	float r = rMin;
	for (int i=0; i<=stacks; ++i)
	{
		const float tex_r0 = (r-rMin)/(rMax-rMin);
		int j;
		for (j=0,cos_sin_theta_p=cos_sin_theta; j<=slices; ++j,cos_sin_theta_p+=2)
		{
			x = r*cos_sin_theta_p[0];
			y = r*cos_sin_theta_p[1];
			model->texCoordArr << tex_r0 << 0.5f;
			model->vertexArr << x << y << 0.f;
		}
		r+=dr;
	}
	for (int i=0; i<stacks; ++i)
	{
		for (int j=0; j<slices; ++j)
		{
			model->indiceArr << i*slices+j << (i+1)*slices+j << i*slices+j+1;
			model->indiceArr << i*slices+j+1 << (i+1)*slices+j << (i+1)*slices+j+1;
		}
	}
}

void Planet::computeModelMatrix(Mat4d &result) const
{
	result = Mat4d::translation(eclipticPos) * rotLocalToParent * Mat4d::zrotation(M_PI/180*(axisRotation + 90.));
	PlanetP p = parent;
	while (p && p->parent)
	{
		result = Mat4d::translation(p->eclipticPos) * result * p->rotLocalToParent;
		p = p->parent;
	}
}

void Planet::drawSphere(StelPainter* painter, float screenSz, bool drawOnlyRing)
{
	if (texMap)
	{
		// For lazy loading, return if texture not yet loaded
		if (!texMap->bind(0))
		{
			return;
		}
	}
	painter->enableTexture2d(true);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	// Draw the spheroid itself
	// Adapt the number of facets according with the size of the sphere for optimization
	int nb_facet = (int)(screenSz * 40.f/50.f);	// 40 facets for 1024 pixels diameter on screen
	if (nb_facet<10) nb_facet = 10;
	if (nb_facet>100) nb_facet = 100;

	// Generates the vertice
	Planet3DModel model;
	sSphere(&model, radius*sphereScale, oneMinusOblateness, nb_facet, nb_facet);
	
	QVector<float> projectedVertexArr;
	projectedVertexArr.resize(model.vertexArr.size());
	for (int i=0;i<model.vertexArr.size()/3;++i)
		painter->getProjector()->project(*((Vec3f*)(model.vertexArr.constData()+i*3)), *((Vec3f*)(projectedVertexArr.data()+i*3)));
	
	const SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		
	if (this==ssm->getSun())
	{
		texMap->bind();
		painter->setColor(2, 2, 2);
		painter->setArrays((Vec3f*)projectedVertexArr.constData(), (Vec2f*)model.texCoordArr.constData());
		painter->drawFromArray(StelPainter::Triangles, model.indiceArr.size(), 0, false, model.indiceArr.constData());
		return;
	}
	
	if (planetShaderProgram==NULL)
		Planet::initShader();
	Q_ASSERT(planetShaderProgram!=NULL);
	Q_ASSERT(ringPlanetShaderProgram!=NULL);
	Q_ASSERT(moonShaderProgram!=NULL);
	
	QOpenGLShaderProgram* shader = planetShaderProgram;
	const PlanetShaderVars* shaderVars = &planetShaderVars;
	if (rings)
	{
		shader = ringPlanetShaderProgram;
		shaderVars = &ringPlanetShaderVars;
	}
	if (this==ssm->getMoon())
	{
		shader = moonShaderProgram;
		shaderVars = &moonShaderVars;
	}
	GL(shader->bind());
	
	const Mat4f& m = painter->getProjector()->getProjectionMatrix();
	const QMatrix4x4 qMat(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
	
	Mat4d modelMatrix;
	computeModelMatrix(modelMatrix);
	// TODO explain this
	const Mat4d mTarget = modelMatrix.inverse();
	
	QMatrix4x4 shadowCandidatesData;
	QVector<const Planet*> shadowCandidates = getCandidatesForShadow();
	// Our shader doesn't support more than 4 planets creating shadow
	if (shadowCandidates.size()>4)
	{
		qDebug() << "Too many satellite shadows, some won't be displayed";
		shadowCandidates.resize(4);
	}
	for (int i=0;i<shadowCandidates.size();++i)
	{
		shadowCandidates.at(i)->computeModelMatrix(modelMatrix);
		const Vec4d position = mTarget * modelMatrix.getColumn(3);
		shadowCandidatesData(0, i) = position[0];
		shadowCandidatesData(1, i) = position[1];
		shadowCandidatesData(2, i) = position[2];
		shadowCandidatesData(3, i) = shadowCandidates.at(i)->getRadius();
	}
	
	const StelProjectorP& projector = painter->getProjector();
	
	Vec3f lightPos3(light.position[0], light.position[1], light.position[2]);
	projector->getModelViewTransform()->backward(lightPos3);
	lightPos3.normalize();
	
	Vec3d eyePos = StelApp::getInstance().getCore()->getObserverHeliocentricEclipticPos();
	StelApp::getInstance().getCore()->getHeliocentricEclipticModelViewTransform()->forward(eyePos);
	projector->getModelViewTransform()->backward(eyePos);
	eyePos.normalize();
	
	GL(shader->setUniformValue(shaderVars->projectionMatrix, qMat));
	GL(shader->setUniformValue(shaderVars->lightDirection, lightPos3[0], lightPos3[1], lightPos3[2]));
	GL(shader->setUniformValue(shaderVars->eyeDirection, eyePos[0], eyePos[1], eyePos[2]));
	GL(shader->setUniformValue(shaderVars->diffuseLight, light.diffuse[0], light.diffuse[1], light.diffuse[2]));
	GL(shader->setUniformValue(shaderVars->ambientLight, light.ambient[0], light.ambient[1], light.ambient[2]));
	GL(shader->setUniformValue(shaderVars->texture, 0));
	GL(shader->setUniformValue(shaderVars->shadowCount, shadowCandidates.size()));
	GL(shader->setUniformValue(shaderVars->shadowData, shadowCandidatesData));
	GL(shader->setUniformValue(shaderVars->sunInfo, mTarget[12], mTarget[13], mTarget[14], ssm->getSun()->getRadius()));
	GL(texMap->bind(1));
	
	if (rings!=NULL)
	{
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.isRing, false));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.ring, true));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.outerRadius, rings->radiusMax));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.innerRadius, rings->radiusMin));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.ringS, 2));
		rings->tex->bind(2);
	}

	if (this==ssm->getMoon())
	{
		GL(normalMap->bind(2));
		GL(moonShaderProgram->setUniformValue(moonShaderVars.normalMap, 2));
		if (!shadowCandidates.isEmpty())
		{
			GL(texEarthShadow->bind(3));
			GL(moonShaderProgram->setUniformValue(moonShaderVars.earthShadow, 3));
		}
	}

	GL(shader->setAttributeArray(shaderVars->vertex, (const GLfloat*)projectedVertexArr.constData(), 3));
	GL(shader->enableAttributeArray(shaderVars->vertex));
	GL(shader->setAttributeArray(shaderVars->unprojectedVertex, (const GLfloat*)model.vertexArr.constData(), 3));
	GL(shader->enableAttributeArray(shaderVars->unprojectedVertex));
	GL(shader->setAttributeArray(shaderVars->texCoord, (const GLfloat*)model.texCoordArr.constData(), 2));
	GL(shader->enableAttributeArray(shaderVars->texCoord));

	if (rings)
	{
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
	}
	
	if (!drawOnlyRing)
		GL(glDrawElements(GL_TRIANGLES, model.indiceArr.size(), GL_UNSIGNED_SHORT, model.indiceArr.constData()));

	if (rings)
	{
		// Draw the rings just after the planet
		glDepthMask(GL_FALSE);
	
		// Normal transparency mode
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	
		Ring3DModel ringModel;
		sRing(&ringModel, rings->radiusMin, rings->radiusMax, 128, 32);
		
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.isRing, true));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.texture, 2));
		
		computeModelMatrix(modelMatrix);
		const Vec4d position = mTarget * modelMatrix.getColumn(3);
		shadowCandidatesData(0, 0) = position[0];
		shadowCandidatesData(1, 0) = position[1];
		shadowCandidatesData(2, 0) = position[2];
		shadowCandidatesData(3, 0) = getRadius();
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.shadowCount, 1));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.shadowData, shadowCandidatesData));
		
		projectedVertexArr.resize(ringModel.vertexArr.size());
		for (int i=0;i<ringModel.vertexArr.size()/3;++i)
			painter->getProjector()->project(*((Vec3f*)(ringModel.vertexArr.constData()+i*3)), *((Vec3f*)(projectedVertexArr.data()+i*3)));
		
		GL(ringPlanetShaderProgram->setAttributeArray(ringPlanetShaderVars.vertex, (const GLfloat*)projectedVertexArr.constData(), 3));
		GL(ringPlanetShaderProgram->enableAttributeArray(ringPlanetShaderVars.vertex));
		GL(ringPlanetShaderProgram->setAttributeArray(ringPlanetShaderVars.unprojectedVertex, (const GLfloat*)ringModel.vertexArr.constData(), 3));
		GL(ringPlanetShaderProgram->enableAttributeArray(ringPlanetShaderVars.unprojectedVertex));
		GL(ringPlanetShaderProgram->setAttributeArray(ringPlanetShaderVars.texCoord, (const GLfloat*)ringModel.texCoordArr.constData(), 2));
		GL(ringPlanetShaderProgram->enableAttributeArray(ringPlanetShaderVars.texCoord));
		
		if (eyePos[2]<0)
			glCullFace(GL_FRONT);
					
		GL(glDrawElements(GL_TRIANGLES, ringModel.indiceArr.size(), GL_UNSIGNED_SHORT, ringModel.indiceArr.constData()));
		
		if (eyePos[2]<0)
			glCullFace(GL_BACK);
		
		glDisable(GL_DEPTH_TEST);
	}
	
	GL(shader->release());
	
	glDisable(GL_CULL_FACE);
}


void Planet::drawHints(const StelCore* core, const QFont& planetNameFont)
{
	if (labelsFader.getInterstate()<=0.f)
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(planetNameFont);
	// Draw nameI18 + scaling if it's not == 1.
	float tmp = (hintFader.getInterstate()<=0 ? 7.f : 10.f) + getAngularSize(core)*M_PI/180.f*prj->getPixelPerRadAtCenter()/1.44f; // Shift for nameI18 printing
	sPainter.setColor(labelColor[0], labelColor[1], labelColor[2],labelsFader.getInterstate());
	sPainter.drawText(screenPos[0],screenPos[1], getSkyLabel(core), 0, tmp, tmp, false);

	// hint disappears smoothly on close view
	if (hintFader.getInterstate()<=0)
		return;
	tmp -= 10.f;
	if (tmp<1) tmp=1;
	sPainter.setColor(labelColor[0], labelColor[1], labelColor[2],labelsFader.getInterstate()*hintFader.getInterstate()/tmp*0.7f);

	// Draw the 2D small circle
	glEnable(GL_BLEND);
	sPainter.enableTexture2d(true);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Planet::hintCircleTex->bind();
	sPainter.drawSprite2dMode(screenPos[0], screenPos[1], 11);
}

Ring::Ring(float radiusMin, float radiusMax, const QString &texname)
	 :radiusMin(radiusMin),radiusMax(radiusMax)
{
	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/"+texname);
}


// draw orbital path of Planet
void Planet::drawOrbit(const StelCore* core)
{
	if (!orbitFader.getInterstate())
		return;
	if (!re.siderealPeriod)
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameHeliocentricEcliptic);

	StelPainter sPainter(prj);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	sPainter.setColor(orbitColor[0], orbitColor[1], orbitColor[2], orbitFader.getInterstate());
	Vec3d onscreen;
	// special case - use current Planet position as center vertex so that draws
	// on it's orbit all the time (since segmented rather than smooth curve)
	Vec3d savePos = orbit[ORBIT_SEGMENTS/2];
	orbit[ORBIT_SEGMENTS/2]=getHeliocentricEclipticPos();
	orbit[ORBIT_SEGMENTS]=orbit[0];
	int nbIter = closeOrbit ? ORBIT_SEGMENTS : ORBIT_SEGMENTS-1;
	QVarLengthArray<float, 1024> vertexArray;

	sPainter.enableClientStates(true, false, false);

	for (int n=0; n<=nbIter; ++n)
	{
		if (prj->project(orbit[n],onscreen) && (vertexArray.size()==0 || !prj->intersectViewportDiscontinuity(orbit[n-1], orbit[n])))
		{
			vertexArray.append(onscreen[0]);
			vertexArray.append(onscreen[1]);
		}
		else if (!vertexArray.isEmpty())
		{
			sPainter.setVertexPointer(2, GL_FLOAT, vertexArray.constData());
			sPainter.drawFromArray(StelPainter::LineStrip, vertexArray.size()/2, 0, false);
			vertexArray.clear();
		}
	}
	orbit[ORBIT_SEGMENTS/2]=savePos;
	if (!vertexArray.isEmpty())
	{
		sPainter.setVertexPointer(2, GL_FLOAT, vertexArray.constData());
		sPainter.drawFromArray(StelPainter::LineStrip, vertexArray.size()/2, 0, false);
	}
	sPainter.enableClientStates(false);
}

void Planet::update(int deltaTime)
{
	hintFader.update(deltaTime);
	labelsFader.update(deltaTime);
	orbitFader.update(deltaTime);
}
