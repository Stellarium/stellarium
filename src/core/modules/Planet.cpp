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
#include "planetsephems/EphemWrapper.hpp"
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
#include "StelHips.hpp"
#include "RefractionExtinction.hpp"
#include "StelObjectMgr.hpp"

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
#include <QElapsedTimer>

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
Vec3f Planet::orbitInterstellarColor = Vec3f(1.0f,0.2f,1.0f);
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

bool Planet::drawMoonHalo = true;
bool Planet::drawSunHalo = true;
bool Planet::permanentDrawingOrbits = false;
Planet::PlanetOrbitColorStyle Planet::orbitColorStyle = Planet::ocsOneColor;

int Planet::orbitsThickness = 1;

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

const QMap<Planet::PlanetType, QString> Planet::pTypeMap = // Maps type to english name.
{
	{Planet::isStar,	"star"},
	{Planet::isPlanet,	"planet"},
	{Planet::isMoon,	"moon"},
	{Planet::isObserver,	"observer"},
	{Planet::isArtificial,	"artificial"},
	{Planet::isAsteroid,	"asteroid"},
	{Planet::isPlutino,	"plutino"},
	{Planet::isComet,	"comet"},
	{Planet::isDwarfPlanet,	"dwarf planet"},
	{Planet::isCubewano,	"cubewano"},
	{Planet::isSDO,		"scattered disc object"},
	{Planet::isOCO,		"Oort cloud object"},
	{Planet::isSednoid,	"sednoid"},
	{Planet::isInterstellar,"interstellar object"},
	{Planet::isUNDEFINED,	"UNDEFINED"} // something must be broken before we ever see this!
};

const QMap<Planet::ApparentMagnitudeAlgorithm, QString> Planet::vMagAlgorithmMap =
{
	{Planet::MallamaHilton_2018,	        "Mallama2018"},
	{Planet::ExplanatorySupplement_2013,	"ExpSup2013"},
	{Planet::ExplanatorySupplement_1992,	"ExpSup1992"},
	{Planet::Mueller_1893,			"Mueller1893"},
	{Planet::AstronomicalAlmanac_1984,	"AstrAlm1984"},
	{Planet::Generic,			"Generic"},
	{Planet::UndefinedAlgorithm,		""}
};

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
	float aScale=static_cast<float>(scale);
	for(int i = 0; i<posArray.size();++i)
	{
		scaledArray[i]*=aScale;
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
	       Orbit* anOrbitPtr,
	       OsculatingFunctType *osculatingFunc,
	       bool acloseOrbit,
	       bool hidden,
	       bool hasAtmosphere,
	       bool hasHalo,
	       const QString& pTypeStr)
	: flagNativeName(true),
	  flagTranslatedName(true),
	  deltaJDE(StelCore::JD_SECOND),
	  deltaOrbitJDE(0.0),
	  closeOrbit(acloseOrbit),
	  englishName(englishName),
	  nameI18(englishName),
	  nativeName(""),
	  texMapName(atexMapName),
	  normalMapName(anormalMapName),
	  siderealPeriod(0.),
	  equatorialRadius(radius),
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
	  axisRotation(0.f),
	  objModel(Q_NULLPTR),
	  objModelLoader(Q_NULLPTR),
	  survey(Q_NULLPTR),
	  rings(Q_NULLPTR),
	  distance(0.0),
	  sphereScale(1.),
	  lastJDE(J2000),
	  coordFunc(coordFunc),
	  orbitPtr(anOrbitPtr),
	  osculatingFunc(osculatingFunc),
	  parent(Q_NULLPTR),
	  flagLabels(true),
	  hidden(hidden),
	  atmosphere(hasAtmosphere),
	  halo(hasHalo),
	  multisamplingEnabled_(StelApp::getInstance().getSettings()->value("video/multisampling", 0).toUInt() != 0),
	  gl(Q_NULLPTR),
	  iauMoonNumber(""),
	  positionsCache(ORBIT_SEGMENTS * 2)
{
	// Initialize pType with the key found in pTypeMap, or mark planet type as undefined.
	// The latter condition should obviously never happen.
	pType = pTypeMap.key(pTypeStr, Planet::isUNDEFINED);
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
	if ((pType <= isDwarfPlanet) && (englishName!="Pluto")) // concentrate on "inner" objects, KBO etc. stay at 1/s recomputation.
	{
		deltaJDE = 0.001*StelCore::JD_SECOND;
	}
}

// called in SolarSystem::init() before first planet is created. May initialize static variables.
void Planet::init()
{
	RotationElements::updatePlanetCorrections(J2000, RotationElements::EarthMoon);
	RotationElements::updatePlanetCorrections(J2000, RotationElements::Mars);
	RotationElements::updatePlanetCorrections(J2000, RotationElements::Jupiter);
	RotationElements::updatePlanetCorrections(J2000, RotationElements::Saturn);
	RotationElements::updatePlanetCorrections(J2000, RotationElements::Uranus);
	RotationElements::updatePlanetCorrections(J2000, RotationElements::Neptune);
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
    if (!iauMoonNumber.isEmpty())
        return QString("%1 (%2)").arg(englishName).arg(iauMoonNumber);
    else
        return englishName;
}

QString Planet::getNameI18n() const
{
    if (!iauMoonNumber.isEmpty())
        return QString("%1 (%2)").arg(nameI18).arg(iauMoonNumber);
    else
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
		case isInterstellar:
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

QString Planet::getInfoStringName(const StelCore *core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core) Q_UNUSED(flags)
	QString str;
	QTextStream oss(&str);
	oss << "<h2>";
	if (englishName=="Pluto") // We must prepend minor planet number here. Actually Dwarf Planet Pluto is still a "Planet" object in Stellarium...
		oss << QString("(134340) ");
	oss << getNameI18n();  // UI translation can differ from sky translation
	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(1);
	if (sphereScale != 1.)
		oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
	oss << "</h2>";
	return str;
}

QString Planet::getInfoStringAbsoluteMagnitude(const StelCore *core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core)
	QString str;
	QTextStream oss(&str);
	if (flags&AbsoluteMagnitude && (getAbsoluteMagnitude() > -99.f))
	{
		oss << QString("%1: %2<br/>").arg(q_("Absolute Magnitude")).arg(getAbsoluteMagnitude(), 0, 'f', 2);
		const float moMag=getMeanOppositionMagnitude();
		if (moMag<50.f)
			oss << QString("%1: %2<br/>").arg(q_("Mean Opposition Magnitude")).arg(moMag, 0, 'f', 2);
	}

	return str;
}

// Return the information string "ready to print" :)
QString Planet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	const double distanceAu = getJ2000EquatorialPos(core).length();

	if (flags&Name)
	{
		oss << getInfoStringName(core, flags);

		QStringList extraNames=getExtraInfoStrings(Name);
		if (extraNames.length()>0)
			oss << q_("Additional names: ") << extraNames.join(", ") << "<br/>";
	}

	if (flags&CatalogNumber)
	{
		QStringList extraCat=getExtraInfoStrings(CatalogNumber);
		if (extraCat.length()>0)
			oss << q_("Additional catalog numbers: ") << extraCat.join(", ") << "<br/>";
	}

	if (flags&ObjectType && getPlanetType()!=isUNDEFINED)
	{
		if (getPlanetType()==isComet)
		{
			const QString cometType = (static_cast<KeplerOrbit*>(orbitPtr)->getEccentricity() < 1.0) ?
						qc_("periodic", "type of comet") :
						qc_("non-periodic", "type of comet");
			oss << QString("%1: <b>%2</b> (%3)<br/>").arg(q_("Type"), q_(getPlanetTypeString()), cometType);
		}
		else		
			oss << QString("%1: <b>%2</b><br/>").arg(q_("Type"), q_(getPlanetTypeString()));		
	}

	if (flags&Magnitude)
	{
		static const QMap<ApparentMagnitudeAlgorithm, int>decMap={
			{ Mueller_1893,               1 },
			{ AstronomicalAlmanac_1984,   1 },
			{ ExplanatorySupplement_1992, 1 },
			{ ExplanatorySupplement_2013, 2 },
			{ MallamaHilton_2018,         2 }};
		if (!fuzzyEquals(getVMagnitude(core), std::numeric_limits<float>::infinity()))
			oss << getMagnitudeInfoString(core, flags, decMap.value(vMagAlgorithm, 1));
		oss << getExtraInfoStrings(Magnitude).join("");
	}

	if (flags&AbsoluteMagnitude)
	{
		oss << getInfoStringAbsoluteMagnitude(core, flags);
		oss << getExtraInfoStrings(AbsoluteMagnitude).join("");
	}

	oss << getInfoStringExtraMag(core, flags);
	oss << getCommonInfoString(core, flags);

	// Debug help.
	//oss << "Apparent Magnitude Algorithm: " << getApparentMagnitudeAlgorithmString() << " " << vMagAlgorithm << "<br>";
	//Vec3d sunAberr=GETSTELMODULE(SolarSystem)->getLightTimeSunPosition()-GETSTELMODULE(SolarSystem)->getEarth()->eclipticPos;
	//double lon, lat;
	//StelUtils::rectToSphe(&lon, &lat, sunAberr);
	//oss << "Sun (light time corrected) at &lambda;=" << StelUtils::radToDmsStr(StelUtils::fmodpos(lon, 2.*M_PI)) << " &beta;=" << StelUtils::radToDmsStr(lat) << "<br>";
#ifndef NDEBUG
	// This is mostly for debugging. Maybe also useful for letting people use our results to cross-check theirs, but we should not act as reference, currently...
	// maybe separate this out into:
	//if (flags&EclipticCoordXYZ)
	// For now: add to EclipticCoordJ2000 group
	if (flags&EclipticCoordJ2000)
	{
		Vec3d eclPos=(englishName=="Sun" ? GETSTELMODULE(SolarSystem)->getLightTimeSunPosition() : eclipticPos);
		QString algoName("VSOP87");
		if (EphemWrapper::use_de431(core->getJDE())) algoName="DE431";
		if (EphemWrapper::use_de430(core->getJDE())) algoName="DE430";
		if (pType>=isAsteroid) algoName="Keplerian"; // TODO: observer/artificial?
		// TRANSLATORS: Ecliptical rectangular coordinates
		oss << QString("%1 XYZ J2000.0 (%2): %3/%4/%5").arg(qc_("Ecliptical","coordinates"), algoName, QString::number(eclPos[0], 'f', 7), QString::number(eclPos[1], 'f', 7), QString::number(eclPos[2], 'f', 7)) << "<br>";
	}
#endif

	// Second test avoids crash when observer is on spaceship
	if (flags&ProperMotion && !core->getCurrentObserver()->isObserverLifeOver())
	{
		const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
		// Setting/resetting the time causes a significant slowdown. We must apply some trickery to keep time in sync.
		Vec3d equPos=getEquinoxEquatorialPos(core);
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,equPos);
		StelCore* core1 = StelApp::getInstance().getCore(); // we need non-const reference here.
		const double currentJD=core1->getJDOfLastJDUpdate();
		const qint64 millis=core1->getMilliSecondsOfLastJDUpdate();
		core1->setJD(currentJD-StelCore::JD_HOUR);
		core1->update(0);
		Vec3d equPosPrev=getEquinoxEquatorialPos(core1);
		const double deltaEq=equPos.angle(equPosPrev);
		double dec_equPrev, ra_equPrev;
		StelUtils::rectToSphe(&ra_equPrev,&dec_equPrev,equPosPrev);
		double pa=atan2(ra_equ-ra_equPrev, dec_equ-dec_equPrev); // position angle: From North counterclockwise!
		if (pa<0) pa += 2.*M_PI;
		oss << QString("%1: %2 %3 %4%5<br/>").arg(q_("Hourly motion"), withDecimalDegree ? StelUtils::radToDecDegStr(deltaEq) : StelUtils::radToDmsStr(deltaEq), qc_("towards", "into the direction of"), QString::number(pa*M_180_PI, 'f', 1), QChar(0x00B0));
		oss << QString("%1: d&alpha;=%2 d&delta;=%3<br/>").arg(q_("Hourly motion"), withDecimalDegree ? StelUtils::radToDecDegStr(ra_equ-ra_equPrev) : StelUtils::radToDmsStr(ra_equ-ra_equPrev), withDecimalDegree ? StelUtils::radToDecDegStr(dec_equ-dec_equPrev) : StelUtils::radToDmsStr(dec_equ-dec_equPrev));
		core1->setJD(currentJD); // this calls sync() which sets millis
		core1->setMilliSecondsOfLastJDUpdate(millis); // restore millis.
		core1->update(0);
	}

	oss << getInfoStringEloPhase(core, flags, pType<=isMoon);


	if (flags&Distance)
	{
		const double hdistanceAu = getHeliocentricEclipticPos().length();
		const double hdistanceKm = AU * hdistanceAu;
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
				// TRANSLATORS: Unit of measure for distance - millions of kilometers
				km = qc_("M km", "distance");
			}

			oss << QString("%1: %2 %3 (%4 %5)<br/>").arg(q_("Distance from Sun"), distAU, au, distKM, km);
		}
		const double distanceKm = AU * distanceAu;
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
			// TRANSLATORS: Unit of measure for distance - millions of kilometers
			km = qc_("M km", "distance");
		}

		oss << QString("%1: %2 %3 (%4 %5)<br/>").arg(q_("Distance"), distAU, au, distKM, km);
		// TRANSLATORS: Distance measured in terms of the speed of light
		oss << QString("%1: %2 <br/>").arg(q_("Light time"), StelUtils::hoursToHmsStr(distanceKm/SPEED_OF_LIGHT/3600.) );
		oss << getExtraInfoStrings(Distance).join("");
	}

	if (flags&Velocity)
	{
		// TRANSLATORS: Unit of measure for speed - kilometers per second
		QString kms = qc_("km/s", "speed");

		Vec3d orbitalVel=getEclipticVelocity();
		const double orbVel=orbitalVel.length();
		if (orbVel>0.)
		{ // AU/d * km/AU /24
			const double orbVelKms=orbVel* AU/86400.;
			oss << QString("%1: %2 %3<br/>").arg(q_("Orbital velocity")).arg(orbVelKms, 0, 'f', 3).arg(kms);
			const double helioVel=getHeliocentricEclipticVelocity().length();
			if (!fuzzyEquals(helioVel, orbVel))
				oss << QString("%1: %2 %3<br/>").arg(q_("Heliocentric velocity")).arg(helioVel* AU/86400., 0, 'f', 3).arg(kms);
		}
		oss << getExtraInfoStrings(Velocity).join("");
	}
	oss << getInfoStringPeriods(core, flags);
	oss << getInfoStringSize(core, flags);
	oss << getInfoStringExtra(core, flags);
	postProcessInfoString(str, flags);
	return str;
}

// Print apparent and equatorial diameters
QString Planet::getInfoStringSize(const StelCore *core, const InfoStringGroup& flags) const
{
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	QString str;
	QTextStream oss(&str);

	const double angularSize = 2.*getAngularSize(core)*M_PI_180;
	if (flags&Size && angularSize>=4.8e-8)
	{
		QString s1, s2, sizeStr = "";
		if (rings)
		{
			double withoutRings = 2.*getSpheroidAngularSize(core)*M_PI/180.;
			if (withDecimalDegree)
			{
				s1 = StelUtils::radToDecDegStr(withoutRings, 5, false, true);
				s2 = StelUtils::radToDecDegStr(angularSize, 5, false, true);
			}
			else
			{
				s1 = StelUtils::radToDmsPStr(withoutRings, 2);
				s2 = StelUtils::radToDmsPStr(angularSize, 2);
			}

			sizeStr = QString("%1, %2: %3").arg(s1, q_("with rings"), s2);
		}
		else
		{
			if (sphereScale!=1.) // We must give correct diameters even if upscaling (e.g. Moon)
			{
				if (withDecimalDegree)
				{
					s1 = StelUtils::radToDecDegStr(angularSize / sphereScale, 5, false, true);
					s2 = StelUtils::radToDecDegStr(angularSize, 5, false, true);
				}
				else
				{
					s1 = StelUtils::radToDmsPStr(angularSize / sphereScale, 2);
					s2 = StelUtils::radToDmsPStr(angularSize, 2);
				}

				sizeStr = QString("%1, %2: %3").arg(s1, q_("scaled up to"), s2);
			}
			else
			{
				if (withDecimalDegree)
					sizeStr = StelUtils::radToDecDegStr(angularSize, 5, false, true);
				else
					sizeStr = StelUtils::radToDmsPStr(angularSize, 2);
			}
		}
		oss << QString("%1: %2<br/>").arg(q_("Apparent diameter"), sizeStr);
	}

	if (flags&Size)
	{
		QString diam = (getPlanetType()==isPlanet ? q_("Equatorial diameter") : q_("Diameter")); // Many asteroids have irregular shape (Currently unhandled)
		oss << QString("%1: %2 %3<br/>").arg(diam, QString::number(AU * getEquatorialRadius() * 2.0, 'f', 1) , qc_("km", "distance"));
		oss << getExtraInfoStrings(Size).join("");
	}
	return str;
}

