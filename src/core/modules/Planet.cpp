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

#include <QOpenGLFunctions_1_0>
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelTexture.hpp"
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "LandscapeMgr.hpp"
#include "Planet.hpp"
#include "Orbit.hpp"
#include "planetsephems/precession.h"
#include "StelObserver.hpp"
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
#include "StelOBJ.hpp"
#include "StelOpenGLArray.hpp"

#include <limits>
#include <QByteArray>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QVarLengthArray>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#ifdef DEBUG_SHADOWMAP
#include <QOpenGLFramebufferObject>
#endif
#include <QOpenGLShader>
#include <QtConcurrent>

const QString Planet::PLANET_TYPE = QStringLiteral("Planet");

bool Planet::shaderError = false;

Vec3f Planet::labelColor = Vec3f(0.4f,0.4f,0.8f);
Vec3f Planet::orbitColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitMajorPlanetsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitMoonsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitMinorPlanetsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitDwarfPlanetsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitCubewanosColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitPlutinosColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitScatteredDiscObjectsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitOortCloudObjectsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitSednoidsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitCometsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitMercuryColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitVenusColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitEarthColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitMarsColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitJupiterColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitSaturnColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitUranusColor = Vec3f(1.0f,0.6f,1.0f);
Vec3f Planet::orbitNeptuneColor = Vec3f(1.0f,0.6f,1.0f);
StelTextureSP Planet::hintCircleTex;
StelTextureSP Planet::texEarthShadow;

bool Planet::permanentDrawingOrbits = false;
Planet::PlanetOrbitColorStyle Planet::orbitColorStyle = Planet::ocsOneColor;

bool Planet::flagCustomGrsSettings = false;
double Planet::customGrsJD = 2456901.5;
double Planet::customGrsDrift = 15.;
int Planet::customGrsLongitude = 216;

QOpenGLShaderProgram* Planet::planetShaderProgram=Q_NULLPTR;
Planet::PlanetShaderVars Planet::planetShaderVars;
QOpenGLShaderProgram* Planet::ringPlanetShaderProgram=Q_NULLPTR;
Planet::PlanetShaderVars Planet::ringPlanetShaderVars;
QOpenGLShaderProgram* Planet::moonShaderProgram=Q_NULLPTR;
Planet::PlanetShaderVars Planet::moonShaderVars;
QOpenGLShaderProgram* Planet::objShaderProgram=Q_NULLPTR;
Planet::PlanetShaderVars Planet::objShaderVars;
QOpenGLShaderProgram* Planet::objShadowShaderProgram=Q_NULLPTR;
Planet::PlanetShaderVars  Planet::objShadowShaderVars;
QOpenGLShaderProgram* Planet::transformShaderProgram=Q_NULLPTR;
Planet::PlanetShaderVars Planet::transformShaderVars;

bool Planet::shadowInitialized = false;
Vec2f Planet::shadowPolyOffset = Vec2f(0.0f, 0.0f);
#ifdef DEBUG_SHADOWMAP
QOpenGLFramebufferObject* Planet::shadowFBO = Q_NULLPTR;
#else
GLuint Planet::shadowFBO = 0;
#endif
GLuint Planet::shadowTex = 0;


QMap<Planet::PlanetType, QString> Planet::pTypeMap;
QMap<Planet::ApparentMagnitudeAlgorithm, QString> Planet::vMagAlgorithmMap;
Planet::ApparentMagnitudeAlgorithm Planet::vMagAlgorithm;


#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)
#define SM_SIZE 1024

Planet::PlanetOBJModel::PlanetOBJModel()
	: needsRescale(true), projPosBuffer(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer)), obj(new StelOBJ()), arr(new StelOpenGLArray())
{
	//The buffer is refreshed completely before each draw, so StreamDraw should be ok
	projPosBuffer->setUsagePattern(QOpenGLBuffer::StreamDraw);
}

Planet::PlanetOBJModel::~PlanetOBJModel()
{
	delete projPosBuffer;
	delete obj;
	delete arr;
}

bool Planet::PlanetOBJModel::loadGL()
{
	if(arr->load(obj,false))
	{
		//delete StelOBJ because the data is no longer needed
		delete obj;
		obj = Q_NULLPTR;
		//make sure the vector has enough space to hold the projected data
		projectedPosArray.resize(posArray.size());
		//create the GL buffer for the projection
		return projPosBuffer->create();
	}
	return false;
}

void Planet::PlanetOBJModel::performScaling(double scale)
{
	scaledArray = posArray;

	//pre-scale the cpu-side array
	for(int i = 0; i<posArray.size();++i)
	{
		scaledArray[i]*=scale;
	}

	needsRescale = false;
}

