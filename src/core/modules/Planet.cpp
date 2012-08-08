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

#include <iomanip>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QVarLengthArray>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"

#include "StelProjector.hpp"
#include "sideral_time.h"
#include "StelTextureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

Vec3f Planet::labelColor = Vec3f(0.4,0.4,0.8);
Vec3f Planet::orbitColor = Vec3f(1,0.6,1);
StelTextureSP Planet::hintCircleTex;
StelTextureSP Planet::texEarthShadow;
Planet::Planet(const QString& englishName,
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
			   bool hasAtmosphere)
	: englishName(englishName),
	  flagLighting(flagLighting),
	  radius(radius), oneMinusOblateness(1.0-oblateness),
	  color(color), albedo(albedo), axisRotation(0.), rings(NULL),
	  sphereScale(1.f),
	  lastJD(J2000),
	  coordFunc(coordFunc),
	  userDataPtr(auserDataPtr),
	  osculatingFunc(osculatingFunc),
	  parent(NULL),
	  hidden(hidden),
	  atmosphere(hasAtmosphere)
{
	texMapName = atexMapName;
	lastOrbitJD =0;
	deltaJD = StelCore::JD_SECOND;
	orbitCached = 0;
	closeOrbit = acloseOrbit;

	eclipticPos=Vec3d(0.,0.,0.);
	rotLocalToParent = Mat4d::identity();
	texMap = StelApp::getInstance().getTextureManager().createTextureThread("textures/"+texMapName, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

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

void Planet::translateName(StelTranslator& trans)
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

	if (flags&Magnitude)
	{
		if (core->getSkyDrawer()->getFlagHasAtmosphere())
		    oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
										    QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
		else
		    oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core, false), 0, 'f', 2) << "<br>";
	}
	if (flags&AbsoluteMagnitude)
		oss << q_("Absolute Magnitude: %1").arg(getVMagnitude(core, false)-5.*(std::log10(getJ2000EquatorialPos(core).length()*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if ((flags&Extra1) && (core->getCurrentLocation().planetName=="Earth"))
	{
		//static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
		//double ecl= -(ssystem->getEarth()->getRotObliquity()); // BUG DETECTED! Earth's obliquity is apparently reported constant.
		double ra_equ, dec_equ, lambda, beta;
		double ecl= get_mean_ecliptical_obliquity(core->getJDay()) *M_PI/180.0;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
		StelUtils::ctRadec2Ecl(ra_equ, dec_equ, ecl, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		oss << q_("Ecliptic Geocentric (of date): %1/%2").arg(StelUtils::radToDmsStr(lambda, true), StelUtils::radToDmsStr(beta, true)) << "<br>";
		oss << q_("Obliquity (of date): %1").arg(StelUtils::radToDmsStr(ecl, true)) << "<br>";
	}

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

	if ((flags&Extra2) && (englishName.compare("Sun")!=0))
	{
		const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
		const double observerRq = observerHelioPos.lengthSquared();
		const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
		const double planetRq = planetHelioPos.lengthSquared();
		const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
		const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
		float planetPhase = 0.5f * std::abs(1.f + cos_chi);
		oss << QString(q_("Phase Angle: %1")).arg(StelUtils::radToDmsStr(getPhase(core->getObserverHeliocentricEclipticPos()))) << "<br>";
		oss << QString(q_("Elongation: %1")).arg(StelUtils::radToDmsStr(getElongation(core->getObserverHeliocentricEclipticPos()))) << "<br>";
		oss << QString(q_("Phase: %1")).arg(planetPhase, 0, 'f', 2) << "<br>";
		oss << QString(q_("Illuminated: %1%")).arg(planetPhase * 100, 0, 'f', 1) << "<br>";		
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
		return getVMagnitude(core, false)-15.f;
	}
	else
	{
		return getVMagnitude(core, false) - 8.f;
	}
}

Vec3f Planet::getInfoColor(void) const
{
	Vec3f col = ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getLabelsColor();
	if (StelApp::getInstance().getVisionModeNight())
		col = StelUtils::getNightColor(col);
	return col;
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

// Set the orbital elements
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
void Planet::computePositionWithoutOrbits(const double date)
{
	if (fabs(lastJD-date)>deltaJD)
	{
		coordFunc(date, eclipticPos, userDataPtr);
		lastJD = date;
	}
}

void Planet::computePosition(const double date)
{

	if (orbitFader.getInterstate()>0.000001 && deltaOrbitJD > 0 && (fabs(lastOrbitJD-date)>deltaOrbitJD || !orbitCached))
	{

		// calculate orbit first (for line drawing)
		double date_increment = re.siderealPeriod/ORBIT_SEGMENTS;
		double calc_date;
		// int delta_points = (int)(0.5 + (date - lastOrbitJD)/date_increment);
		int delta_points;

		if( date > lastOrbitJD )
		{
			delta_points = (int)(0.5 + (date - lastOrbitJD)/date_increment);
		}
		else
		{
			delta_points = (int)(-0.5 + (date - lastOrbitJD)/date_increment);
		}
		double new_date = lastOrbitJD + delta_points*date_increment;

		// qDebug( "Updating orbit coordinates for %s (delta %f) (%d points)\n", name.c_str(), deltaOrbitJD, delta_points);

		if( delta_points > 0 && delta_points < ORBIT_SEGMENTS && orbitCached)
		{

			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				if(d + delta_points >= ORBIT_SEGMENTS )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;

					// date increments between points will not be completely constant though
					computeTransMatrix(calc_date);
					if (osculatingFunc)
					{
						(*osculatingFunc)(date,calc_date,eclipticPos);
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
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;

					computeTransMatrix(calc_date);
					if (osculatingFunc) {
						(*osculatingFunc)(date,calc_date,eclipticPos);
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
				calc_date = date + (d-ORBIT_SEGMENTS/2)*date_increment;
				computeTransMatrix(calc_date);
				if (osculatingFunc)
				{
					(*osculatingFunc)(date,calc_date,eclipticPos);
				}
				else
				{
					coordFunc(calc_date, eclipticPos, userDataPtr);
				}
				orbitP[d] = eclipticPos;
				orbit[d] = getHeliocentricEclipticPos();
			}

			lastOrbitJD = date;
			if (!osculatingFunc) orbitCached = 1;
		}


		// calculate actual Planet position
		coordFunc(date, eclipticPos, userDataPtr);

		lastJD = date;

	}
	else if (fabs(lastJD-date)>deltaJD)
	{
		// calculate actual Planet position
		coordFunc(date, eclipticPos, userDataPtr);
		for( int d=0; d<ORBIT_SEGMENTS; d++ )
			orbit[d]=getHeliocentricPos(orbitP[d]);
		lastJD = date;
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

	return remainder * 360. + re.offset;
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
	Vec3d pos = p;
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
double Planet::computeDistance(const Vec3d& obsHelioPos)
{
	distance = (obsHelioPos-getHeliocentricEclipticPos()).length();
	return distance;
}

// Get the phase angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getPhase(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	//return std::acos(observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	return std::acos((observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq)));
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
float Planet::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}


	if (parent == 0)
	{
		// sun, compute the apparent magnitude for the absolute mag (4.83) and observer's distance
		const double distParsec = std::sqrt(core->getObserverHeliocentricEclipticPos().lengthSquared())*AU/PARSEC;
		return 4.83 + 5.*(std::log10(distParsec)-1.) + extinctionMag;
	}

	// Compute the angular phase
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const double observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	double phase = std::acos(cos_chi);

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
			return -0.36 + d + 3.8*f1 - 2.73*f1*f1 + 2*f1*f1*f1 + extinctionMag;
		}
		if (englishName=="Venus")
			return -4.29 + d + 0.09*f1 + 2.39*f1*f1 - 0.65*f1*f1*f1 + extinctionMag;
		if (englishName=="Mars")
			return -1.52 + d + 0.016*phaseDeg + extinctionMag;
		if (englishName=="Jupiter")
			return -9.25 + d + 0.005*phaseDeg + extinctionMag;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
			// double rings = -2.6*sinx + 1.25*sinx*sinx;
			return -8.88 + d + 0.044*phaseDeg + extinctionMag;// + rings;
		}
		if (englishName=="Uranus")
			return -7.19 + d + 0.0028*phaseDeg + extinctionMag;
		if (englishName=="Neptune")
			return -6.87 + d + extinctionMag;
		if (englishName=="Pluto")
			return -1.01 + d + 0.041*phaseDeg + extinctionMag;
		*/
		// GZ: I prefer the values given by Meeus, Astronomical Algorithms (1992).
		// There are two solutions:
		// (1) G. Mller, based on visual observations 1877-91. [Expl.Suppl.1961]
		// (2) Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes.
		// The structure is almost identical, just the numbers are different!
		// I activate (1) for now, because we want to simulate the eye's impression. (Esp. Venus!)
		// (1)
		if (englishName=="Mercury")
		    {
			double ph50=phaseDeg-50.0;
			return 1.16 + d + 0.02838*ph50 + 0.0001023*ph50*ph50 + extinctionMag;
		    }
		if (englishName=="Venus")
			return -4.0 + d + 0.01322*phaseDeg + 0.0000004247*phaseDeg*phaseDeg*phaseDeg + extinctionMag;
		if (englishName=="Mars")
			return -1.3 + d + 0.01486*phaseDeg + extinctionMag;
		if (englishName=="Jupiter")
			return -8.93 + d + extinctionMag;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
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
			return -8.68 + d + 0.044*phaseDeg + rings + extinctionMag;
		}
		if (englishName=="Uranus")
			return -6.85 + d + extinctionMag;
		if (englishName=="Neptune")
			return -7.05 + d + extinctionMag;
		if (englishName=="Pluto")
			return -1.0 + d + extinctionMag;
		/*
		// (2)
		if (englishName=="Mercury")
			return 0.42 + d + .038*phaseDeg - 0.000273*phaseDeg*phaseDeg + 0.000002*phaseDeg*phaseDeg*phaseDeg + extinctionMag;
		if (englishName=="Venus")
			return -4.40 + d + 0.0009*phaseDeg + 0.000239*phaseDeg*phaseDeg - 0.00000065*phaseDeg*phaseDeg*phaseDeg + extinctionMag;
		if (englishName=="Mars")
			return -1.52 + d + 0.016*phaseDeg + extinctionMag;
		if (englishName=="Jupiter")
			return -9.40 + d + 0.005*phaseDeg + extinctionMag;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
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
			return -8.88 + d + 0.044*phaseDeg + rings + extinctionMag;
		}
		if (englishName=="Uranus")
			return -7.19f + d + extinctionMag;
		if (englishName=="Neptune")
			return -6.87f + d + extinctionMag;
		if (englishName=="Pluto")
			return -1.00f + d + extinctionMag;
	*/
	// TODO: decide which set of formulae is best?
	}

	// This formula seems to give wrong results
	const double p = (1.0 - phase/M_PI) * cos_chi + std::sqrt(1.0 - cos_chi*cos_chi) / M_PI;
	double F = 2.0 * albedo * radius * radius * p / (3.0*observerPlanetRq*planetRq) * shadowFactor;
	return -26.73 - 2.5 * std::log10(F) + extinctionMag;
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
			StelPainter sPainter(core->getProjection(transfo));
			rings->draw(&sPainter,transfo,1000.0);
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
		// this prevents name overlaping (ie for jupiter satellites)
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

void Planet::draw3dModel(StelCore* core, StelProjector::ModelViewTranformP transfo, float screenSz)
{
	// This is the main method drawing a planet 3d model
	// Some work has to be done on this method to make the rendering nicer

	if (screenSz>1.)
	{
		StelProjector::ModelViewTranformP transfo2 = transfo->clone();
		transfo2->combine(Mat4d::zrotation(M_PI/180*(axisRotation + 90.)));
		StelPainter* sPainter = new StelPainter(core->getProjection(transfo2));

		if (flagLighting)
		{
			sPainter->getLight().enable();

			// Set the main source of light to be the sun
			Vec3d sunPos(0);
			core->getHeliocentricEclipticModelViewTransform()->forward(sunPos);
			sPainter->getLight().setPosition(Vec4f(sunPos[0],sunPos[1],sunPos[2],1.f));

			// Set the light parameters taking sun as the light source
			static Vec4f diffuse = Vec4f(2.f,2.f,2.f,1.f);
			static Vec4f zero = Vec4f(0.f,0.f,0.f,0.f);
			static Vec4f ambient = Vec4f(0.02f,0.02f,0.02f,0.02f);
			if (StelApp::getInstance().getVisionModeNight())
			{
				diffuse[1] = 0.; diffuse[2] = 0.;
				ambient[1] = 0.; ambient[2] = 0.;
			}
			else
			{
				diffuse[1] = 2.; diffuse[2] = 2.;
				ambient[1] = 0.02; ambient[2] = 0.02;
			}
			sPainter->getLight().setAmbient(ambient);
			sPainter->getLight().setDiffuse(diffuse);
			sPainter->getLight().setSpecular(zero);

			sPainter->getMaterial().setAmbient(ambient);
			sPainter->getMaterial().setEmission(zero);
			sPainter->getMaterial().setShininess(0.f);
			sPainter->getMaterial().setSpecular(zero);
		}
		else
		{
			sPainter->getLight().disable();
			sPainter->setColor(1.f,1.f,1.f);
		}

		if (rings)
		{
			const double dist = getEquinoxEquatorialPos(core).length();
			double z_near = 0.9*(dist - rings->getSize());
			double z_far  = 1.1*(dist + rings->getSize());
			if (z_near < 0.0) z_near = 0.0;
			double n,f;
			core->getClippingPlanes(&n,&f); // Save clipping planes
			core->setClippingPlanes(z_near,z_far);
			glDepthMask(GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			drawSphere(sPainter, screenSz);
			glDepthMask(GL_FALSE);
			sPainter->getLight().disable();
			rings->draw(sPainter,transfo,screenSz);
			sPainter->getLight().enable();
			glDisable(GL_DEPTH_TEST);
			core->setClippingPlanes(n,f);  // Restore old clipping planes
		}
		else
		{
			SolarSystem* ssm = GETSTELMODULE(SolarSystem);
			if (this==ssm->getMoon() && core->getCurrentLocation().planetName=="Earth" && ssm->nearLunarEclipse())
			{
				// Draw earth shadow over moon using stencil buffer if appropriate
				// This effect curently only looks right from earth viewpoint
				// TODO: moon magnitude label during eclipse isn't accurate...
				glClearStencil(0x0);
				glClear(GL_STENCIL_BUFFER_BIT);
				glStencilFunc(GL_ALWAYS, 0x1, 0x1);
				glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
				glEnable(GL_STENCIL_TEST);
				drawSphere(sPainter, screenSz);
				glDisable(GL_STENCIL_TEST);

				sPainter->getLight().disable();
				drawEarthShadow(core, sPainter);
			}
			else
			{
				// Normal planet
				drawSphere(sPainter, screenSz);
			}
		}
		if (sPainter)
			delete sPainter;
		sPainter=NULL;
	}

	// Draw the halo

	// Prepare openGL lighting parameters according to luminance
	float surfArcMin2 = getSpheroidAngularSize(core)*60;
	surfArcMin2 = surfArcMin2*surfArcMin2*M_PI; // the total illuminated area in arcmin^2

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	Vec3d tmp = getJ2000EquatorialPos(core);
	core->getSkyDrawer()->postDrawSky3dModel(&sPainter, tmp, surfArcMin2, getVMagnitude(core, true), color);
}


void Planet::drawSphere(StelPainter* painter, float screenSz)
{
	if (texMap)
	{
		// For lazy loading, return if texture not yet loaded
		if (!texMap->bind())
		{
			return;
		}
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (StelApp::getInstance().getVisionModeNight())
		painter->setColor(1.f, 0.f, 0.f);
	else
		painter->setColor(1.f, 1.f, 1.f);

	painter->enableTexture2d(true);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	// Draw the spheroid itself
	// Adapt the number of facets according with the size of the sphere for optimization
	int nb_facet = (int)(screenSz * 40/50);	// 40 facets for 1024 pixels diameter on screen
	if (nb_facet<10) nb_facet = 10;
	if (nb_facet>40) nb_facet = 40;
	painter->setShadeModel(StelPainter::ShadeModelSmooth);
	// Rotate and add an extra quarter rotation so that the planet texture map
	// fits to the observers position. No idea why this is necessary,
	// perhaps some openGl strangeness, or confusing sin/cos.

	painter->sSphere(radius*sphereScale, oneMinusOblateness, nb_facet, nb_facet);
	painter->setShadeModel(StelPainter::ShadeModelFlat);
	glDisable(GL_CULL_FACE);
}

// draws earth shadow overlapping the moon using stencil buffer
// umbra and penumbra are sized separately for accuracy
void Planet::drawEarthShadow(StelCore* core, StelPainter* sPainter)
{
	SolarSystem* ssm = GETSTELMODULE(SolarSystem);
	Vec3d e = ssm->getEarth()->getEclipticPos();
	Vec3d m = ssm->getMoon()->getEclipticPos();  // relative to earth
	Vec3d mh = ssm->getMoon()->getHeliocentricEclipticPos();  // relative to sun
	float mscale = ssm->getMoon()->getSphereScale();

	// shadow location at earth + moon distance along earth vector from sun
	Vec3d en = e;
	en.normalize();
	Vec3d shadow = en * (e.length() + m.length());

	// find shadow radii in AU
	double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000./AU;
	double r_umbra = 6378.1/AU - m.length()*(689621.9/AU/e.length());

	// find vector orthogonal to sun-earth vector using cross product with
	// a non-parallel vector
	Vec3d rpt = shadow^Vec3d(0,0,1);
	rpt.normalize();
	Vec3d upt = rpt*r_umbra*mscale*1.02;  // point on umbra edge
	rpt *= r_penumbra*mscale;  // point on penumbra edge

	// modify shadow location for scaled moon
	Vec3d mdist = shadow - mh;
	if (mdist.length() > r_penumbra + 2000./AU)
		return;   // not visible so don't bother drawing

	shadow = mh + mdist*mscale;

	StelProjectorP saveProj = sPainter->getProjector();
	sPainter->setProjector(core->getProjection(StelCore::FrameHeliocentricEcliptic));

	sPainter->enableTexture2d(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter->setColor(1,1,1);

	glEnable(GL_STENCIL_TEST);
	// We draw only where the stencil buffer is at 1, i.e. where the moon was drawn
	glStencilFunc(GL_EQUAL, 0x1, 0x1);
	// Don't change stencil buffer value
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// shadow radial texture
	texEarthShadow->bind();

	Vec3d r;

	// Draw umbra first
	QVector<Vec2f> texCoordArray;
	QVector<Vec3d> vertexArray;
	texCoordArray.reserve(210);
	vertexArray.reserve(210);
	texCoordArray << Vec2f(0.f, 0.5f);
	// johannes: work-around for nasty ATI rendering bug: use y-texture coordinate of 0.5 instead of 0.0
	vertexArray << shadow;

	const Mat4d& rotMat = Mat4d::rotation(shadow, 2.*M_PI/100.);
	r = upt;
	for (int i=1; i<=101; ++i)
	{
		// position in texture of umbra edge
		texCoordArray << Vec2f(0.6f, 0.5f);
		r.transfo4d(rotMat);
		vertexArray << shadow + r;
	}
	sPainter->setArrays(vertexArray.constData(), texCoordArray.constData());
	sPainter->drawFromArray(StelPainter::TriangleFan, 102);

	// now penumbra
	vertexArray.resize(0);
	texCoordArray.resize(0);
	Vec3d u;
	r = rpt;
	u = upt;
	for (int i=0; i<=200; i+=2)
	{
		r.transfo4d(rotMat);
		u.transfo4d(rotMat);
		texCoordArray << Vec2f(0.6f, 0.5f) << Vec2f(1.f, 0.5f); // position in texture of umbra edge
		vertexArray <<  shadow + u << shadow + r;
	}
	sPainter->setArrays(vertexArray.constData(), texCoordArray.constData());
	sPainter->drawFromArray(StelPainter::TriangleStrip, 202);
	glDisable(GL_STENCIL_TEST);
	glClearStencil(0x0);
	glClear(GL_STENCIL_BUFFER_BIT);	// Clean again to let a clean buffer for later Qt display
	sPainter->setProjector(saveProj);
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

	// hint disapears smoothly on close view
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

Ring::Ring(double radiusMin,double radiusMax,const QString &texname)
	 :radiusMin(radiusMin),radiusMax(radiusMax)
{
	tex = StelApp::getInstance().getTextureManager().createTexture("textures/"+texname);
}

Ring::~Ring(void)
{
}

void Ring::draw(StelPainter* sPainter,StelProjector::ModelViewTranformP transfo,double screenSz)
{
	screenSz -= 50;
	screenSz /= 250.0;
	if (screenSz < 0.0) screenSz = 0.0;
	else if (screenSz > 1.0) screenSz = 1.0;
	const int slices = 128+(int)((256-128)*screenSz);
	const int stacks = 8+(int)((32-8)*screenSz);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (StelApp::getInstance().getVisionModeNight())
		sPainter->setColor(1.f, 0.f, 0.f);
	else
		sPainter->setColor(1.f, 1.f, 1.f);

	sPainter->enableTexture2d(true);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	if (tex) tex->bind();

	Mat4d mat = transfo->getApproximateLinearTransfo();
	  // solve the ring wraparound by culling:
	  // decide if we are above or below the ring plane
	const double h = mat.r[ 8]*mat.r[12]
				   + mat.r[ 9]*mat.r[13]
				   + mat.r[10]*mat.r[14];
	sPainter->sRing(radiusMin,radiusMax,(h<0.0)?slices:-slices,stacks, 0);
	glDisable(GL_CULL_FACE);
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