QString Planet::getInfoStringExtraMag(const StelCore *core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core) Q_UNUSED(flags)
	return "";
}

QString Planet::getInfoStringEloPhase(const StelCore *core, const InfoStringGroup& flags, const bool withIllum) const
{
	QString str;
	QTextStream oss(&str);
	if ((flags&Elongation) && (englishName!="Sun") && (core->getCurrentPlanet()->englishName!="Sun"))
	{
		const bool withTables = StelApp::getInstance().getFlagUseFormattingOutput();
		const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
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

		if (withTables)
		{
			oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
			oss << QString("<tr><td>%1:</td><td align=\"right\">%2</td></tr>").arg(q_("Elongation"), elo);
			oss << QString("<tr><td>%1:</td><td align=\"right\">%2</td></tr>").arg(q_("Phase angle"), pha);
			if (withIllum)
				oss << QString("<tr><td>%1:</td><td align=\"right\">%2%</td></tr>").arg(q_("Illuminated")).arg(QString::number(getPhase(observerHelioPos) * 100., 'f', 1));
			oss << "</table>";
		}
		else
		{
			oss << QString("%1: %2<br/>").arg(q_("Elongation"), elo);
			oss << QString("%1: %2<br/>").arg(q_("Phase angle"), pha);
			if (withIllum)
				oss << QString("%1: %2%<br/>").arg(q_("Illuminated"), QString::number(getPhase(observerHelioPos) * 100., 'f', 1));
		}

		if (getPlanetType()==isMoon && this->parent!=core->getCurrentPlanet())
		{
			QString ad;
			const double angularDistance = getJ2000EquatorialPos(core).angle(this->parent->getJ2000EquatorialPos(core));
			if (withDecimalDegree)
				ad = StelUtils::radToDecDegStr(angularDistance,4,false,true);
			else
				ad = StelUtils::radToDmsStr(angularDistance, true);

			oss << QString("%1 %2 &mdash; %3: %4<br/>").arg(q_("Angular distance"), getNameI18n(), this->parent->getNameI18n(), ad);
		}
	}
	return str;
}

QString Planet::getInfoStringPeriods(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Extra)
	{
		const bool withTables = StelApp::getInstance().getFlagUseFormattingOutput();
		if (withTables)
			oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
		const QByteArray fmt=withTables ? "<tr><td>%1:</td><td style='text-align: right;'> %2</td><td> %3 (%4 a)</td></tr>" : "%1: %2 %3 (%4 a)<br/>";
		// TRANSLATORS: Unit of measure for period - days
		QString days = qc_("days", "duration");
		// Sidereal and synodic periods are better sorted here
		PlanetP currentPlanet = core->getCurrentPlanet();
		const double siderealPeriod = getSiderealPeriod(); // days required for revolution around parent.
		const double siderealPeriodCurrentPlanet = currentPlanet->getSiderealPeriod();
		QString celestialObject = getEnglishName();
		if ((siderealPeriod>0.0) && (celestialObject != "Sun"))
		{
			// Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
			oss << QString(fmt).arg(q_("Sidereal period"), QString::number(siderealPeriod, 'f', 2), days, QString::number(siderealPeriod/365.25, 'f', 3));
		}
		if (celestialObject!="Sun")
			celestialObject = getParent()->getEnglishName();
		// Synodic revolution period
		if (siderealPeriodCurrentPlanet > 0.0 && siderealPeriod > 0.0 && currentPlanet->getPlanetType()==Planet::isPlanet && (getPlanetType()==Planet::isPlanet || currentPlanet->getEnglishName()==celestialObject))
		{
			double synodicPeriod = qAbs(1/(1/siderealPeriodCurrentPlanet - 1/siderealPeriod));
			// Synodic period for major planets in days and in Julian years (symbol: a)
			oss << QString(fmt).arg(q_("Synodic period")).arg(QString::number(synodicPeriod, 'f', 2)).arg(days).arg(QString::number(synodicPeriod/365.25, 'f', 3));
		}
		if (withTables)
			oss << "</table>";
	}
	return str;
}

class SolarEclipse
{
private:
	double raSun = 0, deSun = 0, sdistanceAu = 0, raMoon = 0, deMoon = 0, mdistanceER = 0, gast = 0;
	double rss = 0, a = 0, b = 0, d = 0, x = 0, y = 0, z = 0, mu = 0, f1 = 0, f2 = 0, tf1 = 0, tf2 = 0;
	double L1 = 0, L2 = 0, lon = 0, mag = 0, cd = 0, rho1 = 0;
	double y1 = 0, xi = 0, eta1 = 0, sd1 = 0, cd1 = 0, rho2 = 0, sd = 0, sd1d2 = 0, cd1d2 = 0;
	double zeta1 = 0, zeta = 0, sd2 = 0, theta = 0, sfn1 = 0, cfn1 = 0;
	const double SunEarth = 109.12278; // ratio of Sun-Earth radius 696000/6378.1366
	double lat = 99; // initialize an impossible latitude to indicate no central eclipse

public:
	Vec3d point(const StelCore* score)
	{
		static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
		StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(score));
		StelUtils::rectToSphe(&raMoon, &deMoon, ssystem->getMoon()->getEquinoxEquatorialPos(score));

		sdistanceAu = ssystem->getSun()->getEquinoxEquatorialPos(score).length();
		// Moon's distance in Earth's radius
		mdistanceER = ssystem->getMoon()->getEquinoxEquatorialPos(score).length() * AU / 6378.1366;
		// Greenwich Apparent Sidereal Time
		gast = (get_apparent_sidereal_time(score->getJD(), score->getJDE()));

		if (raSun < 0.) raSun += M_PI * 2.;
		if (raMoon < 0.) raMoon += M_PI * 2.;

		// Besselian elements
		// based on Explanatory supplement to the astronomical ephemeris
		// and the American ephemeris and nautical almanac (1961)
		rss = sdistanceAu * 23454.7925; // from 1 AU/Earth's radius : 149597870.8/6378.1366
		b = mdistanceER / rss;
		a = raSun - ((b * cos(deMoon) * (raMoon - raSun)) / ((1 - b) * cos(deSun)));
		d = deSun - (b * (deMoon - deSun) / (1 - b));
		x = cos(deMoon) * sin((raMoon - a));
		x = mdistanceER * x;
		y = cos(d) * sin(deMoon);
		y = y - cos(deMoon) * sin(d) * cos((raMoon - a));
		y = mdistanceER * y;
		z = sin(deMoon) * sin(d);
		z = z + cos(deMoon) * cos(d) * cos((raMoon - a));
		z = mdistanceER * z;
		// parameters of the shadow cone
		f1 = asin((SunEarth + 0.272488) / (rss * (1 - b)));
		tf1 = tan(f1);
		f2 = asin((SunEarth - 0.272281) / (rss * (1 - b)));
		tf2 = tan(f2);
		L1 = z * tf1 + (0.272488 / cos(f1));
		L2 = z * tf2 - (0.272281 / cos(f2));
		mu = gast - a / M_PI_180;

		// Find Lat./Long. of center line on Earth's surface
		cd = cos(d);
		rho1 = sqrt(1 - 0.00669398 * cd * cd);
		// e^2 = 0.00669398 : Earth flattening parameter
		// IERS 2010 : f = 298.25642 : e^2 = 2f-f^2
		y1 = y / rho1;
		xi = x;
		eta1 = y1;
		sd = sin(d);
		sd1 = sd / rho1;
		cd1 = sqrt(1 - 0.00669398) * cd / rho1;
		rho2 = sqrt(1 - 0.00669398 * sd * sd);
		sd1d2 = 0.00669398 * sd * cd / (rho1 * rho2);
		cd1d2 = sqrt(1 - sd1d2 * sd1d2);

		if ((1 - x * x - y1 * y1) > 0)
		{
			zeta1 = sqrt(1 - x * x - y1 * y1);
			zeta = rho2 * (zeta1 * cd1d2 - eta1 * sd1d2);
			sd2 = sd * 1.0033641 / rho2;
			L2 = L2 - zeta * tf2;
			b = -y * sd + zeta * cd;
			theta = atan2(xi, b) / M_PI_180;
			if (theta < 0) theta += 360;
			if (mu > 360) mu -= 360;
			lon = mu - theta;
			if (lon < -180) lon += 360;
			if (lon > 180) lon -= 360;
			lon = -lon; // + East, - West
			sfn1 = eta1 * cd1 + zeta1 * sd1;
			cfn1 = sqrt(1 - sfn1 * sfn1);
			lat = 1.0033641 * sfn1 / cfn1;
			lat = atan(lat) / M_PI_180;
			L1 = L1 - zeta * tf1;
			// Magnitude of eclipse
			// mag < 1 = annular
			mag = L1 / (L1 + L2);
		}
		return Vec3d(lat, lon, mag);
	}
};