Planet::Planet(const QString& englishName,
	       double radius,
	       double oblateness,
	       Vec3f halocolor,
	       float albedo,
	       float roughness,
	       const QString& atexMapName,
	       const QString& anormalMapName,
	       const QString& aobjModelName,
	       posFuncType coordFunc,
	       void* anOrbitPtr,
	       OsculatingFunctType *osculatingFunc,
	       bool acloseOrbit,
	       bool hidden,
	       bool hasAtmosphere,
	       bool hasHalo,
	       const QString& pTypeStr)
	: flagNativeName(true),
	  flagTranslatedName(true),
	  lastOrbitJDE(0.0),
	  deltaJDE(StelCore::JD_SECOND),
	  deltaOrbitJDE(0.0),
	  orbitCached(false),
	  closeOrbit(acloseOrbit),
	  englishName(englishName),
	  nameI18(englishName),
	  nativeName(""),
	  texMapName(atexMapName),
	  normalMapName(anormalMapName),
	  radius(radius),
	  oneMinusOblateness(1.0-oblateness),
	  eclipticPos(0.,0.,0.),
	  eclipticVelocity(0.,0.,0.),
	  haloColor(halocolor),
	  absoluteMagnitude(-99.0f),
	  albedo(albedo),
	  roughness(roughness),
	  outgas_intensity(0.f),
	  outgas_falloff(0.f),
	  rotLocalToParent(Mat4d::identity()),
	  axisRotation(0.),
	  objModel(Q_NULLPTR),
	  objModelLoader(Q_NULLPTR),
	  rings(Q_NULLPTR),
	  distance(0.0),
	  sphereScale(1.f),
	  lastJDE(J2000),
	  coordFunc(coordFunc),
	  orbitPtr(anOrbitPtr),
	  osculatingFunc(osculatingFunc),
	  parent(Q_NULLPTR),
	  flagLabels(true),
	  hidden(hidden),
	  atmosphere(hasAtmosphere),
	  halo(hasHalo),
	  gl(Q_NULLPTR),
	  iauMoonNumber("")
{
	// Initialize pType with the key found in pTypeMap, or mark planet type as undefined.
	// The latter condition should obviously never happen.
	pType = pTypeMap.key(pTypeStr, Planet::isUNDEFINED);
	// 0.16: Ensure type is always given!
	// AW: I've commented the code to the allow flying on spaceship (implemented as an artificial planet)!
	if (pType==Planet::isUNDEFINED)
	{
		qCritical() << "Planet " << englishName << "has no type. Please edit one of ssystem_major.ini or ssystem_minor.ini to ensure operation.";
		exit(-1);
	}
	Q_ASSERT(pType != Planet::isUNDEFINED);

	//only try loading textures when there is actually something to load!
	//prevents some overhead when starting
	if(!texMapName.isEmpty())
	{
		// TODO: use StelFileMgr::findFileInAllPaths() after introducing an Add-On Manager
		QString texMapFile = StelFileMgr::findFile("textures/"+texMapName, StelFileMgr::File);
		if (!texMapFile.isEmpty())
			texMap = StelApp::getInstance().getTextureManager().createTextureThread(texMapFile, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
		else
			qWarning()<<"Cannot resolve path to texture file"<<texMapName<<"of object"<<englishName;
	}

	if(!normalMapName.isEmpty())
	{
		// TODO: use StelFileMgr::findFileInAllPaths() after introducing an Add-On Manager
		QString normalMapFile = StelFileMgr::findFile("textures/"+normalMapName, StelFileMgr::File);
		if (!normalMapFile.isEmpty())
			normalMap = StelApp::getInstance().getTextureManager().createTextureThread(normalMapFile, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
	}
	//the OBJ is lazily loaded when first required
	if(!aobjModelName.isEmpty())
	{
		// TODO: use StelFileMgr::findFileInAllPaths() after introducing an Add-On Manager
		objModelPath = StelFileMgr::findFile("models/"+aobjModelName, StelFileMgr::File);
		if(objModelPath.isEmpty())
		{
			qWarning()<<"Cannot resolve path to model file"<<aobjModelName<<"of object"<<englishName;
		}
	}
	if (englishName!="Pluto") // TODO: add some far-out slow object types: other Plutoids, KBO, SDO, OCO, ...
	{
		deltaJDE = 0.001*StelCore::JD_SECOND;
	}
}

// called in SolarSystem::init() before first planet is created. Loads pTypeMap.
void Planet::init()
{
	if (pTypeMap.count() > 0 )
	{
		// This should never happen. But it's uncritical.
		qDebug() << "Planet::init(): Non-empty static map. This is a programming error, but we can fix that.";
		pTypeMap.clear();
	}
	pTypeMap.insert(Planet::isStar,		"star");
	pTypeMap.insert(Planet::isPlanet,	"planet");
	pTypeMap.insert(Planet::isMoon,		"moon");
	pTypeMap.insert(Planet::isObserver,	"observer");
	pTypeMap.insert(Planet::isArtificial,	"artificial");
	pTypeMap.insert(Planet::isAsteroid,	"asteroid");
	pTypeMap.insert(Planet::isPlutino,	"plutino");
	pTypeMap.insert(Planet::isComet,	"comet");
	pTypeMap.insert(Planet::isDwarfPlanet,	"dwarf planet");
	pTypeMap.insert(Planet::isCubewano,	"cubewano");
	pTypeMap.insert(Planet::isSDO,		"scattered disc object");
	pTypeMap.insert(Planet::isOCO,		"Oort cloud object");
	pTypeMap.insert(Planet::isSednoid,	"sednoid");
	pTypeMap.insert(Planet::isUNDEFINED,	"UNDEFINED"); // something must be broken before we ever see this!

	if (vMagAlgorithmMap.count() > 0)
	{
		qDebug() << "Planet::init(): Non-empty static map. This is a programming error, but we can fix that.";
		vMagAlgorithmMap.clear();
	}
	vMagAlgorithmMap.insert(Planet::ExplanatorySupplement_2013,	"ExpSup2013");
	vMagAlgorithmMap.insert(Planet::ExplanatorySupplement_1992,	"ExpSup1992");
	vMagAlgorithmMap.insert(Planet::Mueller_1893,	"Mueller1893"); // better name	
	vMagAlgorithmMap.insert(Planet::AstronomicalAlmanac_1984,	"AstrAlm1984"); // consistent name
	vMagAlgorithmMap.insert(Planet::Generic,	"Generic"),	
	vMagAlgorithmMap.insert(Planet::UndefinedAlgorithm, "");
}

Planet::~Planet()
{
	delete rings;
	delete objModel;
}

void Planet::translateName(const StelTranslator& trans)
{
	if (!nativeName.isEmpty() && getFlagNativeName())
	{
		if (getFlagTranslatedName())
			nameI18 = trans.qtranslate(nativeName);
		else
			nameI18 = nativeName;
	}
	else
	{
		if (getFlagTranslatedName())
			nameI18 = trans.qtranslate(englishName, getContextString());
		else
			nameI18 = englishName;
	}
}

void Planet::setIAUMoonNumber(QString designation)
{
	if (!iauMoonNumber.isEmpty())
		return;

	iauMoonNumber = designation;
}

QString Planet::getEnglishName() const
{
	return englishName;
}

QString Planet::getNameI18n() const
{
	return nameI18;
}

const QString Planet::getContextString() const
{
	QString context = "";
	switch (getPlanetType())
	{
		case isStar:
			context = "star";
			break;
		case isPlanet:
			context = "major planet";
			break;
		case isMoon:
			context = "moon";
			break;
		case isObserver:
		case isArtificial:
			context = "special celestial body";
			break;
		case isAsteroid:
		case isPlutino:
		case isDwarfPlanet:
		case isCubewano:
		case isSDO:
		case isOCO:
		case isSednoid:
			context = "minor planet";
			break;
		case isComet:
			context = "comet";
			break;
		default:
			context = "";
			break;
	}

	return context;
}

// Return the information string "ready to print" :)
QString Planet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double distanceAu = getJ2000EquatorialPos(core).length();
	Q_UNUSED(az_app);

	if (flags&Name)
	{
		oss << "<h2>" << getNameI18n();  // UI translation can differ from sky translation
		if (!iauMoonNumber.isEmpty())
			oss << QString(" (%1)").arg(iauMoonNumber);
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
	}

	if (flags&ObjectType && getPlanetType()!=isUNDEFINED)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_(getPlanetTypeString())) << "<br />";
	}

	if (flags&Magnitude && getVMagnitude(core)!=std::numeric_limits<float>::infinity())
	{
		QString emag = "";
		if (core->getSkyDrawer()->getFlagHasAtmosphere() && (alt_app>-3.0*M_PI/180.0)) // Don't show extincted magnitude much below horizon where model is meaningless.
			emag = QString(" (%1: <b>%2</b>)").arg(q_("extincted to"), QString::number(getVMagnitudeWithExtinction(core), 'f', 2));

		oss << QString("%1: <b>%2</b>%3").arg(q_("Magnitude"), QString::number(getVMagnitude(core), 'f', 2), emag) << "<br />";
	}
	if (flags&AbsoluteMagnitude && (getAbsoluteMagnitude() > -99.))
	{
		oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getAbsoluteMagnitude(), 0, 'f', 2) << "<br />";
		const float moMag=getMeanOppositionMagnitude();
		if (moMag<50.f)
			oss << QString("%1: %2").arg(q_("Mean Opposition Magnitude")).arg(moMag, 0, 'f', 2) << "<br />";
	}

	oss << getCommonInfoString(core, flags);

	// Debug help.
	//oss << "Apparent Magnitude Algorithm: " << getApparentMagnitudeAlgorithmString() << " " << vMagAlgorithm << "<br>";

	// GZ This is mostly for debugging. Maybe also useful for letting people use our results to cross-check theirs, but we should not act as reference, currently...
	// TODO: maybe separate this out into:
	//if (flags&EclipticCoordXYZ)
	// For now: add to EclipticCoordJ2000 group
	//if (flags&EclipticCoordJ2000)
	//{
	//	Vec3d eclPos=(englishName=="Sun" ? GETSTELMODULE(SolarSystem)->getLightTimeSunPosition() : eclipticPos);
	//	oss << q_("Ecliptical XYZ (VSOP87A): %1/%2/%3").arg(QString::number(eclPos[0], 'f', 7), QString::number(eclPos[1], 'f', 7), QString::number(eclPos[2], 'f', 7)) << "<br>";
	//}

	if (flags&Distance)
	{
		double hdistanceAu = getHeliocentricEclipticPos().length();
		double hdistanceKm = AU * hdistanceAu;
		// TRANSLATORS: Unit of measure for distance - astronomical unit
		QString au = qc_("AU", "distance, astronomical unit");
		// TRANSLATORS: Unit of measure for distance - kilometers
		QString km = qc_("km", "distance");
		QString distAU, distKM;
		if (englishName!="Sun")
		{
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

			oss << QString("%1: %2%3 (%4 %5)").arg(q_("Distance from Sun"), distAU, au, distKM, km) << "<br />";
		}
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
			// TRANSLATORS: Unit of measure for distance - milliones kilometers
			km = qc_("M km", "distance");
		}

		oss << QString("%1: %2%3 (%4 %5)").arg(q_("Distance"), distAU, au, distKM, km) << "<br />";
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
//			if (englishName=="Moon")
//				orbVelKms=orbVel;
			oss << QString("%1: %2 %3").arg(q_("Orbital Velocity")).arg(orbVelKms, 0, 'f', 3).arg(kms) << "<br />";
			double helioVel=getHeliocentricEclipticVelocity().length();
			if (helioVel!=orbVel)
				oss << QString("%1: %2 %3").arg(q_("Heliocentric Velocity")).arg(helioVel* AU/86400., 0, 'f', 3).arg(kms) << "<br />";
		}
	}


	double angularSize = 2.*getAngularSize(core)*M_PI/180.;
	if (flags&Size && angularSize>=4.8e-7)
	{
		QString s1, s2, sizeStr = "";
		if (rings)
		{
			double withoutRings = 2.*getSpheroidAngularSize(core)*M_PI/180.;
			if (withDecimalDegree)
			{
				s1 = StelUtils::radToDecDegStr(withoutRings,4,false,true);
				s2 = StelUtils::radToDecDegStr(angularSize,4,false,true);
			}
			else
			{
				s1 = StelUtils::radToDmsStr(withoutRings, true);
				s2 = StelUtils::radToDmsStr(angularSize, true);
			}

			sizeStr = QString("%1, %2: %3").arg(s1, q_("with rings"), s2);
		}
		else
		{
			if (sphereScale!=1.f) // We must give correct diameters even if upscaling (e.g. Moon)
			{
				if (withDecimalDegree)
				{
					s1 = StelUtils::radToDecDegStr(angularSize / sphereScale,5,false,true);
					s2 = StelUtils::radToDecDegStr(angularSize,5,false,true);
				}
				else
				{
					s1 = StelUtils::radToDmsStr(angularSize / sphereScale, true);
					s2 = StelUtils::radToDmsStr(angularSize, true);
				}

				sizeStr = QString("%1, %2: %3").arg(s1, q_("scaled up to"), s2);
			}
			else
			{
				if (withDecimalDegree)
					sizeStr = StelUtils::radToDecDegStr(angularSize,5,false,true);
				else
					sizeStr = StelUtils::radToDmsStr(angularSize, true);
			}
		}
		oss << QString("%1: %2").arg(q_("Apparent diameter"), sizeStr) << "<br />";
	}

	double siderealPeriod = getSiderealPeriod();
	double siderealDay = getSiderealDay();
	if (flags&Extra)
	{
		// This is a string you can activate for debugging. It shows the distance between observer and center of the body you are standing on.
		// May be helpful for debugging critical parallax corrections for eclipses.
		// For general use, find a better location first.
		// oss << q_("Planetocentric distance &rho;: %1 (km)").arg(core->getCurrentObserver()->getDistanceFromCenter() * AU) <<"<br>";
		if (siderealPeriod>0)
		{
			QString days = qc_("days", "duration");
			// Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
			oss << QString("%1: %2 %3 (%4 a)").arg(q_("Sidereal period"), QString::number(siderealPeriod, 'f', 2), days, QString::number(siderealPeriod/365.25, 'f', 3)) << "<br />";

			if (qAbs(siderealDay)>0)
			{
				oss << QString("%1: %2").arg(q_("Sidereal day"), StelUtils::hoursToHmsStr(qAbs(siderealDay*24))) << "<br />";
				oss << QString("%1: %2").arg(q_("Mean solar day"), StelUtils::hoursToHmsStr(qAbs(getMeanSolarDay()*24))) << "<br />";
			}
			else if (re.period==0.)
			{
				oss << q_("The period of rotation is chaotic") << "<br />";
			}
		}
		if (englishName != "Sun")
		{
			const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
			const double elongation = getElongation(observerHelioPos);
			QString moonPhase = "";
			if (englishName=="Moon" && core->getCurrentLocation().planetName=="Earth")
			{
				double eclJDE = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(core->getJDE());
				double ra_equ, dec_equ, lambdaMoon, lambdaSun, beta;
				StelUtils::rectToSphe(&ra_equ,&dec_equ, getEquinoxEquatorialPos(core));
				StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaMoon, &beta);
				StelUtils::rectToSphe(&ra_equ,&dec_equ, GETSTELMODULE(SolarSystem)->searchByEnglishName("Sun")->getEquinoxEquatorialPos(core));
				StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaSun, &beta);
				int deltaLong = (int)(lambdaMoon*180.f/M_PI - lambdaSun*180.f/M_PI);
				if (deltaLong<0) deltaLong+=360;
				if (deltaLong==45)
					moonPhase = qc_("Waxing Crescent", "Moon phase");
				if (deltaLong==90)
					moonPhase = qc_("First Quarter", "Moon phase");
				if (deltaLong==135)
					moonPhase = qc_("Waxing Gibbous", "Moon phase");
				if (deltaLong==180)
					moonPhase = qc_("Full Moon", "Moon phase");
				if (deltaLong==225)
					moonPhase = qc_("Waning Gibbous", "Moon phase");
				if (deltaLong==270)
					moonPhase = qc_("Third Quarter", "Moon phase");
				if (deltaLong==315)
					moonPhase = qc_("Waning Crescent", "Moon phase");
				if (deltaLong==0 || deltaLong==360)
					moonPhase = qc_("New Moon", "Moon phase");
			}

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
			oss << QString("%1: %2%").arg(q_("Illuminated"), QString::number(getPhase(observerHelioPos) * 100, 'f', 1)) << "<br />";
			oss << QString("%1: %2").arg(q_("Albedo"), QString::number(getAlbedo(), 'f', 3)) << "<br />";
			if (!moonPhase.isEmpty())
				oss << QString("%1: %2").arg(q_("Phase"), moonPhase) << "<br />";

		}
		if (englishName=="Sun")
		{
			// Only show during eclipse, show percent?
			static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
			float eclipseFactor = 100.f*(1.f-ssystem->getEclipseFactor(core));
			if (eclipseFactor>1.e-7) // needed to avoid false display of 1e-14 or so.
				oss << QString("%1: %2%").arg(q_("Eclipse Factor")).arg(QString::number(eclipseFactor, 'f', 2)) << "<br />";

		}
	}

	postProcessInfoString(str, flags);

	return str;
}

QVariantMap Planet::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	if (getEnglishName()!="Sun")
	{
		const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
		map.insert("distance", getJ2000EquatorialPos(core).length());
		double phase=getPhase(observerHelioPos);
		map.insert("phase", phase);
		map.insert("illumination", 100.*phase);
		double phaseAngle = getPhaseAngle(observerHelioPos);
		map.insert("phase-angle", phaseAngle);
		map.insert("phase-angle-dms", StelUtils::radToDmsStr(phaseAngle));
		map.insert("phase-angle-deg", StelUtils::radToDecDegStr(phaseAngle));
		double elongation = getElongation(observerHelioPos);
		map.insert("elongation", elongation);
		map.insert("elongation-dms", StelUtils::radToDmsStr(elongation));
		map.insert("elongation-deg", StelUtils::radToDecDegStr(elongation));
		map.insert("type", getPlanetTypeString()); // replace existing "type=Planet" by something more detailed.
		// TBD: Is there ANY reason to keep "type"="Planet" and add a "ptype"=getPlanetTypeString() field?
		map.insert("velocity", getEclipticVelocity().toString());
		map.insert("heliocentric-velocity", getHeliocentricEclipticVelocity().toString());

	}

	return map;
}


//! Get sky label (sky translation)
QString Planet::getSkyLabel(const StelCore*) const
{
	QString str;
	QTextStream oss(&str);
	oss.setRealNumberPrecision(3);
	oss << getNameI18n();

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

	deltaOrbitJDE = re.siderealPeriod/ORBIT_SEGMENTS;
}

Vec3d Planet::getJ2000EquatorialPos(const StelCore *core) const
{
	if (englishName=="Sun")
		return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(GETSTELMODULE(SolarSystem)->getLightTimeSunPosition() - core->getObserverHeliocentricEclipticPos());
	else
		return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(getHeliocentricEclipticPos() - core->getObserverHeliocentricEclipticPos());
}

// Compute the position in the parent Planet coordinate system
// Actually call the provided function to compute the ecliptical position
void Planet::computePositionWithoutOrbits(const double dateJDE)
{
	if (fabs(lastJDE-dateJDE)>deltaJDE)
	{
		coordFunc(dateJDE, eclipticPos, eclipticVelocity, orbitPtr);
		lastJDE = dateJDE;
	}
}

// return value in radians!
// For Earth, this is epsilon_A, the angle between earth's rotational axis and mean ecliptic of date.
// Details: e.g. Hilton etal, Report on Precession and the Ecliptic, Cel.Mech.Dyn.Astr.94:351-67 (2006), Fig1.
double Planet::getRotObliquity(double JDE) const
{
	// JDay=2451545.0 for J2000.0
	if (englishName=="Earth")
		return getPrecessionAngleVondrakEpsilon(JDE);
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
	// Test satellites mutual occultations.
	if (parent.data() != sun) {
		foreach (const PlanetP& planet, parent.data()->satellites)
		{
			//skip self-shadowing
			if(planet.data() == this )
				continue;
			if (willCastShadow(this, planet.data()))
				res.append(planet.data());
		}
	}
	
	return res;
}