QString Planet::getInfoStringExtra(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Extra)
	{
		const bool withTables = StelApp::getInstance().getFlagUseFormattingOutput();
		const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
		const double angularSize = 2.*getAngularSize(core)*M_PI_180;
		const double siderealPeriod = getSiderealPeriod(); // days required for revolution around parent.
		const double siderealDay = getSiderealDay(); // =re.period
		static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
		PlanetP earth = ssystem->getEarth();
		//PlanetP currentPlanet = core->getCurrentPlanet();
		const bool onEarth = (core->getCurrentPlanet()==earth);
		// TRANSLATORS: Unit of measure for speed - kilometers per second
		QString kms = qc_("km/s", "speed");

		// This is a string you can activate for debugging. It shows the distance between observer and center of the body you are standing on.
		// May be helpful for debugging critical parallax corrections for eclipses.
		// For general use, find a better location first.
		// oss << q_("Planetocentric distance &rho;: %1 (km)").arg(core->getCurrentObserver()->getDistanceFromCenter() * AU) <<"<br>";

		// TRANSLATORS: Unit of measure for period - days
		QString days = qc_("days", "duration");
		if (siderealPeriod>0.0)
		{
			if (qAbs(siderealDay)>0 && getPlanetType()!=isArtificial)
			{
				const QByteArray fmt=withTables ? "<tr><td>%1: </td><td>%2</td></tr>" : "%1: %2<br/>";

				if (withTables)
					oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
				oss << QString(fmt).arg(q_("Sidereal day"), StelUtils::hoursToHmsStr(qAbs(siderealDay*24)));
				if (englishName!="Sun")
					oss << QString(fmt).arg(q_("Mean solar day"), StelUtils::hoursToHmsStr(qAbs(getMeanSolarDay()*24)));
				if (withTables)
					oss << "</table>";
			}
			else if (re.period==0.)
			{
				oss << q_("The period of rotation is chaotic") << "<br />";
			}
			if (qAbs(re.W1)>0.)
			{
				const double eqRotVel = (2.0*M_PI*AU/(360.*86400.0))*getEquatorialRadius()*re.W1;
				oss << QString("%1: %2 %3<br/>").arg(q_("Equatorial rotation velocity")).arg(qAbs(eqRotVel), 0, 'f', 3).arg(kms);
			}
			else if (qAbs(re.period)>0.)
			{
				const double eqRotVel = 2.0*M_PI*(AU*getEquatorialRadius())/(getSiderealDay()*86400.0);
				if (eqRotVel>0.1) // it seems that objects with unspecified re.period have re.period=orbitPeriod, with useless eqRotVel ~0.
					oss << QString("%1: %2 %3<br/>").arg(q_("Equatorial rotation velocity")).arg(qAbs(eqRotVel), 0, 'f', 3).arg(kms);
			}
		}

		// PHYSICAL EPHEMERIS DATA
		// Lunar phase names, libration, or axis orientation/rotation data
		if (englishName=="Moon" && onEarth)
		{
			// For computing the Moon age we use geocentric coordinates
			StelCore* core1 = StelApp::getInstance().getCore(); // we need non-const reference here.
			const bool useTopocentric = core1->getUseTopocentricCoordinates();
			core1->setUseTopocentricCoordinates(false);
			core1->update(0); // enforce update cache!
			const double eclJDE = earth->getRotObliquity(core1->getJDE());
			double ra_equ, dec_equ, lambdaMoon, lambdaSun, betaMoon, betaSun, raSun, deSun;
			StelUtils::rectToSphe(&ra_equ,&dec_equ, getEquinoxEquatorialPos(core1));
			StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaMoon, &betaMoon);
			StelUtils::rectToSphe(&raSun,&deSun, ssystem->getSun()->getEquinoxEquatorialPos(core1));
			StelUtils::equToEcl(raSun, deSun, eclJDE, &lambdaSun, &betaSun);
			core1->setUseTopocentricCoordinates(useTopocentric);
			core1->update(0); // enforce update cache to avoid odd selection of Moon details!
			const double deltaLong = StelUtils::fmodpos((lambdaMoon-lambdaSun)*M_180_PI, 360.);
			QString moonPhase = "";
			if (deltaLong<0.5 || deltaLong>359.5)
				moonPhase = qc_("New Moon", "Moon phase");
			else if (deltaLong<89.5)
				moonPhase = qc_("Waxing Crescent", "Moon phase");
			else if (deltaLong<90.5)
				moonPhase = qc_("First Quarter", "Moon phase");
			else if (deltaLong<179.5)
				moonPhase = qc_("Waxing Gibbous", "Moon phase");
			else if (deltaLong<180.5)
				moonPhase = qc_("Full Moon", "Moon phase");
			else if (deltaLong<269.5)
				moonPhase = qc_("Waning Gibbous", "Moon phase");
			else if (deltaLong<270.5)
				moonPhase = qc_("Third Quarter", "Moon phase");
			else if (deltaLong<359.5)
				moonPhase = qc_("Waning Crescent", "Moon phase");
			else
			{
				qWarning() << "ERROR IN PHASE STRING PROGRAMMING!";
				Q_ASSERT(0);
			}

			const double age = deltaLong*29.530588853/360.;
			oss << QString("%1: %2 %3").arg(q_("Moon age"), QString::number(age, 'f', 1), q_("days old"));
			if (!moonPhase.isEmpty())
				oss << QString(" (%4)").arg(moonPhase);
			oss << "<br />";

			if (useTopocentric)
			{
				// we must repeat the position lookup from above in case we have topocentric corrections.
				StelUtils::rectToSphe(&ra_equ,&dec_equ, getEquinoxEquatorialPos(core));
				StelUtils::rectToSphe(&raSun,&deSun, ssystem->getSun()->getEquinoxEquatorialPos(core));
			}
			const double chi=atan2(cos(deSun)*sin(raSun-ra_equ), sin(deSun)*cos(dec_equ)-cos(deSun)*sin(dec_equ)*cos(raSun-ra_equ));
			QString chiStr;
			if (withDecimalDegree)
				chiStr=StelUtils::radToDecDegStr(StelUtils::fmodpos(chi, M_PI*2.0), 1);
			else
				chiStr=StelUtils::radToDmsStr(chi, false);
			if (withTables)
			{
				oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
				oss << QString("<tr><td colspan=\"2\">%1:</td><td align=\"right\"> %2</td></tr>").arg(q_("Position angle of bright limb"), chiStr);
			}
			else
				oss << QString("%1: %2<br/>").arg(q_("Position angle of bright limb"), chiStr);

			// Everything around libration
			const QStringList compassDirs={
				qc_("S",   "compass direction"),
				qc_("SSW", "compass direction"),
				qc_("SW",  "compass direction"),
				qc_("WSW", "compass direction"),
				qc_("W",   "compass direction"),
				qc_("WNW", "compass direction"),
				qc_("NW",  "compass direction"),
				qc_("NNW", "compass direction"),
				qc_("N",   "compass direction"),
				qc_("NNE", "compass direction"),
				qc_("NE",  "compass direction"),
				qc_("ENE", "compass direction"),
				qc_("E",   "compass direction"),
				qc_("ESE", "compass direction"),
				qc_("SE",  "compass direction"),
				qc_("SSE", "compass direction")};

			QPair<Vec4d, Vec3d> ssop=getSubSolarObserverPoints(core);

			const double Be=ssop.first[1];
			double Le   =StelUtils::fmodpos(-ssop.first[2],  M_PI*2.0); if (Le>M_PI) Le-=2.0*M_PI;
			const double Bs=ssop.second[1];
			double Ls   =StelUtils::fmodpos(-ssop.second[2], M_PI*2.0); if (Ls>M_PI) Ls-=2.0*M_PI;
			const double totalLibr=sqrt(Le*Le+Be*Be);
			double librAngle=StelUtils::fmodpos(atan2(Le, -Be), 2.0*M_PI);
			// find out and indicate which limb is optimally visible
			const int limbSector= std::lround(floor(StelUtils::fmodpos(librAngle*M_180_PI+11.25, 360.)/22.5));
			QString limbStr=compassDirs.at(limbSector);
			if (totalLibr>3.*M_PI_180)
				limbStr.append("!");
			if (totalLibr>5.*M_PI_180)
				limbStr.append("!");
			if (totalLibr>7.*M_PI_180)
				limbStr.append("!");
			QString paAxisStr, libLStr, libBStr, subsolarLStr, subsolarBStr, colongitudeStr, totalLibrationStr, librationAngleStr;
			if (withDecimalDegree)
			{
				paAxisStr=StelUtils::radToDecDegStr(ssop.first[3], 1);
				libLStr=StelUtils::radToDecDegStr(Le, 1);
				libBStr=StelUtils::radToDecDegStr(Be, 1);
				subsolarLStr=StelUtils::radToDecDegStr(Ls, 1);
				subsolarBStr=StelUtils::radToDecDegStr(Bs, 1);
				colongitudeStr=StelUtils::radToDecDegStr(StelUtils::fmodpos(450.0*M_PI_180-Ls, M_PI*2.0), 1);
				totalLibrationStr=StelUtils::radToDecDegStr(totalLibr, 1);
				librationAngleStr=StelUtils::radToDecDegStr(librAngle, 1);
			}
			else
			{
				paAxisStr=StelUtils::radToDmsStr(ssop.first[3]);
				libLStr=StelUtils::radToDmsStr(Le);
				libBStr=StelUtils::radToDmsStr(Be);
				subsolarLStr=StelUtils::radToDmsStr(Ls);
				subsolarBStr=StelUtils::radToDmsStr(Bs);
				colongitudeStr=StelUtils::radToDmsStr(StelUtils::fmodpos(450.0*M_PI_180-Ls, M_PI*2.0));
				totalLibrationStr=StelUtils::radToDmsStr(totalLibr);
				librationAngleStr=StelUtils::radToDmsStr(librAngle);
			}
			if (withTables)
			{
				//oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
				oss << QString("<tr><td colspan=\"2\">%1:</td><td align=\"right\"> %2</td></tr>").arg(q_("Position Angle of axis"), paAxisStr);
				oss << QString("<tr><td>%1:</td><td align=\"right\">%2 %3</td><td align=\"right\"> %4</td><td>(%5)</td></tr>").arg(q_("Libration"), totalLibrationStr, qc_("towards", "into the direction of"), librationAngleStr, limbStr);
				oss << QString("<tr><td>%1:</td><td align=\"right\">L: %2</td><td align=\"right\">B: %3</td></tr>").arg(q_("Libration"), libLStr, libBStr);
				oss << QString("<tr><td>%1:</td><td align=\"right\">L<sub>s</sub>: %2</td><td align=\"right\">B<sub>s</sub>: %3</td></tr>").arg(q_("Subsolar point"), subsolarLStr, subsolarBStr);
				oss << QString("<tr><td>%1:</td><td align=\"right\">c<sub>0</sub>: %2</td></tr>").arg(q_("Colongitude"), colongitudeStr);
				oss << "</table>";
			}
			else
			{
				oss << QString("%1: %2<br/>").arg(q_("Position Angle of axis"), paAxisStr);
				oss << QString("%1: %2 %3 %4 (%5)<br/>").arg(q_("Libration"), totalLibrationStr, qc_("towards", "into the direction of"), librationAngleStr, limbStr);
				oss << QString("%1: %2/%3").arg(q_("Libration"), libLStr, libBStr) << "<br/>";
				oss << QString("%1: %2/%3<br/>").arg(q_("Subsolar point"), subsolarLStr, subsolarBStr);
				oss << QString("%1: %2<br/>").arg(q_("Colongitude"), colongitudeStr);
			}
		}
		else if (englishName!="Sun" && onEarth)
		{
			// The planetographic longitudes (central meridian etc) are counted in the other direction than on Moon.
			QPair<Vec4d, Vec3d> ssop=getSubSolarObserverPoints(core);

			const double Le=StelUtils::fmodpos(ssop.first[2],  M_PI*2.0);
			const double Ls=StelUtils::fmodpos(ssop.second[2], M_PI*2.0);

			QString paAxisStr, subearthLStr, subearthBStr, subsolarLStr, subsolarBStr;
			const QString lngSystem=(englishName=="Jupiter" ? "II" : (englishName=="Saturn" ? "III" : ""));
			if (withDecimalDegree)
			{
				paAxisStr=StelUtils::radToDecDegStr(ssop.first[3], 1);
				subearthLStr=StelUtils::radToDecDegStr(Le, 1);
				subearthBStr=StelUtils::radToDecDegStr(ssop.first[1], 1);
				subsolarLStr=StelUtils::radToDecDegStr(Ls, 1);
				subsolarBStr=StelUtils::radToDecDegStr(ssop.second[1], 1);
			}
			else
			{
				paAxisStr=StelUtils::radToDmsStr(ssop.first[3]);
				subearthLStr=StelUtils::radToDmsStr(Le);
				subearthBStr=StelUtils::radToDmsStr(ssop.first[1]);
				subsolarLStr=StelUtils::radToDmsStr(Ls);
				subsolarBStr=StelUtils::radToDmsStr(ssop.second[1]);
			}
			if (withTables)
			{
				oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
				oss << QString("<tr><td colspan=\"2\">%1:</td><td align=\"right\"> %2</td></tr>").arg(q_("Position Angle of axis"), paAxisStr);
				oss << QString("<tr><td>%1:</td><td align=\"right\">L<sub>%2e</sub>: %3</td><td align=\"right\">&phi;<sub>e</sub>: %4</td></tr>").arg(q_("Center point"),   lngSystem, subearthLStr, subearthBStr);
				oss << QString("<tr><td>%1:</td><td align=\"right\">L<sub>%2s</sub>: %3</td><td align=\"right\">&phi;<sub>s</sub>: %4</td></tr>").arg(q_("Subsolar point"), lngSystem, subsolarLStr, subsolarBStr);
				oss << "</table>";
			}
			else
			{
				oss << QString("%1: %2<br/>").arg(q_("Position Angle of axis"), paAxisStr);
				oss << QString("%1: L<sub>%2e</sub>=%3 &phi;<sub>e</sub>: %4<br/>").arg(q_("Center point"),   lngSystem, subearthLStr, subearthBStr);
				oss << QString("%1: L<sub>%2s</sub>=%3 &phi;<sub>s</sub>: %4<br/>").arg(q_("Subsolar point"), lngSystem, subsolarLStr, subsolarBStr);
			}
		}

		if (englishName=="Sun")
		{
			// Only show during eclipse or transit, show percent?
			QPair<double, PlanetP> eclObj = ssystem->getEclipseFactor(core);
			const double eclipseObscuration = 100.*(1.-eclObj.first);
			if (eclipseObscuration>1.e-7) // needed to avoid false display of 1e-14 or so.
			{
				oss << QString("%1: %2%<br />").arg(q_("Eclipse obscuration")).arg(QString::number(eclipseObscuration, 'f', 2));
				PlanetP obj = eclObj.second;
				if (onEarth && obj == ssystem->getMoon())
				{
					const double eclipseMagnitude =
							(0.5 * angularSize
							 + (obj->getAngularSize(core) * M_PI_180) / obj->getInfoMap(core)["scale"].toDouble()
							- getJ2000EquatorialPos(core).angle(obj->getJ2000EquatorialPos(core)))
							/ angularSize;
					oss << QString("%1: %2<br />").arg(q_("Eclipse magnitude")).arg(QString::number(eclipseMagnitude, 'f', 3));
				}
			}

			if (onEarth)
			{
				// Solar eclipse information
				// Use geocentric coordinates
				StelCore* core1 = StelApp::getInstance().getCore();
				const bool useTopocentric = core1->getUseTopocentricCoordinates();
				core1->setUseTopocentricCoordinates(false);
				core1->update(0);

				double raSun, deSun, raMoon, deMoon;
				StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(core1));
				StelUtils::rectToSphe(&raMoon, &deMoon, ssystem->getMoon()->getEquinoxEquatorialPos(core1));

				double raDiff = StelUtils::fmodpos((raMoon - raSun)/M_PI_180, 360.0);
				if (raDiff < 3. || raDiff > 357.)
				{
					SolarEclipse center;
					Vec3d pos = center.point(core1);

					if (pos[0] < 90.) // only display when shadow axis is touching Earth
					{
						QString info = q_("Center of solar eclipse (Lat./Long.)");
						if (withDecimalDegree)
							oss << QString("%1: %2%3/%4%5<br />").arg(info).arg(pos[0], 5, 'f', 4).arg(QChar(0x00B0)).arg(pos[1], 5, 'f', 4).arg(QChar(0x00B0));
						else
							oss << QString("%1: %2/%3<br />").arg(info).arg(StelUtils::decDegToDmsStr(pos[0])).arg(StelUtils::decDegToDmsStr(pos[1]));
						StelLocation loc = core->getCurrentLocation();
						// distance between center point and current location
						double distance = loc.distanceKm(pos[1], pos[0]);
						double azimuth = loc.getAzimuthForLocation(pos[1], pos[0]);

						oss << QString("%1 %2 %3 %4%5")
								 .arg(q_("Shadow center point is"))
								 .arg(QString::number(distance, 'f', 1))
								 .arg(q_("km towards azimuth"))
								 .arg(QString::number(azimuth, 'f', 1))
								 .arg(QChar(0x00B0)) << "<br/>";
						oss << QString("%1: %2 ")
								 .arg(q_("Magnitude of central eclipse"))
								 .arg(QString::number(pos[2], 'f', 3));
						if (pos[2] < 1.0)
							oss << QString(qc_("(annular)","type of solar eclipse"));
						else
							oss << QString(qc_("(total)","type of solar eclipse"));
						oss << "<br/>";
					}
				}
				core1->setUseTopocentricCoordinates(useTopocentric);
				core1->update(0); // enforce update cache to avoid odd selection of Moon details!
			}
		}

		if (englishName == "Moon" && onEarth)
		{
			// Show magnitude of lunar eclipse
			// Use geocentric coordinates
			StelCore* core1 = StelApp::getInstance().getCore();
			const bool saveTopocentric = core1->getUseTopocentricCoordinates();
			core1->setUseTopocentricCoordinates(false);
			core1->update(0);

			double raMoon, deMoon, raSun, deSun;
			StelUtils::rectToSphe(&raMoon, &deMoon, getEquinoxEquatorialPos(core1));
			StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(core1));

			// R.A./Dec of Earth's shadow
			const double raShadow = StelUtils::fmodpos(raSun + M_PI, 2.*M_PI);
			const double deShadow = -(deSun);
			const double raDiff = StelUtils::fmodpos(raMoon - raShadow, 2.*M_PI);

			if (raDiff < 3.*M_PI_180 || raDiff > 357.*M_PI_180)
			{
				// Moon's semi-diameter
				const double mSD=atan(getEquatorialRadius()/eclipticPos.length()) * M_180_PI*3600.; // arcsec
				const QPair<Vec3d,Vec3d>shadowRadii=ssystem->getEarthShadowRadiiAtLunarDistance();
				const double f1 = shadowRadii.second[0]; // radius of penumbra at the distance of the Moon
				const double f2 = shadowRadii.first[0];  // radius of umbra at the distance of the Moon

				double x = cos(deMoon) * sin(raDiff);
				x *= 3600. * M_180_PI;
				double y = cos(deShadow) * sin(deMoon) - sin(deShadow) * cos(deMoon) * cos(raDiff);
				y *= 3600. * M_180_PI;
				const double m = sqrt(x * x + y * y); // distance between lunar centre and shadow centre
				const double L1 = f1 + mSD; // distance between center of the Moon and shadow at beginning and end of penumbral eclipse
				const double L2 = f2 + mSD; // distance between center of the Moon and shadow at beginning and end of partial eclipse
				const double pMag = (L1 - m) / (2. * mSD); // penumbral magnitude
				const double uMag = (L2 - m) / (2. * mSD); // umbral magnitude

				if (pMag > 1.e-3)
				{
					oss << QString("%1: %2%").arg(q_("Penumbral eclipse magnitude")).arg(QString::number(pMag*100., 'f', 1)) << "<br />";
					if (uMag > 1.e-3)
					{
						oss << QString("%1: %2%").arg(q_("Umbral eclipse magnitude")).arg(QString::number(uMag*100., 'f', 1)) << "<br />";
					}
				}
			}
			core1->setUseTopocentricCoordinates(saveTopocentric);
			core1->update(0); // enforce update cache to avoid odd selection of Moon details!
		}		

		// Not sure if albedo is at all interesting?
		if (englishName != "Sun")
			oss << QString("%1: %2<br/>").arg(q_("Albedo"), QString::number(getAlbedo(), 'f', 2));
	}
	return str;
}

QVariantMap Planet::getInfoMap(const StelCore *core) const
{
	static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
	PlanetP earth = ssystem->getEarth();
	const bool onEarth = (core->getCurrentPlanet()==earth);
	QVariantMap map = StelObject::getInfoMap(core);

	if (getEnglishName()!="Sun")
	{
		const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
		map.insert("distance", getJ2000EquatorialPos(core).length());
		float phase=getPhase(observerHelioPos);
		map.insert("phase", phase);
		map.insert("illumination", 100.f*phase);
		double phaseAngle = getPhaseAngle(observerHelioPos);
		map.insert("phase-angle", phaseAngle);
		map.insert("phase-angle-dms", StelUtils::radToDmsStr(phaseAngle));
		map.insert("phase-angle-deg", StelUtils::radToDecDegStr(phaseAngle));
		double elongation = getElongation(observerHelioPos);
		map.insert("elongation", elongation);
		map.insert("elongation-dms", StelUtils::radToDmsStr(elongation));
		map.insert("elongation-deg", StelUtils::radToDecDegStr(elongation));		
		map.insert("velocity", getEclipticVelocity().toString());
		map.insert("velocity-kms", QString::number(getEclipticVelocity().length()* AU/86400., 'f', 5));
		map.insert("heliocentric-velocity", getHeliocentricEclipticVelocity().toString());
		map.insert("heliocentric-velocity-kms", QString::number(getHeliocentricEclipticVelocity().length()* AU/86400., 'f', 5));
		map.insert("scale", sphereScale);		
		map.insert("albedo", getAlbedo());
	}
	else
	{
		SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
		QPair<double, PlanetP> eclObj = ssystem->getEclipseFactor(core);
		const double eclipseObscuration = 100.*(1.-eclObj.first);
		if (eclipseObscuration>1.e-7)
		{
			map.insert("eclipse-obscuration", eclipseObscuration);
			PlanetP obj = eclObj.second;
			if (core->getCurrentPlanet()==ssystem->getEarth() && obj==ssystem->getMoon())
			{
				double angularSize = 2.*getAngularSize(core)*M_PI_180;
				const double eclipseMagnitude = (0.5*angularSize + (obj->getAngularSize(core)*M_PI_180)/obj->getInfoMap(core)["scale"].toDouble() - getJ2000EquatorialPos(core).angle(obj->getJ2000EquatorialPos(core)))/angularSize;
				map.insert("eclipse-magnitude", eclipseMagnitude);
			}
			else
				map.insert("eclipse-magnitude", 0.0);
		}
		else
		{
			map.insert("eclipse-obscuration", 0.0);
			map.insert("eclipse-magnitude", 0.0);
		}
	}
	map.insert("type", getPlanetTypeString()); // replace existing "type=Planet" by something more detailed.

	if (onEarth && (getEnglishName()=="Moon"))
	{
		// Everything around libration:
		QPair<Vec4d, Vec3d>phys=getSubSolarObserverPoints(core);
		map.insert("libration_l", -phys.first[2]*M_180_PI); // longitude counted the other way!
		map.insert("libration_b", phys.first[1]*M_180_PI);
		map.insert("pa_axis", phys.first[3]*M_180_PI);
		map.insert("subsolar_l", -phys.second[2]*M_180_PI);
		map.insert("subsolar_b", phys.second[1]*M_180_PI);
		map.insert("colongitude", StelUtils::fmodpos(450.0+phys.second[2]*M_PI_180, 360.));
	}
	else if (onEarth && (getEnglishName()!="Sun"))
	{
		QPair<Vec4d, Vec3d>phys=getSubSolarObserverPoints(core);
		map.insert("central_l", phys.first[2]*M_180_PI);
		map.insert("central_b", phys.first[1]*M_180_PI);
		map.insert("pa_axis", phys.first[3]*M_180_PI);
		map.insert("subsolar_l", phys.second[2]*M_180_PI);
		map.insert("subsolar_b", phys.second[1]*M_180_PI);
	}
	return map;
}


//! Get sky label (sky translation)
QString Planet::getSkyLabel(const StelCore*) const
{
	QString str;
	QTextStream oss(&str);
	oss.setRealNumberPrecision(4);
	oss << getNameI18n();

	if (sphereScale != 1.)
	{
		oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
	}
	return str;
}

float Planet::getSelectPriority(const StelCore* core) const
{
	if( (static_cast<SolarSystem*>(StelApp::getInstance().getModuleMgr().getModule("SolarSystem")))->getFlagHints() )
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
	return (static_cast<SolarSystem*>(StelApp::getInstance().getModuleMgr().getModule("SolarSystem")))->getLabelsColor();
}


double Planet::getCloseViewFov(const StelCore* core) const
{
	return std::atan(equatorialRadius*sphereScale*2./getEquinoxEquatorialPos(core).length())*M_180_PI * 4.;
}

double Planet::getSatellitesFov(const StelCore* core) const
{
	// TODO: calculate from satellite orbits rather than hard code
	if (englishName=="Jupiter") return std::atan(0.005 /getEquinoxEquatorialPos(core).length())*M_180_PI * 4.;
	if (englishName=="Saturn")  return std::atan(0.005 /getEquinoxEquatorialPos(core).length())*M_180_PI * 4.;
	if (englishName=="Mars")    return std::atan(0.0001/getEquinoxEquatorialPos(core).length())*M_180_PI * 4.;
	if (englishName=="Uranus")  return std::atan(0.002 /getEquinoxEquatorialPos(core).length())*M_180_PI * 4.;
	return -1.;
}

double Planet::getParentSatellitesFov(const StelCore* core) const
{
	if (parent && parent->parent) return parent->getSatellitesFov(core);
	return -1.0;
}

// Set the rotational elements of the planet body.
void Planet::setRotationElements(const QString name,
				 const double _period, const double _offset, const double _epoch, const double _obliquity, const double _ascendingNode,
				 const double _ra0, const double _ra1, const double _de0, const double _de1, const double _w0, const double _w1)
{
	re.period = _period;
	re.offset = _offset;
	re.epoch = _epoch;
	re.obliquity = _obliquity;
	re.ascendingNode = _ascendingNode;
	re.method=(_ra0==0. ? RotationElements::Traditional : RotationElements::WGCCRE);
	re.ra0=_ra0;
	re.ra1=_ra1;
	re.de0=_de0;
	re.de1=_de1;
	re.W0=_w0;
	re.W1=_w1;

	// Assign fine-tuning corrective functions for axis rotation angle W and orientation.
	re.corrW  =RotationElements::axisRotCorrFuncMap.value(name, &RotationElements::corrWnil);
	re.corrOri=RotationElements::axisOriCorrFuncMap.value(name, &RotationElements::corrOriNil);
}

void Planet::setSiderealPeriod(const double siderealPeriod)
{
	this->siderealPeriod = siderealPeriod;
	if (orbitPtr && pType!=isObserver)
	{
		const double semiMajorAxis=static_cast<KeplerOrbit*>(orbitPtr)->getSemimajorAxis();
		const double eccentricity=static_cast<KeplerOrbit*>(orbitPtr)->getEccentricity();
		if (semiMajorAxis>0 && eccentricity<0.9)
		{
			//qDebug() << "Planet " << englishName << "replace siderealPeriod " << re.siderealPeriod << "by";
			this->siderealPeriod=static_cast<KeplerOrbit*>(orbitPtr)->calculateSiderealPeriod();
			//qDebug() << re.siderealPeriod;
			closeOrbit=true;
		}
		else
			closeOrbit=false;
	}
	deltaOrbitJDE = siderealPeriod/ORBIT_SEGMENTS;
}

Vec3d Planet::getJ2000EquatorialPos(const StelCore *core) const
{
	// A Planet's own eclipticPos is in VSOP87 ref. frame (practically equal to ecliptic of J2000 for us) coordinates relative to the parent body (sun, planet).
	// To get J2000 equatorial coordinates, we require heliocentric ecliptical positions (adding up parent positions) of observer and Planet.
	// Then we use the matrix rotation multiplication with an existing matrix in StelCore to orient from eclipticalJ2000 to equatorialJ2000.
	if (englishName=="Sun")
		return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(GETSTELMODULE(SolarSystem)->getLightTimeSunPosition() - core->getObserverHeliocentricEclipticPos());
	else
		return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(getHeliocentricEclipticPos() - core->getObserverHeliocentricEclipticPos());
}

// return value in radians!
// For Earth, this is epsilon_A, the angle between earth's rotational axis and pole of mean ecliptic of date.
// Details: e.g. Hilton etal, Report on Precession and the Ecliptic, Cel.Mech.Dyn.Astr.94:351-67 (2006), Fig1.
// For the other planets, it must be the angle between axis and Normal to the VSOP_J2000 coordinate frame.
// For moons, it may be the obliquity against its planet's equatorial plane.
// GZ: Note that such a scheme is highly confusing, and should be avoided. IAU models use the J2000 ICRF frame.
// TODO: It is unclear what other planets should deliver here.
//     In any case, re.obliquity could now be updated during computeTransMatrix()
double Planet::getRotObliquity(double JDE) const
{
	// JDay=2451545.0 for J2000.0
	if (englishName=="Earth")
		return getPrecessionAngleVondrakEpsilon(JDE);
	else
		return static_cast<double>(re.obliquity);
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
	double d = planetPos.length() / (p->getEquatorialRadius()/sunRadius+1);
	double penumbraRadius = (shadowDistance-d)/d*sunRadius;
	// TODO: Note that Earth's shadow should be enlarged a bit. (6-7% following Danjon?)
	
	double penumbraCenterToThisPlanetCenterDistance = (ppVector*shadowDistance-thisPos).length();
	
	if (penumbraCenterToThisPlanetCenterDistance<penumbraRadius+thisPlanet->getEquatorialRadius())
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
	
	for (const auto& planet : satellites)
	{
		if (willCastShadow(this, planet.data()))
			res.append(planet.data());
	}
	if (willCastShadow(this, parent.data()))
		res.append(parent.data());
	// Test satellites mutual occultations.
	if (parent.data() != sun)
	{
		for (const auto& planet : parent->satellites)
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
	if (fabs(lastJDE-dateJDE)>deltaJDE)
	{
		coordFunc(dateJDE, eclipticPos, eclipticVelocity, orbitPtr);
		lastJDE = dateJDE;
	}
}

// Compute the transformation matrix from the local Planet coordinate system to the parent Planet coordinate system.
// In case of the planets, this makes the axis point to their respective celestial poles.
// If only old-style rotational elements exist, we use the original algorithm (as of ~2010).
void Planet::computeTransMatrix(double JD, double JDE)
{
	//QString debugAid; // We have to collect all debug strings to keep some order in the output.

	// We have to call with both to correct this for earth with the new model.
	// For Earth, this is sidereal time for Greenwich, i.e. hour angle between meridian and First Point of Aries.
	// OLD: Return angle between ascending node of planet's equator and (J2000) ecliptic (?)
	// NEW: For the planets, this should return angle W between ascending node of the planet's equator with ICRF equator and the planet's zero meridian.
	re.currentAxisW=getSiderealTime(JD, JDE); // Store to later compute central meridian data etc.
	axisRotation = static_cast<float>(re.currentAxisW);

	// We can inject a proper precession plus even nutation matrix in this stage, if available.
	if (englishName=="Earth")
	{
		// rotLocalToParent = Mat4d::zrotation(re.ascendingNode - re.precessionRate*(jd-re.epoch)) * Mat4d::xrotation(-getRotObliquity(jd));
		// We follow Capitaine's (2003) formulation P=Rz(Chi_A)*Rx(-omega_A)*Rz(-psi_A)*Rx(eps_o). (Explan.Suppl. 2013, 6.28)
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
			// Note: The sign for zrotation(-deltaPsi) was suggested by email by German Marques 2020-05-28 who referred to the SOFA library also used in Stellarium Web. This is then also ExplanSup3rd, 6.41.
			Mat4d nut2000B=Mat4d::xrotation(eps_A) * Mat4d::zrotation(-deltaPsi)* Mat4d::xrotation(-eps_A-deltaEps); // eq.21 in Hilton et al. wrongly had a positive deltaPsi rotation.
			rotLocalToParent=rotLocalToParent*nut2000B;
		}
		return;
	}

	if (re.method==RotationElements::WGCCRE)
	{
		const double t=(JDE-J2000);
		const double T=t/36525.0;
		double J2000NPoleRA=re.ra0+re.ra1*T;
		double J2000NPoleDE=re.de0+re.de1*T;

		// Apply detailed corrections from ExplSup2013 and WGCCRE2009/WGCCRE2015.
		// Maybe later: With DE43x, get orientation from ephemeris lookup.
		re.corrOri(t, T, &J2000NPoleRA, &J2000NPoleDE);

		// keep for computation of central meridian etc.
		re.currentAxisRA=J2000NPoleRA;
		re.currentAxisDE=J2000NPoleDE;

		// The next call ONLY sets rotLocalToParent
		setRotEquatorialToVsop87(StelCore::matJ2000ToVsop87              // From VSOP87 into ICRS
					 * Mat4d::zrotation(J2000NPoleRA+M_PI_2) // rotate along ICRS EQUATOR to ascending node
					 * Mat4d::xrotation(M_PI_2-J2000NPoleDE) // node angle
					 );
		//debugAid=QString("Axis in ICRF: &alpha;: %1 &delta;: %2, W: %3<br/>").arg(StelUtils::radToDecDegStr(J2000NPoleRA), StelUtils::radToDecDegStr(J2000NPoleDE), QString::number(re.currentAxisW, 'f', 3));
	}
	else //	RotationElements::Traditional
	{
		// 0.21+: This used to be the old solution. Those axes were defined w.r.t. J2000 Ecliptic (VSOP87)
		// Also here, the preliminary version for Earth's precession was modelled, before the Vondrak2011 model which came in V0.14.
		// No other Planet had precessionRate defined, so it's safe to remove it here.
		//rotLocalToParent = Mat4d::zrotation(re.ascendingNode - re.precessionRate*(JDE-re.epoch)) * Mat4d::xrotation(re.obliquity);
		rotLocalToParent = Mat4d::zrotation(re.ascendingNode) * Mat4d::xrotation(re.obliquity);
		//debugAid=QString("Axis (OLDSTYLE): re.obliquity=%1, re.ascendingNode=%2, axisrotation=%3<br/>").arg(StelUtils::radToDecDegStr(re.obliquity), StelUtils::radToDecDegStr(re.ascendingNode), QString::number(axisRotation, 'f', 3));
	}
	//addToExtraInfoString(DebugAid, debugAid);
}

Mat4d Planet::getRotEquatorialToVsop87(void) const
{
	Mat4d rval = rotLocalToParent;
	if (re.method==RotationElements::Traditional)
	{
		if (parent)
		{
			for (PlanetP p=parent;p->parent;p=p->parent)
			{
				// The Sun is the ultimate parent. However, we don't want its matrix!
				if (p->pType!=isStar)
					rval = p->rotLocalToParent * rval;
			}
		}
	}
	return rval;
}

void Planet::setRotEquatorialToVsop87(const Mat4d &m)
{
	switch (re.method) {
		case RotationElements::Traditional:
		{
			Mat4d a = Mat4d::identity();
			if (parent)
			{
				for (PlanetP p=parent;p->parent;p=p->parent)
				{
					// The Sun is the ultimate parent. However, we don't want its matrix!
					if (p->pType!=isStar)
					{
						addToExtraInfoString(DebugAid, QString("This involves localToParent of %1 <br/>").arg(p->englishName));
						a = p->rotLocalToParent * a;
					}
				}
			}
			rotLocalToParent = a.transpose() * m;
		}
			break;
		case RotationElements::WGCCRE:
			rotLocalToParent = m;
			break;
	}
}


// Compute the axial z rotation (daily rotation around the polar axis) [degrees] to use from equatorial to hour angle based coordinates.
// On Earth, sidereal time on the other hand is the angle along the planet equator from RA0 to the meridian, i.e. hour angle of the first point of Aries.
// For Earth (of course) it is sidereal time at Greenwich.
// V0.21+ update:
// For planets and Moons, in this context this is the rotation angle W of the Prime meridian from the ascending node of the planet equator on the ICRF equator.
// The usual WGCCRE model is W=W0+d*W1. Some planets/moons have more complicated rotations though, these are also handled in here.
// The planet objects with old-style data are computed like in earlier versions of Stellarium. Their computational model is however questionable.
// We need both JD and JDE here for Earth. (For other planets only JDE.)
double Planet::getSiderealTime(double JD, double JDE) const
{
	if (englishName=="Earth")
	{	// Check to make sure that nutation is just those few arcseconds.
		if (StelApp::getInstance().getCore()->getUseNutation())
			return get_apparent_sidereal_time(JD, JDE); // degrees
		else
			return get_mean_sidereal_time(JD, JDE); // degrees
	}

	// V0.21+: new rotational values from ExplSup2013 or WGCCRE2009/2015.
	if (re.method==RotationElements::WGCCRE)
	{
		// This returns angle W, the longitude of the prime meridian measured along the planet equator
		// from the ascending node (intersection) of the planet equator with the ICRF equator.
		const double t=JDE-J2000;
		const double T=t/36525.0;
		double w=re.W0+remainder(t*re.W1, 360.); // W is given and also returned in degrees, clamped to small angles so that adding small corrections makes sense.
		w+=re.corrW(t, T); // Apply the bespoke corrections from Explanatory Supplement 2013/WGCCRE2009/WGCCRE2015.
		return w;
	}

	// OLD MODEL, BEFORE V0.21
	// This is still used for a few solar system objects where we don't have modern elements from the WGCCRE.

	const double t = JDE - re.epoch;
	// avoid division by zero (typical case for moons with chaotic period of rotation)
	double rotations = (re.period==0. ? 1.  // moon with chaotic period of rotation
					  : t / static_cast<double>(re.period));
	rotations = remainder(rotations, 1.0); // remove full rotations to limit angle.
	return rotations * 360. + static_cast<double>(re.offset);
}

// Get duration of mean solar day (in earth days)
double Planet::getMeanSolarDay() const
{
	double msd = 0.;

	if (englishName=="Sun")
	{
		// A mean solar day (equals to Earth's day) has been added here for educational purposes
		// Details: https://sourceforge.net/p/stellarium/discussion/278769/thread/fbe282db/
		return 1.;
	}

	const double sday = getSiderealDay();
	const double coeff = qAbs(sday/getSiderealPeriod());
	double sign = 1.;
	// planets with retrograde rotation
	if (englishName=="Venus" || englishName=="Uranus" || englishName=="Pluto")
		sign = -1.;

	if (pType==Planet::isMoon)
	{
		// duration of mean solar day on moon are same as synodic month on this moon
		const double a = parent->getSiderealPeriod()/sday;
		msd = sday*(a/(a-1));
	}
	else
		msd = sign*sday/(1 - sign*coeff);

	return msd;
}

// Get the Planet position in Cartesian ecliptic (J2000) coordinates in AU, centered on the parent Planet
Vec3d Planet::getEclipticPos(double dateJDE) const
{
	// Use current position if the time match.
	if (fuzzyEquals(dateJDE, lastJDE))
		return eclipticPos;

	// Otherwise try to use a cached position.
	Vec3d *pos = positionsCache[dateJDE];
	if (!pos)
	{
		pos = new Vec3d;
		Vec3d velocity;
		coordFunc(dateJDE, *pos, velocity, orbitPtr);
		positionsCache.insert(dateJDE, pos);
	}
	return *pos;
}