void Planet::computePosition(const double dateJDE)
{
	// Make sure the parent position is computed for the dateJDE, otherwise
	// getHeliocentricPos() would return incorrect values.
	if (parent)
		parent->computePositionWithoutOrbits(dateJDE);

	if (orbitFader.getInterstate()>0.000001 && deltaOrbitJDE > 0 && (fabs(lastOrbitJDE-dateJDE)>deltaOrbitJDE || !orbitCached))
	{
		StelCore *core=StelApp::getInstance().getCore();

		double calc_date;
		// int delta_points = (int)(0.5 + (date - lastOrbitJD)/date_increment);
		int delta_points;

		if( dateJDE > lastOrbitJDE )
		{
			delta_points = (int)(0.5 + (dateJDE - lastOrbitJDE)/deltaOrbitJDE);
		}
		else
		{
			delta_points = (int)(-0.5 + (dateJDE - lastOrbitJDE)/deltaOrbitJDE);
		}
		double new_date = lastOrbitJDE + delta_points*deltaOrbitJDE;

		// qDebug( "Updating orbit coordinates for %s (delta %f) (%d points)\n", getEnglishName().toUtf8().data(), deltaOrbitJDE, delta_points);

		if( delta_points > 0 && delta_points < ORBIT_SEGMENTS && orbitCached)
		{

			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				if(d + delta_points >= ORBIT_SEGMENTS-1 )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*deltaOrbitJDE;

					// date increments between points will not be completely constant though
					computeTransMatrix(calc_date-core->computeDeltaT(calc_date)/86400.0, calc_date);
					if (osculatingFunc)
					{
						(*osculatingFunc)(dateJDE,calc_date,eclipticPos, eclipticVelocity);
					}
					else
					{
						coordFunc(calc_date, eclipticPos, eclipticVelocity, orbitPtr);
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

			lastOrbitJDE = new_date;
		}
		else if( delta_points < 0 && abs(delta_points) < ORBIT_SEGMENTS  && orbitCached)
		{

			for( int d=ORBIT_SEGMENTS-1; d>=0; d-- )
			{
				if(d + delta_points < 0 )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*deltaOrbitJDE;

					computeTransMatrix(calc_date-core->computeDeltaT(calc_date)/86400.0, calc_date);
					if (osculatingFunc) {
						(*osculatingFunc)(dateJDE,calc_date,eclipticPos, eclipticVelocity);
					}
					else
					{
						coordFunc(calc_date, eclipticPos, eclipticVelocity, orbitPtr);
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

			lastOrbitJDE = new_date;

		}
		else if( delta_points || !orbitCached)
		{

			// update all points (less efficient)
			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				calc_date = dateJDE + (d-ORBIT_SEGMENTS/2)*deltaOrbitJDE;
				computeTransMatrix(calc_date-core->computeDeltaT(calc_date)/86400.0, calc_date);
				if (osculatingFunc)
				{
					(*osculatingFunc)(dateJDE,calc_date,eclipticPos, eclipticVelocity);
				}
				else
				{
					coordFunc(calc_date, eclipticPos, eclipticVelocity, orbitPtr);
				}
				orbitP[d] = eclipticPos;
				orbit[d] = getHeliocentricEclipticPos();
			}

			lastOrbitJDE = dateJDE;
			if (!osculatingFunc) orbitCached = 1;
		}


		// calculate actual Planet position
		coordFunc(dateJDE, eclipticPos, eclipticVelocity, orbitPtr);

		lastJDE = dateJDE;

	}
	else if (fabs(lastJDE-dateJDE)>deltaJDE)
	{
		// calculate actual Planet position
		coordFunc(dateJDE, eclipticPos, eclipticVelocity, orbitPtr);
		if (orbitFader.getInterstate()>0.000001)
			for( int d=0; d<ORBIT_SEGMENTS; d++ )
				orbit[d]=getHeliocentricPos(orbitP[d]);
		lastJDE = dateJDE;
	}

}

// Compute the transformation matrix from the local Planet coordinate system to the parent Planet coordinate system.
// In case of the planets, this makes the axis point to their respective celestial poles.
// TODO: Verify for the other planets if their axes are relative to J2000 ecliptic (VSOP87A XY plane) or relative to (precessed) ecliptic of date?
void Planet::computeTransMatrix(double JD, double JDE)
{
	// We have to call with both to correct this for earth with the new model.
	axisRotation = getSiderealTime(JD, JDE);

	// Special case - heliocentric coordinates are relative to eclipticJ2000 (VSOP87A XY plane),
	// not solar equator...

	if (parent)
	{
		// We can inject a proper precession plus even nutation matrix in this stage, if available.
		if (englishName=="Earth")
		{
			// rotLocalToParent = Mat4d::zrotation(re.ascendingNode - re.precessionRate*(jd-re.epoch)) * Mat4d::xrotation(-getRotObliquity(jd));
			// We follow Capitaine's (2003) formulation P=Rz(Chi_A)*Rx(-omega_A)*Rz(-psi_A)*Rx(eps_o).
			// ADS: 2011A&A...534A..22V = A&A 534, A22 (2011): Vondrak, Capitane, Wallace: New Precession Expressions, valid for long time intervals:
			// See also Hilton et al., Report on Precession and the Ecliptic. Cel.Mech.Dyn.Astr. 94:351-367 (2006), eqn (6) and (21).
			double eps_A, chi_A, omega_A, psi_A;
			getPrecessionAnglesVondrak(JDE, &eps_A, &chi_A, &omega_A, &psi_A);
			// Canonical precession rotations: Nodal rotation psi_A,
			// then rotation by omega_A, the angle between EclPoleJ2000 and EarthPoleOfDate.
			// The final rotation by chi_A rotates the equinox (zero degree).
			// To achieve ecliptical coords of date, you just have now to add a rotX by epsilon_A (obliquity of date).

			rotLocalToParent= Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(chi_A);
			// Plus nutation IAU-2000B:
			if (StelApp::getInstance().getCore()->getUseNutation())
			{
				double deltaEps, deltaPsi;
				getNutationAngles(JDE, &deltaPsi, &deltaEps);
				//qDebug() << "deltaEps, arcsec" << deltaEps*180./M_PI*3600. << "deltaPsi" << deltaPsi*180./M_PI*3600.;
				Mat4d nut2000B=Mat4d::xrotation(eps_A) * Mat4d::zrotation(deltaPsi)* Mat4d::xrotation(-eps_A-deltaEps);
				rotLocalToParent=rotLocalToParent*nut2000B;
			}
		}
		else
			rotLocalToParent = Mat4d::zrotation(re.ascendingNode - re.precessionRate*(JDE-re.epoch)) * Mat4d::xrotation(re.obliquity);
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


// Compute the z rotation to use from equatorial to geographic coordinates.
// We need both JD and JDE here for Earth. (For other planets only JDE.)
double Planet::getSiderealTime(double JD, double JDE) const
{
	if (englishName=="Earth")
	{	// Check to make sure that nutation is just those few arcseconds.
		if (StelApp::getInstance().getCore()->getUseNutation())
			return get_apparent_sidereal_time(JD, JDE);
		else
			return get_mean_sidereal_time(JD, JDE);
	}

	double t = JDE - re.epoch;
	// oops... avoid division by zero (typical case for moons with chaotic period of rotation)
	double rotations = 1.f; // NOTE: Maybe 1e-3 will be better?
	if (re.period!=0.) // OK, it's not a moon with chaotic period of rotation :)
	{
		rotations = t / (double) re.period;
	}
	double wholeRotations = floor(rotations);
	double remainder = rotations - wholeRotations;

	if (englishName=="Jupiter")
	{
		// http://www.projectpluto.com/grs_form.htm
		// CM( System II) =  181.62 + 870.1869147 * jd + correction [870d rotation every day]
		const double rad  = M_PI/180.;
		double jup_mean = (JDE - 2455636.938) * 360. / 4332.89709;
		double eqn_center = 5.55 * sin( rad*jup_mean);
		double angle = (JDE - 2451870.628) * 360. / 398.884 - eqn_center;
		//double correction = 11 * sin( rad*angle) + 5 * cos( rad*angle)- 1.25 * cos( rad*jup_mean) - eqn_center; // original correction
		double correction = 25.8 + 11 * sin( rad*angle) - 2.5 * cos( rad*jup_mean) - eqn_center; // light speed correction not used because in stellarium the jd is manipulated for that
		double cm2=181.62 + 870.1869147 * JDE + correction; // Central Meridian II
		cm2=cm2 - 360.0*(int)(cm2/360.);
		// http://www.skyandtelescope.com/observing/transit-times-of-jupiters-great-red-spot/ writes:
		// The predictions assume the Red Spot was at Jovian System II longitude 216° in September 2014 and continues to drift 1.25° per month, based on historical trends noted by JUPOS.
		// GRS longitude was at 2014-09-08 216d with a drift of 1.25d every month
		double longitudeGRS = 0.;
		if (flagCustomGrsSettings)
			longitudeGRS = customGrsLongitude + customGrsDrift*(JDE - customGrsJD)/365.25;
		else
			longitudeGRS=216+1.25*( JDE - 2456908)/30;
		// qDebug() << "Jupiter: CM2 = " << cm2 << " longitudeGRS = " << longitudeGRS << " --> rotation = " << (cm2 - longitudeGRS);
		return cm2 - longitudeGRS + 50.; // Jupiter Texture not 0d
		// To verify:
		// GRS at 2015-02-26 23:07 UT on picture at https://maximusphotography.files.wordpress.com/2015/03/jupiter-febr-26-2015.jpg
		//        2014-02-25 19:03 UT    http://www.damianpeach.com/jup1314/2014_02_25rgb0305.jpg
		//	  2013-05-01 10:29 UT    http://astro.christone.net/jupiter/jupiter2012/jupiter20130501.jpg
		//        2012-10-26 00:12 UT at http://www.lunar-captures.com//jupiter2012_files/121026_JupiterGRS_Tar.jpg
		//	  2011-08-28 02:29 UT at http://www.damianpeach.com/jup1112/2011_08_28rgb.jpg
		// stellarium 2h too early: 2010-09-21 23:37 UT http://www.digitalsky.org.uk/Jupiter/2010-09-21_23-37-30_R-G-B_800.jpg
	}
	else
		return remainder * 360. + re.offset;
}

double Planet::getMeanSolarDay() const
{
	double msd = 0.;

	if (englishName=="Sun")
	{
		// A mean solar day (equals to Earth's day) has been added here for educational purposes
		// Details: https://sourceforge.net/p/stellarium/discussion/278769/thread/fbe282db/
		return 1.;
	}

	double sday = getSiderealDay();
	double coeff = qAbs(sday/getSiderealPeriod());
	float sign = 1;
	// planets with retrograde rotation
	if (englishName=="Venus" || englishName=="Uranus" || englishName=="Pluto")
		sign = -1;

	if (pType==Planet::isMoon)
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
// Return heliocentric velocity of planet.
Vec3d Planet::getHeliocentricEclipticVelocity() const
{
	// Note: using shared copies is too slow here.  So we use direct access
	// instead.
	Vec3d vel = eclipticVelocity;
	const Planet* pp = parent.data();
	if (pp)
	{
		while (pp->parent.data())
		{
			vel += pp->eclipticVelocity;
			pp = pp->parent.data();
		}
	}
	return vel;
}

// Compute the distance to the given position in heliocentric coordinate (in AU)
// This is called by SolarSystem::draw()
double Planet::computeDistance(const Vec3d& obsHelioPos)
{
	distance = (obsHelioPos-getHeliocentricEclipticPos()).length();
	// improve fps by juggling updates for asteroids and other minor bodies. They must be fast if close to observer, but can be slow if further away.
	if (pType >= Planet::isAsteroid)
		deltaJDE=distance*StelCore::JD_SECOND;
	return distance;
}

// Get the phase angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getPhaseAngle(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	return std::acos((observerPlanetRq + planetRq - observerRq)/(2.0*std::sqrt(observerPlanetRq*planetRq)));
}

// Get the planet phase[0..1] for an observer at pos obsPos in heliocentric coordinates (in AU)
float Planet::getPhase(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*std::sqrt(observerPlanetRq*planetRq));
	return 0.5f * qAbs(1.f + cos_chi);
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

// Source: Explanatory Supplement 2013, Table 10.6 and formula (10.5) with semimajorAxis a from Table 8.7.
float Planet::getMeanOppositionMagnitude() const
{
	if (absoluteMagnitude==-99.)
		return 100.;
	if (englishName=="Sun")
		return 100;

	double semimajorAxis=0.;
	if (englishName=="Moon")
		return -12.74f;
	else 	if (englishName=="Mars")
		return -2.01f;
	else if (englishName=="Jupiter")
		return -2.7f;
	else if (englishName=="Saturn")
		return 0.67f;
	else if (englishName=="Uranus")
		return 5.52f;
	else if (englishName=="Neptune")
		return 7.84f;
	else if (englishName=="Pluto")
		return 15.12f;
	else if (englishName=="Io")
		return 5.02f;
	else if (englishName=="Europa")
		return 5.29f;
	else if (englishName=="Ganymede")
		return 4.61f;
	else if (englishName=="Callisto")
		return 5.65f;
	else if (parent->englishName=="Mars")
		semimajorAxis=1.52371034;
	else if (parent->englishName=="Jupiter")
		semimajorAxis=5.202887;
	else if (parent->englishName=="Saturn")
		semimajorAxis=9.53667594;
	else if (parent->englishName=="Uranus")
		semimajorAxis=19.18916464;
	else if (parent->englishName=="Neptune")
		semimajorAxis=30.06992276;
	else if (parent->englishName=="Pluto")
		semimajorAxis=39.48211675;
	else if (pType>= isAsteroid)
	{
		if (orbitPtr)
			semimajorAxis=((CometOrbit*)orbitPtr)->getSemimajorAxis();
	}

	if (semimajorAxis>0.)
		return absoluteMagnitude+5*log10(semimajorAxis*(semimajorAxis-1));

	return 100.;
}

// Computation of the visual magnitude (V band) of the planet.
float Planet::getVMagnitude(const StelCore* core) const
{
	if (parent == 0)
	{
		// Sun, compute the apparent magnitude for the absolute mag (V: 4.83) and observer's distance
		// Hint: Absolute Magnitude of the Sun in Several Bands: http://mips.as.arizona.edu/~cnaw/sun.html
		const double distParsec = std::sqrt(core->getObserverHeliocentricEclipticPos().lengthSquared())*AU/PARSEC;

		// check how much of it is visible
		const SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		double shadowFactor = ssm->getEclipseFactor(core);
		// See: Hughes, D. W., Brightness during a solar eclipse // Journal of the British Astronomical Association, vol.110, no.4, p.203-205
		// URL: http://adsabs.harvard.edu/abs/2000JBAA..110..203H
		if(shadowFactor < 0.000128)
			shadowFactor = 0.000128;

		return 4.83 + 5.*(std::log10(distParsec)-1.) - 2.5*(std::log10(shadowFactor));
	}

	// Compute the phase angle i. We need the intermediate results also below, therefore we don't just call getPhaseAngle.
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const double observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*std::sqrt(observerPlanetRq*planetRq));
	const double phaseAngle = std::acos(cos_chi);

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
			double d = sun_radius - sun_minus_parent_radius*quot - std::sqrt((1.-sun_minus_parent_radius/std::sqrt(parent_Rq)) * (planetRq-pos_times_parent_pos*quot));
			if (d>=radius)
			{
				// The satellite is totally inside the inner shadow.
				if (englishName=="Moon")
				{
					// Fit a more realistic magnitude for the Moon case.
					// I used some empirical data for fitting. --AW
					// TODO: This factor should be improved!
					shadowFactor = 2.718e-5;
				}
				else
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
		const double phaseDeg=phaseAngle*180./M_PI;
		const double d = 5. * log10(std::sqrt(observerPlanetRq*planetRq));

		// GZ: I prefer the values given by Meeus, Astronomical Algorithms (1992).
		// There are three solutions:
		// (0) "Planesas": original solution in Stellarium, present around 2010.
		// (1) G. Mueller, based on visual observations 1877-91. [Expl.Suppl.1961 p.312ff]
		// (2) Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes.
		// The structure is almost identical, just the numbers are different!
		// I activate (1) for now, because we want to simulate the eye's impression. (Esp. Venus!)
		// AW: (2) activated by default
		// GZ Note that calling (2) "Harris" is an absolute misnomer. Meeus clearly describes this in AstrAlg1998 p.286.
		// The values should likely be named:
		// Planesas --> Expl_Suppl_1992  AND THIS SHOULD BECOME DEFAULT
		// Mueller  --> Mueller_1893
		// Harris   --> Astr_Eph_1984

		switch (Planet::getApparentMagnitudeAlgorithm())
		{
			case UndefinedAlgorithm:	// The most recent solution should be activated by default			
			case ExplanatorySupplement_2013:
			{
				// GZ2017: This is taken straight from the Explanatory Supplement to the Astronomical Ephemeris 2013 (chap. 10.3)
				// AW2017: Updated data from Errata in The Explanatory Supplement to the Astronomical Almanac (3rd edition, 1st printing)
				//         http://aa.usno.navy.mil/publications/docs/exp_supp_errata.pdf (Last update: 1 December 2016)
				if (englishName=="Mercury")
					return -0.6 + d + (((3.02e-6*phaseDeg - 0.000488)*phaseDeg + 0.0498)*phaseDeg);
				if (englishName=="Venus")
				{ // there are two regions strongly enclosed per phaseDeg (2.7..163.6..170.2). However, we must deliver a solution for every case.
					if (phaseDeg<163.6)
						return -4.47 + d + ((0.13e-6*phaseDeg + 0.000057)*phaseDeg + 0.0103)*phaseDeg;
					else
						return 0.98 + d -0.0102*phaseDeg;
				}
				if (englishName=="Earth")
					return -3.87 + d + (((0.48e-6*phaseDeg + 0.000019)*phaseDeg + 0.0130)*phaseDeg);
				if (englishName=="Mars")
					return -1.52 + d + 0.016*phaseDeg;
				if (englishName=="Jupiter")
					return -9.40 + d + 0.005*phaseDeg;
				if (englishName=="Saturn")
				{
					// add rings computation
					// implemented from Meeus, Astr.Alg.1992
					const double jde=core->getJDE();
					const double T=(jde-2451545.0)/36525.0;
					const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
					const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
					static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
					const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
					double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinx=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					double rings = -2.6*fabs(sinx) + 1.25*sinx*sinx; // ExplSup2013: added term as (10.81)
					return -8.88 + d + 0.044*phaseDeg + rings;
				}
				if (englishName=="Uranus")
					return -7.19 + d + 0.002*phaseDeg;
				if (englishName=="Neptune")
					return -6.87 + d;
				if (englishName=="Pluto")
					return -1.01 + d;

				// AW2017: I've added special case for Jupiter's moons when they are in the shadow of Jupiter.
				//         FIXME: Need experimental data to fitting to real world or the scientific paper with description of model.
				// GZ 2017-09: Phase coefficients for I and III corrected, based on original publication (Stebbins&Jacobsen 1928) now.
				if (englishName=="Io")
					return shadowFactor<1.0 ? 21.0 : (-1.68 + d + phaseDeg*(0.046  - 0.0010 *phaseDeg));
				if (englishName=="Europa")
					return shadowFactor<1.0 ? 21.0 : (-1.41 + d + phaseDeg*(0.0312 - 0.00125*phaseDeg));
				if (englishName=="Ganymede")
					return shadowFactor<1.0 ? 21.0 : (-2.09 + d + phaseDeg*(0.0323 - 0.00066*phaseDeg));
				if (englishName=="Callisto")
					return shadowFactor<1.0 ? 21.0 : (-1.05 + d + phaseDeg*(0.078  - 0.00274*phaseDeg));

				if ((absoluteMagnitude!=-99.) && (englishName!="Moon"))
					return absoluteMagnitude+d;

				break;
			}

			case ExplanatorySupplement_1992:
			{
				// Algorithm provided by Pere Planesas (Observatorio Astronomico Nacional)
				// GZ2016: Actually, this is taken straight from the Explanatory Supplement to the Astronomical Ephemeris 1992! (chap. 7.12)
				// The value -8.88 for Saturn V(1,0) seems to be a correction of a typo, where Suppl.Astr. gives -7.19 just like for Uranus.
				double f1 = phaseDeg/100.;

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
					// add rings computation
					// implemented from Meeus, Astr.Alg.1992
					const double jde=core->getJDE();
					const double T=(jde-2451545.0)/36525.0;
					const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
					const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
					static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
					const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
					double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinx=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					double rings = -2.6*fabs(sinx) + 1.25*sinx*sinx;
					return -8.88 + d + 0.044*phaseDeg + rings;
				}
				if (englishName=="Uranus")
					return -7.19 + d + 0.0028*phaseDeg;
				if (englishName=="Neptune")
					return -6.87 + d;
				if (englishName=="Pluto")
					return -1.01 + d + 0.041*phaseDeg;

				break;
			}

			case Mueller_1893:
			{
				// (1)
				// Publicationen des Astrophysikalischen Observatoriums zu Potsdam, 8, 366, 1893.
				if (englishName=="Mercury")
				{
					double ph50=phaseDeg-50.0;
					return 1.16 + d + 0.02838*ph50 + 0.0001023*ph50*ph50;
				}
				if (englishName=="Venus")
					return -4.00 + d + 0.01322*phaseDeg + 0.0000004247*phaseDeg*phaseDeg*phaseDeg;
				if (englishName=="Mars")
					return -1.30 + d + 0.01486*phaseDeg;
				if (englishName=="Jupiter")
					return -8.93 + d;
				if (englishName=="Saturn")
				{
					// add rings computation
					// implemented from Meeus, Astr.Alg.1992
					const double jde=core->getJDE();
					const double T=(jde-2451545.0)/36525.0;
					const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
					const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
					SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
					const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
					double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					double rings = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
					return -8.68 + d + 0.044*phaseDeg + rings;
				}
				if (englishName=="Uranus")
					return -6.85 + d;
				if (englishName=="Neptune")
					return -7.05 + d;
				// Original formulae doesn't have equation for Pluto
				if (englishName=="Pluto")
					return -1.0 + d;

				break;
			}
			case AstronomicalAlmanac_1984:
			{
				// (2)
				if (englishName=="Mercury")
					return -0.42 + d + .038*phaseDeg - 0.000273*phaseDeg*phaseDeg + 0.000002*phaseDeg*phaseDeg*phaseDeg;
				if (englishName=="Venus")
					return -4.40 + d + 0.0009*phaseDeg + 0.000239*phaseDeg*phaseDeg - 0.00000065*phaseDeg*phaseDeg*phaseDeg;
				if (englishName=="Mars")
					return -1.52 + d + 0.016*phaseDeg;
				if (englishName=="Jupiter")
					return -9.40 + d + 0.005*phaseDeg;
				if (englishName=="Saturn")
				{
					// add rings computation
					// implemented from Meeus, Astr.Alg.1992
					const double jde=core->getJDE();
					const double T=(jde-2451545.0)/36525.0;
					const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
					const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
					static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
					const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
					double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
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

				break;
			}
			case Generic:
			{
				// drop down to calculation of visual magnitude from phase angle and albedo of the planet
				break;
			}
		}
	}

	// This formula source is unknown. But this is actually used even for the Moon!
	const double p = (1.0 - phaseAngle/M_PI) * cos_chi + std::sqrt(1.0 - cos_chi*cos_chi) / M_PI;
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

//the Planet and all the related infos : name, circle etc..
void Planet::draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont)
{
	if (hidden)
		return;

	// Exclude drawing if user set a hard limit magnitude.
	if (core->getSkyDrawer()->getFlagPlanetMagnitudeLimit() && (getVMagnitude(core) > core->getSkyDrawer()->getCustomPlanetMagnitudeLimit()))
	{
		// Get the eclipse factor to avoid hiding the Moon during a total solar eclipse.
		// Details: https://answers.launchpad.net/stellarium/+question/395139
		if (GETSTELMODULE(SolarSystem)->getEclipseFactor(core)==1.0)
			return;
	}

	// Try to improve speed for minor planets: test if visible at all.
	// For a full catalog of NEAs (11000 objects), with this and resetting deltaJD according to distance, rendering time went 4.5fps->12fps.
	// TBD: Note that taking away the asteroids at this stage breaks dim-asteroid occultation of stars!
	//      Maybe make another configurable flag for those interested?
	// Problematic: Early-out here of course disables the wanted hint circles for dim asteroids.
	// The line makes hints for asteroids 5 magnitudes below sky limiting magnitude visible.
	// If asteroid is too faint to be seen, don't bother rendering. (Massive speedup if people have hundreds of orbital elements!)
	// AW: Added a special case for educational purpose to drawing orbits for the Solar System Observer
	// Details: https://sourceforge.net/p/stellarium/discussion/278769/thread/4828ebe4/
	if (((getVMagnitude(core)-5.0f) > core->getSkyDrawer()->getLimitMagnitude()) && pType>=Planet::isAsteroid && !core->getCurrentLocation().planetName.contains("Observer", Qt::CaseInsensitive))
	{
		return;
	}

	Mat4d mat;
	if (englishName=="Sun")
	{
		mat = Mat4d::translation(GETSTELMODULE(SolarSystem)->getLightTimeSunPosition()) * rotLocalToParent;
	}
	else
	{
		mat = Mat4d::translation(eclipticPos) * rotLocalToParent;
	}

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
	float viewportBufferSz=screenSz;
	// enlarge if this is sun with its huge halo.
	if (englishName=="Sun")
		viewportBufferSz+=125.f;
	float viewport_left = prj->getViewportPosX();
	float viewport_bottom = prj->getViewportPosY();

	if ((prj->project(Vec3d(0.), screenPos)
	     && screenPos[1]>viewport_bottom - viewportBufferSz && screenPos[1] < viewport_bottom + prj->getViewportHeight()+viewportBufferSz
	     && screenPos[0]>viewport_left - viewportBufferSz && screenPos[0] < viewport_left + prj->getViewportWidth() + viewportBufferSz))
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
	else if (permanentDrawingOrbits) // A special case for demos
		drawOrbit(core);

	return;
}

class StelPainterLight
{
public:
	Vec3d position;
	Vec3f diffuse;
	Vec3f ambient;
};
static StelPainterLight light;

void Planet::PlanetShaderVars::initLocations(QOpenGLShaderProgram* p)
{
	GL(p->bind());
	//attributes
	GL(texCoord = p->attributeLocation("texCoord"));
	GL(unprojectedVertex = p->attributeLocation("unprojectedVertex"));
	GL(vertex = p->attributeLocation("vertex"));
	GL(normalIn = p->attributeLocation("normalIn"));

	//common uniforms
	GL(projectionMatrix = p->uniformLocation("projectionMatrix"));
	GL(tex = p->uniformLocation("tex"));
	GL(lightDirection = p->uniformLocation("lightDirection"));
	GL(eyeDirection = p->uniformLocation("eyeDirection"));
	GL(diffuseLight = p->uniformLocation("diffuseLight"));
	GL(ambientLight = p->uniformLocation("ambientLight"));
	GL(shadowCount = p->uniformLocation("shadowCount"));
	GL(shadowData = p->uniformLocation("shadowData"));
	GL(sunInfo = p->uniformLocation("sunInfo"));
	GL(skyBrightness = p->uniformLocation("skyBrightness"));
	GL(orenNayarParameters = p->uniformLocation("orenNayarParameters"));
	GL(outgasParameters = p->uniformLocation("outgasParameters"));

	// Moon-specific variables
	GL(earthShadow = p->uniformLocation("earthShadow"));
	GL(normalMap = p->uniformLocation("normalMap"));

	// Rings-specific variables
	GL(isRing = p->uniformLocation("isRing"));
	GL(ring = p->uniformLocation("ring"));
	GL(outerRadius = p->uniformLocation("outerRadius"));
	GL(innerRadius = p->uniformLocation("innerRadius"));
	GL(ringS = p->uniformLocation("ringS"));

	// Shadowmap variables
	GL(shadowMatrix = p->uniformLocation("shadowMatrix"));
	GL(shadowTex = p->uniformLocation("shadowTex"));
	GL(poissonDisk = p->uniformLocation("poissonDisk"));

	GL(p->release());
}

QOpenGLShaderProgram* Planet::createShader(const QString& name, PlanetShaderVars& vars, const QByteArray& vSrc, const QByteArray& fSrc, const QByteArray& prefix, const QMap<QByteArray, int> &fixedAttributeLocations)
{
	QOpenGLShaderProgram* program = new QOpenGLShaderProgram();
	if(!program->create())
	{
		qCritical()<<"Planet: Cannot create shader program object for"<<name;
		delete program;
		return Q_NULLPTR;
	}

	// We HAVE to create QOpenGLShader on the heap (with automatic QObject memory management), or the shader may be destroyed before the program has been linked
	// This was FUN to debug - OGL simply accepts an empty shader, no errors generated but funny colors drawn :)
	// We could also use QOpenGLShaderProgram::addShaderFromSourceCode directly, but this would prevent having access to warnings (because log is copied only on errors there...)
	if(!vSrc.isEmpty())
	{
		QOpenGLShader* shd = new QOpenGLShader(QOpenGLShader::Vertex, program);
		bool ok = shd->compileSourceCode(prefix + vSrc);
		QString log = shd->log();
		if (!log.isEmpty() && !log.contains("no warnings", Qt::CaseInsensitive)) { qWarning() << "Planet: Warnings/Errors while compiling" << name << "vertex shader: " << log; }
		if(!ok)
		{
			qCritical()<<name<<"vertex shader could not be compiled";
			delete program;
			return Q_NULLPTR;
		}
		if(!program->addShader(shd))
		{
			qCritical()<<name<<"vertex shader could not be added to program";
			delete program;
			return Q_NULLPTR;
		}
	}

	if(!fSrc.isEmpty())
	{
		QOpenGLShader* shd = new QOpenGLShader(QOpenGLShader::Fragment, program);
		bool ok = shd->compileSourceCode(prefix + fSrc);
		QString log = shd->log();
		if (!log.isEmpty() && !log.contains("no warnings", Qt::CaseInsensitive)) { qWarning() << "Planet: Warnings/Errors while compiling" << name << "fragment shader: " << log; }
		if(!ok)
		{
			qCritical()<<name<<"fragment shader could not be compiled";
			delete program;
			return Q_NULLPTR;
		}
		if(!program->addShader(shd))
		{
			qCritical()<<name<<"fragment shader could not be added to program";
			delete program;
			return Q_NULLPTR;
		}
	}

	//process fixed attribute locations
	QMap<QByteArray,int>::const_iterator it = fixedAttributeLocations.begin();
	while(it!=fixedAttributeLocations.end())
	{
		program->bindAttributeLocation(it.key(),it.value());
		++it;
	}

	if(!StelPainter::linkProg(program,name))
	{
		delete program;
		return Q_NULLPTR;
	}

	vars.initLocations(program);

	return program;
}

void Planet::initShader()
{
	qDebug() << "Initializing planets GL shaders... ";
	shaderError = true;

	QSettings* settings = StelApp::getInstance().getSettings();
	settings->sync();
	shadowPolyOffset = StelUtils::strToVec2f(settings->value("astro/planet_shadow_polygonoffset", StelUtils::vec2fToStr(Vec2f(0.0f, 0.0f))).toString());
	//qDebug()<<"Shadow poly offset"<<shadowPolyOffset;

	// Shader text is loaded from file
	QString vFileName = StelFileMgr::findFile("data/shaders/planet.vert",StelFileMgr::File);
	QString fFileName = StelFileMgr::findFile("data/shaders/planet.frag",StelFileMgr::File);

	if(vFileName.isEmpty())
	{
		qCritical()<<"Cannot find 'data/shaders/planet.vert', can't use planet rendering!";
		return;
	}
	if(fFileName.isEmpty())
	{
		qCritical()<<"Cannot find 'data/shaders/planet.frag', can't use planet rendering!";
		return;
	}

	QFile vFile(vFileName);
	QFile fFile(fFileName);

	if(!vFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical()<<"Cannot load planet vertex shader file"<<vFileName<<vFile.errorString();
		return;
	}
	QByteArray vsrc = vFile.readAll();
	vFile.close();

	if(!fFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical()<<"Cannot load planet fragment shader file"<<fFileName<<fFile.errorString();
		return;
	}
	QByteArray fsrc = fFile.readAll();
	fFile.close();

	shaderError = false;

	// Default planet shader program
	planetShaderProgram = createShader("planetShaderProgram",planetShaderVars,vsrc,fsrc);
	// Planet with ring shader program
	ringPlanetShaderProgram = createShader("ringPlanetShaderProgram",ringPlanetShaderVars,vsrc,fsrc,"#define RINGS_SUPPORT\n\n");
	// Moon shader program
	moonShaderProgram = createShader("moonShaderProgram",moonShaderVars,vsrc,fsrc,"#define IS_MOON\n\n");
	// OBJ model shader program
	// we REQUIRE some fixed attribute locations here
	QMap<QByteArray,int> attrLoc;
	attrLoc.insert("unprojectedVertex", StelOpenGLArray::ATTLOC_VERTEX);
	attrLoc.insert("texCoord", StelOpenGLArray::ATTLOC_TEXCOORD);
	attrLoc.insert("normalIn", StelOpenGLArray::ATTLOC_NORMAL);
	objShaderProgram = createShader("objShaderProgram",objShaderVars,vsrc,fsrc,"#define IS_OBJ\n\n",attrLoc);
	//OBJ shader with shadowmap support
	objShadowShaderProgram = createShader("objShadowShaderProgram",objShadowShaderVars,vsrc,fsrc,
					      "#define IS_OBJ\n"
					      "#define SHADOWMAP\n"
					      "#define SM_SIZE " STRINGIFY(SM_SIZE) "\n"
										    "\n",attrLoc);

	//set the poisson disk as uniform, this seems to be the only way to get an (const) array into GLSL 110 on all drivers
	if(objShadowShaderProgram)
	{
		objShadowShaderProgram->bind();
		const float poissonDisk[] ={
			-0.610470f, -0.702763f,
			 0.609267f,  0.765488f,
			-0.817537f, -0.412950f,
			 0.777710f, -0.446717f,
			-0.668764f, -0.524195f,
			 0.425181f,  0.797780f,
			-0.766728f, -0.065185f,
			 0.266692f,  0.917346f,
			-0.578028f, -0.268598f,
			 0.963767f,  0.079058f,
			-0.968971f, -0.039291f,
			 0.174263f, -0.141862f,
			-0.348933f, -0.505110f,
			 0.837686f, -0.083142f,
			-0.462722f, -0.072878f,
			 0.701887f, -0.281632f,
			-0.377209f, -0.247278f,
			 0.765589f,  0.642157f,
			-0.678950f,  0.128138f,
			 0.418512f, -0.186050f,
			-0.442419f,  0.242444f,
			 0.442748f, -0.456745f,
			-0.196461f,  0.084314f,
			 0.536558f, -0.770240f,
			-0.190154f, -0.268138f,
			 0.643032f, -0.584872f,
			-0.160193f, -0.457076f,
			 0.089220f,  0.855679f,
			-0.200650f, -0.639838f,
			 0.220825f,  0.710969f,
			-0.330313f, -0.812004f,
			-0.046886f,  0.721859f,
			 0.070102f, -0.703208f,
			-0.161384f,  0.952897f,
			 0.034711f, -0.432054f,
			-0.508314f,  0.638471f,
			-0.026992f, -0.163261f,
			 0.702982f,  0.089288f,
			-0.004114f, -0.901428f,
			 0.656819f,  0.387131f,
			-0.844164f,  0.526829f,
			 0.843124f,  0.220030f,
			-0.802066f,  0.294509f,
			 0.863563f,  0.399832f,
			 0.268762f, -0.576295f,
			 0.465623f,  0.517930f,
			 0.340116f, -0.747385f,
			 0.223493f,  0.516709f,
			 0.240980f, -0.942373f,
			-0.689804f,  0.649927f,
			 0.272309f, -0.297217f,
			 0.378957f,  0.162593f,
			 0.061461f,  0.067313f,
			 0.536957f,  0.249192f,
			-0.252331f,  0.265096f,
			 0.587532f, -0.055223f,
			 0.034467f,  0.289122f,
			 0.215271f,  0.278700f,
			-0.278059f,  0.615201f,
			-0.369530f,  0.791952f,
			-0.026918f,  0.542170f,
			 0.274033f,  0.010652f,
			-0.561495f,  0.396310f,
			-0.367752f,  0.454260f
		};
		objShadowShaderProgram->setUniformValueArray(objShadowShaderVars.poissonDisk,poissonDisk,64,2);
		objShadowShaderProgram->release();
	}

	//this is a simple transform-only shader (used for filling the depth map for OBJ shadows)
	QByteArray transformVShader(
				"uniform mat4 projectionMatrix;\n"
				"attribute vec4 unprojectedVertex;\n"
			#ifdef DEBUG_SHADOWMAP
				"attribute mediump vec2 texCoord;\n"
				"varying mediump vec2 texc; //texture coord\n"
				"varying highp vec4 pos; //projected pos\n"
			#endif
				"void main()\n"
				"{\n"
			#ifdef DEBUG_SHADOWMAP
				"   texc = texCoord;\n"
				"   pos = projectionMatrix * unprojectedVertex;\n"
			#endif
				"   gl_Position = projectionMatrix * unprojectedVertex;\n"
				"}\n"
				);

#ifdef DEBUG_SHADOWMAP
	const QByteArray transformFShader(
				"uniform lowp sampler2D tex;\n"
				"varying mediump vec2 texc; //texture coord\n"
				"varying highp vec4 pos; //projected pos\n"
				"void main()\n"
				"{\n"
				"   lowp vec4 texCol = texture2D(tex,texc);\n"
				"   highp float zNorm = (pos.z + 1.0) / 2.0;\n" //from [-1,1] to [0,1]
				"   gl_FragColor = vec4(texCol.rgb,zNorm);\n"
				"}\n"
				);
#else
	//On ES2, we have to create an empty dummy FShader or the compiler may complain
	//but don't do this on desktop, at least my Intel fails linking with this (with no log message, yay...)
	QByteArray transformFShader;
	if(QOpenGLContext::currentContext()->isOpenGLES())
	{
		transformFShader = "void main()\n{ }\n";
	}
#endif
	GL(transformShaderProgram = createShader("transformShaderProgam", transformShaderVars, transformVShader, transformFShader,QByteArray(),attrLoc));

	//check if ALL shaders have been created correctly
	shaderError = !(planetShaderProgram&&
			ringPlanetShaderProgram&&
			moonShaderProgram&&
			objShaderProgram&&
			objShadowShaderProgram&&
			transformShaderProgram);
}

void Planet::deinitShader()
{
	//note that it is not necessary to check for Q_NULLPTR before delete
	delete planetShaderProgram;
	planetShaderProgram = Q_NULLPTR;
	delete ringPlanetShaderProgram;
	ringPlanetShaderProgram = Q_NULLPTR;
	delete moonShaderProgram;
	moonShaderProgram = Q_NULLPTR;
	delete objShaderProgram;
	objShaderProgram = Q_NULLPTR;
	delete objShadowShaderProgram;
	objShadowShaderProgram = Q_NULLPTR;
	delete transformShaderProgram;
	transformShaderProgram = Q_NULLPTR;
}

bool Planet::initFBO()
{
	if(shadowInitialized)
		return false;

	QOpenGLContext* ctx  = QOpenGLContext::currentContext();
	QOpenGLFunctions* gl = ctx->functions();

	bool isGLESv2 = false;
	bool error = false;
	//check if support for the required features is available
	if(!ctx->functions()->hasOpenGLFeature(QOpenGLFunctions::Framebuffers))
	{
		qWarning()<<"Your GL driver does not support framebuffer objects, OBJ model self-shadows will not be available";
		error = true;
	}
	else if(ctx->isOpenGLES() &&
			ctx->format().majorVersion()<3)
	{
		isGLESv2 = true;
		//GLES v2 requires extensions for depth textures
		if(!(ctx->hasExtension("GL_OES_depth_texture") || ctx->hasExtension("GL_ANGLE_depth_texture")))
		{
			qWarning()<<"Your GL driver has no support for depth textures, OBJ model self-shadows will not be available";
			error = true;
		}
	}
	//on desktop, depth textures should be available on all GL >= 1.4 contexts, so no check should be required

	if(!error)
	{
		//all seems ok, create our objects
		GL(gl->glGenTextures(1, &shadowTex));
		GL(gl->glActiveTexture(GL_TEXTURE1));
		GL(gl->glBindTexture(GL_TEXTURE_2D, shadowTex));

#ifndef QT_OPENGL_ES_2
		if(!isGLESv2)
		{
			GL(gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			GL(gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
			GL(gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
			GL(gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
			const float ones[] = {1.0f, 1.0f, 1.0f, 1.0f};
			GL(gl->glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, ones));
		}
#endif
		GL(gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL(gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

#ifndef DEBUG_SHADOWMAP
		//create the texture
		//note that the 'type' must be GL_UNSIGNED_SHORT or GL_UNSIGNED_INT for ES2 compatibility, even if the 'pixels' are Q_NULLPTR
		GL(gl->glTexImage2D(GL_TEXTURE_2D, 0, isGLESv2?GL_DEPTH_COMPONENT:GL_DEPTH_COMPONENT16, SM_SIZE, SM_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, Q_NULLPTR));

		//we dont use QOpenGLFramebuffer because we dont want a color buffer...
		GL(gl->glGenFramebuffers(1, &shadowFBO));
		GL(gl->glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO));

		//attach shadow tex to FBO
		gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

		//on desktop, we must disable the read/draw buffer because we have no color buffer
		//else, it would be an FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER error
		//see GL_EXT_framebuffer_object and GL_ARB_framebuffer_object
		//on ES 2, this seems to be allowed (there are no glDrawBuffers/glReadBuffer functions there), see GLES spec section 4.4.4
		//probably same on ES 3: though it has glDrawBuffers/glReadBuffer but no mention of it in section 4.4.4 and no FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is defined
#ifndef QT_OPENGL_ES_2
		if(!ctx->isOpenGLES())
		{
			QOpenGLFunctions_1_0* gl10 = ctx->versionFunctions<QOpenGLFunctions_1_0>();
			if(Q_LIKELY(gl10))
			{
				//use DrawBuffer instead of DrawBuffers
				//because it is available since GL 1.0 instead of only on 3+
				gl10->glDrawBuffer(GL_NONE);
				gl10->glReadBuffer(GL_NONE);
			}
			else
			{
				//something is probably not how we want it
				Q_ASSERT(0);
			}
		}
#endif

		//check for completeness
		GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			error = true;
			qWarning()<<"Planet self-shadow framebuffer is incomplete, cannot use. Status:"<<status;
		}

		GL(gl->glBindFramebuffer(GL_FRAMEBUFFER, StelApp::getInstance().getDefaultFBO()));
		gl->glActiveTexture(GL_TEXTURE0);
#else
		GL(shadowFBO = new QOpenGLFramebufferObject(SM_SIZE,SM_SIZE,QOpenGLFramebufferObject::Depth));
		error = !shadowFBO->isValid();
#endif

		qDebug()<<"Planet self-shadow framebuffer initialized";
	}

	shadowInitialized = true;
	return !error;
}

void Planet::deinitFBO()
{
	if(!shadowInitialized)
		return;

	QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();

#ifndef DEBUG_SHADOWMAP
	//zeroed names are ignored by GL
	gl->glDeleteFramebuffers(1,&shadowFBO);
	shadowFBO = 0;
#else
	delete shadowFBO;
	shadowFBO = Q_NULLPTR;
#endif
	gl->glDeleteTextures(1,&shadowTex);
	shadowTex = 0;

	shadowInitialized = false;
}

void Planet::draw3dModel(StelCore* core, StelProjector::ModelViewTranformP transfo, float screenSz, bool drawOnlyRing)
{
	// This is the main method drawing a planet 3d model
	// Some work has to be done on this method to make the rendering nicer
	SolarSystem* ssm = GETSTELMODULE(SolarSystem);

	// Find extinction settings to change colors. The method is rather ad-hoc.
	float extinctedMag=getVMagnitudeWithExtinction(core)-getVMagnitude(core); // this is net value of extinction, in mag.
	float magFactorGreen=pow(0.85f, 0.6f*extinctedMag);
	float magFactorBlue=pow(0.6f, 0.5f*extinctedMag);

	if (screenSz>1.)
	{
		//make better use of depth buffer by adjusting clipping planes
		//must be done before creating StelPainter
		//depth buffer is used for object with rings or with OBJ models
		//it is not used for normal spherical object rendering without rings!
		//but it does not hurt to adjust it in all cases
		double n,f;
		core->getClippingPlanes(&n,&f); // Save clipping planes

		//determine the minimum size of the clip space
		double r = radius*sphereScale;
		if(rings)
			r+=rings->getSize();

		const double dist = getEquinoxEquatorialPos(core).length();
		double z_near = (dist - r); //near Z should be as close as possible to the actual geometry
		double z_far  = (dist + 10*r); //far Z should be quite a bit further behind (Z buffer accuracy is worse near the far plane)
		if (z_near < 0.0) z_near = 0.0;
		core->setClippingPlanes(z_near,z_far);

		StelProjector::ModelViewTranformP transfo2 = transfo->clone();
		transfo2->combine(Mat4d::zrotation(M_PI/180*(axisRotation + 90.)));
		StelPainter* sPainter = new StelPainter(core->getProjection(transfo2));
		gl = sPainter->glFuncs();
		
		// Set the main source of light to be the sun
		Vec3d sunPos(0.);
		core->getHeliocentricEclipticModelViewTransform()->forward(sunPos);
		light.position=Vec4d(sunPos);

		// Set the light parameters taking sun as the light source
		light.diffuse = Vec4f(1.f,magFactorGreen*1.f,magFactorBlue*1.f);
		light.ambient = Vec4f(0.02f,magFactorGreen*0.02f,magFactorBlue*0.02f);

		if (this==ssm->getMoon())
		{
			// ambient for the moon can provide the Ashen light!
			// during daylight, this still should not make moon visible. We grab sky brightness and dim the moon.
			// This approach here is again pretty ad-hoc.
			// We have 5000cd/m^2 at sunset returned (Note this may be unnaturally much. Should be rather 10, but the 5000 may include the sun).
			// When atm.brightness has fallen to 2000cd/m^2, we allow ashen light to appear visible. Its impact is full when atm.brightness is below 1000.
			LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
			Q_ASSERT(lmgr);
			float atmLum=(lmgr->getFlagAtmosphere() ? lmgr->getAtmosphereAverageLuminance() : 0.0f);
			if (atmLum<2000.0f)
			{
				float atmScaling=1.0f- (qMax(1000.0f, atmLum)-1000.0f)*0.001f; // full impact when atmLum<1000.
				float ashenFactor=(1.0f-getPhase(ssm->getEarth()->getHeliocentricEclipticPos())); // We really mean the Earth for this! (Try observing from Mars ;-)
				ashenFactor*=ashenFactor*0.15f*atmScaling;
				light.ambient = Vec4f(ashenFactor,magFactorGreen*ashenFactor,magFactorBlue*ashenFactor);
			}
			float fov=core->getProjection(transfo)->getFov();
			float fovFactor=1.6f;
			// scale brightness to reduce if fov smaller than 5 degrees. Min brightness (to avoid glare) if fov=2deg.
			if (fov<5.0f)
			{
				fovFactor -= 0.1f*(5.0f-qMax(2.0f, fov));
			}
			// Special case for the Moon. Was 1.6, but this often is too bright.
			light.diffuse = Vec4f(fovFactor,magFactorGreen*fovFactor,magFactorBlue*fovFactor,1.f);
		}

		// possibly tint sun's color from extinction. This should deliberately cause stronger reddening than for the other objects.
		if (this==ssm->getSun())
		{
			// when we zoom in, reduce the overbrightness. (LP:#1421173)
			const float fov=core->getProjection(transfo)->getFov();
			const float overbright=qBound(0.85f, 0.5f*fov, 2.0f); // scale full brightness to 0.85...2. (<2 when fov gets under 4 degrees)
			sPainter->setColor(overbright, pow(0.75f, extinctedMag)*overbright, pow(0.42f, 0.9f*extinctedMag)*overbright);
		}

		//if (rings) /// GZ This was the previous condition. Not sure why rings were dropped?
		if(ssm->getFlagUseObjModels() && !objModelPath.isEmpty())
		{
			if(!drawObjModel(sPainter, screenSz))
			{
				drawSphere(sPainter, screenSz, drawOnlyRing);
			}
		}
		else
			drawSphere(sPainter, screenSz, drawOnlyRing);

		core->setClippingPlanes(n,f);  // Restore old clipping planes

		delete sPainter;
		sPainter=Q_NULLPTR;
	}

	bool allowDrawHalo = true;
	if ((this!=ssm->getSun()) && ((this !=ssm->getMoon() && core->getCurrentLocation().planetName=="Earth" )))
	{
		// Let's hide halo when inner planet between Sun and observer (or moon between planet and observer).
		// Do not hide Earth's moon's halo below ~-45degrees when observing from earth.
		Vec3d obj = getJ2000EquatorialPos(core);
		Vec3d par = getParent()->getJ2000EquatorialPos(core);
		double angle = obj.angle(par)*180.f/M_PI;
		double asize = getParent()->getSpheroidAngularSize(core);
		if (angle<=asize)
			allowDrawHalo = false;
	}

	// Draw the halo if enabled in the ssystem_*.ini files (+ special case for backward compatible for the Sun)
	if ((hasHalo() || this==ssm->getSun()) && allowDrawHalo)
	{
		// Prepare openGL lighting parameters according to luminance
		float surfArcMin2 = getSpheroidAngularSize(core)*60;
		surfArcMin2 = surfArcMin2*surfArcMin2*M_PI; // the total illuminated area in arcmin^2

		StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
		Vec3d tmp = getJ2000EquatorialPos(core);

		// Find new extincted color for halo. The method is again rather ad-hoc, but does not look too bad.
		// For the sun, we have again to use the stronger extinction to avoid color mismatch.
		Vec3f haloColorToDraw;
		if (this==ssm->getSun())
			haloColorToDraw.set(haloColor[0], pow(0.75f, extinctedMag) * haloColor[1], pow(0.42f, 0.9f*extinctedMag) * haloColor[2]);
		else
			haloColorToDraw.set(haloColor[0], magFactorGreen * haloColor[1], magFactorBlue * haloColor[2]);

		core->getSkyDrawer()->postDrawSky3dModel(&sPainter, Vec3f(tmp[0], tmp[1], tmp[2]), surfArcMin2, getVMagnitudeWithExtinction(core), haloColorToDraw);

		if ((englishName=="Sun") && (core->getCurrentLocation().planetName == "Earth"))
		{
			float eclipseFactor = ssm->getEclipseFactor(core);
			// This alpha ensures 0 for complete sun, 1 for eclipse better 1e-10, with a strong increase towards full eclipse. We still need to square it.
			float alpha=-0.1f*qMax(-10.0f, (float) std::log10(eclipseFactor));
			core->getSkyDrawer()->drawSunCorona(&sPainter, Vec3f(tmp[0], tmp[1], tmp[2]), 512.f/192.f*screenSz, haloColorToDraw, alpha*alpha);
		}
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

Planet::RenderData Planet::setCommonShaderUniforms(const StelPainter& painter, QOpenGLShaderProgram* shader, const PlanetShaderVars& shaderVars) const
{
	RenderData data;

	const PlanetP sun = GETSTELMODULE(SolarSystem)->getSun();
	const StelProjectorP& projector = painter.getProjector();

	const Mat4f& m = projector->getProjectionMatrix();
	const QMatrix4x4 qMat = m.convertToQMatrix();

	computeModelMatrix(data.modelMatrix);
	// used to project from solar system into local space
	data.mTarget = data.modelMatrix.inverse();

	data.shadowCandidates = getCandidatesForShadow();
	// Our shader doesn't support more than 4 planets creating shadow
	if (data.shadowCandidates.size()>4)
	{
		qDebug() << "Too many satellite shadows, some won't be displayed";
		data.shadowCandidates.resize(4);
	}
	Mat4d shadowModelMatrix;
	for (int i=0;i<data.shadowCandidates.size();++i)
	{
		data.shadowCandidates.at(i)->computeModelMatrix(shadowModelMatrix);
		const Vec4d position = data.mTarget * shadowModelMatrix.getColumn(3);
		data.shadowCandidatesData(0, i) = position[0];
		data.shadowCandidatesData(1, i) = position[1];
		data.shadowCandidatesData(2, i) = position[2];
		data.shadowCandidatesData(3, i) = data.shadowCandidates.at(i)->getRadius();
	}

	Vec3f lightPos3(light.position[0], light.position[1], light.position[2]);
	projector->getModelViewTransform()->backward(lightPos3);
	lightPos3.normalize();

	data.eyePos = StelApp::getInstance().getCore()->getObserverHeliocentricEclipticPos();
	//qDebug() << eyePos[0] << " " << eyePos[1] << " " << eyePos[2] << " --> ";
	// Use refractionOff for avoiding flickering Moon. (Bug #1411958)
	StelApp::getInstance().getCore()->getHeliocentricEclipticModelViewTransform(StelCore::RefractionOff)->forward(data.eyePos);
	//qDebug() << "-->" << eyePos[0] << " " << eyePos[1] << " " << eyePos[2];
	projector->getModelViewTransform()->backward(data.eyePos);
	data.eyePos.normalize();
	//qDebug() << " -->" << eyePos[0] << " " << eyePos[1] << " " << eyePos[2];
	LandscapeMgr* lmgr=GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);

	GL(shader->setUniformValue(shaderVars.projectionMatrix, qMat));
	GL(shader->setUniformValue(shaderVars.lightDirection, lightPos3[0], lightPos3[1], lightPos3[2]));
	GL(shader->setUniformValue(shaderVars.eyeDirection, data.eyePos[0], data.eyePos[1], data.eyePos[2]));
	GL(shader->setUniformValue(shaderVars.diffuseLight, light.diffuse[0], light.diffuse[1], light.diffuse[2]));
	GL(shader->setUniformValue(shaderVars.ambientLight, light.ambient[0], light.ambient[1], light.ambient[2]));
	GL(shader->setUniformValue(shaderVars.tex, 0));
	GL(shader->setUniformValue(shaderVars.shadowCount, data.shadowCandidates.size()));
	GL(shader->setUniformValue(shaderVars.shadowData, data.shadowCandidatesData));
	GL(shader->setUniformValue(shaderVars.sunInfo, data.mTarget[12], data.mTarget[13], data.mTarget[14], sun->getRadius()));
	GL(shader->setUniformValue(shaderVars.skyBrightness, lmgr->getLuminance()));

	if(shaderVars.orenNayarParameters>=0)
	{
		//calculate and set oren-nayar parameters
		float roughnessSq = roughness * roughness;
		QVector3D vec(
					1.0f - 0.5f * roughnessSq / (roughnessSq + 0.57f), //x = A
					0.45f * roughnessSq / (roughnessSq + 0.09f),	//y = B
					1.85f	//z = scale factor
					);

		GL(shader->setUniformValue(shaderVars.orenNayarParameters, vec));
	}

	float outgas_intensity_distanceScaled=outgas_intensity/getHeliocentricEclipticPos().lengthSquared(); // ad-hoc function: assume square falloff by distance.
	GL(shader->setUniformValue(shaderVars.outgasParameters, QVector2D(outgas_intensity_distanceScaled, outgas_falloff)));

	return data;
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

	painter->setBlending(false);
	painter->setCullFace(true);

	// Draw the spheroid itself
	// Adapt the number of facets according with the size of the sphere for optimization
	int nb_facet = qBound(10, (int)(screenSz * 40.f/50.f), 100);	// 40 facets for 1024 pixels diameter on screen

	// Generates the vertice
	Planet3DModel model;
	sSphere(&model, radius*sphereScale, oneMinusOblateness, nb_facet, nb_facet);
	
	QVector<float> projectedVertexArr(model.vertexArr.size());
	for (int i=0;i<model.vertexArr.size()/3;++i)
		painter->getProjector()->project(*((Vec3f*)(model.vertexArr.constData()+i*3)), *((Vec3f*)(projectedVertexArr.data()+i*3)));
	
	const SolarSystem* ssm = GETSTELMODULE(SolarSystem);

	if (this==ssm->getSun())
	{
		texMap->bind();
		//painter->setColor(2, 2, 0.2); // This is now in draw3dModel() to apply extinction
		painter->setArrays((Vec3f*)projectedVertexArr.constData(), (Vec2f*)model.texCoordArr.constData());
		painter->drawFromArray(StelPainter::Triangles, model.indiceArr.size(), 0, false, model.indiceArr.constData());
		return;
	}

	//cancel out if shaders are invalid
	if(shaderError)
		return;
	
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
	//check if shaders are loaded
	if(!shader)
	{
		Planet::initShader();

		if(shaderError)
		{
			qCritical()<<"Can't use planet drawing, shaders invalid!";
			return;
		}

		shader = planetShaderProgram;
		if (rings)
		{
			shader = ringPlanetShaderProgram;
		}
		if (this==ssm->getMoon())
		{
			shader = moonShaderProgram;
		}
	}

	GL(shader->bind());

	RenderData rData = setCommonShaderUniforms(*painter,shader,*shaderVars);
	
	if (rings!=Q_NULLPTR)
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
		if (!rData.shadowCandidates.isEmpty())
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
		painter->setDepthMask(true);
		painter->setDepthTest(true);
		gl->glClear(GL_DEPTH_BUFFER_BIT);
	}
	
	if (!drawOnlyRing)
		GL(gl->glDrawElements(GL_TRIANGLES, model.indiceArr.size(), GL_UNSIGNED_SHORT, model.indiceArr.constData()));

	if (rings)
	{
		// Draw the rings just after the planet
		painter->setDepthMask(false);

		// Normal transparency mode
		painter->setBlending(true);

		Ring3DModel ringModel;
		sRing(&ringModel, rings->radiusMin, rings->radiusMax, 128, 32);
		
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.isRing, true));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.tex, 2));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.ringS, 1));
		
		QMatrix4x4 shadowCandidatesData;
		const Vec4d position = rData.mTarget * rData.modelMatrix.getColumn(3);
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
		
		if (rData.eyePos[2]<0)
			gl->glCullFace(GL_FRONT);

		GL(gl->glDrawElements(GL_TRIANGLES, ringModel.indiceArr.size(), GL_UNSIGNED_SHORT, ringModel.indiceArr.constData()));
		
		if (rData.eyePos[2]<0)
			gl->glCullFace(GL_BACK);
		
		painter->setDepthTest(false);
	}
	
	GL(shader->release());
	
	painter->setCullFace(false);
}