// Return heliocentric ecliptical coordinate of p [AU]
Vec3d Planet::getHeliocentricPos(Vec3d p) const
{
	// Note: using shared copies is too slow here.  So we use direct access instead.
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

Vec3d Planet::getHeliocentricEclipticPos(double dateJDE) const
{
	Vec3d pos = getEclipticPos(dateJDE);
	const Planet* pp = parent.data();
	if (pp)
	{
		while (pp->parent.data())
		{
			pos += pp->getEclipticPos(dateJDE);
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
	// Note: using shared copies is too slow here.  So we use direct access instead.
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

// Get the planet phase ([0..1] illuminated fraction of the planet disk) for an observer at pos obsPos in heliocentric coordinates (in AU)
float Planet::getPhase(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*std::sqrt(observerPlanetRq*planetRq));
	return 0.5f * static_cast<float>(qAbs(1. + cos_chi));
}

float Planet::getPAsun(const Vec3d &sunPos, const Vec3d &objPos)
{
	float ra0, de0, ra, de, dra;
	StelUtils::rectToSphe(&ra0, &de0, sunPos);
	StelUtils::rectToSphe(&ra, &de, objPos);
	dra=ra0-ra;
	return atan2f(cos(de0)*sin(dra), sin(de0)*cos(de) - cos(de0)*sin(de)*cos(dra));
}


// Get planetographic coordinates of subsolar and sub-observer points.
// Source: Explanatory Supplement 2013, 10.4.1
// Erroneous expression 10.27 fixed by Explan. Suppl. 1992, 7.12-26.
QPair<Vec4d, Vec3d> Planet::getSubSolarObserverPoints(const StelCore *core) const
{
//	QString debugAid;
	QPair<Vec4d, Vec3d>ret;
	// In this case Precession/Nutation matrix has to be built as written in the books, not as made in other places in the program
	double eps_A, chi_A, omega_A, psi_A;
	getPrecessionAnglesVondrak(core->getJDE(), &eps_A, &chi_A, &omega_A, &psi_A);
	// Standard formulation from Explanatory Supplement 2013, 6.28.
	// NOTE: For higher accuracy, there may be need for adding a Frame Bias rotation, but this should influence results by sub-arseconds.
	Mat4d PrecNut= Mat4d::zrotation(chi_A)*Mat4d::xrotation(-omega_A)*Mat4d::zrotation(-psi_A)*Mat4d::xrotation(EPS_0*M_PI_180);
	if (core->getUseNutation())
	{
		double deltaEps, deltaPsi;
		getNutationAngles(core->getJDE(), &deltaPsi, &deltaEps);
		Mat4d nut2000B=Mat4d::xrotation(-eps_A-deltaEps) * Mat4d::zrotation(-deltaPsi) * Mat4d::xrotation(eps_A);
		PrecNut = nut2000B*PrecNut;
	}

	const double f=1.-oneMinusOblateness; // flattening term
	const double fTerm=1-f*f;
	// When using the last computed elements, light time should already be accounted for.
	const Vec3d r  = PrecNut*StelCore::matVsop87ToJ2000*getHeliocentricEclipticPos();
	const Vec3d r_e= PrecNut*StelCore::matVsop87ToJ2000*core->getCurrentPlanet()->getHeliocentricEclipticPos();
	const Vec3d Dr= r-r_e; // should be regular vector resembling RA/DE of object in rectangular equatorial PrecNut*(J2000) coords.
//	// verify this assumption...
//	double ra, de;
//	StelUtils::rectToSphe(&ra, &de, r); ra=StelUtils::fmodpos(ra, 2.*M_PI);
//	debugAid.append(QString("r: &alpha;=%1=%2, &delta;=%3=%4 <br/>").arg(
//				StelUtils::radToDecDegStr(ra), StelUtils::radToHmsStr(ra),
//				StelUtils::radToDecDegStr(de), StelUtils::radToDmsStr(de)));
//	StelUtils::rectToSphe(&ra, &de, r_e); ra=StelUtils::fmodpos(ra, 2.*M_PI);
//	debugAid.append(QString("r<sub>e</sub>: &alpha;=%1=%2, &delta;=%3=%4 <br/>").arg(
//				StelUtils::radToDecDegStr(ra), StelUtils::radToHmsStr(ra),
//				StelUtils::radToDecDegStr(de), StelUtils::radToDmsStr(de)));
//	StelUtils::rectToSphe(&ra, &de, Dr); ra=StelUtils::fmodpos(ra, 2.*M_PI);
//	debugAid.append(QString("&Delta;r: &alpha;=%1=%2, &delta;=%3=%4 <br/>").arg(
//				StelUtils::radToDecDegStr(ra), StelUtils::radToHmsStr(ra),
//				StelUtils::radToDecDegStr(de), StelUtils::radToDmsStr(de)));

	Vec3d s=-r;  s.normalize();
	Vec3d e=-Dr; e.normalize();
	const double sina0=sin(re.currentAxisRA);
	const double cosa0=cos(re.currentAxisRA);
	const double sind0=sin(re.currentAxisDE);
	const double cosd0=cos(re.currentAxisDE);
	// sub-earth point (10.19)
	Vec3d n=PrecNut*Vec3d(cosd0*cosa0, cosd0*sina0, sind0);
	// Rotation W is OK for all planets except Jupiter: return simple W_II to remove GRS adaptation shift.
	const double W= (englishName=="Jupiter" ?
			re.W0+ remainder( (core->getJDE()-J2000 - Dr.length()*(AU/(SPEED_OF_LIGHT*86400.)))*re.W1, 360.) :
			re.currentAxisW);
	const double sinW=sin(W*M_PI_180);
	const double sindw=sinW*cosd0;
	const double cosdw=cos(asin(sindw));
	const double sinpsi=sinW*sind0/cosdw;
	const double cospsi=cos(W*M_PI_180)/cosdw;
	const double psi=atan2(sinpsi, cospsi);
	const double aw=re.currentAxisRA+M_PI_2+psi;
	const Vec3d w=PrecNut*Vec3d(cosdw*cos(aw), cosdw*sin(aw), sindw);
	const Vec3d y=w^n;
	const Vec3d subEarth(e.dot(w), e.dot(y), e.dot(n)); // 10.25
	const double phi_e=asin(subEarth[2]);               // 10.26
	const double phiP_e=atan(tan(phi_e)/fTerm);
	double lambdaP_e=StelUtils::fmodpos(atan2(subEarth[1], subEarth[0]), 2.0*M_PI);
	if (re.W1<0) lambdaP_e=2.*M_PI-lambdaP_e;

	// PA of axis: 10.29 with elements of P from 10.28, but fixed error in Explan.Sup.2013 with Explan.Sup.1992!
	const Vec3d P(n.dot(e), n.dot(e^Vec3d(0., 0., 1.)), n.dot((e^Vec3d(0., 0., 1.))^e));

	ret.first.set(phi_e, phiP_e, lambdaP_e, StelUtils::fmodpos(atan(P.v[1]/ P.v[2]), 2.*M_PI));

	// Subsolar point:
	const Vec3d subSol(s.dot(w), s.dot(y), s.dot(n));
	const double phi_s=asin(subSol[2]);
	const double phiP_s=atan2(tan(phi_s), fTerm);
	double lambdaP_s=StelUtils::fmodpos(atan2(subSol[1], subSol[0]), 2.0*M_PI);
	if (re.W1<0) lambdaP_s=2.*M_PI-lambdaP_s;

	ret.second.set(phi_s, phiP_s, lambdaP_s);

//	debugAid.append(QString("&phi;<sub>e</sub>: %1, &phi;'<sub>e</sub>: %2, &lambda;<sub>e</sub>: %3, PA<sub>n</sub>: %4<br/>").arg(
//			StelUtils::radToDecDegStr(ret.first[0]),
//			StelUtils::radToDecDegStr(ret.first[1]),
//			StelUtils::radToDecDegStr(StelUtils::fmodpos(ret.first[2], 2.*M_PI)),
//			StelUtils::radToDecDegStr(StelUtils::fmodpos(ret.first[3], 2.*M_PI))
//		       ));
//	debugAid.append(QString("&phi;<sub>s</sub>: %1, &phi;'<sub>s</sub>: %2, &lambda;<sub>s</sub>: %3<br/>").arg(
//			StelUtils::radToDecDegStr(ret.second[0]),
//			StelUtils::radToDecDegStr(ret.second[1]),
//			StelUtils::radToDecDegStr(StelUtils::fmodpos(ret.second[2], 2.*M_PI))
//		       ));
//
//	StelObjectMgr& objMgr = StelApp::getInstance().getStelObjectMgr();
//		if (objMgr.getSelectedObject().length()>0)
//			objMgr.getSelectedObject()[0]->addToExtraInfoString(StelObject::DebugAid, debugAid);
	return ret;
}


// Get the elongation angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getElongation(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	return std::acos((observerPlanetRq  + observerRq - planetRq)/(2.0*std::sqrt(observerPlanetRq*observerRq)));
}

// Source: Explanatory Supplement 2013, Table 10.6 and formula (10.5) with semimajorAxis a from Table 8.7.
float Planet::getMeanOppositionMagnitude() const
{
	if (absoluteMagnitude<=-99.f)
		return 100.f;

	static const QMap<QString, float>momagMap = {
		{ "Sun",    100.f},
		{ "Moon",   -12.74f},
		{ "Mars",    -2.01f},
		{ "Jupiter", -2.7f},
		{ "Saturn",   0.67f},
		{ "Uranus",   5.52f},
		{ "Neptune",  7.84f},
		{ "Pluto",   15.12f},
		{ "Io",       5.02f},
		{ "Europa",   5.29f},
		{ "Ganymede", 4.61f},
		{ "Callisto", 5.65f}};
	if (momagMap.contains(englishName))
		return momagMap.value(englishName);

	static const QMap<QString, double>smaMap = {
		{ "Mars",     1.52371034 },
		{ "Jupiter",  5.202887   },
		{ "Saturn",   9.53667594 },
		{ "Uranus",  19.18916464 },
		{ "Neptune", 30.06992276 },
		{ "Pluto",   39.48211675 }};
	double semimajorAxis=smaMap.value(parent->englishName, 0.);
	if (pType>= isAsteroid)
	{
		Q_ASSERT(orbitPtr);
		if (orbitPtr)
			semimajorAxis=static_cast<KeplerOrbit*>(orbitPtr)->getSemimajorAxis();
		else
			qDebug() << "WARNING: No orbitPtr for " << englishName;
	}

	if (semimajorAxis>0.)
		return absoluteMagnitude+5.f*static_cast<float>(log10(semimajorAxis*(semimajorAxis-1.)));

	return 100.;
}

// Computation of the visual magnitude (V band) of the planet.
float Planet::getVMagnitude(const StelCore* core) const
{
	if (parent == Q_NULLPTR)
	{
		// Sun, compute the apparent magnitude for the absolute mag (V: 4.83) and observer's distance
		// Hint: Absolute Magnitude of the Sun in Several Bands: http://mips.as.arizona.edu/~cnaw/sun.html
		const double distParsec = std::sqrt(core->getObserverHeliocentricEclipticPos().lengthSquared())*AU/PARSEC;

		// check how much of it is visible
		const SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		const double shadowFactor = qMax(0.000128, ssm->getEclipseFactor(core).first);
		// See: Hughes, D. W., Brightness during a solar eclipse // Journal of the British Astronomical Association, vol.110, no.4, p.203-205
		// URL: http://adsabs.harvard.edu/abs/2000JBAA..110..203H

		return static_cast<float>(4.83 + 5.*(std::log10(distParsec)-1.) - 2.5*(std::log10(shadowFactor)));
	}

	// Compute the phase angle i. We need the intermediate results also below, therefore we don't just call getPhaseAngle.
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const double observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const double dr = std::sqrt(observerPlanetRq*planetRq);
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*dr);
	const double phaseAngle = std::acos(cos_chi);

	double shadowFactor = 1.;
	// Check if the satellite is inside the inner shadow of the parent planet:
	if (parent->parent != Q_NULLPTR)
	{
		const Vec3d& parentHeliopos = parent->getHeliocentricEclipticPos();
		const double parent_Rq = parentHeliopos.lengthSquared();
		const double pos_times_parent_pos = planetHelioPos * parentHeliopos;
		if (pos_times_parent_pos > parent_Rq)
		{
			// The satellite is farther away from the sun than the parent planet.
			if (englishName=="Moon")
			{
				static const double totalityFactor=2.710e-5; // defined previously by AW
				const QPair<Vec3d,Vec3d>shadowRadii=GETSTELMODULE(SolarSystem)->getEarthShadowRadiiAtLunarDistance();
				const double dist=getEclipticPos().length();  // Lunar distance [AU]
				const double u=shadowRadii.first[0]  / 3600.; // geocentric angle of earth umbra radius at lunar distance [degrees]
				const double p=shadowRadii.second[0] / 3600.; // geocentric angle of earth penumbra radius at lunar distance [degrees]
				const double r=atan(getEquatorialRadius()/dist) * M_180_PI; // geocentric angle of Lunar radius at lunar distance [degrees]
				const double od=180.-getElongation(GETSTELMODULE(SolarSystem)->getEarth()->getEclipticPos()) * (180.0/M_PI); // opposition distance [degrees]
				if (od>p+r) shadowFactor=1.0;
				else if (od>u+r) // penumbral transition zone: gradual decline (square curve)
					shadowFactor=0.6+0.4*sqrt((od-u-r)/(p-u));
				else if (od>u-r) // umbral transition zone
					shadowFactor=totalityFactor+(0.6-totalityFactor)*(od-u+r)/(2.*r);
				else // totality. Still, center is darker...
				{
					// Fit a more realistic magnitude for the Moon case.
					// I used some empirical data for fitting. --AW
					// TODO: This factor should be improved!
					shadowFactor=totalityFactor*0.5*(1+od/(u-r));
				}
			}
			else
			{
				const double sun_radius = parent->parent->equatorialRadius;
				const double sun_minus_parent_radius = sun_radius - parent->equatorialRadius;
				const double quot = pos_times_parent_pos/parent_Rq;

				// Compute d = distance from satellite center to border of inner shadow.
				// d>0 means inside the shadow cone.
				double d = sun_radius - sun_minus_parent_radius*quot - std::sqrt((1.-sun_minus_parent_radius/std::sqrt(parent_Rq)) * (planetRq-pos_times_parent_pos*quot));
				if (d>=equatorialRadius)
				{
					// The satellite is totally inside the inner shadow.
					shadowFactor = 1e-9;
				}
				else if (d>-equatorialRadius)
				{
					// The satellite is partly inside the inner shadow,
					// compute a fantasy value for the magnitude:
					d /= equatorialRadius;
					shadowFactor = (0.5 - (std::asin(d)+d*std::sqrt(1.0-d*d))/M_PI);
				}
			}
		}
	}

	// Lunar Magnitude from Earth: This is a combination of Russell 1916 (!) with its albedo dysbalance, Krisciunas-Schaefer (1991) for the opposition surge, and Agrawal (2016) for the contribution of earthshine.
	if ((core->getCurrentLocation().planetName=="Earth") && (englishName=="Moon"))
	{
		double lEarth, bEarth, lMoon, bMoon;
		StelUtils::rectToSphe(&lEarth, &bEarth, observerHelioPos);
		StelUtils::rectToSphe(&lMoon, &bMoon, eclipticPos);
		double dLong=StelUtils::fmodpos(lMoon-lEarth, 2.*M_PI); if (dLong>M_PI) dLong-=2.*M_PI; // now dLong<0 for waxing phases.
		const double p=dLong*M_180_PI;
		// main magnitude term from Russell 1916. Polynomes from Excel fitting with mag(dLong=180)=0.
		// Measurements support only dLong -150...150, and the New Moon area is mere guesswork.
		double magIll=(p<0 ?
				(((((4.208547E-12*p + 1.754857E-09)*p + 2.749700E-07)*p + 1.860811E-05)*p + 5.590310E-04)*p - 1.628691E-02)*p + 4.807056E-03 :
				(((((4.609790E-12*p - 1.977692E-09)*p + 3.305454E-07)*p - 2.582825E-05)*p + 9.593360E-04)*p + 1.213761E-02)*p + 7.710015E-03);
		magIll-=12.73;
		static const double rf=2.56e-6; // Reference flux [lx] from Agrawal (14)
		double fluxIll=rf*pow(10., -0.4*magIll);

		// apply opposition surge where needed
		const double psi=getPhaseAngle(observerHelioPos);
		const double surge=qMax(1., 1.35-2.865*abs(psi));
		fluxIll *= surge; // This is now shape of Russell's magnitude curve with peak brightness matched with Krisciunas-Schaefer
		// apply distance factor
		static const double lunarMeanDist=384399./AU;
		static const double lunarMeanDistSq=lunarMeanDist*lunarMeanDist;
		fluxIll *= (lunarMeanDistSq/observerPlanetRq);

		// compute flux of ashen light: Agrawal 2016.
		const double beta=parent->equatorialRadius*parent->equatorialRadius/eclipticPos.lengthSquared();
		const double gamma=equatorialRadius*equatorialRadius/eclipticPos.lengthSquared();

		const double slfoe=133100.; // https://www.allthingslighting.org/index.php/2019/02/15/solar-illumination/
		const double LumEarth=slfoe * static_cast<double>(core->getCurrentObserver()->getHomePlanet()->albedo);
		const double elfom=LumEarth*beta;
		const double elfoe=elfom*static_cast<double>(albedo)*gamma; // brightness of full earthshine.
		const double pfac=1.-(0.5*(1.+cos(dLong))); // diminishing earthshine with phase angle
		const double fluxTotal=fluxIll + elfoe*pfac;
		return -2.5f*static_cast<float>(log10(fluxTotal*shadowFactor/rf));
	}

	// Use empirical formulae for main planets when seen from earth. MallamaHilton_2018 also work from other locations.
	if ((Planet::getApparentMagnitudeAlgorithm()==MallamaHilton_2018) || (core->getCurrentLocation().planetName=="Earth"))
	{
		const double phaseDeg=phaseAngle*M_180_PI;
		const double d = 5. * log10(dr);

		// There are several solutions:
		// (0) "ExplanatorySupplement_1992" original solution in Stellarium, present around 2010.
		// (1) "Mueller_1893" G. Müller, based on visual observations 1877-91. [Expl.Suppl.1961 p.312ff]
		// (2) "AstronomicalAlmanac_1984" Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes.
		//     The structure is almost identical, just the numbers are different!
		//     Note that calling (2) "Harris" is an absolute misnomer. Meeus clearly describes this in AstrAlg1998 p.286.
		// (3) "ExplanatorySupplement_2013" More modern.
		// (4) "MallamaHilton_2018" seems the best available. Mercury-Neptune. Pluto and Jovian moons copied from (3).
		switch (Planet::getApparentMagnitudeAlgorithm())
		{
			case UndefinedAlgorithm:	// The most recent solution should be activated by default
			case MallamaHilton_2018:
			{
				if (englishName=="Mercury")
					return static_cast<float>(-0.613 + d + ((((((-3.0334e-12*phaseDeg + 1.6893e-9)*phaseDeg -3.4265e-7)*phaseDeg) + 3.3644e-5)*phaseDeg - 1.6336e-3)*phaseDeg + 6.3280e-2)*phaseDeg);
				if (englishName=="Venus")
				{
					if (phaseDeg<=163.7)
						return static_cast<float>(-4.384 + d + (((8.938e-9*phaseDeg - 2.814e-6)*phaseDeg + 3.687e-4)*phaseDeg - 1.044e-3)*phaseDeg);
					else
						return static_cast<float>(236.05828 + d + (8.39034e-3*phaseDeg - 2.81914)*phaseDeg);
				}
				if (englishName=="Earth")
					return static_cast<float>(-3.99 + d + ((2.054e-4*phaseDeg - 1.060e-3)*phaseDeg));
				if (englishName=="Mars")
				{
					double V=d;
					const QPair<Vec4d,Vec3d>axis=getSubSolarObserverPoints(core);
					V+=re.getMarsMagLs(0.5*(axis.first[2]+axis.second[2]), true); // albedo effect
					Q_ASSERT(abs(re.getMarsMagLs(0.5*(axis.first[2]+axis.second[2]), true)) < 0.2);
					// determine orbital longitude
					const Vec3d pos=getHeliocentricEclipticPos();
					double lng, lat;
					StelUtils::rectToSphe(&lng, &lat, pos);
					const double orbLong=StelUtils::fmodpos(lng-getRotAscendingNode(), 2.*M_PI);
					V+=re.getMarsMagLs(orbLong, false); // Orbital Longitude effect
					Q_ASSERT(abs(re.getMarsMagLs(orbLong, false)) < 0.1 );
					if(phaseDeg<=50)
						V += (-0.0001302*phaseDeg + 0.02267)*phaseDeg -1.601;
					else
						V += ( 0.0003445*phaseDeg - 0.02573)*phaseDeg -0.367;
					return static_cast<float>(V);
				}
				if (englishName=="Jupiter")
				{
					if (phaseDeg<=12)
						return static_cast<float>(-9.395 + d + (6.16e-4*phaseDeg -3.7e-4)*phaseDeg);
					else {
						const double p= phaseDeg/180.;
						const double bracket= 1.0 - (((((-1.876*p + 2.809)*p - 0.062)*p -0.363)*p -1.507)*p);
						return static_cast<float>(-9.428 + d - 2.5*log10(bracket));
					}
				}
				if (englishName=="Saturn")
				{
					if (phaseDeg<6.5)
					{
						// Note: this is really only for phaseAngle<6, i.e. from Earth.
						// add rings computation
						const QPair<Vec4d,Vec3d>axis=getSubSolarObserverPoints(core);
						const double be=axis.first[0];
						const double bs=axis.second[0];
						double beta= be*bs; beta=(beta<=0 ? 0. : sqrt(beta));
						return static_cast<float>(-8.914 + d + 0.026*phaseDeg - (1.825+0.378*exp(-2.25*phaseDeg))*sin(beta) );
					}
					else
					{
						// Expression (12). This gives magV for the globe only, no ring.
						return static_cast<float>(-8.94+d+(((4.767e-9*phaseDeg-1.505e-6)*phaseDeg + 2.672e-4)*phaseDeg + 2.446e-4)*phaseDeg);
					}
				}
				if (englishName=="Uranus")
				{
					const QPair<Vec4d,Vec3d>axis=getSubSolarObserverPoints(core);
					const double phiP=0.5*M_180_PI*(abs(axis.first[1])+abs(axis.second[1]));

					return static_cast<float>(-7.110 + d - 8.4e-4*phiP + (1.045e-4*phaseDeg+6.587e-3)*phaseDeg );
				}
				if (englishName=="Neptune")
				{
					int yy, mm, dd;
					StelUtils::getDateFromJulianDay(core->getJD(), &yy, &mm, &dd);
					const double t=StelUtils::yearFraction(yy, mm, dd);
					double V=d-6.89;
					if ((1980.0<=t) && (t<=2000.0))
						V-=0.0054*(t-1980.);
					else if (t>2000.0)
						V-=0.11;

					return static_cast<float>(V +(9.617e-5*phaseDeg +7.944e-3)*phaseDeg);
				}
				if (englishName=="Pluto")
					return static_cast<float>(-1.01 + d);

				// AW 2017: I've added special case for Jupiter's moons when they are in the shadow of Jupiter.
				// TODO: Need experimental data to fitting to real world or the scientific paper with description of model.
				// GZ 2017-09: Phase coefficients for I and III corrected, based on original publication (Stebbins&Jacobsen 1928) now.
				// AW 2020-02: Let's use linear model in the first approximation for smooth reduce the brightness of Jovian moons for get more realistic look
				if (core->getCurrentLocation().planetName=="Earth") // phase angle corrections only work for the small phase angles visible on earth.
				{
					if (englishName=="Io")
					{
						const float mag = static_cast<float>(-1.68 + d + phaseDeg*(0.046  - 0.0010 *phaseDeg));
						return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
					}
					if (englishName=="Europa")
					{
						const float mag = static_cast<float>(-1.41 + d + phaseDeg*(0.0312 - 0.00125*phaseDeg));
						return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
					}
					if (englishName=="Ganymede")
					{
						const float mag = static_cast<float>(-2.09 + d + phaseDeg*(0.0323 - 0.00066*phaseDeg));
						return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
					}
					if (englishName=="Callisto")
					{
						const float mag = static_cast<float>(-1.05 + d + phaseDeg*(0.078  - 0.00274*phaseDeg));
						return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
					}
					if ((!fuzzyEquals(absoluteMagnitude,-99.f)) && (englishName!="Moon"))
						return absoluteMagnitude+static_cast<float>(d);
				}
				break;
			}

			case ExplanatorySupplement_2013:
			{
				// GZ2017: This is taken straight from the Explanatory Supplement to the Astronomical Ephemeris 2013 (chap. 10.3)
				// AW2017: Updated data from Errata in The Explanatory Supplement to the Astronomical Almanac (3rd edition, 1st printing)
				//         http://aa.usno.navy.mil/publications/docs/exp_supp_errata.pdf (Last update: 1 December 2016)
				if (englishName=="Mercury")
					return static_cast<float>(-0.6 + d + (((3.02e-6*phaseDeg - 0.000488)*phaseDeg + 0.0498)*phaseDeg));
				if (englishName=="Venus")
				{
					// there are two regions strongly enclosed per phaseDeg (2.2..163.6..170.2). However, we must deliver a solution for every case.
					// GZ: The model seems flawed. See https://sourceforge.net/p/stellarium/discussion/278769/thread/b7cab45f62/?limit=25#907d
					// In this case, it seems better to deviate from the paper and --- only for the inferior conjunction --
					// use a more modern value from Mallama&Hilton, https://doi.org/10.1016/j.ascom.2018.08.002
					// The reversal and intermediate peak is real and due to forward scattering on sulphur acide droplets.
					if (phaseDeg<163.6)
						return static_cast<float>(-4.47 + d + ((0.13e-6*phaseDeg + 0.000057)*phaseDeg + 0.0103)*phaseDeg);
					else
						return static_cast<float>(236.05828 + d - 2.81914*phaseDeg + 8.39034E-3*phaseDeg*phaseDeg);
				}
				if (englishName=="Earth")
					return static_cast<float>(-3.87 + d + (((0.48e-6*phaseDeg + 0.000019)*phaseDeg + 0.0130)*phaseDeg));
				if (englishName=="Mars")
					return static_cast<float>(-1.52 + d + 0.016*phaseDeg);
				if (englishName=="Jupiter")
					return static_cast<float>(-9.40 + d + 0.005*phaseDeg);
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
					const double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					const double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinx=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					const double ringsIllum = -2.6*fabs(sinx) + 1.25*sinx*sinx; // ExplSup2013: added term as (10.81)
					return static_cast<float>(-8.88 + d + 0.044*phaseDeg + ringsIllum);
				}
				if (englishName=="Uranus")
					return static_cast<float>(-7.19 + d + 0.002*phaseDeg);
				if (englishName=="Neptune")
					return static_cast<float>(-6.87 + d);
				if (englishName=="Pluto")
					return static_cast<float>(-1.01 + d);

				// AW 2017: I've added special case for Jupiter's moons when they are in the shadow of Jupiter.
				// TODO: Need experimental data to fitting to real world or the scientific paper with description of model.
				// GZ 2017-09: Phase coefficients for I and III corrected, based on original publication (Stebbins&Jacobsen 1928) now.
				// AW 2020-02: Let's use linear model in the first approximation for smooth reduce the brightness of Jovian moons for get more realistic look
				if (englishName=="Io")
				{
					const float mag = static_cast<float>(-1.68 + d + phaseDeg*(0.046  - 0.0010 *phaseDeg));
					return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
				}
				if (englishName=="Europa")
				{
					const float mag = static_cast<float>(-1.41 + d + phaseDeg*(0.0312 - 0.00125*phaseDeg));
					return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
				}
				if (englishName=="Ganymede")
				{
					const float mag = static_cast<float>(-2.09 + d + phaseDeg*(0.0323 - 0.00066*phaseDeg));
					return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
				}
				if (englishName=="Callisto")
				{
					const float mag = static_cast<float>(-1.05 + d + phaseDeg*(0.078  - 0.00274*phaseDeg));
					return shadowFactor<1.0 ? static_cast<float>(13.*(1.-shadowFactor)) + mag : mag;
				}

				if ((!fuzzyEquals(absoluteMagnitude,-99.f)) && (englishName!="Moon"))
					return absoluteMagnitude+static_cast<float>(d);

				break;
			}
			case ExplanatorySupplement_1992:
			{
				// Algorithm contributed by Pere Planesas (Observatorio Astronomico Nacional)
				// GZ2016: Actually, this is taken straight from the Explanatory Supplement to the Astronomical Ephemeris 1992! (chap. 7.12)
				// The value -8.88 for Saturn V(1,0) seems to be a correction of a typo, where Suppl.Astr. gives -7.19 just like for Uranus.
				double f1 = phaseDeg/100.;

				if (englishName=="Mercury")
				{
					if ( phaseDeg > 150. ) f1 = 1.5;
					return static_cast<float>(-0.36 + d + 3.8*f1 - 2.73*f1*f1 + 2*f1*f1*f1);
				}
				if (englishName=="Venus")
					return static_cast<float>(-4.29 + d + 0.09*f1 + 2.39*f1*f1 - 0.65*f1*f1*f1);
				if (englishName=="Mars")
					return static_cast<float>(-1.52 + d + 0.016*phaseDeg);
				if (englishName=="Jupiter")
					return static_cast<float>(-9.25 + d + 0.005*phaseDeg);
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
					const double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					const double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinx=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					const double ringsIllum = -2.6*fabs(sinx) + 1.25*sinx*sinx;
					return static_cast<float>(-8.88 + d + 0.044*phaseDeg + ringsIllum);
				}
				if (englishName=="Uranus")
					return static_cast<float>(-7.19 + d + 0.0028*phaseDeg);
				if (englishName=="Neptune")
					return static_cast<float>(-6.87 + d);
				if (englishName=="Pluto")
					return static_cast<float>(-1.01 + d + 0.041*phaseDeg);

				break;
			}
			case Mueller_1893:
			{
				// (1)
				// Publicationen des Astrophysikalischen Observatoriums zu Potsdam, 8, 366, 1893.
				if (englishName=="Mercury")
				{
					double ph50=phaseDeg-50.0;
					return static_cast<float>(1.16 + d + 0.02838*ph50 + 0.0001023*ph50*ph50);
				}
				if (englishName=="Venus")
					return static_cast<float>(-4.00 + d + 0.01322*phaseDeg + 0.0000004247*phaseDeg*phaseDeg*phaseDeg);
				if (englishName=="Mars")
					return static_cast<float>(-1.30 + d + 0.01486*phaseDeg);
				if (englishName=="Jupiter")
					return static_cast<float>(-8.93 + d);
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
					const double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					const double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					const double ringsIllum = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
					return static_cast<float>(-8.68 + d + 0.044*phaseDeg + ringsIllum);
				}
				if (englishName=="Uranus")
					return static_cast<float>(-6.85 + d);
				if (englishName=="Neptune")
					return static_cast<float>(-7.05 + d);
				// Original formulae doesn't have equation for Pluto
				if (englishName=="Pluto")
					return static_cast<float>(-1.0 + d);

				break;
			}
			case AstronomicalAlmanac_1984:
			{
				// (2)
				if (englishName=="Mercury")
					return static_cast<float>(-0.42 + d + .038*phaseDeg - 0.000273*phaseDeg*phaseDeg + 0.000002*phaseDeg*phaseDeg*phaseDeg);
				if (englishName=="Venus")
					return static_cast<float>(-4.40 + d + 0.0009*phaseDeg + 0.000239*phaseDeg*phaseDeg - 0.00000065*phaseDeg*phaseDeg*phaseDeg);
				if (englishName=="Mars")
					return static_cast<float>(-1.52 + d + 0.016*phaseDeg);
				if (englishName=="Jupiter")
					return static_cast<float>(-9.40 + d + 0.005*phaseDeg);
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
					const double lambda=atan2(saturnEarth[1], saturnEarth[0]);
					const double beta=atan2(saturnEarth[2], std::sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
					const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
					const double ringsIllum = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
					return static_cast<float>(-8.88 + d + 0.044*phaseDeg + ringsIllum);
				}
				if (englishName=="Uranus")
					return static_cast<float>(-7.19 + d);
				if (englishName=="Neptune")
					return static_cast<float>(-6.87 + d);
				if (englishName=="Pluto")
					return static_cast<float>(-1.00 + d);

				break;
			}
			case Generic:
			{
				// drop down to calculation of visual magnitude from phase angle and albedo of the planet
				break;
			}
		}
	}

	// This formula source is unknown. But this was originally used even for the Moon!
	const double p = (1.0 - phaseAngle/M_PI) * cos_chi + std::sqrt(1.0 - cos_chi*cos_chi) / M_PI;
	const double F = 2.0 * static_cast<double>(albedo) * equatorialRadius * equatorialRadius * p / (3.0*observerPlanetRq*planetRq) * shadowFactor;
	return -26.73f - 2.5f * static_cast<float>(log10(F));
}

double Planet::getAngularSize(const StelCore* core) const
{
	const double rad = (rings ? rings->getSize() : equatorialRadius);
	return std::atan2(rad*sphereScale,getJ2000EquatorialPos(core).length()) * M_180_PI;
}


double Planet::getSpheroidAngularSize(const StelCore* core) const
{
	return std::atan2(equatorialRadius*sphereScale,getJ2000EquatorialPos(core).length()) * M_180_PI;
}

//the Planet and all the related infos : name, circle etc..
void Planet::draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont)
{
	if (hidden)
		return;

	// Exclude drawing if user set a hard limit magnitude.
	if (core->getSkyDrawer()->getFlagPlanetMagnitudeLimit() && (getVMagnitude(core) > static_cast<float>(core->getSkyDrawer()->getCustomPlanetMagnitudeLimit())))
	{
		// Get the eclipse factor to avoid hiding the Moon during a total solar eclipse, or planets in transit over the Solar disk.
		// Details: https://answers.launchpad.net/stellarium/+question/395139
		if (GETSTELMODULE(SolarSystem)->getEclipseFactor(core).first==1.0)
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
	switch (re.method) {
		case RotationElements::Traditional:
			while (p && p->parent)
			{
				mat = Mat4d::translation(p->eclipticPos) * mat * p->rotLocalToParent;
				p = p->parent;
			}
			break;
		case RotationElements::WGCCRE:
			while (p && p->parent)
			{
				mat = Mat4d::translation(p->eclipticPos) * mat; // * p->rotLocalToParent;
				p = p->parent;
			}
			break;
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
	const double screenSz = (getAngularSize(core))*M_PI_180*static_cast<double>(prj->getPixelPerRadAtCenter());
	const double viewportBufferSz= (englishName=="Sun" ? screenSz+125. : screenSz);	// enlarge if this is sun with its huge halo.
	const double viewport_left = prj->getViewportPosX();
	const double viewport_bottom = prj->getViewportPosY();

	if ((prj->project(Vec3d(0.), screenPos)
	     && screenPos[1]>viewport_bottom - viewportBufferSz && screenPos[1] < viewport_bottom + prj->getViewportHeight()+viewportBufferSz
	     && screenPos[0]>viewport_left - viewportBufferSz && screenPos[0] < viewport_left + prj->getViewportWidth() + viewportBufferSz))
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlapping (e.g. for Jupiter's satellites)
		float ang_dist = 300.f*static_cast<float>(atan(getEclipticPos().length()/getEquinoxEquatorialPos(core).length())/core->getMovementMgr()->getCurrentFov());
		if (ang_dist==0.f)
			ang_dist = 1.f; // if ang_dist == 0, the Planet is sun..

		// by putting here, only draw orbit if Planet is visible for clarity
		drawOrbit(core);  // TODO - fade in here also...

		if (flagLabels && ang_dist>0.25f && maxMagLabels>getVMagnitude(core))
		{
			labelsFader=true;
		}
		else
		{
			labelsFader=false;
		}
		drawHints(core, planetNameFont);

		draw3dModel(core,transfo,static_cast<float>(screenSz));
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
	GL(eclipsePush = p->uniformLocation("eclipsePush"));
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
	for (auto it = fixedAttributeLocations.begin(); it != fixedAttributeLocations.end(); ++it)
	{
		program->bindAttributeLocation(it.key(),it.value());
	}

	if(!StelPainter::linkProg(program,name))
	{
		delete program;
		return Q_NULLPTR;
	}

	vars.initLocations(program);

	return program;
}

bool Planet::initShader()
{
	if (planetShaderProgram || shaderError) return !shaderError; // Already done.
	qDebug() << "Initializing planets GL shaders... ";
	shaderError = true;

	QSettings* settings = StelApp::getInstance().getSettings();
	settings->sync();
	shadowPolyOffset = Vec2f(settings->value("astro/planet_shadow_polygonoffset", Vec2f(0.0f, 0.0f).toStr()).toString());
	//qDebug()<<"Shadow poly offset"<<shadowPolyOffset;

	// Shader text is loaded from file
	QString vFileName = StelFileMgr::findFile("data/shaders/planet.vert",StelFileMgr::File);
	QString fFileName = StelFileMgr::findFile("data/shaders/planet.frag",StelFileMgr::File);

	if(vFileName.isEmpty())
	{
		qCritical()<<"Cannot find 'data/shaders/planet.vert', can't use planet rendering!";
		return false;
	}
	if(fFileName.isEmpty())
	{
		qCritical()<<"Cannot find 'data/shaders/planet.frag', can't use planet rendering!";
		return false;
	}

	QFile vFile(vFileName);
	QFile fFile(fFileName);

	if(!vFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical()<<"Cannot load planet vertex shader file"<<vFileName<<vFile.errorString();
		return false;
	}
	QByteArray vsrc = vFile.readAll();
	vFile.close();

	if(!fFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical()<<"Cannot load planet fragment shader file"<<fFileName<<fFile.errorString();
		return false;
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
	GL(transformShaderProgram = createShader("transformShaderProgram", transformShaderVars, transformVShader, transformFShader,QByteArray(),attrLoc));

	//check if ALL shaders have been created correctly
	shaderError = !(planetShaderProgram&&
			ringPlanetShaderProgram&&
			moonShaderProgram&&
			objShaderProgram&&
			objShadowShaderProgram&&
			transformShaderProgram);
	return true;
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
	float magFactorGreen=powf(0.85f, 0.6f*extinctedMag);
	float magFactorBlue=powf(0.6f, 0.5f*extinctedMag);

	if (screenSz>1.f)
	{
		//make better use of depth buffer by adjusting clipping planes
		//must be done before creating StelPainter
		//depth buffer is used for object with rings or with OBJ models
		//it is not used for normal spherical object rendering without rings!
		//but it does not hurt to adjust it in all cases
		double n,f;
		core->getClippingPlanes(&n,&f); // Save clipping planes

		//determine the minimum size of the clip space
		double r = equatorialRadius*sphereScale;
		if(rings)
			r+=rings->getSize()*sphereScale;

		const double dist = getEquinoxEquatorialPos(core).length();
		const double z_near = qMax(0.00001, (dist - r)); //near Z should be as close as possible to the actual geometry
		const double z_far  = (dist + 10*r); //far Z should be quite a bit further behind (Z buffer accuracy is worse near the far plane)
		core->setClippingPlanes(z_near,z_far);

		StelProjector::ModelViewTranformP transfo2 = transfo->clone();
		transfo2->combine(Mat4d::zrotation(M_PI_180*static_cast<double>(axisRotation + 90.f)));
		StelPainter sPainter(core->getProjection(transfo2));
		gl = sPainter.glFuncs();

		#ifdef GL_MULTISAMPLE
		if(multisamplingEnabled_)
			gl->glEnable(GL_MULTISAMPLE);
		else
			gl->glDisable(GL_MULTISAMPLE);
		#endif
		
		// Set the main source of light to be the sun
		Vec3d sunPos(0.);
		core->getHeliocentricEclipticModelViewTransform()->forward(sunPos);
		light.position=sunPos;

		// Set the light parameters taking sun as the light source
		light.diffuse.set(1.f,static_cast<float>(magFactorGreen)*1.f,static_cast<float>(magFactorBlue)*1.f);
		light.ambient.set(0.02f,static_cast<float>(magFactorGreen)*0.02f,static_cast<float>(magFactorBlue)*0.02f);

		if (this==ssm->getMoon())
		{
			// ambient for the moon can provide the Ashen light!
			// during daylight, this still should not make moon visible. We grab sky brightness and dim the moon.
			// This approach here is again pretty ad-hoc.
			// We have 5000cd/m^2 at sunset returned (Note this may be unnaturally much. Should be rather 10, but the 5000 may include the sun).
			// When atm.brightness has fallen to 2000cd/m^2, we allow ashen light to appear visible. Its impact is full when atm.brightness is below 1000.
			LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
			Q_ASSERT(lmgr);
			const float atmLum=(lmgr->getFlagAtmosphere() ? lmgr->getAtmosphereAverageLuminance() : 0.0f);
			if (atmLum<2000.0f)
			{
				float atmScaling=1.0f - (qMax(1000.0f, atmLum)-1000.0f)*0.001f; // full impact when atmLum<1000.
				float ashenFactor=(1.0f-getPhase(ssm->getEarth()->getHeliocentricEclipticPos())); // We really mean the Earth for this! (Try observing from Mars ;-)
				ashenFactor*=ashenFactor*0.15f*atmScaling;
				light.ambient = Vec4f(ashenFactor, magFactorGreen*ashenFactor, magFactorBlue*ashenFactor);
			}
			const float fov=core->getProjection(transfo)->getFov();
			float fovFactor=1.6f;
			// scale brightness to reduce if fov smaller than 5 degrees. Min brightness (to avoid glare) if fov=2deg.
			if (fov<5.0f)
			{
				fovFactor -= 0.1f*(5.0f-qMax(2.0f, fov));
			}
			// Special case for the Moon. Was 1.6, but this often is too bright.
			light.diffuse.set(fovFactor,magFactorGreen*fovFactor,magFactorBlue*fovFactor);
		}

		// possibly tint sun's color from extinction. This should deliberately cause stronger reddening than for the other objects.
		if (this==ssm->getSun())
		{
			// when we zoom in, reduce the overbrightness. (LP:#1421173)
			const float fov=core->getProjection(transfo)->getFov();
			const float overbright=qBound(0.85f, 0.5f*fov, 2.0f); // scale full brightness to 0.85...2. (<2 when fov gets under 4 degrees)
			sPainter.setColor(overbright, powf(0.75f, extinctedMag)*overbright, powf(0.42f, 0.9f*extinctedMag)*overbright);
		}

		//if (rings) /// GZ This was the previous condition. Not sure why rings were dropped?
		if(ssm->getFlagUseObjModels() && !objModelPath.isEmpty())
		{
			if(!drawObjModel(&sPainter, screenSz))
			{
				drawSphere(&sPainter, screenSz, drawOnlyRing);
			}
		}
		else if (!survey || survey->getInterstate() < 1.0f)
		{
			drawSphere(&sPainter, screenSz, drawOnlyRing);
		}

		if (survey && survey->getInterstate() > 0.0f)
		{
			drawSurvey(core, &sPainter);
			drawSphere(&sPainter, screenSz, true);
		}


		core->setClippingPlanes(n,f);  // Restore old clipping planes
		#ifdef GL_MULTISAMPLE
		if(multisamplingEnabled_)
			gl->glDisable(GL_MULTISAMPLE);
		#endif
	}

	bool allowDrawHalo = true;
	if ((this!=ssm->getSun()) && ((this !=ssm->getMoon() && core->getCurrentLocation().planetName=="Earth" )))
	{
		// Let's hide halo when inner planet between Sun and observer (or moon between planet and observer).
		// Do not hide Earth's moon's halo below ~-45degrees when observing from earth.
		Vec3d obj = getJ2000EquatorialPos(core);
		Vec3d par = getParent()->getJ2000EquatorialPos(core);
		const double angle = obj.angle(par)*M_180_PI;
		const double asize = getParent()->getSpheroidAngularSize(core);
		if (angle<=asize)
			allowDrawHalo = false;
	}

	if (this==ssm->getMoon() && !drawMoonHalo)
		allowDrawHalo = false;

	// Draw the halo if enabled in the ssystem_*.ini files (+ special case for backward compatible for the Sun)
	if ((hasHalo() || this==ssm->getSun()) && allowDrawHalo)
	{
		// Prepare OpenGL lighting parameters according to luminance. For scaled-up planets, reduce brightness of the halo.
		float surfArcMin2 = static_cast<float>(getSpheroidAngularSize(core)*qMax(1.0, (englishName=="Moon" ? 1.0 : 0.025)*sphereScale))*60.f;
		surfArcMin2 = surfArcMin2*surfArcMin2*M_PIf; // the total illuminated area in arcmin^2

		StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
		Vec3d tmp = getJ2000EquatorialPos(core);

		// Find new extincted color for halo. The method is again rather ad-hoc, but does not look too bad.
		// For the sun, we have again to use the stronger extinction to avoid color mismatch.
		Vec3f haloColorToDraw;
		if (this==ssm->getSun())
			haloColorToDraw.set(haloColor[0], powf(0.75f, extinctedMag) * haloColor[1], powf(0.42f, 0.9f*extinctedMag) * haloColor[2]);
		else
			haloColorToDraw.set(haloColor[0], magFactorGreen * haloColor[1], static_cast<float>(magFactorBlue) * haloColor[2]);
		if (this==ssm->getMoon())
			haloColorToDraw*=0.6f; //GZ: Attempt to make lunar halo less glaring...

		if (this!=ssm->getSun() || drawSunHalo)
			core->getSkyDrawer()->postDrawSky3dModel(&sPainter, tmp.toVec3f(), surfArcMin2, getVMagnitudeWithExtinction(core), haloColorToDraw);

		if ((this==ssm->getSun()) && (core->getCurrentLocation().planetName == "Earth"))
		{
			LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
			const float eclipseFactor = static_cast<float>(ssm->getEclipseFactor(core).first);
			// This alpha ensures 0 for complete sun, 1 for eclipse better 1e-10, with a strong increase towards full eclipse. We still need to square it.
			// But without atmosphere we should indeed draw a visible corona!
			const float alpha= ( !lmgr->getFlagAtmosphere() ? 0.7f : -0.1f*qMax(-10.0f, log10f(eclipseFactor)));
			StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
			float rotationAngle=(mmgr->getEquatorialMount() ? 0.0f : getParallacticAngle(core) * M_180_PIf);

			// Add ecliptic/equator angle. Meeus, Astr. Alg. 2nd, p100.
			const double jde=core->getJDE();
			const double eclJDE = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(jde);
			double ra_equ, dec_equ, lambdaJDE, betaJDE;
			StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
			StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaJDE, &betaJDE);
			// We can safely assume beta=0 and ignore nutation.
			const float q0=static_cast<float>(atan(-cos(lambdaJDE)*tan(eclJDE)));
			rotationAngle -= q0*static_cast<float>(180.0/M_PI);

			core->getSkyDrawer()->drawSunCorona(&sPainter, tmp.toVec3f(), 512.f/192.f*screenSz, haloColorToDraw, alpha*alpha, rotationAngle);
		}
	}
}

struct Planet3DModel
{
	QVector<float> vertexArr;
	QVector<float> texCoordArr;
	QVector<unsigned short> indiceArr;
};


void sSphere(Planet3DModel* model, const float radius, const float oneMinusOblateness, const unsigned short int slices, const unsigned short int stacks)
{
	model->indiceArr.resize(0);
	model->vertexArr.resize(0);
	model->texCoordArr.resize(0);
	
	GLfloat x, y, z;
	GLfloat s=0.f, t=1.f;
	GLushort i, j;

	const float* cos_sin_rho = StelUtils::ComputeCosSinRho(stacks);
	const float* cos_sin_theta =  StelUtils::ComputeCosSinTheta(slices);
	
	const float* cos_sin_rho_p;
	const float *cos_sin_theta_p;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from 0.0/+1.0 at z = -radius/+radius (linear along longitudes)
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
		unsigned short int offset = i*(slices+1)*2;
		unsigned short int limit = slices*2u+2u;
		for (j = 2u;j<limit;j+=2u)
		{
			model->indiceArr << offset+j-2u << offset+j-1u << offset+j;
			model->indiceArr << offset+j << offset+j-1u << offset+j+1u;
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


void sRing(Ring3DModel* model, const float rMin, const float rMax, unsigned short int slices, const unsigned short int stacks)
{
	float x,y;
	
	const float dr = (rMax-rMin) / stacks;
	const float* cos_sin_theta = StelUtils::ComputeCosSinTheta(slices);
	const float* cos_sin_theta_p;

	model->vertexArr.resize(0);
	model->texCoordArr.resize(0);
	model->indiceArr.resize(0);

	float r = rMin;
	for (unsigned short int i=0; i<=stacks; ++i)
	{
		const float tex_r0 = (r-rMin)/(rMax-rMin);
		unsigned short int j;
		for (j=0,cos_sin_theta_p=cos_sin_theta; j<=slices; ++j,cos_sin_theta_p+=2)
		{
			x = r*cos_sin_theta_p[0];
			y = r*cos_sin_theta_p[1];
			model->texCoordArr << tex_r0 << 0.5f;
			model->vertexArr << x << y << 0.f;
		}
		r+=dr;
	}
	for (unsigned short int i=0; i<stacks; ++i)
	{
		for (unsigned short int j=0; j<slices; ++j)
		{
			model->indiceArr << i*slices+j << (i+1)*slices+j << i*slices+j+1u;
			model->indiceArr << i*slices+j+1u << (i+1u)*slices+j << (i+1u)*slices+j+1u;
		}
	}
}

// Used in drawSphere() to compute shadows.
void Planet::computeModelMatrix(Mat4d &result) const
{
	result = Mat4d::translation(eclipticPos) * rotLocalToParent;
	PlanetP p = parent;
	switch (re.method)
	{
		case RotationElements::Traditional:
			while (p && p->parent)
			{
				result = Mat4d::translation(p->eclipticPos) * result * p->rotLocalToParent;
				p = p->parent;
			}
			break;
		case RotationElements::WGCCRE:
			while (p && p->parent)
			{
				result = Mat4d::translation(p->eclipticPos) * result;
				p = p->parent;
			}
			break;
	}
	result = result * Mat4d::zrotation(M_PI/180.*static_cast<double>(axisRotation + 90.f));
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
		data.shadowCandidatesData(0, i) = static_cast<float>(position[0]);
		data.shadowCandidatesData(1, i) = static_cast<float>(position[1]);
		data.shadowCandidatesData(2, i) = static_cast<float>(position[2]);
		data.shadowCandidatesData(3, i) = static_cast<float>(data.shadowCandidates.at(i)->getEquatorialRadius());
	}

	Vec3f lightPos3=light.position.toVec3f();
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
	GL(shader->setUniformValue(shaderVars.eyeDirection, static_cast<GLfloat>(data.eyePos[0]), static_cast<GLfloat>(data.eyePos[1]), static_cast<GLfloat>(data.eyePos[2])));
	GL(shader->setUniformValue(shaderVars.diffuseLight, light.diffuse[0], light.diffuse[1], light.diffuse[2]));
	GL(shader->setUniformValue(shaderVars.ambientLight, light.ambient[0], light.ambient[1], light.ambient[2]));
	GL(shader->setUniformValue(shaderVars.tex, 0));
	GL(shader->setUniformValue(shaderVars.shadowCount, data.shadowCandidates.size()));
	GL(shader->setUniformValue(shaderVars.shadowData, data.shadowCandidatesData));
	GL(shader->setUniformValue(shaderVars.sunInfo, static_cast<GLfloat>(data.mTarget[12]), static_cast<GLfloat>(data.mTarget[13]), static_cast<GLfloat>(data.mTarget[14]), static_cast<GLfloat>(sun->getEquatorialRadius())));
	GL(shader->setUniformValue(shaderVars.skyBrightness, lmgr->getAtmosphereAverageLuminance()));

	if(shaderVars.orenNayarParameters>=0)
	{
		//calculate and set oren-nayar parameters. The 75 in the scaling computation is arbitrary, to make the terminator visible.
		const float roughnessSq = roughness * roughness;
		QVector4D vec(
					1.0f - 0.5f * roughnessSq / (roughnessSq + 0.33f), // 0.57f), //x = A. If interreflection term is removed from shader, use 0.57 instead of 0.33.
					0.45f * roughnessSq / (roughnessSq + 0.09f),	//y = B
					50.0f * albedo/M_PIf, // was: 1.85f, but unclear why. //z = scale factor=rho/pi*Eo. rho=albedo=0.12, Eo~50? Higher Eo looks better!
					roughnessSq);
		GL(shader->setUniformValue(shaderVars.orenNayarParameters, vec));
	}

	float outgas_intensity_distanceScaled=static_cast<float>(static_cast<double>(outgas_intensity)/getHeliocentricEclipticPos().lengthSquared()); // ad-hoc function: assume square falloff by distance.
	GL(shader->setUniformValue(shaderVars.outgasParameters, QVector2D(outgas_intensity_distanceScaled, outgas_falloff)));

	return data;
}

void Planet::drawSphere(StelPainter* painter, float screenSz, bool drawOnlyRing)
{
	const float sphereScaleF=static_cast<float>(sphereScale);
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
	const unsigned short int nb_facet = static_cast<unsigned short int>(qBound(10u, static_cast<uint>(screenSz * 40.f/50.f * sqrt(sphereScaleF)), 100u));	// 40 facets for 1024 pixels diameter on screen

	// Generates the vertices
	Planet3DModel model;
	sSphere(&model, static_cast<float>(equatorialRadius), static_cast<float>(oneMinusOblateness), nb_facet, nb_facet);

	QVector<float> projectedVertexArr(model.vertexArr.size());
	for (int i=0;i<model.vertexArr.size()/3;++i)
	{
		Vec3f p = *(reinterpret_cast<const Vec3f*>(model.vertexArr.constData()+i*3));
		p *= sphereScaleF;
		painter->getProjector()->project(p, *(reinterpret_cast<Vec3f*>(projectedVertexArr.data()+i*3)));
	}
	
	const SolarSystem* ssm = GETSTELMODULE(SolarSystem);

	if (this==ssm->getSun())
	{
		texMap->bind();
		//painter->setColor(2, 2, 0.2); // This is now in draw3dModel() to apply extinction
		painter->setArrays(reinterpret_cast<const Vec3f*>(projectedVertexArr.constData()), reinterpret_cast<const Vec2f*>(model.texCoordArr.constData()));
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
			// Ad-hoc visibility improvement during lunar eclipses:
			// During partial umbra phase, make moon brighter so that the bright limb and umbra border has more visibility.
			// When the moon is half in umbra, we start to raise its brightness.
			GLfloat push=1.0f;
			const double od=180.-getElongation(ssm->getEarth()->getEclipticPos()) * (180.0/M_PI); // opposition distance [degrees]

			// Compute umbra radius at lunar distance.
			const double Lambda=getEclipticPos().length();                             // Lunar distance [AU]
			const double sigma=ssm->getEarthShadowRadiiAtLunarDistance().first[0]/3600.;
			const double tau=atan(getEquatorialRadius()/Lambda) * M_180_PI; // geocentric angle of Lunar radius [degrees]

			if (od<sigma-tau)     // if the Moon is fully immersed in the shadow
				push=4.0f;
			else if (od<sigma)    // If the Moon is half immersed, start pushing with a strong power function that make it apparent only in the last few percents.
				push+=3.f*(1.f-pow(static_cast<float>((od-sigma+tau)/tau), 1.f/6.f));

			GL(moonShaderProgram->setUniformValue(moonShaderVars.eclipsePush, push)); // constant for now...
		}
	}

	GL(shader->setAttributeArray(shaderVars->vertex, static_cast<const GLfloat*>(projectedVertexArr.constData()), 3));
	GL(shader->enableAttributeArray(shaderVars->vertex));
	GL(shader->setAttributeArray(shaderVars->unprojectedVertex, static_cast<const GLfloat*>(model.vertexArr.constData()), 3));
	GL(shader->enableAttributeArray(shaderVars->unprojectedVertex));
	GL(shader->setAttributeArray(shaderVars->texCoord, static_cast<const GLfloat*>(model.texCoordArr.constData()), 2));
	GL(shader->enableAttributeArray(shaderVars->texCoord));

	if (rings && !drawOnlyRing)
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
		shadowCandidatesData(0, 0) = static_cast<float>(position[0]);
		shadowCandidatesData(1, 0) = static_cast<float>(position[1]);
		shadowCandidatesData(2, 0) = static_cast<float>(position[2]);
		shadowCandidatesData(3, 0) = static_cast<float>(getEquatorialRadius());
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.shadowCount, 1));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.shadowData, shadowCandidatesData));
		
		projectedVertexArr.resize(ringModel.vertexArr.size());
		for (int i=0;i<ringModel.vertexArr.size()/3;++i)
		{
			Vec3f p = *(reinterpret_cast<const Vec3f*>(ringModel.vertexArr.constData()+i*3));
			p *= sphereScaleF;
			painter->getProjector()->project(p, *(reinterpret_cast<Vec3f*>(projectedVertexArr.data()+i*3)));
		}
		
		GL(ringPlanetShaderProgram->setAttributeArray(ringPlanetShaderVars.vertex, reinterpret_cast<const GLfloat*>(projectedVertexArr.constData()), 3));
		GL(ringPlanetShaderProgram->enableAttributeArray(ringPlanetShaderVars.vertex));
		GL(ringPlanetShaderProgram->setAttributeArray(ringPlanetShaderVars.unprojectedVertex, reinterpret_cast<const GLfloat*>(ringModel.vertexArr.constData()), 3));
		GL(ringPlanetShaderProgram->enableAttributeArray(ringPlanetShaderVars.unprojectedVertex));
		GL(ringPlanetShaderProgram->setAttributeArray(ringPlanetShaderVars.texCoord, reinterpret_cast<const GLfloat*>(ringModel.texCoordArr.constData()), 2));
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