Planet::PlanetOBJModel* Planet::loadObjModel() const
{
	PlanetOBJModel* mdl = new PlanetOBJModel();
	if(!mdl->obj->load(objModelPath))
	{
		//object loading failed
		qCritical()<<"Could not load planet OBJ model for"<<englishName;
		delete mdl;
		return Q_NULLPTR;
	}

	//ideally, all planet OBJs should only have a single object with a single material
	if(mdl->obj->getObjectList().size()>1)
		qWarning()<<"Planet OBJ model has more than one object defined, this may cause problems ...";
	if(mdl->obj->getMaterialList().size()>1)
		qWarning()<<"Planet OBJ model has more than one material defined, this may cause problems ...";

	//start texture loading
	const StelOBJ::Material& mat = mdl->obj->getMaterialList().at(mdl->obj->getObjectList().first().groups.first().materialIndex);
	if(mat.map_Kd.isEmpty())
	{
		//we use a custom 1x1 pixel texture in this case
		qWarning()<<"Planet OBJ model for"<<englishName<<"has no diffuse texture";
	}
	else
	{
		//this call starts loading the tex in background
		mdl->texture = StelApp::getInstance().getTextureManager().createTextureThread(mat.map_Kd,StelTexture::StelTextureParams(true,GL_LINEAR,GL_REPEAT,true),false);
	}

	//extract the pos array into separate vector, it is the only one we need on CPU side for drawing
	mdl->obj->splitVertexData(&mdl->posArray);
	mdl->bbox = mdl->obj->getAABBox();

	return mdl;
}

bool Planet::ensureObjLoaded()
{
	if(!objModel && !objModelLoader)
	{
		qDebug()<<"Queueing aysnc load of OBJ model for"<<englishName;
		//create the async OBJ model loader
		objModelLoader = new QFuture<PlanetOBJModel*>(QtConcurrent::run(this,&Planet::loadObjModel));
	}

	if(objModelLoader)
	{
		if(objModelLoader->isFinished())
		{
			//the model loading has just finished, save the result
			objModel = objModelLoader->result();
			delete objModelLoader; //we dont need the result anymore
			objModelLoader = Q_NULLPTR;

			if(!objModel)
			{
				//model load failed, fall back to sphere mode
				objModelPath.clear();
				qWarning()<<"Cannot load OBJ model for solar system object"<<getEnglishName();
				return false;
			}
			else
			{
				//load model data into GL
				if(!objModel->loadGL())
				{
					delete objModel;
					objModel = Q_NULLPTR;
					objModelPath.clear();
					qWarning()<<"Cannot load OBJ model into OpenGL for solar system object"<<getEnglishName();
					return false;
				}
				GL(;);
			}
		}
		else
		{
			//we are still loading, use the sphere method for drawing
			return false;
		}
	}

	//OBJ model is valid!
	return true;
}

bool Planet::drawObjModel(StelPainter *painter, float screenSz)
{
	Q_UNUSED(screenSz); //screen size unused for now, use it for LOD or something?

	//make sure the OBJ is loaded, or start loading it
	if(!ensureObjLoaded())
		return false;

	if(shaderError)
	{
		qDebug() << "Planet::drawObjModel: Something went wrong with shader initialisation. Cannot draw OBJs, using spheres instead.";
		return false;
	}

	const SolarSystem* ssm = GETSTELMODULE(SolarSystem);

	QMatrix4x4 shadowMatrix;
	bool shadowmapping = false;
	if(ssm->getFlagShowObjSelfShadows())
		shadowmapping = drawObjShadowMap(painter,shadowMatrix);

	if(objModel->texture)
	{
		if(!objModel->texture->bind())
		{
			//the texture is still loading, use the sphere method
			return false;
		}
	}
	else
	{
		//HACK: there is no texture defined, we create a 1x1 pixel texture with color*albedo
		//this is not the most efficient method, but prevents having to rewrite the shader to work without a texture
		//removing some complexity in managing this use-case
		Vec3f texCol = haloColor * albedo * 255.0f + Vec3f(0.5f);
		//convert to byte
		Vector3<GLubyte> colByte(texCol[0],texCol[1],texCol[2]);
		GLuint tex;
		gl->glActiveTexture(GL_TEXTURE0);
		gl->glGenTextures(1, &tex);
		gl->glBindTexture(GL_TEXTURE_2D,tex);
		GLint oldalignment;
		gl->glGetIntegerv(GL_UNPACK_ALIGNMENT,&oldalignment);
		gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, colByte.v );
		gl->glPixelStorei(GL_UNPACK_ALIGNMENT, oldalignment);

		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// create a StelTexture from this
		objModel->texture = StelApp::getInstance().getTextureManager().wrapperForGLTexture(tex);
	}

	if(objModel->needsRescale)
		objModel->performScaling(AU_KM * sphereScale);

	//the model is ready to draw!
	painter->setBlending(false);
	painter->setCullFace(true);
	gl->glCullFace(GL_BACK);
	//depth testing is required here
	painter->setDepthTest(true);
	painter->setDepthMask(true);
	gl->glClear(GL_DEPTH_BUFFER_BIT);

	// Bind the array
	GL(objModel->arr->bind());

	//set up shader
	QOpenGLShaderProgram* shd = objShaderProgram;
	PlanetShaderVars* shdVars = &objShaderVars;
	if(shadowmapping)
	{
		shd = objShadowShaderProgram;
		shdVars = &objShadowShaderVars;
		gl->glActiveTexture(GL_TEXTURE1);
		gl->glBindTexture(GL_TEXTURE_2D,shadowTex);
		GL(shd->bind());
		GL(shd->setUniformValue(shdVars->shadowMatrix, shadowMatrix));
		GL(shd->setUniformValue(shdVars->shadowTex, 1));
	}
	else
		shd->bind();

	//project the data
	//because the StelOpenGLArray might use a VAO, we have to use a OGL buffer here in all cases
	objModel->projPosBuffer->bind();
	const int vtxCount = objModel->posArray.size();

	const StelProjectorP& projector = painter->getProjector();

	// I tested buffer orphaning (https://www.opengl.org/wiki/Buffer_Object_Streaming#Buffer_re-specification),
	// but it seems there is not really much of a effect here (probably because we are already pretty CPU-bound).
	// Also, map()-ing the buffer directly, like:
	//	Vec3f* bufPtr = static_cast<Vec3f*>(objModel->projPosBuffer->map(QOpenGLBuffer::WriteOnly));
	//	projector->project(vtxCount,objModel->posArray.constData(),bufPtr);
	//	objModel->projPosBuffer->unmap();
	// caused a 40% FPS drop for some reason!
	// (in theory, this should be faster because it should avoid copying the array)
	// So, lets not do that and just use this simple way in all cases:
	projector->project(vtxCount, objModel->scaledArray.constData(),objModel->projectedPosArray.data());
	objModel->projPosBuffer->allocate(objModel->projectedPosArray.constData(),vtxCount * sizeof(Vec3f));

	//unprojectedVertex, normalIn and texCoord are set by the StelOpenGLArray
	//we only need to set the freshly projected data
	GL(shd->setAttributeArray("vertex",GL_FLOAT,Q_NULLPTR,3,0));
	GL(shd->enableAttributeArray("vertex"));
	objModel->projPosBuffer->release();

	setCommonShaderUniforms(*painter,shd,*shdVars);

	//draw that model using the array wrapper
	objModel->arr->draw();

	shd->disableAttributeArray("vertex");
	shd->release();
	objModel->arr->release();

	painter->setCullFace(false);
	painter->setDepthTest(false);

	return true;
}