// Draw the Hips survey.
void Planet::drawSurvey(StelCore* core, StelPainter* painter)
{
	if (!Planet::initShader()) return;
	const SolarSystem* ssm = GETSTELMODULE(SolarSystem);

	painter->setDepthMask(true);
	painter->setDepthTest(true);

	// Backup transformation so that we can restore it later.
	StelProjector::ModelViewTranformP transfo = painter->getProjector()->getModelViewTransform()->clone();
	Vec4f color = painter->getColor();
	painter->getProjector()->getModelViewTransform()->combine(Mat4d::scaling(equatorialRadius * sphereScale));

	QOpenGLShaderProgram* shader = planetShaderProgram;
	const PlanetShaderVars* shaderVars = &planetShaderVars;
	if (rings)
	{
		shader = ringPlanetShaderProgram;
		shaderVars = &ringPlanetShaderVars;
	}
	if (this == ssm->getMoon())
	{
		shader = moonShaderProgram;
		shaderVars = &moonShaderVars;
	}

	GL(shader->bind());
	RenderData rData = setCommonShaderUniforms(*painter, shader, *shaderVars);
	QVector<Vec3f> projectedVertsArray;
	QVector<Vec3f> vertsArray;
	double angle = getSpheroidAngularSize(core) * M_PI / 180.;

	if (rings)
	{
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.isRing, false));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.ring, true));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.outerRadius, rings->radiusMax));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.innerRadius, rings->radiusMin));
		GL(ringPlanetShaderProgram->setUniformValue(ringPlanetShaderVars.ringS, 2));
		rings->tex->bind(2);
	}

	if (this == ssm->getMoon())
	{
		GL(normalMap->bind(2));
		GL(moonShaderProgram->setUniformValue(moonShaderVars.normalMap, 2));
		if (!rData.shadowCandidates.isEmpty())
		{
			GL(texEarthShadow->bind(3));
			GL(moonShaderProgram->setUniformValue(moonShaderVars.earthShadow, 3));
		}
	}

	// Apply a rotation otherwize the hips surveys don't get rendered at the
	// proper position.  Not sure why...
	painter->getProjector()->getModelViewTransform()->combine(Mat4d::zrotation(M_PI / 2.0));
	painter->getProjector()->getModelViewTransform()->combine(Mat4d::scaling(Vec3d(1, 1, oneMinusOblateness)));

	survey->draw(painter, angle, [&](const QVector<Vec3d>& verts, const QVector<Vec2f>& tex, const QVector<uint16_t>& indices) {
		projectedVertsArray.resize(verts.size());
		vertsArray.resize(verts.size());
		for (int i = 0; i < verts.size(); i++)
		{
			Vec3d v;
			v = verts[i];
			painter->getProjector()->project(v, v);
			projectedVertsArray[i] = Vec3f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
			v = Mat4d::scaling(equatorialRadius) * verts[i];
			v = Mat4d::scaling(Vec3d(1, 1, oneMinusOblateness)) * v;
			// Undo the rotation we applied for the survey fix.
			v = Mat4d::zrotation(M_PI / 2.0) * v;
			vertsArray[i] = Vec3f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
		}
		GL(shader->setAttributeArray(shaderVars->vertex, reinterpret_cast<const GLfloat*>(projectedVertsArray.constData()), 3));
		GL(shader->enableAttributeArray(shaderVars->vertex));
		GL(shader->setAttributeArray(shaderVars->unprojectedVertex, reinterpret_cast<const GLfloat*>(vertsArray.constData()), 3));
		GL(shader->enableAttributeArray(shaderVars->unprojectedVertex));
		GL(shader->setAttributeArray(shaderVars->texCoord, reinterpret_cast<const GLfloat*>(tex.constData()), 2));
		GL(shader->enableAttributeArray(shaderVars->texCoord));
		GL(gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, indices.constData()));
	});

	// Restore painter state.
	painter->setProjector(core->getProjection(transfo));
	painter->setColor(color);
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
		Vector3<GLubyte> colByte(static_cast<unsigned char>(texCol[0]),static_cast<unsigned char>(texCol[1]),static_cast<unsigned char>(texCol[2]));
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
	objModel->projPosBuffer->allocate(objModel->projectedPosArray.constData(),vtxCount * static_cast<int>(sizeof(Vec3f)));

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
	modelView.lookAt(QVector3D(static_cast<float>(lightPosScaled[0]),static_cast<float>(lightPosScaled[1]),static_cast<float>(lightPosScaled[2])), QVector3D(), QVector3D(0.f,0.f,1.f));

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
	proj.ortho(static_cast<float>(minRight),static_cast<float>(maxRight),
		   static_cast<float>(minUp),static_cast<float>(maxUp),
		   static_cast<float>(minZ),static_cast<float>(maxZ));

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
	float tmp = (hintFader.getInterstate()<=0.f ? 7.f : 10.f) + static_cast<float>(getAngularSize(core)*M_PI/180.)*prj->getPixelPerRadAtCenter()/1.44f; // Shift for nameI18 printing
	sPainter.setColor(labelColor,labelsFader.getInterstate());
	sPainter.drawText(static_cast<float>(screenPos[0]),static_cast<float>(screenPos[1]), getSkyLabel(core), 0, tmp, tmp, false);

	// hint disappears smoothly on close view
	if (hintFader.getInterstate()<=0)
		return;
	tmp -= 10.f;
	if (tmp<1) tmp=1;
	sPainter.setColor(labelColor,labelsFader.getInterstate()*hintFader.getInterstate()/tmp*0.7f);

	// Draw the 2D small circle
	sPainter.setBlending(true);
	Planet::hintCircleTex->bind();
	sPainter.drawSprite2dMode(static_cast<float>(screenPos[0]), static_cast<float>(screenPos[1]), 11);
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
			const QMap<Planet::PlanetType, Vec3f> typeColorMap = {
				{ isMoon,         orbitMoonsColor       },
				{ isPlanet,       orbitMajorPlanetsColor},
				{ isAsteroid,     orbitMinorPlanetsColor},
				{ isDwarfPlanet,  orbitDwarfPlanetsColor},
				{ isCubewano,     orbitCubewanosColor   },
				{ isPlutino,      orbitPlutinosColor    },
				{ isSDO,          orbitScatteredDiscObjectsColor},
				{ isOCO,          orbitOortCloudObjectsColor},
				{ isComet,        orbitCometsColor      },
				{ isSednoid,      orbitSednoidsColor    },
				{ isInterstellar, orbitInterstellarColor}};
			orbColor = typeColorMap.value(pType, orbitColor);
			break;
		}
		case ocsMajorPlanets:
		{
			const QString pName = getEnglishName().toLower();
			const QMap<QString, Vec3f>majorPlanetColorMap = {
				{ "mercury", orbitMercuryColor},
				{ "venus",   orbitVenusColor  },
				{ "earth",   orbitEarthColor  },
				{ "mars",    orbitMarsColor   },
				{ "jupiter", orbitJupiterColor},
				{ "saturn",  orbitSaturnColor },
				{ "uranus",  orbitUranusColor },
				{ "neptune", orbitNeptuneColor}};
			orbColor=majorPlanetColorMap.value(pName, orbitColor);
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

void Planet::computeOrbit()
{
	double dateJDE = lastJDE;
	double calc_date;
	Vec3d parentPos;
	if (parent)
		parentPos = parent->getHeliocentricEclipticPos(dateJDE);

	for(int d = 0; d < ORBIT_SEGMENTS; d++)
	{
		calc_date = dateJDE + (d-ORBIT_SEGMENTS/2)*deltaOrbitJDE;
		// Round to a number of deltaOrbitJDE to improve caching.
		if (d != ORBIT_SEGMENTS / 2)
		{
			calc_date = nearbyint(calc_date / deltaOrbitJDE) * deltaOrbitJDE;
		}
		orbit[d] = getEclipticPos(calc_date) + parentPos;
	}
}

// draw orbital path of Planet
void Planet::drawOrbit(const StelCore* core)
{
	if (!static_cast<bool>(orbitFader.getInterstate()))
		return;
	if (!static_cast<bool>(siderealPeriod))
		return;
	if (hidden || (pType==isObserver)) return;
	if (orbitPtr && pType>=isArtificial)
	{
		if (!static_cast<KeplerOrbit*>(orbitPtr)->objectDateValid(lastJDE))
			return;
	}

	// Update the orbit positions to the current planet date.
	computeOrbit();

	const StelProjectorP prj = core->getProjection(StelCore::FrameHeliocentricEclipticJ2000);

	StelPainter sPainter(prj);

	// Normal transparency mode
	sPainter.setBlending(true);

	sPainter.setColor(getCurrentOrbitColor(), orbitFader.getInterstate());
	Vec3d onscreen;
	// special case - use current Planet position as center vertex so that draws
	// on its orbit all the time (since segmented rather than smooth curve)
	Vec3d savePos = orbit[ORBIT_SEGMENTS/2];
	orbit[ORBIT_SEGMENTS/2]=getHeliocentricEclipticPos();
	orbit[ORBIT_SEGMENTS]=orbit[0];
	int nbIter = closeOrbit ? ORBIT_SEGMENTS : ORBIT_SEGMENTS-1;
	QVarLengthArray<float, 1024> vertexArray;

	sPainter.enableClientStates(true, false, false);
	if (orbitsThickness>1)
		sPainter.setLineWidth(orbitsThickness);

	for (int n=0; n<=nbIter; ++n)
	{
		if (prj->project(orbit[n],onscreen) && (vertexArray.size()==0 || !prj->intersectViewportDiscontinuity(orbit[n-1], orbit[n])))
		{
			vertexArray.append(static_cast<float>(onscreen[0]));
			vertexArray.append(static_cast<float>(onscreen[1]));
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
	if (orbitsThickness>1)
		sPainter.setLineWidth(1);
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
	vMagAlgorithm = vMagAlgorithmMap.key(algorithm, Planet::MallamaHilton_2018);
}