bool Planet::drawObjShadowMap(StelPainter *painter, QMatrix4x4& shadowMatrix)
{
	if(!shadowInitialized)
		if(!initFBO())
		{
			qDebug()<<"Cannot draw OBJ self-shadow";
			return false;
		}

	const StelProjectorP projector = painter->getProjector();

	//find the light direction in model space
	//Mat4d modelMatrix;
	//computeModelMatrix(modelMatrix);
	//Mat4d worldToModel = modelMatrix.inverse();

	Vec3d lightDir = light.position.toVec3d();
	projector->getModelViewTransform()->backward(lightDir);
	//Vec3d lightDir(worldToModel[12], worldToModel[13], worldToModel[14]);
	lightDir.normalize();

	//use a distance of 1km to the origin for additional precision, instead of 1AU
	Vec3d lightPosScaled = lightDir;

	//the camera looks to the origin
	QMatrix4x4 modelView;
	modelView.lookAt(QVector3D(lightPosScaled[0],lightPosScaled[1],lightPosScaled[2]), QVector3D(), QVector3D(0,0,1));

	//create an orthographic projection encompassing the AABB
	double maxZ = -std::numeric_limits<double>::max();
	double minZ = std::numeric_limits<double>::max();
	double maxUp = -std::numeric_limits<double>::max();
	double minUp = std::numeric_limits<double>::max();
	double maxRight = -std::numeric_limits<double>::max();
	double minRight = std::numeric_limits<double>::max();

	//create an orthonormal system using 2-step Gram-Schmidt + cross product
	// https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
	Vec3d vDir = -lightDir; //u1/e1, view direction
	Vec3d up = Vec3d(0.0, 0.0, 1.0); //v2
	up = up - up.dot(vDir) * vDir; //u2 = v2 - proj_u1(v2)
	up.normalize(); //e2

	Vec3d right = vDir^up;
	right.normalize();

	for(unsigned int i=0; i<AABBox::CORNERCOUNT; i++)
	{
		Vec3d v = objModel->bbox.getCorner(static_cast<AABBox::Corner>(i)).toVec3d();
		Vec3d fromCam = v - lightPosScaled; //vector from cam to vertex

		//project the fromCam vector onto the 3 vectors of the orthonormal system
		double dist = fromCam.dot(vDir);
		maxZ = std::max(dist, maxZ);
		minZ = std::min(dist, minZ);

		dist = fromCam.dot(right);
		minRight = std::min(dist, minRight);
		maxRight = std::max(dist, maxRight);

		dist = fromCam.dot(up);
		minUp = std::min(dist, minUp);
		maxUp = std::max(dist, maxUp);
	}

	QMatrix4x4 proj;
	proj.ortho(minRight,maxRight,minUp,maxUp,minZ,maxZ);

	QMatrix4x4 mvp = proj * modelView;

	//bias matrix for lookup
	static const QMatrix4x4 biasMatrix(0.5f, 0.0f, 0.0f, 0.5f,
					   0.0f, 0.5f, 0.0f, 0.5f,
					   0.0f, 0.0f, 0.5f, 0.5f,
					   0.0f, 0.0f, 0.0f, 1.0f);
	shadowMatrix = biasMatrix * mvp;

	painter->setDepthTest(true);
	painter->setDepthMask(true);
	painter->setCullFace(true);
	gl->glCullFace(GL_BACK);
	bool useOffset = !qFuzzyIsNull(shadowPolyOffset.lengthSquared());

	if(useOffset)
	{
		gl->glEnable(GL_POLYGON_OFFSET_FILL);
		gl->glPolygonOffset(shadowPolyOffset[0], shadowPolyOffset[1]);
	}

	gl->glViewport(0,0,SM_SIZE,SM_SIZE);

	GL(objModel->arr->bind());
	GL(transformShaderProgram->bind());
	GL(transformShaderProgram->setUniformValue(transformShaderVars.projectionMatrix, mvp));

#ifdef DEBUG_SHADOWMAP
	shadowFBO->bind();
	objModel->texture->bind();
	transformShaderProgram->setUniformValue(transformShaderVars.tex, 0);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	gl->glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	gl->glClear(GL_DEPTH_BUFFER_BIT);
#endif

	GL(objModel->arr->draw());

	transformShaderProgram->release();
	objModel->arr->release();

#ifdef DEBUG_SHADOWMAP
	//copy depth buffer into shadowTex
	gl->glActiveTexture(GL_TEXTURE1);
	gl->glBindTexture(GL_TEXTURE_2D,shadowTex);

	//this is probably unsupported on an OGL ES2 context! just don't use DEBUG_SHADOWMAP here...
	GL(QOpenGLContext::currentContext()->functions()->glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, SM_SIZE, SM_SIZE, 0));
#endif
	gl->glBindFramebuffer(GL_FRAMEBUFFER, StelApp::getInstance().getDefaultFBO());

	//reset viewport (see StelPainter::setProjector)
	const Vec4i& vp = projector->getViewport();
	gl->glViewport(vp[0], vp[1], vp[2], vp[3]);

	if(useOffset)
	{
		gl->glDisable(GL_POLYGON_OFFSET_FILL);
		GL(gl->glPolygonOffset(0.0f,0.0f));
	}

#ifdef DEBUG_SHADOWMAP
	//display the FB contents on-screen
	QOpenGLFramebufferObject::blitFramebuffer(Q_NULLPTR,shadowFBO);
#endif

	return true;
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
	sPainter.setBlending(true);
	Planet::hintCircleTex->bind();
	sPainter.drawSprite2dMode(screenPos[0], screenPos[1], 11);
}

Ring::Ring(float radiusMin, float radiusMax, const QString &texname)
	:radiusMin(radiusMin),radiusMax(radiusMax)
{
	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/"+texname);
}

Vec3f Planet::getCurrentOrbitColor() const
{
	Vec3f orbColor = orbitColor;
	switch(orbitColorStyle)
	{
		case ocsGroups:
		{
			switch (pType)
			{
				case isMoon:
					orbColor = orbitMoonsColor;
					break;
				case isPlanet:
					orbColor = orbitMajorPlanetsColor;
					break;
				case isAsteroid:
					orbColor = orbitMinorPlanetsColor;
					break;
				case isDwarfPlanet:
					orbColor = orbitDwarfPlanetsColor;
					break;
				case isCubewano:
					orbColor = orbitCubewanosColor;
					break;
				case isPlutino:
					orbColor = orbitPlutinosColor;
					break;
				case isSDO:
					orbColor = orbitScatteredDiscObjectsColor;
					break;
				case isOCO:
					orbColor = orbitOortCloudObjectsColor;
					break;
				case isComet:
					orbColor = orbitCometsColor;
					break;
				case isSednoid:
					orbColor = orbitSednoidsColor;
					break;
				default:
					orbColor = orbitColor;
			}
			break;
		}
		case ocsMajorPlanets:
		{
			QString pName = getEnglishName().toLower();
			if (pName=="mercury")
				orbColor = orbitMercuryColor;
			else if (pName=="venus")
				orbColor = orbitVenusColor;
			else if (pName=="earth")
				orbColor = orbitEarthColor;
			else if (pName=="mars")
				orbColor = orbitMarsColor;
			else if (pName=="jupiter")
				orbColor = orbitJupiterColor;
			else if (pName=="saturn")
				orbColor = orbitSaturnColor;
			else if (pName=="uranus")
				orbColor = orbitUranusColor;
			else if (pName=="neptune")
				orbColor = orbitNeptuneColor;
			else
				orbColor = orbitColor;
			break;
		}
		case ocsOneColor:
		{
			orbColor = orbitColor;
			break;
		}
	}

	return orbColor;
}

// draw orbital path of Planet
void Planet::drawOrbit(const StelCore* core)
{
	if (!orbitFader.getInterstate())
		return;
	if (!re.siderealPeriod)
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameHeliocentricEclipticJ2000);

	StelPainter sPainter(prj);

	// Normal transparency mode
	sPainter.setBlending(true);

	Vec3f orbColor = getCurrentOrbitColor();

	sPainter.setColor(orbColor[0], orbColor[1], orbColor[2], orbitFader.getInterstate());
	Vec3d onscreen;
	// special case - use current Planet position as center vertex so that draws
	// on its orbit all the time (since segmented rather than smooth curve)
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

void Planet::setApparentMagnitudeAlgorithm(QString algorithm)
{
	// sync default value with ViewDialog and SolarSystem!
	vMagAlgorithm = vMagAlgorithmMap.key(algorithm, Planet::ExplanatorySupplement_2013);
}
