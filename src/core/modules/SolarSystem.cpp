/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2010 Bogdan Marinov
 * Copyright (C) 2011 Alexander Wolf
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

#include "SolarSystem.hpp"
#include "StelTexture.hpp"
#include "EphemWrapper.hpp"
#include "Orbit.hpp"

#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelIniParser.hpp"
#include "Planet.hpp"
#include "MinorPlanet.hpp"
#include "Comet.hpp"
#include "StelMainView.hpp"

#include "StelSkyDrawer.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "TrailGroup.hpp"
#include "RefractionExtinction.hpp"

#include "AstroCalcDialog.hpp"

#include <functional>
#include <algorithm>

#include <QTextStream>
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QMultiMap>
#include <QMapIterator>
#include <QDebug>
#include <QDir>
#include <QHash>

SolarSystem::SolarSystem()
	: shadowPlanetCount(0)
	, flagMoonScale(false)
	, moonScale(1.0)
	, flagMinorBodyScale(false)
	, minorBodyScale(1.0)
	, labelsAmount(false)
	, flagOrbits(false)
	, flagLightTravelTime(true)
	, flagUseObjModels(false)
	, flagShowObjSelfShadows(true)
	, flagShow(false)
	, flagPointer(false)
	, flagNativePlanetNames(false)
	, flagTranslatedNames(false)
	, flagIsolatedTrails(true)
	, flagIsolatedOrbits(true)
	, flagPlanetsOrbitsOnly(false)
	, ephemerisMarkersDisplayed(true)
	, ephemerisDatesDisplayed(false)
	, ephemerisMagnitudesDisplayed(false)
	, ephemerisHorizontalCoordinates(false)
	, ephemerisLineDisplayed(false)
	, ephemerisSkipDataDisplayed(false)
	, ephemerisDataStep(1)
	, ephemerisSmartDatesDisplayed(true)
	, ephemerisGenericMarkerColor(Vec3f(1.0f, 1.0f, 0.0f))
	, ephemerisSelectedMarkerColor(Vec3f(1.0f, 0.7f, 0.0f))
	, ephemerisMercuryMarkerColor(Vec3f(1.0f, 1.0f, 0.0f))
	, ephemerisVenusMarkerColor(Vec3f(1.0f, 1.0f, 1.0f))
	, ephemerisMarsMarkerColor(Vec3f(1.0f, 0.0f, 0.0f))
	, ephemerisJupiterMarkerColor(Vec3f(0.3f, 1.0f, 1.0f))
	, ephemerisSaturnMarkerColor(Vec3f(0.0f, 1.0f, 0.0f))
	, allTrails(Q_NULLPTR)
	, conf(StelApp::getInstance().getSettings())
{
	planetNameFont.setPixelSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
	setObjectName("SolarSystem");
}

void SolarSystem::setFontSize(int newFontSize)
{
	planetNameFont.setPixelSize(newFontSize);
}

SolarSystem::~SolarSystem()
{
	// release selected:
	selected.clear();
	selectedSSO.clear();
	for (auto* orb : orbits)
	{
		delete orb;
		orb = Q_NULLPTR;
	}
	sun.clear();
	moon.clear();
	earth.clear();
	Planet::hintCircleTex.clear();
	Planet::texEarthShadow.clear();

	texCircle.clear();
	texPointer.clear();

	delete allTrails;
	allTrails = Q_NULLPTR;

	// Get rid of circular reference between the shared pointers which prevent proper destruction of the Planet objects.
	for (const auto& p : systemPlanets)
	{
		p->satellites.clear();
	}

	//delete comet textures created in loadPlanets
	Comet::comaTexture.clear();
	Comet::tailTexture.clear();

	//deinit of SolarSystem is NOT called at app end automatically
	deinit();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SolarSystem::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)+10;
	return 0;
}

// Init and load the solar system data
void SolarSystem::init()
{
	Q_ASSERT(conf);

	Planet::init();
	loadPlanets();	// Load planets data

	// Compute position and matrix of sun and all the satellites (ie planets)
	// for the first initialization Q_ASSERT that center is sun center (only impacts on light speed correction)	
	computePositions(StelApp::getInstance().getCore()->getJDE(), getSun());

	setSelected("");	// Fix a bug on macosX! Thanks Fumio!
	setFlagMoonScale(conf->value("viewing/flag_moon_scaled", conf->value("viewing/flag_init_moon_scaled", "false").toBool()).toBool());  // name change
	setMinorBodyScale(conf->value("viewing/minorbodies_scale", 10.0).toDouble());
	setFlagMinorBodyScale(conf->value("viewing/flag_minorbodies_scaled", false).toBool());
	setMoonScale(conf->value("viewing/moon_scale", 4.0).toDouble());
	setFlagPlanets(conf->value("astro/flag_planets").toBool());
	setFlagHints(conf->value("astro/flag_planets_hints").toBool());
	setFlagLabels(conf->value("astro/flag_planets_labels", true).toBool());
	setLabelsAmount(conf->value("astro/labels_amount", 3.).toDouble());
	setFlagOrbits(conf->value("astro/flag_planets_orbits").toBool());
	setFlagLightTravelTime(conf->value("astro/flag_light_travel_time", true).toBool());
	setFlagUseObjModels(conf->value("astro/flag_use_obj_models", false).toBool());
	setFlagShowObjSelfShadows(conf->value("astro/flag_show_obj_self_shadows", true).toBool());
	setFlagPointer(conf->value("astro/flag_planets_pointers", true).toBool());
	// Set the algorithm from Astronomical Almanac for computation of apparent magnitudes for
	// planets in case  observer on the Earth by default
	setApparentMagnitudeAlgorithmOnEarth(conf->value("astro/apparent_magnitude_algorithm", "ExplSup2013").toString());
	setFlagNativePlanetNames(conf->value("viewing/flag_planets_native_names", true).toBool());
	// Is enabled the showing of isolated trails for selected objects only?
	setFlagIsolatedTrails(conf->value("viewing/flag_isolated_trails", true).toBool());
	setNumberIsolatedTrails(conf->value("viewing/number_isolated_trails", 1).toInt());
	setFlagIsolatedOrbits(conf->value("viewing/flag_isolated_orbits", true).toBool());
	setFlagPlanetsOrbitsOnly(conf->value("viewing/flag_planets_orbits_only", false).toBool());
	setFlagPermanentOrbits(conf->value("astro/flag_permanent_orbits", false).toBool());
	setOrbitColorStyle(conf->value("astro/planets_orbits_color_style", "one_color").toString());

	// Settings for calculation of position of Great Red Spot on Jupiter
	setFlagCustomGrsSettings(conf->value("astro/flag_grs_custom", false).toBool());
	setCustomGrsLongitude(conf->value("astro/grs_longitude", 216).toInt());
	setCustomGrsDrift(conf->value("astro/grs_drift", 15.).toDouble());
	setCustomGrsJD(conf->value("astro/grs_jd", 2456901.5).toDouble());

	// Load colors from config file
	QString defaultColor = conf->value("color/default_color").toString();
	setLabelsColor(StelUtils::strToVec3f(conf->value("color/planet_names_color", defaultColor).toString()));
	setOrbitsColor(StelUtils::strToVec3f(conf->value("color/sso_orbits_color", defaultColor).toString()));
	setMajorPlanetsOrbitsColor(StelUtils::strToVec3f(conf->value("color/major_planet_orbits_color", "0.7,0.2,0.2").toString()));
	setMoonsOrbitsColor(StelUtils::strToVec3f(conf->value("color/moon_orbits_color", "0.7,0.2,0.2").toString()));
	setMinorPlanetsOrbitsColor(StelUtils::strToVec3f(conf->value("color/minor_planet_orbits_color", "0.7,0.5,0.5").toString()));
	setDwarfPlanetsOrbitsColor(StelUtils::strToVec3f(conf->value("color/dwarf_planet_orbits_color", "0.7,0.5,0.5").toString()));
	setCubewanosOrbitsColor(StelUtils::strToVec3f(conf->value("color/cubewano_orbits_color", "0.7,0.5,0.5").toString()));
	setPlutinosOrbitsColor(StelUtils::strToVec3f(conf->value("color/plutino_orbits_color", "0.7,0.5,0.5").toString()));
	setScatteredDiskObjectsOrbitsColor(StelUtils::strToVec3f(conf->value("color/sdo_orbits_color", "0.7,0.5,0.5").toString()));
	setOortCloudObjectsOrbitsColor(StelUtils::strToVec3f(conf->value("color/oco_orbits_color", "0.7,0.5,0.5").toString()));
	setCometsOrbitsColor(StelUtils::strToVec3f(conf->value("color/comet_orbits_color", "0.7,0.8,0.8").toString()));
	setSednoidsOrbitsColor(StelUtils::strToVec3f(conf->value("color/sednoid_orbits_color", "0.7,0.5,0.5").toString()));
	setInterstellarOrbitsColor(StelUtils::strToVec3f(conf->value("color/interstellar_orbits_color", "1.0,0.6,1.0").toString()));
	setMercuryOrbitColor(StelUtils::strToVec3f(conf->value("color/mercury_orbit_color", "0.5,0.5,0.5").toString()));
	setVenusOrbitColor(StelUtils::strToVec3f(conf->value("color/venus_orbit_color", "0.9,0.9,0.7").toString()));
	setEarthOrbitColor(StelUtils::strToVec3f(conf->value("color/earth_orbit_color", "0.0,0.0,1.0").toString()));
	setMarsOrbitColor(StelUtils::strToVec3f(conf->value("color/mars_orbit_color", "0.8,0.4,0.1").toString()));
	setJupiterOrbitColor(StelUtils::strToVec3f(conf->value("color/jupiter_orbit_color", "1.0,0.6,0.0").toString()));
	setSaturnOrbitColor(StelUtils::strToVec3f(conf->value("color/saturn_orbit_color", "1.0,0.8,0.0").toString()));
	setUranusOrbitColor(StelUtils::strToVec3f(conf->value("color/uranus_orbit_color", "0.0,0.7,1.0").toString()));
	setNeptuneOrbitColor(StelUtils::strToVec3f(conf->value("color/neptune_orbit_color", "0.0,0.3,1.0").toString()));
	setTrailsColor(StelUtils::strToVec3f(conf->value("color/object_trails_color", defaultColor).toString()));
	setPointerColor(StelUtils::strToVec3f(conf->value("color/planet_pointers_color", "1.0,0.3,0.3").toString()));

	// Ephemeris stuff
	setFlagEphemerisMarkers(conf->value("astrocalc/flag_ephemeris_markers", true).toBool());
	setFlagEphemerisDates(conf->value("astrocalc/flag_ephemeris_dates", false).toBool());
	setFlagEphemerisMagnitudes(conf->value("astrocalc/flag_ephemeris_magnitudes", false).toBool());
	setFlagEphemerisHorizontalCoordinates(conf->value("astrocalc/flag_ephemeris_horizontal", false).toBool());
	setFlagEphemerisLine(conf->value("astrocalc/flag_ephemeris_line", false).toBool());
	setFlagEphemerisSkipData(conf->value("astrocalc/flag_ephemeris_skip_data", false).toBool());
	setEphemerisDataStep(conf->value("astrocalc/ephemeris_data_step", 1).toInt());
	setFlagEphemerisSmartDates(conf->value("astrocalc/flag_ephemeris_smart_dates", true).toBool());
	setEphemerisGenericMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_generic_marker_color", "1.0,1.0,0.0").toString()));
	setEphemerisSelectedMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_selected_marker_color", "1.0,0.7,0.0").toString()));
	setEphemerisMercuryMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_mercury_marker_color", "1.0,1.0,0.0").toString()));
	setEphemerisVenusMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_venus_marker_color", "1.0,1.0,1.0").toString()));
	setEphemerisMarsMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_mars_marker_color", "1.0,0.0,0.0").toString()));
	setEphemerisJupiterMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_jupiter_marker_color", "0.3,1.0,1.0").toString()));
	setEphemerisSaturnMarkerColor(StelUtils::strToVec3f(conf->value("color/ephemeris_saturn_marker_color", "0.0,1.0,0.0").toString()));

	recreateTrails();

	setFlagTrails(conf->value("astro/flag_object_trails", false).toBool());

	StelObjectMgr *objectManager = GETSTELMODULE(StelObjectMgr);
	objectManager->registerStelObjectMgr(this);
	connect(objectManager, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)),
		this, SLOT(selectedObjectChange(StelModule::StelModuleSelectAction)));

	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur4.png");
	texCircle = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb.png");	// Load circle texture
	Planet::hintCircleTex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/planet-indicator.png");
	
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(updateSkyCulture(QString)));
	connect(&StelMainView::getInstance(), SIGNAL(reloadShadersRequested()), this, SLOT(reloadShaders()));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Planets", displayGroup, N_("Planets"), "planetsDisplayed", "P");
	addAction("actionShow_Planets_Labels", displayGroup, N_("Planet labels"), "labelsDisplayed", "Alt+P");
	addAction("actionShow_Planets_Orbits", displayGroup, N_("Planet orbits"), "flagOrbits", "O");
	addAction("actionShow_Planets_Trails", displayGroup, N_("Planet trails"), "trailsDisplayed", "Shift+T");
	//there is a small discrepancy in the GUI: "Show planet markers" actually means show planet hints
	addAction("actionShow_Planets_Hints", displayGroup, N_("Planet markers"), "flagHints", "Ctrl+P");
	addAction("actionShow_Planets_Pointers", displayGroup, N_("Planet selection marker"), "flagPointer", "Ctrl+Shift+P");
	addAction("actionShow_Planets_EnlargeMoon", displayGroup, N_("Enlarge Moon"), "flagMoonScale");
	addAction("actionShow_Planets_EnlargeMinor", displayGroup, N_("Enlarge minor bodies"), "flagMinorBodyScale");
	addAction("actionShow_Skyculture_NativePlanetNames", displayGroup, N_("Native planet names (from starlore)"), "flagNativePlanetNames", "Ctrl+Shift+N");

	connect(StelApp::getInstance().getModule("HipsMgr"), SIGNAL(gotNewSurvey(HipsSurveyP)),
			this, SLOT(onNewSurvey(HipsSurveyP)));

	// Fill ephemeris dates
	connect(this, SIGNAL(requestEphemerisVisualization()), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisDataStepChanged(int)), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisSkipDataChanged(bool)), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisSmartDatesChanged(bool)), this, SLOT(fillEphemerisDates()));
}

void SolarSystem::deinit()
{
	Planet::deinitShader();
	Planet::deinitFBO();
}

void SolarSystem::recreateTrails()
{
	// Create a trail group containing all the planets orbiting the sun (not including satellites)
	if (allTrails!=Q_NULLPTR)
		delete allTrails;
	allTrails = new TrailGroup(365.f);

	unsigned long cnt = (unsigned long) selectedSSO.size();
	if (cnt>0 && getFlagIsolatedTrails())
	{
		unsigned long limit = getNumberIsolatedTrails();
		if (cnt<limit)
			limit = cnt;
		for (unsigned long i=0; i<limit; i++)
		{
			allTrails->addObject((QSharedPointer<StelObject>)selectedSSO[cnt - i - 1], &trailColor);
		}
	}
	else
	{
		for (const auto& p : getSun()->satellites)
		{
			allTrails->addObject((QSharedPointer<StelObject>)p, &trailColor);
		}
	}
}


void SolarSystem::updateSkyCulture(const QString& skyCultureDir)
{
	planetNativeNamesMap.clear();

	QString namesFile = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/planet_names.fab");

	if (namesFile.isEmpty())
	{
		for (const auto& p : systemPlanets)
		{
			if (p->getPlanetType()==Planet::isPlanet || p->getPlanetType()==Planet::isMoon || p->getPlanetType()==Planet::isStar)
				p->setNativeName("");
		}
		updateI18n();
		return;
	}

	// Open file
	QFile planetNamesFile(namesFile);
	if (!planetNamesFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << " Cannot open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	QRegExp commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	QRegExp recRx("^\\s*(\\w+)\\s+\"(.+)\"\\s+_[(]\"(.+)\"[)]\\n");

	QString record, planetId, nativeName;

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!planetNamesFile.atEnd())
	{
		record = QString::fromUtf8(planetNamesFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;

		if (!recRx.exactMatch(record))
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in planet names file" << QDir::toNativeSeparators(namesFile);
		}
		else
		{
			planetId = recRx.capturedTexts().at(1).trimmed();
			nativeName = recRx.capturedTexts().at(3).trimmed(); // Use translatable text
			planetNativeNamesMap[planetId] = nativeName;
			readOk++;
		}
	}
	planetNamesFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "native names of planets";

	for (const auto& p : systemPlanets)
	{
		if (p->getPlanetType()==Planet::isPlanet || p->getPlanetType()==Planet::isMoon || p->getPlanetType()==Planet::isStar)
			p->setNativeName(planetNativeNamesMap[p->getEnglishName()]);
	}

	updateI18n();
}

void SolarSystem::reloadShaders()
{
	Planet::deinitShader();
	Planet::initShader();
}

void SolarSystem::drawPointer(const StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Planet");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos))
			return;


		StelPainter sPainter(prj);
		Vec3f color = getPointerColor();
		sPainter.setColor(color[0],color[1],color[2]);

		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter()*2.;
		
		const float scale = prj->getDevicePixelsPerPixel()*StelApp::getInstance().getGlobalScalingRatio();
		size+= scale * (45.f + 10.f*std::sin(2.f * StelApp::getInstance().getAnimationTime()));

		texPointer->bind();

		sPainter.setBlending(true);

		size*=0.5;
		const float angleBase = StelApp::getInstance().getAnimationTime() * 10;
		// We draw 4 instances of the sprite at the corners of the pointer
		for (int i = 0; i < 4; ++i)
		{
			const float angle = angleBase + i * 90;
			const double x = screenpos[0] + size * cos(angle / 180 * M_PI);
			const double y = screenpos[1] + size * sin(angle / 180 * M_PI);
			sPainter.drawSprite2dMode(x, y, 10, angle);
		}
	}
}

void ellipticalOrbitPosFunc(double jd,double xyz[3], double xyzdot[3], void* orbitPtr)
{
	static_cast<EllipticalOrbit*>(orbitPtr)->positionAtTimevInVSOP87Coordinates(jd, xyz);
	// TODO: Implement a way to retrieve velocities.
	xyzdot[0]=xyzdot[1]=xyzdot[2]=0.0;
}
void cometOrbitPosFunc(double jd,double xyz[3], double xyzdot[3], void* orbitPtr)
{
	static_cast<CometOrbit*>(orbitPtr)->positionAtTimevInVSOP87Coordinates(jd, xyz, true);
	static_cast<CometOrbit*>(orbitPtr)->getVelocity(xyzdot);
}

// Init and load the solar system data (2 files)
void SolarSystem::loadPlanets()
{
	minorBodies.clear();
	systemMinorBodies.clear();
	qDebug() << "Loading Solar System data (1: planets and moons) ...";
	QString solarSystemFile = StelFileMgr::findFile("data/ssystem_major.ini");
	if (solarSystemFile.isEmpty())
	{
		qWarning() << "ERROR while loading ssystem_major.ini (unable to find data/ssystem_major.ini): " << endl;
		return;
	}

	if (!loadPlanets(solarSystemFile))
	{
		qWarning() << "ERROR while loading ssystem_major.ini: " << endl;
		return;
	}

	qDebug() << "Loading Solar System data (2: minor bodies)...";
	QStringList solarSystemFiles = StelFileMgr::findFileInAllPaths("data/ssystem_minor.ini");
	if (solarSystemFiles.isEmpty())
	{
		qWarning() << "ERROR while loading ssystem_minor.ini (unable to find data/ssystem_minor.ini): " << endl;
		return;
	}

	for (const auto& solarSystemFile : solarSystemFiles)
	{
		if (loadPlanets(solarSystemFile))
		{
			qDebug() << "File ssystem_minor.ini is loaded successfully...";
			break;
		}
		else
		{
//			sun.clear();
//			moon.clear();
//			earth.clear();
			//qCritical() << "We should not be here!";

			qDebug() << "Removing minor bodies";
			for (const auto& p : systemPlanets)
			{
				// We can only delete minor objects now!
				if (p->pType >= Planet::isAsteroid)
				{
					p->satellites.clear();
				}
			}			
			systemPlanets.clear();			
			//Memory leak? What's the proper way of cleaning shared pointers?

			// TODO: 0.16pre what about the orbits list?

			//If the file is in the user data directory, rename it:
			if (solarSystemFile.contains(StelFileMgr::getUserDir()))
			{
				QString newName = QString("%1/data/ssystem-%2.ini").arg(StelFileMgr::getUserDir()).arg(QDateTime::currentDateTime().toString("yyyyMMddThhmmss"));
				if (QFile::rename(solarSystemFile, newName))
					qWarning() << "Invalid Solar System file" << QDir::toNativeSeparators(solarSystemFile) << "has been renamed to" << QDir::toNativeSeparators(newName);
				else
				{
					qWarning() << "Invalid Solar System file" << QDir::toNativeSeparators(solarSystemFile) << "cannot be removed!";
					qWarning() << "Please either delete it, rename it or move it elsewhere.";
				}
			}
		}
	}

	shadowPlanetCount = 0;

	for (const auto& planet : systemPlanets)
		if(planet->parent != sun || !planet->satellites.isEmpty())
			shadowPlanetCount++;
}

unsigned char SolarSystem::BvToColorIndex(float bV)
{
	double dBV = bV;
	dBV *= 1000.0;
	if (dBV < -500)
		dBV = -500;
	else if (dBV > 3499)
		dBV = 3499;

	return (unsigned int)floor(0.5+127.0*((500.0+dBV)/4000.0));
}

bool SolarSystem::loadPlanets(const QString& filePath)
{
	StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
	qDebug() << "Loading from :"  << filePath;
	int readOk = 0;
	QSettings pd(filePath, StelIniFormat);
	if (pd.status() != QSettings::NoError)
	{
		qWarning() << "ERROR while parsing" << QDir::toNativeSeparators(filePath);
		return false;
	}

	// QSettings does not allow us to say that the sections of the file
	// will be listed in the same order  as in the file like the old
	// InitParser used to so we can no longer assume that.
	//
	// This means we must first decide what order to read the sections
	// of the file in (each section contains one planet/moon/asteroid/comet/...) to avoid setting
	// the parent Planet* to one which has not yet been created.
	//
	// Stage 1: Make a map of body names back to the section names
	// which they come from. Also make a map of body name to parent body
	// name. These two maps can be made in a single pass through the
	// sections of the file.
	//
	// Stage 2: Make an ordered list of section names such that each
	// item is only ever dependent on items which appear earlier in the
	// list.
	// 2a: Make a QMultiMap relating the number of levels of dependency
	//     to the body name, i.e.
	//     0 -> Sun
	//     1 -> Mercury
	//     1 -> Venus
	//     1 -> Earth
	//     2 -> Moon
	//     etc.
	// 2b: Populate an ordered list of section names by iterating over
	//     the QMultiMap.  This type of container is always sorted on the
	//     key in ascending order, so it's easy.
	//     i.e. [sun, earth, moon] is fine, but not [sun, moon, earth]
	//
	// Stage 3: iterate over the ordered sections decided in stage 2,
	// creating the planet objects from the QSettings data.

	// Stage 1 (as described above).
	QMap<QString, QString> secNameMap;
	QMap<QString, QString> parentMap;
	QStringList sections = pd.childGroups();
	// qDebug() << "Stage 1: load ini file with" << sections.size() << "entries: "<< sections;
	for (int i=0; i<sections.size(); ++i)
	{
		const QString secname = sections.at(i);
		const QString englishName = pd.value(secname+"/name").toString();
		const QString strParent = pd.value(secname+"/parent", "Sun").toString();
		secNameMap[englishName] = secname;
		if (strParent!="none" && !strParent.isEmpty() && !englishName.isEmpty())
		{
			parentMap[englishName] = strParent;
			// qDebug() << "parentmap[" << englishName << "] = " << strParent;
		}
	}

	// Stage 2a (as described above).
	QMultiMap<int, QString> depLevelMap;
	for (int i=0; i<sections.size(); ++i)
	{
		const QString englishName = pd.value(sections.at(i)+"/name").toString();

		// follow dependencies, incrementing level when we have one
		// till we run out.
		QString p=englishName;
		int level = 0;
		while(parentMap.contains(p) && parentMap[p]!="none")
		{
			level++;
			p = parentMap[p];
		}

		depLevelMap.insert(level, secNameMap[englishName]);
		// qDebug() << "2a: Level" << level << "secNameMap[" << englishName << "]="<< secNameMap[englishName];
	}

	// Stage 2b (as described above).
	// qDebug() << "Stage 2b:";
	QStringList orderedSections;
	QMapIterator<int, QString> levelMapIt(depLevelMap);
	while(levelMapIt.hasNext())
	{
		levelMapIt.next();
		orderedSections << levelMapIt.value();
	}
	// qDebug() << orderedSections;

	// Stage 3 (as described above).
	//int readOk=0;
	//int totalPlanets=0;

	// qDebug() << "Adding " << orderedSections.size() << "objects...";
	for (int i = 0;i<orderedSections.size();++i)
	{
		// qDebug() << "Processing entry" << orderedSections.at(i);

		//totalPlanets++;
		const QString secname = orderedSections.at(i);
		const QString englishName = pd.value(secname+"/name").toString().simplified();
		const QString strParent = pd.value(secname+"/parent", "Sun").toString(); // Obvious default, keep file entries simple.
		PlanetP parent;
		if (strParent!="none")
		{
			// Look in the other planets the one named with strParent
			for (const auto& p : systemPlanets)
			{
				if (p->getEnglishName()==strParent)
				{
					parent = p;
					break;
				}
			}
			if (parent.isNull())
			{
				qWarning() << "ERROR : can't find parent solar system body for " << englishName;
				//abort();
				continue;
			}
		}

		const QString funcName = pd.value(secname+"/coord_func").toString();
		// qDebug() << "englishName:" << englishName << ", parent:" << strParent <<  ", coord_func:" << funcName;
		posFuncType posfunc=Q_NULLPTR;
		void* orbitPtr=Q_NULLPTR;
		OsculatingFunctType *osculatingFunc = Q_NULLPTR;
		bool closeOrbit = pd.value(secname+"/closeOrbit", true).toBool();

		if (funcName=="ell_orbit")
		{
			// GZ TODO: It seems ell_orbit is only used for planet moons. Just assert eccentricity<1 and remove a few extra calculations?
			// Read the orbital elements
			const double epoch = pd.value(secname+"/orbit_Epoch",J2000).toDouble();
			const double eccentricity = pd.value(secname+"/orbit_Eccentricity", 0.0).toDouble();
			if (eccentricity >= 1.0) closeOrbit = false;
			double pericenterDistance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			double semi_major_axis;
			if (pericenterDistance <= 0.0) {
				semi_major_axis = pd.value(secname+"/orbit_SemiMajorAxis",-1e100).toDouble();
				if (semi_major_axis <= -1e100) {
					qDebug() << "ERROR loading " << englishName
						 << ": you must provide orbit_PericenterDistance or orbit_SemiMajorAxis";
					continue;
				} else {
					semi_major_axis /= AU;
					Q_ASSERT(eccentricity != 1.0); // parabolic orbits have no semi_major_axis
					pericenterDistance = semi_major_axis * (1.0-eccentricity);
				}
			} else {
				pericenterDistance /= AU;
				semi_major_axis = (eccentricity == 1.0)
								? 0.0 // parabolic orbits have no semi_major_axis
								: pericenterDistance / (1.0-eccentricity);
			}
			double meanMotion = pd.value(secname+"/orbit_MeanMotion",-1e100).toDouble();
			double period;
			if (meanMotion <= -1e100) {
				period = pd.value(secname+"/orbit_Period",-1e100).toDouble();
				if (period <= -1e100) {
					meanMotion = (eccentricity == 1.0)
								? 0.01720209895 * (1.5/pericenterDistance) * std::sqrt(0.5/pericenterDistance)
								: (semi_major_axis > 0.0)
								? 0.01720209895 / (semi_major_axis*std::sqrt(semi_major_axis))
								: 0.01720209895 / (-semi_major_axis*std::sqrt(-semi_major_axis));
					period = 2.0*M_PI/meanMotion;
				} else {
					meanMotion = 2.0*M_PI/period;
				}
			} else {
				period = 2.0*M_PI/meanMotion;
			}
			const double inclination = pd.value(secname+"/orbit_Inclination", 0.0).toDouble()*(M_PI/180.0);
			const double ascending_node = pd.value(secname+"/orbit_AscendingNode", 0.0).toDouble()*(M_PI/180.0);
			double arg_of_pericenter = pd.value(secname+"/orbit_ArgOfPericenter",-1e100).toDouble();
			double long_of_pericenter;
			if (arg_of_pericenter <= -1e100) {
				long_of_pericenter = pd.value(secname+"/orbit_LongOfPericenter", 0.0).toDouble()*(M_PI/180.0);
				arg_of_pericenter = long_of_pericenter - ascending_node;
			} else {
				arg_of_pericenter *= (M_PI/180.0);
				long_of_pericenter = arg_of_pericenter + ascending_node;
			}
			double mean_anomaly = pd.value(secname+"/orbit_MeanAnomaly",-1e100).toDouble();
			double mean_longitude;
			if (mean_anomaly <= -1e100) {
				mean_longitude = pd.value(secname+"/orbit_MeanLongitude").toDouble()*(M_PI/180.0);
				mean_anomaly = mean_longitude - long_of_pericenter;
			} else {
				mean_anomaly *= (M_PI/180.0);
				mean_longitude = mean_anomaly + long_of_pericenter;
			}

			// when the parent is the sun use ecliptic rather than sun equator:
			const double parentRotObliquity  = parent->getParent() ? parent->getRotObliquity(J2000) : 0.0;
			const double parent_rot_asc_node = parent->getParent() ? parent->getRotAscendingNode()  : 0.0;
			double parent_rot_j2000_longitude = 0.0;
			if (parent->getParent()) {
				const double c_obl = cos(parentRotObliquity);
				const double s_obl = sin(parentRotObliquity);
				const double c_nod = cos(parent_rot_asc_node);
				const double s_nod = sin(parent_rot_asc_node);
				const Vec3d OrbitAxis0( c_nod,       s_nod,        0.0);
				const Vec3d OrbitAxis1(-s_nod*c_obl, c_nod*c_obl,s_obl);
				const Vec3d OrbitPole(  s_nod*s_obl,-c_nod*s_obl,c_obl);
				const Vec3d J2000Pole(StelCore::matJ2000ToVsop87.multiplyWithoutTranslation(Vec3d(0,0,1)));
				Vec3d J2000NodeOrigin(J2000Pole^OrbitPole);
				J2000NodeOrigin.normalize();
				parent_rot_j2000_longitude = atan2(J2000NodeOrigin*OrbitAxis1,J2000NodeOrigin*OrbitAxis0);
			}

			// Create an elliptical orbit
			EllipticalOrbit *orb = new EllipticalOrbit(pericenterDistance,     // [AU]
								   eccentricity,           // 0..>1, but practically only 0..1
								   inclination,            // [radians]
								   ascending_node,         // [radians]
								   arg_of_pericenter,      // [radians]
								   mean_anomaly,           // [radians]
								   period,                 // [days]
								   epoch,                  // [JDE]
								   parentRotObliquity,     // [radians]
								   parent_rot_asc_node,    // [radians]
								   parent_rot_j2000_longitude);
			orbits.push_back(orb);

			orbitPtr = orb;
			posfunc = &ellipticalOrbitPosFunc;
		}
		else if (funcName=="comet_orbit")
		{
			// Read the orbital elements
			// orbit_PericenterDistance,orbit_SemiMajorAxis: given in AU
			// orbit_MeanMotion: given in degrees/day
			// orbit_Period: given in days
			// orbit_TimeAtPericenter,orbit_Epoch: JD
			// orbit_MeanAnomaly,orbit_Inclination,orbit_ArgOfPericenter,orbit_AscendingNode: given in degrees
			const double eccentricity = pd.value(secname+"/orbit_Eccentricity",0.0).toDouble();
			if (eccentricity >= 1.0) closeOrbit = false;
			double pericenterDistance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			double semi_major_axis;
			if (pericenterDistance <= 0.0) {
				semi_major_axis = pd.value(secname+"/orbit_SemiMajorAxis",-1e100).toDouble();
				if (semi_major_axis <= -1e100) {
					qWarning() << "ERROR: " << englishName
						   << ": you must provide orbit_PericenterDistance or orbit_SemiMajorAxis";
					//abort();
					continue;
				} else {
					Q_ASSERT(eccentricity != 1.0); // parabolic orbits have no semi_major_axis
					pericenterDistance = semi_major_axis * (1.0-eccentricity);
				}
			} else {
				semi_major_axis = (eccentricity == 1.0)
								? 0.0 // parabolic orbits have no semi_major_axis
								: pericenterDistance / (1.0-eccentricity);
			}
			double meanMotion = pd.value(secname+"/orbit_MeanMotion",-1e100).toDouble();
			if (meanMotion <= -1e100) {
				const double period = pd.value(secname+"/orbit_Period",-1e100).toDouble();
				if (period <= -1e100) {
					if (parent->getParent()) {
						qWarning() << "ERROR: " << englishName
							   << ": when the parent body is not the sun, you must provide "
							   << "either orbit_MeanMotion or orbit_Period";
					} else {
						// in case of parent=sun: use Gaussian gravitational constant
						// for calculating meanMotion:
						//meanMotion = (eccentricity >= 0.9999 && eccentricity <= 1.0)
						//			? 0.01720209895 * (1.5/pericenterDistance) * sqrt(0.5/pericenterDistance)
						//			: (semi_major_axis > 0.0)
						//			? 0.01720209895 / (semi_major_axis*sqrt(semi_major_axis))
						//			: 0.01720209895 / (-semi_major_axis*sqrt(-semi_major_axis));
						meanMotion = (eccentricity == 1.0)
									? 0.01720209895 * (1.5/pericenterDistance) * std::sqrt(0.5/pericenterDistance)  // GZ: This is Heafner's W / dt
									: 0.01720209895 / (fabs(semi_major_axis)*std::sqrt(fabs(semi_major_axis)));
					}
				} else {
					meanMotion = 2.0*M_PI/period;
				}
			} else {
				meanMotion *= (M_PI/180.0);
			}
			double time_at_pericenter = pd.value(secname+"/orbit_TimeAtPericenter",-1e100).toDouble();
			if (time_at_pericenter <= -1e100) {
				const double epoch = pd.value(secname+"/orbit_Epoch",J2000).toDouble(); // prev. default -1e100
				double mean_anomaly = pd.value(secname+"/orbit_MeanAnomaly",-1e100).toDouble();
				if (epoch <= -1e100 || mean_anomaly <= -1e100) {
					qWarning() << "ERROR: " << englishName
						   << ": when you do not provide orbit_TimeAtPericenter, you must provide both "
						   << "orbit_Epoch and orbit_MeanAnomaly";
					//abort();
					continue;
				} else {
					mean_anomaly *= (M_PI/180.0);
					time_at_pericenter = epoch - mean_anomaly / meanMotion;
				}
			}
			const double orbitGoodDays=pd.value(secname+"/orbit_good", 1000).toDouble();
			const double inclination = pd.value(secname+"/orbit_Inclination", 0.0).toDouble()*(M_PI/180.0);
			const double arg_of_pericenter = pd.value(secname+"/orbit_ArgOfPericenter", 0.0).toDouble()*(M_PI/180.0);
			const double ascending_node = pd.value(secname+"/orbit_AscendingNode", 0.0).toDouble()*(M_PI/180.0);
			const double parentRotObliquity = parent->getParent() ? parent->getRotObliquity(2451545.0) : 0.0;
			const double parent_rot_asc_node = parent->getParent() ? parent->getRotAscendingNode() : 0.0;
			double parent_rot_j2000_longitude = 0.0;
						if (parent->getParent()) {
							const double c_obl = cos(parentRotObliquity);
							const double s_obl = sin(parentRotObliquity);
							const double c_nod = cos(parent_rot_asc_node);
							const double s_nod = sin(parent_rot_asc_node);
							const Vec3d OrbitAxis0( c_nod,       s_nod,        0.0);
							const Vec3d OrbitAxis1(-s_nod*c_obl, c_nod*c_obl,s_obl);
							const Vec3d OrbitPole(  s_nod*s_obl,-c_nod*s_obl,c_obl);
							const Vec3d J2000Pole(StelCore::matJ2000ToVsop87.multiplyWithoutTranslation(Vec3d(0,0,1)));
							Vec3d J2000NodeOrigin(J2000Pole^OrbitPole);
							J2000NodeOrigin.normalize();
							parent_rot_j2000_longitude = atan2(J2000NodeOrigin*OrbitAxis1,J2000NodeOrigin*OrbitAxis0);
						}
			//qDebug() << "Creating CometOrbit for" << englishName;
			CometOrbit *orb = new CometOrbit(pericenterDistance,
							 eccentricity,
							 inclination,
							 ascending_node,
							 arg_of_pericenter,
							 time_at_pericenter,
							 orbitGoodDays,
							 meanMotion,
							 parentRotObliquity,
							 parent_rot_asc_node,
							 parent_rot_j2000_longitude);
			orbits.push_back(orb);
			orbitPtr = orb;
			posfunc = &cometOrbitPosFunc;
		}

		else if (funcName=="sun_special")
			posfunc = &get_sun_helio_coordsv;

		else if (funcName=="mercury_special") {
			posfunc = &get_mercury_helio_coordsv;
			osculatingFunc = &get_mercury_helio_osculating_coords;
		}

		else if (funcName=="venus_special") {
			posfunc = &get_venus_helio_coordsv;
			osculatingFunc = &get_venus_helio_osculating_coords;
		}

		else if (funcName=="earth_special") {
			posfunc = &get_earth_helio_coordsv;
			osculatingFunc = &get_earth_helio_osculating_coords;
		}

		else if (funcName=="lunar_special")
			posfunc = &get_lunar_parent_coordsv;

		else if (funcName=="mars_special") {
			posfunc = &get_mars_helio_coordsv;
			osculatingFunc = &get_mars_helio_osculating_coords;
		}

		else if (funcName=="phobos_special")
			posfunc = &get_phobos_parent_coordsv;

		else if (funcName=="deimos_special")
			posfunc = &get_deimos_parent_coordsv;

		else if (funcName=="jupiter_special") {
			posfunc = &get_jupiter_helio_coordsv;
			osculatingFunc = &get_jupiter_helio_osculating_coords;
		}

		else if (funcName=="europa_special")
			posfunc = &get_europa_parent_coordsv;

		else if (funcName=="calisto_special")
			posfunc = &get_callisto_parent_coordsv;

		else if (funcName=="io_special")
			posfunc = &get_io_parent_coordsv;

		else if (funcName=="ganymede_special")
			posfunc = &get_ganymede_parent_coordsv;

		else if (funcName=="saturn_special") {
			posfunc = &get_saturn_helio_coordsv;
			osculatingFunc = &get_saturn_helio_osculating_coords;
		}

		else if (funcName=="mimas_special")
			posfunc = &get_mimas_parent_coordsv;

		else if (funcName=="enceladus_special")
			posfunc = &get_enceladus_parent_coordsv;

		else if (funcName=="tethys_special")
			posfunc = &get_tethys_parent_coordsv;

		else if (funcName=="dione_special")
			posfunc = &get_dione_parent_coordsv;

		else if (funcName=="rhea_special")
			posfunc = &get_rhea_parent_coordsv;

		else if (funcName=="titan_special")
			posfunc = &get_titan_parent_coordsv;

		else if (funcName=="iapetus_special")
			posfunc = &get_iapetus_parent_coordsv;

		else if (funcName=="hyperion_special")
			posfunc = &get_hyperion_parent_coordsv;

		else if (funcName=="uranus_special") {
			posfunc = &get_uranus_helio_coordsv;
			osculatingFunc = &get_uranus_helio_osculating_coords;
		}

		else if (funcName=="miranda_special")
			posfunc = &get_miranda_parent_coordsv;

		else if (funcName=="ariel_special")
			posfunc = &get_ariel_parent_coordsv;

		else if (funcName=="umbriel_special")
			posfunc = &get_umbriel_parent_coordsv;

		else if (funcName=="titania_special")
			posfunc = &get_titania_parent_coordsv;

		else if (funcName=="oberon_special")
			posfunc = &get_oberon_parent_coordsv;

		else if (funcName=="neptune_special") {
			posfunc = &get_neptune_helio_coordsv;
			osculatingFunc = &get_neptune_helio_osculating_coords;
		}

		else if (funcName=="pluto_special")
			posfunc = &get_pluto_helio_coordsv;


		if (posfunc==Q_NULLPTR)
		{
			qCritical() << "ERROR in section " << secname << ": can't find posfunc " << funcName << " for " << englishName;
			exit(-1);
		}

		// Create the Solar System body and add it to the list
		QString type = pd.value(secname+"/type").toString();

		//TODO: Refactor the subclass selection to reduce duplicate code mess here,
		// by at least using this base class pointer and using setXXX functions instead of mega-constructors
		// that have to pass most of it on to the Planet class
		PlanetP p;

		// New class objects, named "plutino", "cubewano", "dwarf planet", "SDO", "OCO", has properties
		// similar to asteroids and we should calculate their positions like for asteroids. Dwarf planets
		// have one exception: Pluto - we should use special function for calculation of orbit of Pluto.
		if ((type == "asteroid" || type == "dwarf planet" || type == "cubewano" || type == "plutino" || type == "scattered disc object" || type == "Oort cloud object" || type == "interstellar object") && !englishName.contains("Pluto"))
		{
			minorBodies << englishName;

			Vec3f color = Vec3f(1.f, 1.f, 1.f);
			float bV = pd.value(secname+"/color_index_bv", 99.f).toFloat();
			if (bV<99.f)
				color = skyDrawer->indexToColor(BvToColorIndex(bV))*0.75f;
			else
				color = StelUtils::strToVec3f(pd.value(secname+"/color", "1.0,1.0,1.0").toString());

			QString normalMapName = "";
			bool hidden = pd.value(secname+"/hidden", false).toBool();
			if (!hidden) // no normal maps for invisible objects!
				normalMapName = englishName.toLower().append("_normals.png");

			p = PlanetP(new MinorPlanet(englishName,
						    pd.value(secname+"/radius", 1.0).toDouble()/AU,
						    pd.value(secname+"/oblateness", 0.0).toDouble(),
						    color, // halo color
						    pd.value(secname+"/albedo", 0.25f).toFloat(),
						    pd.value(secname+"/roughness",0.9f).toFloat(),
						    pd.value(secname+"/tex_map", "nomap.png").toString(),
						    pd.value(secname+"/normals_map", normalMapName).toString(),
						    pd.value(secname+"/model").toString(),
						    posfunc,
						    orbitPtr,
						    osculatingFunc,
						    closeOrbit,
						    hidden,
						    type));

			QSharedPointer<MinorPlanet> mp =  p.dynamicCast<MinorPlanet>();

			//Number
			int minorPlanetNumber = pd.value(secname+"/minor_planet_number", 0).toInt();
			if (minorPlanetNumber)
			{
				mp->setMinorPlanetNumber(minorPlanetNumber);
			}

			//Provisional designation
			QString provisionalDesignation = pd.value(secname+"/provisional_designation").toString();
			if (!provisionalDesignation.isEmpty())
			{
				mp->setProvisionalDesignation(provisionalDesignation);
			}

			//H-G magnitude system
			const float magnitude = pd.value(secname+"/absolute_magnitude", -99).toFloat();
			const float slope = pd.value(secname+"/slope_parameter", 0.15).toFloat();
			if (magnitude > -99)
			{
				if (slope >= 0 && slope <= 1)
				{
					mp->setAbsoluteMagnitudeAndSlope(magnitude, slope);
				}
				else
				{
					mp->setAbsoluteMagnitudeAndSlope(magnitude, 0.15f);
				}
			}

			mp->setSemiMajorAxis(pd.value(secname+"/orbit_SemiMajorAxis", 0).toDouble());
			mp->setColorIndexBV(bV);
			mp->setSpectralType(pd.value(secname+"/spec_t", "").toString(), pd.value(secname+"/spec_b", "").toString());

			systemMinorBodies.push_back(p);
		}
		else if (type == "comet")
		{
			minorBodies << englishName;
			p = PlanetP(new Comet(englishName,
					      pd.value(secname+"/radius", 1.0).toDouble()/AU,
					      pd.value(secname+"/oblateness", 0.0).toDouble(),
					      StelUtils::strToVec3f(pd.value(secname+"/color", "1.0,1.0,1.0").toString()), // halo color
					      pd.value(secname+"/albedo", 0.25f).toFloat(),
					      pd.value(secname+"/roughness",0.9f).toFloat(),
					      pd.value(secname+"/outgas_intensity",0.1f).toFloat(),
					      pd.value(secname+"/outgas_falloff", 0.1f).toFloat(),
					      pd.value(secname+"/tex_map", "nomap.png").toString(),
					      pd.value(secname+"/model").toString(),
					      posfunc,
					      orbitPtr,
					      osculatingFunc,
					      closeOrbit,
					      pd.value(secname+"/hidden", false).toBool(),
					      type,
					      pd.value(secname+"/dust_widthfactor", 1.5f).toFloat(),
					      pd.value(secname+"/dust_lengthfactor", 0.4f).toFloat(),
					      pd.value(secname+"/dust_brightnessfactor", 1.5f).toFloat()
					      ));

			QSharedPointer<Comet> mp =  p.dynamicCast<Comet>();

			//g,k magnitude system
			const float magnitude = pd.value(secname+"/absolute_magnitude", -99).toFloat();
			const float slope = pd.value(secname+"/slope_parameter", 4.0f).toFloat();
			if (magnitude > -99)
			{
				if (slope >= 0 && slope <= 20)
				{
					mp->setAbsoluteMagnitudeAndSlope(magnitude, slope);
				}
				else
				{
					mp->setAbsoluteMagnitudeAndSlope(magnitude, 4.0f);
				}
			}

			const double eccentricity = pd.value(secname+"/orbit_Eccentricity",0.0).toDouble();
			const double pericenterDistance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			if (eccentricity<1 && pericenterDistance>0)
			{
				mp->setSemiMajorAxis(pericenterDistance / (1.0-eccentricity));
			}
			systemMinorBodies.push_back(p);
		}
		else
		{
			// Set possible default name of the normal map for avoiding yin-yang shaped moon
			// phase when normal map key not exists. Example: moon_normals.png
			// Details: https://bugs.launchpad.net/stellarium/+bug/1335609
			QString normalMapName = "";
			bool hidden = pd.value(secname+"/hidden", false).toBool();
			if (!hidden) // no normal maps for invisible objects!
				normalMapName = englishName.toLower().append("_normals.png");
			p = PlanetP(new Planet(englishName,
					       pd.value(secname+"/radius", 1.0).toDouble()/AU,
					       pd.value(secname+"/oblateness", 0.0).toDouble(),
					       StelUtils::strToVec3f(pd.value(secname+"/color", "1.0,1.0,1.0").toString()), // halo color
					       pd.value(secname+"/albedo", 0.25f).toFloat(),
					       pd.value(secname+"/roughness",0.9f).toFloat(),
					       pd.value(secname+"/tex_map", "nomap.png").toString(),
					       pd.value(secname+"/normals_map", normalMapName).toString(),
					       pd.value(secname+"/model").toString(),
					       posfunc,
					       orbitPtr,
					       osculatingFunc,
					       closeOrbit,
					       hidden,
					       pd.value(secname+"/atmosphere", false).toBool(),
					       pd.value(secname+"/halo", true).toBool(),          // GZ new default. Avoids clutter in ssystem.ini.
					       type));
			p->absoluteMagnitude = pd.value(secname+"/absolute_magnitude", -99.).toFloat();

			// Moon designation (planet index + IAU moon number)
			QString moonDesignation = pd.value(secname+"/iau_moon_number", "").toString();
			if (!moonDesignation.isEmpty())
			{
				p->setIAUMoonNumber(moonDesignation);
			}
		}


		if (!parent.isNull())
		{
			parent->satellites.append(p);
			p->parent = parent;
		}
		if (secname=="earth") earth = p;
		if (secname=="sun") sun = p;
		if (secname=="moon") moon = p;

		float rotObliquity = pd.value(secname+"/rot_obliquity",0.).toFloat()*(M_PI_180f);
		float rotAscNode = pd.value(secname+"/rot_equator_ascending_node",0.).toFloat()*(M_PI_180f);

		// Use more common planet North pole data if available
		// NB: N pole as defined by IAU (NOT right hand rotation rule)
		// NB: J2000 epoch
		const double J2000NPoleRA = pd.value(secname+"/rot_pole_ra", 0.).toDouble()*M_PI/180.;
		const double J2000NPoleDE = pd.value(secname+"/rot_pole_de", 0.).toDouble()*M_PI/180.;

		if(J2000NPoleRA || J2000NPoleDE)
		{
			Vec3d J2000NPole;
			StelUtils::spheToRect(J2000NPoleRA,J2000NPoleDE,J2000NPole);

			Vec3d vsop87Pole(StelCore::matJ2000ToVsop87.multiplyWithoutTranslation(J2000NPole));

			float ra, de;
			StelUtils::rectToSphe(&ra, &de, vsop87Pole);

			rotObliquity = (M_PI_2f - de);
			rotAscNode = (ra + M_PI_2f);

			// qDebug() << "\tCalculated rotational obliquity: " << rotObliquity*180./M_PI << endl;
			// qDebug() << "\tCalculated rotational ascending node: " << rotAscNode*180./M_PI << endl;
		}

		// rot_periode given in hours, or orbit_Period given in days, orbit_visualization_period in days. The latter should have a meaningful default.
		p->setRotationElements(
			pd.value(secname+"/rot_periode", pd.value(secname+"/orbit_Period", 1.).toDouble()*24.).toFloat()/24.f,
			pd.value(secname+"/rot_rotation_offset",0.).toFloat(),
			pd.value(secname+"/rot_epoch", J2000).toDouble(),
			rotObliquity,
			rotAscNode,
			pd.value(secname+"/rot_precession_rate",0.).toFloat()*M_PIf/(180*36525),
			pd.value(secname+"/orbit_visualization_period", fabs(pd.value(secname+"/orbit_Period", 1.).toDouble())).toDouble()); // this is given in days...


		if (pd.value(secname+"/rings", 0).toBool()) {
			const float rMin = pd.value(secname+"/ring_inner_size").toFloat()/AUf;
			const float rMax = pd.value(secname+"/ring_outer_size").toFloat()/AUf;
			Ring *r = new Ring(rMin,rMax,pd.value(secname+"/tex_ring").toString());
			p->setRings(r);
		}

		systemPlanets.push_back(p);
		readOk++;
	}

	if (systemPlanets.isEmpty())
	{
		qWarning() << "No Solar System objects loaded from" << QDir::toNativeSeparators(filePath);
		return false;
	}

	// special case: load earth shadow texture
	if (!Planet::texEarthShadow)
		Planet::texEarthShadow = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/earth-shadow.png");

	// Also comets just have static textures.
	if (!Comet::comaTexture)
		Comet::comaTexture = StelApp::getInstance().getTextureManager().createTextureThread(StelFileMgr::getInstallationDir()+"/textures/cometComa.png", StelTexture::StelTextureParams(true, GL_LINEAR, GL_CLAMP_TO_EDGE));
	//tail textures. We use paraboloid tail bodies, textured like a fisheye sphere, i.e. center=head. The texture should be something like a mottled star to give some structure.
	if (!Comet::tailTexture)
		Comet::tailTexture = StelApp::getInstance().getTextureManager().createTextureThread(StelFileMgr::getInstallationDir()+"/textures/cometTail.png", StelTexture::StelTextureParams(true, GL_LINEAR, GL_CLAMP_TO_EDGE));

	if (readOk>0)
		qDebug() << "Loaded" << readOk << "Solar System bodies";

	return true;
}

// Compute the position for every elements of the solar system.
// The order is not important since the position is computed relatively to the mother body
void SolarSystem::computePositions(double dateJDE, PlanetP observerPlanet)
{
	if (flagLightTravelTime)
	{
		for (const auto& p : systemPlanets)
		{
			p->computePosition(dateJDE);
		}
		// BEGIN HACK: 0.16.0post for solar aberration/light time correction
		// This fixes eclipse bug LP:#1275092) and outer planet rendering bug (LP:#1699648) introduced by the first fix in 0.16.0.
		// We compute a "light time corrected position" for the sun and apply it only for rendering, not for other computations.
		// A complete solution should likely "just" implement aberration for all objects.
		const Vec3d obsPosJDE=observerPlanet->getHeliocentricEclipticPos();
		const double obsDist=obsPosJDE.length();

		observerPlanet->computePosition(dateJDE-obsDist * (AU / (SPEED_OF_LIGHT * 86400.)));
		const Vec3d obsPosJDEbefore=observerPlanet->getHeliocentricEclipticPos();
		lightTimeSunPosition=obsPosJDE-obsPosJDEbefore;

		// We must reset observerPlanet for the next step!
		observerPlanet->computePosition(dateJDE);
		// END HACK FOR SOLAR LIGHT TIME/ABERRATION
		for (const auto& p : systemPlanets)
		{
			const double light_speed_correction = (p->getHeliocentricEclipticPos()-obsPosJDE).length() * (AU / (SPEED_OF_LIGHT * 86400.));
			p->computePosition(dateJDE-light_speed_correction);
		}
	}
	else
	{
		for (const auto& p : systemPlanets)
		{
			p->computePosition(dateJDE);
		}
		lightTimeSunPosition.set(0.,0.,0.);
	}
	computeTransMatrices(dateJDE, observerPlanet->getHeliocentricEclipticPos());
}

// Compute the transformation matrix for every elements of the solar system.
// The elements have to be ordered hierarchically, eg. it's important to compute earth before moon.
void SolarSystem::computeTransMatrices(double dateJDE, const Vec3d& observerPos)
{
	double dateJD=dateJDE - (StelApp::getInstance().getCore()->computeDeltaT(dateJDE))/86400.0;

	if (flagLightTravelTime)
	{
		for (const auto& p : systemPlanets)
		{
			const double light_speed_correction = (p->getHeliocentricEclipticPos()-observerPos).length() * (AU / (SPEED_OF_LIGHT * 86400));
			p->computeTransMatrix(dateJD-light_speed_correction, dateJDE-light_speed_correction);
		}
	}
	else
	{
		for (const auto& p : systemPlanets)
		{
			p->computeTransMatrix(dateJD, dateJDE);
		}
	}
}

// And sort them from the furthest to the closest to the observer
struct biggerDistance : public std::binary_function<PlanetP, PlanetP, bool>
{
	bool operator()(PlanetP p1, PlanetP p2)
	{
		return p1->getDistance() > p2->getDistance();
	}
};

// Draw all the elements of the solar system
// We are supposed to be in heliocentric coordinate
void SolarSystem::draw(StelCore* core)
{
	if (!flagShow)
		return;

	// Compute each Planet distance to the observer
	const Vec3d obsHelioPos = core->getObserverHeliocentricEclipticPos();

	for (const auto& p : systemPlanets)
	{
		p->computeDistance(obsHelioPos);
	}

	// And sort them from the furthest to the closest
	sort(systemPlanets.begin(),systemPlanets.end(),biggerDistance());

	if (trailFader.getInterstate()>0.0000001f)
	{
		StelPainter sPainter(core->getProjection2d());
		allTrails->setOpacity(trailFader.getInterstate());
		allTrails->draw(core, &sPainter);
	}

	// Make some voodoo to determine when labels should be displayed
	const float sdLimitMag=static_cast<float>(core->getSkyDrawer()->getLimitMagnitude());
	const float maxMagLabel = (sdLimitMag<5.f ? sdLimitMag :
			5.f+(sdLimitMag-5.f)*1.2f) +(static_cast<float>(labelsAmount)-3.f)*1.2f;

	// Draw the elements
	for (const auto& p : systemPlanets)
	{
		p->draw(core, maxMagLabel, planetNameFont);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer() && getFlagPointer())
		drawPointer(core);

	// AstroCalcDialog
	if (getFlagEphemerisMarkers())
		drawEphemerisMarkers(core);		

	if (getFlagEphemerisLine())
		drawEphemerisLine(core);
}

Vec3f SolarSystem::getEphemerisMarkerColor(int index) const
{
	// Sync index with AstroCalcDialog::generateEphemeris()
	Vec3f colorMarker = getEphemerisGenericMarkerColor();
	switch (index)
	{
		case 0:
			colorMarker = getEphemerisGenericMarkerColor();
			break;
		case 1:
			colorMarker = getEphemerisMercuryMarkerColor();
			break;
		case 2:
			colorMarker = getEphemerisVenusMarkerColor();
			break;
		case 3:
			colorMarker = getEphemerisMarsMarkerColor();
			break;
		case 4:
			colorMarker = getEphemerisJupiterMarkerColor();
			break;
		case 5:
			colorMarker = getEphemerisSaturnMarkerColor();
			break;
	}
	return colorMarker;
}

void SolarSystem::drawEphemerisMarkers(const StelCore *core)
{
	int fsize = AstroCalcDialog::EphemerisList.count();
	if (fsize>0) // The array of data is not empty - good news!
	{
		StelProjectorP prj;
		if (getFlagEphemerisHorizontalCoordinates())
			prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		else
			prj = core->getProjection(StelCore::FrameJ2000);
		StelPainter sPainter(prj);

		float size, shift, baseSize = 4.f;
		bool showDates = getFlagEphemerisDates();
		bool showMagnitudes = getFlagEphemerisMagnitudes();
		bool showSkippedData = getFlagEphemerisSkipData();
		int dataStep = getEphemerisDataStep();
		QString info = "";
		Vec3d win;
		Vec3f colorMarker;

		if (getFlagEphemerisLine())
			baseSize = 2.f;

		for (int i =0; i < fsize; i++)
		{
			// Check visibility of pointer
			if (!(sPainter.getProjector()->projectCheck(AstroCalcDialog::EphemerisList[i].coord, win)))
				continue;

			if (i == AstroCalcDialog::DisplayedPositionIndex)
			{
				colorMarker = getEphemerisSelectedMarkerColor();
				size = 6.f;
			}
			else
			{
				colorMarker = getEphemerisMarkerColor(AstroCalcDialog::EphemerisList[i].colorIndex);
				size = baseSize;
			}
			sPainter.setColor(colorMarker[0], colorMarker[1], colorMarker[2], 1.0f);
			sPainter.setBlending(true, GL_ONE, GL_ONE);
			texCircle->bind();
			sPainter.drawSprite2dMode(AstroCalcDialog::EphemerisList[i].coord, size);

			if (showDates || showMagnitudes)
			{
				if (showSkippedData && ((i + 1)%dataStep)!=1 && dataStep!=1)
					continue;

				shift = 3.f + size/1.6f;
				if (showDates && showMagnitudes)
					info = QString("%1 (%2)").arg(AstroCalcDialog::EphemerisList[i].objDateStr, QString::number(AstroCalcDialog::EphemerisList[i].magnitude, 'f', 2));
				if (showDates && !showMagnitudes)
					info = AstroCalcDialog::EphemerisList[i].objDateStr;
				if (!showDates && showMagnitudes)
					info = QString::number(AstroCalcDialog::EphemerisList[i].magnitude, 'f', 2);

				sPainter.drawText(AstroCalcDialog::EphemerisList[i].coord, info, 0, shift, shift, false);				
			}
		}
	}
}

void SolarSystem::drawEphemerisLine(const StelCore *core)
{
	int size = AstroCalcDialog::EphemerisList.count();
	if (size>0) // The array of data is not empty - good news!
	{
		StelProjectorP prj;
		if (getFlagEphemerisHorizontalCoordinates())
			prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		else
			prj = core->getProjection(StelCore::FrameJ2000);
		StelPainter sPainter(prj);

		if (size>=3)
		{
			Vec3f color;
			QVector<Vec3d> vertexArray;
			QVector<Vec4f> colorArray;
			if (AstroCalcDialog::EphemerisList[0].colorIndex!=AstroCalcDialog::EphemerisList[size-1].colorIndex)
			{
				// Oops... the color of first 3 items are different - looks like we got 5 planets on sky!
				int nsize = static_cast<int>(size/5);
				vertexArray.resize(nsize);
				colorArray.resize(nsize);
				for (int j=0; j<5; j++)
				{
					for (int i =0; i < nsize; i++)
					{
						color = getEphemerisMarkerColor(AstroCalcDialog::EphemerisList[i + j*nsize].colorIndex);
						colorArray[i].set(color[0], color[1], color[2], 1.f);
						vertexArray[i]=AstroCalcDialog::EphemerisList[i + j*nsize].coord;
					}
					sPainter.drawPath(vertexArray, colorArray);
				}
			}
			else
			{
				vertexArray.resize(size);
				colorArray.resize(size);
				for (int i =0; i < size; i++)
				{
					color = getEphemerisMarkerColor(AstroCalcDialog::EphemerisList[i].colorIndex);
					colorArray[i].set(color[0], color[1], color[2], 1.f);
					vertexArray[i]=AstroCalcDialog::EphemerisList[i].coord;
				}
				sPainter.drawPath(vertexArray, colorArray);
			}
		}
	}
}

void SolarSystem::fillEphemerisDates()
{
	int fsize = AstroCalcDialog::EphemerisList.count();
	if (fsize>0) // The array of data is not empty - good news!
	{
		StelLocaleMgr* localeMgr = &StelApp::getInstance().getLocaleMgr();
		bool showSmartDates = getFlagEphemerisSmartDates();
		double JD = AstroCalcDialog::EphemerisList.first().objDate;
		bool withTime = false;
		if (fsize>1 && (AstroCalcDialog::EphemerisList[1].objDate-JD<1.0))
			withTime = true;

		int fYear, fMonth, fDay, sYear, sMonth, sDay, h, m, s;
		QString info;
		const double shift = StelApp::getInstance().getCore()->getUTCOffset(JD)*0.041666666666;
		StelUtils::getDateFromJulianDay(JD+shift, &fYear, &fMonth, &fDay);
		bool sFlag = true;
		sYear = fYear;
		sMonth = fMonth;
		sDay = fDay;
		bool showSkippedData = getFlagEphemerisSkipData();
		int dataStep = getEphemerisDataStep();

		for (int i = 0; i < fsize; i++)
		{
			JD = AstroCalcDialog::EphemerisList[i].objDate;
			StelUtils::getDateFromJulianDay(JD+shift, &fYear, &fMonth, &fDay);

			if (showSkippedData && ((i + 1)%dataStep)!=1 && dataStep!=1)
				continue;

			if (showSmartDates)
			{
				if (sFlag)
					info = QString("%1").arg(fYear);

				if (info.isEmpty() && !sFlag && fYear!=sYear)
					info = QString("%1").arg(fYear);

				if (!info.isEmpty())
					info.append(QString("/%1").arg(localeMgr->romanMonthName(fMonth)));
				else if (fMonth!=sMonth)
					info = QString("%1").arg(localeMgr->romanMonthName(fMonth));

				if (!info.isEmpty())
					info.append(QString("/%1").arg(fDay));
				else
					info = QString("%1").arg(fDay);

				if (withTime) // very short step
				{
					if (fDay==sDay && !sFlag)
						info.clear();

					StelUtils::getTimeFromJulianDay(JD+shift, &h, &m, &s);
					if (!info.isEmpty())
						info.append(QString(" %1:%2").arg(h).arg(m));
					else
						info = QString("%1:%2").arg(h).arg(m);
				}

				AstroCalcDialog::EphemerisList[i].objDateStr = info;
				info.clear();
				sYear = fYear;
				sMonth = fMonth;
				sDay = fDay;
				sFlag = false;
			}
			else
			{
				// OK, let's use standard formats for date and time (as defined for whole planetarium)
				if (withTime)
					AstroCalcDialog::EphemerisList[i].objDateStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD));
				else
					AstroCalcDialog::EphemerisList[i].objDateStr = localeMgr->getPrintableDateLocal(JD);
			}
		}
	}
}

PlanetP SolarSystem::searchByEnglishName(QString planetEnglishName) const
{
	for (const auto& p : systemPlanets)
	{
		if (p->getEnglishName().toUpper() == planetEnglishName.toUpper() || p->getCommonEnglishName().toUpper() == planetEnglishName.toUpper())
			return p;
	}
	return PlanetP();
}

PlanetP SolarSystem::searchMinorPlanetByEnglishName(QString planetEnglishName) const
{
	for (const auto& p : systemMinorBodies)
	{
		if (p->getCommonEnglishName().toUpper() == planetEnglishName.toUpper() || p->getEnglishName().toUpper() == planetEnglishName.toUpper())
			return p;
	}
	return PlanetP();
}


StelObjectP SolarSystem::searchByNameI18n(const QString& planetNameI18) const
{
	for (const auto& p : systemPlanets)
	{
		if (p->getNameI18n().toUpper() == planetNameI18.toUpper())
			return qSharedPointerCast<StelObject>(p);
	}
	return StelObjectP();
}


StelObjectP SolarSystem::searchByName(const QString& name) const
{
	for (const auto& p : systemPlanets)
	{
		if (p->getEnglishName().toUpper() == name.toUpper() || p->getCommonEnglishName().toUpper() == name.toUpper())
			return qSharedPointerCast<StelObject>(p);
	}
	return StelObjectP();
}

float SolarSystem::getPlanetVMagnitude(QString planetName, bool withExtinction) const
{
	PlanetP p = searchByEnglishName(planetName);
	if (p.isNull()) // Possible was asked the common name of minor planet?
		p = searchMinorPlanetByEnglishName(planetName);
	float r = 0.f;
	if (withExtinction)
		r = p->getVMagnitudeWithExtinction(StelApp::getInstance().getCore());
	else
		r = p->getVMagnitude(StelApp::getInstance().getCore());
	return r;
}

QString SolarSystem::getPlanetType(QString planetName) const
{
	PlanetP p = searchByEnglishName(planetName);
	if (p.isNull()) // Possible was asked the common name of minor planet?
		p = searchMinorPlanetByEnglishName(planetName);
	return p->getPlanetTypeString();
}

double SolarSystem::getDistanceToPlanet(QString planetName) const
{
	PlanetP p = searchByEnglishName(planetName);
	if (p.isNull()) // Possible was asked the common name of minor planet?
		p = searchMinorPlanetByEnglishName(planetName);
	return p->getDistance();
}

double SolarSystem::getElongationForPlanet(QString planetName) const
{
	PlanetP p = searchByEnglishName(planetName);
	if (p.isNull()) // Possible was asked the common name of minor planet?
		p = searchMinorPlanetByEnglishName(planetName);
	return p->getElongation(StelApp::getInstance().getCore()->getObserverHeliocentricEclipticPos());
}

double SolarSystem::getPhaseAngleForPlanet(QString planetName) const
{
	PlanetP p = searchByEnglishName(planetName);
	if (p.isNull()) // Possible was asked the common name of minor planet?
		p = searchMinorPlanetByEnglishName(planetName);
	return p->getPhaseAngle(StelApp::getInstance().getCore()->getObserverHeliocentricEclipticPos());
}

float SolarSystem::getPhaseForPlanet(QString planetName) const
{
	PlanetP p = searchByEnglishName(planetName);
	if (p.isNull()) // Possible was asked the common name of minor planet?
		p = searchMinorPlanetByEnglishName(planetName);
	return p->getPhase(StelApp::getInstance().getCore()->getObserverHeliocentricEclipticPos());
}

QStringList SolarSystem::getObjectsList(QString objType) const
{
	QStringList r;
	if (objType.toLower()=="all")
	{
		r = listAllObjects(true);
		// Remove the Sun
		r.removeOne("Sun");
		// Remove special objects
		r.removeOne("Solar System Observer");
		r.removeOne("Earth Observer");
		r.removeOne("Mars Observer");
		r.removeOne("Jupiter Observer");
		r.removeOne("Saturn Observer");
		r.removeOne("Uranus Observer");
		r.removeOne("Neptune Observer");
	}
	else
		r = listAllObjectsByType(objType, true);

	return r;
}

// Search if any Planet is close to position given in earth equatorial position and return the distance
StelObjectP SolarSystem::search(Vec3d pos, const StelCore* core) const
{
	pos.normalize();
	PlanetP closest;
	double cos_angle_closest = 0.;
	Vec3d equPos;

	for (const auto& p : systemPlanets)
	{
		equPos = p->getEquinoxEquatorialPos(core);
		equPos.normalize();
		double cos_ang_dist = equPos*pos;
		if (cos_ang_dist>cos_angle_closest)
		{
			closest = p;
			cos_angle_closest = cos_ang_dist;
		}
	}

	if (cos_angle_closest>0.999)
	{
		return qSharedPointerCast<StelObject>(closest);
	}
	else return StelObjectP();
}

// Return a stl vector containing the planets located inside the limFov circle around position v
QList<StelObjectP> SolarSystem::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!getFlagPlanets())
		return result;

	Vec3d v = core->j2000ToEquinoxEqu(vv, StelCore::RefractionOff);
	v.normalize();
	double cosLimFov = std::cos(limitFov * M_PI/180.);
	Vec3d equPos;
	double cosAngularSize;

	QString weAreHere = core->getCurrentPlanet()->getEnglishName();
	for (const auto& p : systemPlanets)
	{
		equPos = p->getEquinoxEquatorialPos(core);
		equPos.normalize();

		cosAngularSize = std::cos(p->getSpheroidAngularSize(core) * M_PI/180.);

		if (equPos*v>=std::min(cosLimFov, cosAngularSize) && p->getEnglishName()!=weAreHere)
		{
			result.append(qSharedPointerCast<StelObject>(p));
		}
	}
	return result;
}

// Update i18 names from english names according to current sky culture translator
void SolarSystem::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	for (const auto& p : systemPlanets)
		p->translateName(trans);
}

void SolarSystem::setFlagTrails(bool b)
{
	if (getFlagTrails() != b)
	{
		trailFader = b;
		if (getFlagIsolatedTrails())
			recreateTrails();
		if (b)
			allTrails->reset();
		emit trailsDisplayedChanged(b);
	}
}

bool SolarSystem::getFlagTrails() const
{
	return (bool)trailFader;
}

void SolarSystem::setFlagHints(bool b)
{
	if (getFlagHints() != b)
	{
		for (const auto& p : systemPlanets)
			p->setFlagHints(b);
		emit flagHintsChanged(b);
	}
}

bool SolarSystem::getFlagHints(void) const
{
	for (const auto& p : systemPlanets)
	{
		if (p->getFlagHints())
			return true;
	}
	return false;
}

void SolarSystem::setFlagLabels(bool b)
{
	if (getFlagLabels() != b)
	{
		for (const auto& p : systemPlanets)
			p->setFlagLabels(b);
		emit labelsDisplayedChanged(b);
	}
}

bool SolarSystem::getFlagLabels() const
{
	for (const auto& p : systemPlanets)
	{
		if (p->getFlagLabels())
			return true;
	}
	return false;
}

void SolarSystem::setFlagOrbits(bool b)
{
	bool old = flagOrbits;
	flagOrbits = b;
	bool flagPlanetsOnly = getFlagPlanetsOrbitsOnly();
	if (!b || !selected || selected==sun)
	{
		if (flagPlanetsOnly)
		{
			for (const auto& p : systemPlanets)
			{
				if (p->getPlanetType()==Planet::isPlanet)
					p->setFlagOrbits(b);
				else
					p->setFlagOrbits(false);
			}
		}
		else
		{
			for (const auto& p : systemPlanets)
				p->setFlagOrbits(b);
		}
	}
	else if (getFlagIsolatedOrbits()) // If a Planet is selected and orbits are on, fade out non-selected ones
	{
		if (flagPlanetsOnly)
		{
			for (const auto& p : systemPlanets)
			{
				if (selected == p && p->getPlanetType()==Planet::isPlanet)
					p->setFlagOrbits(b);
				else
					p->setFlagOrbits(false);
			}
		}
		else
		{
			for (const auto& p : systemPlanets)
			{
				if (selected == p)
					p->setFlagOrbits(b);
				else
					p->setFlagOrbits(false);
			}
		}
	}
	else
	{
		// A planet is selected and orbits are on - draw orbits for the planet and their moons
		for (const auto& p : systemPlanets)
		{
			if (selected == p || selected == p->parent)
				p->setFlagOrbits(b);
			else
				p->setFlagOrbits(false);
		}
	}
	if(old != flagOrbits)
		emit flagOrbitsChanged(flagOrbits);
}

void SolarSystem::setFlagLightTravelTime(bool b)
{
	if(b!=flagLightTravelTime)
	{
		flagLightTravelTime = b;
		emit flagLightTravelTimeChanged(b);
	}
}

void SolarSystem::setFlagShowObjSelfShadows(bool b)
{
	if(b!=flagShowObjSelfShadows)
	{
		flagShowObjSelfShadows = b;
		if(!b)
			Planet::deinitFBO();
		emit flagShowObjSelfShadowsChanged(b);
	}
}

void SolarSystem::setSelected(PlanetP obj)
{
	if (obj && obj->getType() == "Planet")
	{
		selected = obj;
		selectedSSO.push_back(obj);
	}
	else
		selected.clear();
	// Undraw other objects hints, orbit, trails etc..
	setFlagHints(getFlagHints());
	setFlagOrbits(getFlagOrbits());
}


void SolarSystem::update(double deltaTime)
{
	trailFader.update(deltaTime*1000);
	if (trailFader.getInterstate()>0.f)
	{
		allTrails->update();
	}

	for (const auto& p : systemPlanets)
	{
		p->update(static_cast<int>(deltaTime*1000));
	}
}

// is a lunar eclipse close at hand?
bool SolarSystem::nearLunarEclipse() const
{
	// TODO: could replace with simpler test

	Vec3d e = getEarth()->getEclipticPos();
	Vec3d m = getMoon()->getEclipticPos();  // relative to earth
	Vec3d mh = getMoon()->getHeliocentricEclipticPos();  // relative to sun

	// shadow location at earth + moon distance along earth vector from sun
	Vec3d en = e;
	en.normalize();
	Vec3d shadow = en * (e.length() + m.length());

	// find shadow radii in AU
	double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000/AU;

	// modify shadow location for scaled moon
	Vec3d mdist = shadow - mh;
	if(mdist.length() > r_penumbra + 2000/AU) return false;   // not visible so don't bother drawing

	return true;
}

QStringList SolarSystem::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		for (const auto& p : systemPlanets)
		{
			result << p->getEnglishName();
		}
	}
	else
	{
		for (const auto& p : systemPlanets)
		{
			result << p->getNameI18n();
		}
	}
	return result;
}

QStringList SolarSystem::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		for (const auto& p : systemPlanets)
		{
			if (p->getPlanetTypeString()==objType)
				result << p->getEnglishName();
		}
	}
	else
	{
		for (const auto& p : systemPlanets)
		{
			if (p->getPlanetTypeString()==objType)
				result << p->getNameI18n();
		}
	}
	return result;
}

void SolarSystem::selectedObjectChange(StelModule::StelModuleSelectAction)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Planet");
	if (!newSelected.empty())
	{
		setSelected(qSharedPointerCast<Planet>(newSelected[0]));
		if (getFlagIsolatedTrails())
			recreateTrails();
	}
	else
		setSelected("");
}

// Activate/Deactivate planets display
void SolarSystem::setFlagPlanets(bool b)
{
	if (b!=flagShow)
	{
		flagShow=b;
		emit flagPlanetsDisplayedChanged(b);
	}
}

bool SolarSystem::getFlagPlanets(void) const
{
	return flagShow;
}

void SolarSystem::setFlagEphemerisMarkers(bool b)
{
	if (b!=ephemerisMarkersDisplayed)
	{
		ephemerisMarkersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_markers", b); // Immediate saving of state
		emit ephemerisMarkersChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisMarkers() const
{
	return ephemerisMarkersDisplayed;
}

void SolarSystem::setFlagEphemerisLine(bool b)
{
	if (b!=ephemerisLineDisplayed)
	{
		ephemerisLineDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_line", b); // Immediate saving of state
		emit ephemerisLineChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisLine() const
{
	return ephemerisLineDisplayed;
}

void SolarSystem::setFlagEphemerisHorizontalCoordinates(bool b)
{
	if (b!=ephemerisHorizontalCoordinates)
	{
		ephemerisHorizontalCoordinates=b;
		conf->setValue("astrocalc/flag_ephemeris_horizontal", b); // Immediate saving of state
		emit ephemerisHorizontalCoordinatesChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisHorizontalCoordinates() const
{
	return ephemerisHorizontalCoordinates;
}

void SolarSystem::setFlagEphemerisDates(bool b)
{
	if (b!=ephemerisDatesDisplayed)
	{
		ephemerisDatesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_dates", b); // Immediate saving of state
		emit ephemerisDatesChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisDates() const
{
	return ephemerisDatesDisplayed;
}

void SolarSystem::setFlagEphemerisMagnitudes(bool b)
{
	if (b!=ephemerisMagnitudesDisplayed)
	{
		ephemerisMagnitudesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_magnitudes", b); // Immediate saving of state
		emit ephemerisMagnitudesChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisMagnitudes() const
{
	return ephemerisMagnitudesDisplayed;
}

void SolarSystem::setFlagEphemerisSkipData(bool b)
{
	if (b!=ephemerisSkipDataDisplayed)
	{
		ephemerisSkipDataDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_skip_data", b); // Immediate saving of state
		emit ephemerisSkipDataChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisSkipData() const
{
	return ephemerisSkipDataDisplayed;
}

void SolarSystem::setFlagEphemerisSmartDates(bool b)
{
	if (b!=ephemerisSmartDatesDisplayed)
	{
		ephemerisSmartDatesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_smart_dates", b); // Immediate saving of state
		emit ephemerisSmartDatesChanged(b);
	}
}

bool SolarSystem::getFlagEphemerisSmartDates() const
{
	return ephemerisSmartDatesDisplayed;
}

void SolarSystem::setEphemerisDataStep(int step)
{
	ephemerisDataStep = step;
	// automatic saving of the setting
	conf->setValue("astrocalc/ephemeris_data_step", step);
	emit ephemerisDataStepChanged(step);
}

int SolarSystem::getEphemerisDataStep() const
{
	return ephemerisDataStep;
}

void SolarSystem::setEphemerisGenericMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisGenericMarkerColor)
	{
		ephemerisGenericMarkerColor = color;
		emit ephemerisGenericMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisGenericMarkerColor() const
{
	return ephemerisGenericMarkerColor;
}

void SolarSystem::setEphemerisSelectedMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisSelectedMarkerColor)
	{
		ephemerisSelectedMarkerColor = color;
		emit ephemerisSelectedMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisSelectedMarkerColor() const
{
	return ephemerisSelectedMarkerColor;
}

void SolarSystem::setEphemerisMercuryMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisMercuryMarkerColor)
	{
		ephemerisMercuryMarkerColor = color;
		emit ephemerisMercuryMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisMercuryMarkerColor() const
{
	return ephemerisMercuryMarkerColor;
}

void SolarSystem::setEphemerisVenusMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisVenusMarkerColor)
	{
		ephemerisVenusMarkerColor = color;
		emit ephemerisVenusMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisVenusMarkerColor() const
{
	return ephemerisVenusMarkerColor;
}

void SolarSystem::setEphemerisMarsMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisMarsMarkerColor)
	{
		ephemerisMarsMarkerColor = color;
		emit ephemerisMarsMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisMarsMarkerColor() const
{
	return ephemerisMarsMarkerColor;
}

void SolarSystem::setEphemerisJupiterMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisJupiterMarkerColor)
	{
		ephemerisJupiterMarkerColor = color;
		emit ephemerisJupiterMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisJupiterMarkerColor() const
{
	return ephemerisJupiterMarkerColor;
}

void SolarSystem::setEphemerisSaturnMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisSaturnMarkerColor)
	{
		ephemerisSaturnMarkerColor = color;
		emit ephemerisSaturnMarkerColorChanged(color);
	}
}

Vec3f SolarSystem::getEphemerisSaturnMarkerColor() const
{
	return ephemerisSaturnMarkerColor;
}

void SolarSystem::setFlagNativePlanetNames(bool b)
{
	if (b!=flagNativePlanetNames)
	{
		flagNativePlanetNames=b;
		for (const auto& p : systemPlanets)
		{
			if (p->getPlanetType()==Planet::isPlanet || p->getPlanetType()==Planet::isMoon || p->getPlanetType()==Planet::isStar)
				p->setFlagNativeName(flagNativePlanetNames);
		}
		updateI18n();
		emit flagNativePlanetNamesChanged(b);
	}
}

bool SolarSystem::getFlagNativePlanetNames() const
{
	return flagNativePlanetNames;
}

void SolarSystem::setFlagTranslatedNames(bool b)
{
	if (b!=flagTranslatedNames)
	{
		flagTranslatedNames=b;
		for (const auto& p : systemPlanets)
		{
			if (p->getPlanetType()==Planet::isPlanet || p->getPlanetType()==Planet::isMoon || p->getPlanetType()==Planet::isStar)
				p->setFlagTranslatedName(flagTranslatedNames);
		}
		updateI18n();
		emit flagTranslatedNamesChanged(b);
	}
}

bool SolarSystem::getFlagTranslatedNames() const
{
	return flagTranslatedNames;
}

void SolarSystem::setFlagIsolatedTrails(bool b)
{
	if(b!=flagIsolatedTrails)
	{
		flagIsolatedTrails = b;
		emit flagIsolatedTrailsChanged(b);
	}
}

bool SolarSystem::getFlagIsolatedTrails() const
{
	return flagIsolatedTrails;
}

int SolarSystem::getNumberIsolatedTrails() const
{
	return numberIsolatedTrails;
}

void SolarSystem::setNumberIsolatedTrails(int n)
{
	// [1..5] - valid range for trails
	if (n<1)
		numberIsolatedTrails = 1;
	else if (n>5)
		numberIsolatedTrails = 5;
	else
		numberIsolatedTrails = n;

	if (getFlagIsolatedTrails())
		recreateTrails();

	emit numberIsolatedTrailsChanged(numberIsolatedTrails);
}

void SolarSystem::setFlagIsolatedOrbits(bool b)
{
	if(b!=flagIsolatedOrbits)
	{
		flagIsolatedOrbits = b;
		emit flagIsolatedOrbitsChanged(b);
		// Reinstall flag for orbits to renew visibility of orbits
		setFlagOrbits(getFlagOrbits());
	}
}

bool SolarSystem::getFlagIsolatedOrbits() const
{
	return flagIsolatedOrbits;
}

void SolarSystem::setFlagPlanetsOrbitsOnly(bool b)
{
	if(b!=flagPlanetsOrbitsOnly)
	{
		flagPlanetsOrbitsOnly = b;
		emit flagPlanetsOrbitsOnlyChanged(b);
		// Reinstall flag for orbits to renew visibility of orbits
		setFlagOrbits(getFlagOrbits());
	}
}

bool SolarSystem::getFlagPlanetsOrbitsOnly() const
{
	return flagPlanetsOrbitsOnly;
}

// Set/Get planets names color
void SolarSystem::setLabelsColor(const Vec3f& c)
{
	if (c!=Planet::getLabelColor())
	{
		Planet::setLabelColor(c);
		emit labelsColorChanged(c);
	}
}

const Vec3f& SolarSystem::getLabelsColor(void) const
{
	return Planet::getLabelColor();
}

// Set/Get orbits lines color
void SolarSystem::setOrbitsColor(const Vec3f& c)
{
	if (c!=Planet::getOrbitColor())
	{
		Planet::setOrbitColor(c);
		emit orbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getOrbitsColor(void) const
{
	return Planet::getOrbitColor();
}

void SolarSystem::setMajorPlanetsOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getMajorPlanetOrbitColor())
	{
		Planet::setMajorPlanetOrbitColor(c);
		emit majorPlanetsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getMajorPlanetsOrbitsColor(void) const
{
	return Planet::getMajorPlanetOrbitColor();
}

void SolarSystem::setMinorPlanetsOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getMinorPlanetOrbitColor())
	{
		Planet::setMinorPlanetOrbitColor(c);
		emit minorPlanetsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getMinorPlanetsOrbitsColor(void) const
{
	return Planet::getMinorPlanetOrbitColor();
}

void SolarSystem::setDwarfPlanetsOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getDwarfPlanetOrbitColor())
	{
		Planet::setDwarfPlanetOrbitColor(c);
		emit dwarfPlanetsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getDwarfPlanetsOrbitsColor(void) const
{
	return Planet::getDwarfPlanetOrbitColor();
}

void SolarSystem::setMoonsOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getMoonOrbitColor())
	{
		Planet::setMoonOrbitColor(c);
		emit moonsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getMoonsOrbitsColor(void) const
{
	return Planet::getMoonOrbitColor();
}

void SolarSystem::setCubewanosOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getCubewanoOrbitColor())
	{
		Planet::setCubewanoOrbitColor(c);
		emit cubewanosOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getCubewanosOrbitsColor(void) const
{
	return Planet::getCubewanoOrbitColor();
}

void SolarSystem::setPlutinosOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getPlutinoOrbitColor())
	{
		Planet::setPlutinoOrbitColor(c);
		emit plutinosOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getPlutinosOrbitsColor(void) const
{
	return Planet::getPlutinoOrbitColor();
}

void SolarSystem::setScatteredDiskObjectsOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getScatteredDiscObjectOrbitColor())
	{
		Planet::setScatteredDiscObjectOrbitColor(c);
		emit scatteredDiskObjectsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getScatteredDiskObjectsOrbitsColor(void) const
{
	return Planet::getScatteredDiscObjectOrbitColor();
}

void SolarSystem::setOortCloudObjectsOrbitsColor(const Vec3f &c)
{
	if (c!=Planet::getOortCloudObjectOrbitColor())
	{
		Planet::setOortCloudObjectOrbitColor(c);
		emit oortCloudObjectsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getOortCloudObjectsOrbitsColor(void) const
{
	return Planet::getOortCloudObjectOrbitColor();
}

void SolarSystem::setCometsOrbitsColor(const Vec3f& c)
{
	if (c!=Planet::getCometOrbitColor())
	{
		Planet::setCometOrbitColor(c);
		emit cometsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getCometsOrbitsColor(void) const
{
	return Planet::getCometOrbitColor();
}

void SolarSystem::setSednoidsOrbitsColor(const Vec3f& c)
{
	if (c!=Planet::getSednoidOrbitColor())
	{
		Planet::setSednoidOrbitColor(c);
		emit sednoidsOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getSednoidsOrbitsColor(void) const
{
	return Planet::getSednoidOrbitColor();
}

void SolarSystem::setInterstellarOrbitsColor(const Vec3f& c)
{
	if (c!=Planet::getInterstellarOrbitColor())
	{
		Planet::setInterstellarOrbitColor(c);
		emit interstellarOrbitsColorChanged(c);
	}
}

Vec3f SolarSystem::getInterstellarOrbitsColor(void) const
{
	return Planet::getInterstellarOrbitColor();
}

void SolarSystem::setMercuryOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getMercuryOrbitColor())
	{
		Planet::setMercuryOrbitColor(c);
		emit mercuryOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getMercuryOrbitColor(void) const
{
	return Planet::getMercuryOrbitColor();
}

void SolarSystem::setVenusOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getVenusOrbitColor())
	{
		Planet::setVenusOrbitColor(c);
		emit venusOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getVenusOrbitColor(void) const
{
	return Planet::getVenusOrbitColor();
}

void SolarSystem::setEarthOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getEarthOrbitColor())
	{
		Planet::setEarthOrbitColor(c);
		emit earthOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getEarthOrbitColor(void) const
{
	return Planet::getEarthOrbitColor();
}

void SolarSystem::setMarsOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getMarsOrbitColor())
	{
		Planet::setMarsOrbitColor(c);
		emit marsOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getMarsOrbitColor(void) const
{
	return Planet::getMarsOrbitColor();
}

void SolarSystem::setJupiterOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getJupiterOrbitColor())
	{
		Planet::setJupiterOrbitColor(c);
		emit jupiterOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getJupiterOrbitColor(void) const
{
	return Planet::getJupiterOrbitColor();
}

void SolarSystem::setSaturnOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getSaturnOrbitColor())
	{
		Planet::setSaturnOrbitColor(c);
		emit saturnOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getSaturnOrbitColor(void) const
{
	return Planet::getSaturnOrbitColor();
}

void SolarSystem::setUranusOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getUranusOrbitColor())
	{
		Planet::setUranusOrbitColor(c);
		emit uranusOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getUranusOrbitColor(void) const
{
	return Planet::getUranusOrbitColor();
}

void SolarSystem::setNeptuneOrbitColor(const Vec3f &c)
{
	if (c!=Planet::getNeptuneOrbitColor())
	{
		Planet::setNeptuneOrbitColor(c);
		emit neptuneOrbitColorChanged(c);
	}
}

Vec3f SolarSystem::getNeptuneOrbitColor(void) const
{
	return Planet::getNeptuneOrbitColor();
}

// Set/Get if Moon display is scaled
void SolarSystem::setFlagMoonScale(bool b)
{
	if(b!=flagMoonScale)
	{
		if (!b) getMoon()->setSphereScale(1);
		else getMoon()->setSphereScale(moonScale);
		flagMoonScale = b;
		emit flagMoonScaleChanged(b);
	}
}

// Set/Get Moon display scaling factor. This goes directly to the Moon object.
void SolarSystem::setMoonScale(double f)
{
	if(moonScale != f)
	{
		moonScale = f;
		if (flagMoonScale)
			getMoon()->setSphereScale(moonScale);
		emit moonScaleChanged(f);
	}
}

// Set/Get if minor body display is scaled. This flag will be queried by all Planet objects except for the Moon.
void SolarSystem::setFlagMinorBodyScale(bool b)
{
	if(b!=flagMinorBodyScale)
	{
		flagMinorBodyScale = b;

		double newScale = b ? minorBodyScale : 1.0;
		//update the bodies with the new scale
		for (const auto& p : systemPlanets)
		{
			if(p == moon) continue;
			if (p->getPlanetType()!=Planet::isPlanet && p->getPlanetType()!=Planet::isStar)
				p->setSphereScale(newScale);
		}
		emit flagMinorBodyScaleChanged(b);
	}
}

// Set/Get minor body display scaling factor. This will be queried by all Planet objects except for the Moon.
void SolarSystem::setMinorBodyScale(double f)
{
	if(minorBodyScale != f)
	{
		minorBodyScale = f;
		if(flagMinorBodyScale) //update the bodies with the new scale
		{
			for (const auto& p : systemPlanets)
			{
				if(p == moon) continue;
				if (p->getPlanetType()!=Planet::isPlanet && p->getPlanetType()!=Planet::isStar)
					p->setSphereScale(minorBodyScale);
			}
		}
		emit minorBodyScaleChanged(f);
	}
}

// Set selected planets by englishName
void SolarSystem::setSelected(const QString& englishName)
{
	setSelected(searchByEnglishName(englishName));
}

// Get the list of all the planet english names
QStringList SolarSystem::getAllPlanetEnglishNames() const
{
	QStringList res;
	for (const auto& p : systemPlanets)
		res.append(p->getEnglishName());
	return res;
}

QStringList SolarSystem::getAllPlanetLocalizedNames() const
{
	QStringList res;
	for (const auto& p : systemPlanets)
		res.append(p->getNameI18n());
	return res;
}

QStringList SolarSystem::getAllMinorPlanetCommonEnglishNames() const
{
	QStringList res;
	for (const auto& p : systemMinorBodies)
		res.append(p->getCommonEnglishName());
	return res;
}


// GZ TODO: This could be modified to only delete&reload the minor objects. For now, we really load both parts again like in the 0.10?-0.15 series.
void SolarSystem::reloadPlanets()
{
	// Save flag states
	bool flagScaleMoon = getFlagMoonScale();
	float moonScale = getMoonScale();
	bool flagScaleMinorBodies=getFlagMinorBodyScale();
	float minorScale= getMinorBodyScale();
	bool flagPlanets = getFlagPlanets();
	bool flagHints = getFlagHints();
	bool flagLabels = getFlagLabels();
	bool flagOrbits = getFlagOrbits();
	bool flagNative = getFlagNativePlanetNames();
	bool flagTrans = getFlagTranslatedNames();
	bool hasSelection = false;

	// Save observer location (fix for LP bug # 969211)
	// TODO: This can probably be done better with a better understanding of StelObserver --BM
	StelCore* core = StelApp::getInstance().getCore();
	StelLocation loc = core->getCurrentLocation();
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);

	// Whether any planet are selected? Save the current selection...
	const QList<StelObjectP> selectedObject = objMgr->getSelectedObject("Planet");
	if (!selectedObject.isEmpty())
	{
		// ... unselect current planet.
		hasSelection = true;
		objMgr->unSelect();
	}
	// Unload all Solar System objects
	selected.clear();//Release the selected one

	// GZ TODO in case this methods gets converted to only reload minor bodies: Only delete Orbits which are not referenced by some Planet.
	for (auto* orb : orbits)
	{
		delete orb;
	}
	orbits.clear();

	sun.clear();
	moon.clear();
	earth.clear();
	Planet::texEarthShadow.clear(); //Loaded in loadPlanets()

	delete allTrails;
	allTrails = Q_NULLPTR;

	for (const auto& p : systemPlanets)
	{
		p->satellites.clear();
	}
	systemPlanets.clear();
	systemMinorBodies.clear();
	// Memory leak? What's the proper way of cleaning shared pointers?

	// Also delete Comet textures (loaded in loadPlanets()
	Comet::tailTexture.clear();
	Comet::comaTexture.clear();

	// Re-load the ssystem_major.ini and ssystem_minor.ini file
	loadPlanets();	
	computePositions(core->getJDE(), getSun());
	setSelected("");
	recreateTrails();
	
	// Restore observer location
	core->moveObserverTo(loc, 0., 0.);

	// Restore flag states
	setFlagMoonScale(flagScaleMoon);
	setMoonScale(moonScale);
	setFlagMinorBodyScale(flagScaleMinorBodies);
	setMinorBodyScale(1.0); // force-reset first to really reach the objects in the next call.
	setMinorBodyScale(minorScale);
	setFlagPlanets(flagPlanets);
	setFlagHints(flagHints);
	setFlagLabels(flagLabels);
	setFlagOrbits(flagOrbits);
	setFlagNativePlanetNames(flagNative);
	setFlagTranslatedNames(flagTrans);

	if (hasSelection)
	{
		// Restore selection...
		objMgr->setSelectedObject(selectedObject);
	}

	// Restore translations
	updateI18n();

	emit solarSystemDataReloaded();
}

// Set the algorithm for computation of apparent magnitudes for planets in case  observer on the Earth
void SolarSystem::setApparentMagnitudeAlgorithmOnEarth(QString algorithm)
{
	Planet::setApparentMagnitudeAlgorithm(algorithm);
	emit apparentMagnitudeAlgorithmOnEarthChanged(algorithm);
}

// Get the algorithm used for computation of apparent magnitudes for planets in case  observer on the Earth
QString SolarSystem::getApparentMagnitudeAlgorithmOnEarth() const
{
	return Planet::getApparentMagnitudeAlgorithmString();
}

void SolarSystem::setFlagPermanentOrbits(bool b)
{
	Planet::permanentDrawingOrbits=b;
	emit flagPermanentOrbitsChanged(b);
}

bool SolarSystem::getFlagPermanentOrbits() const
{
	return Planet::permanentDrawingOrbits;
}

void SolarSystem::setFlagCustomGrsSettings(bool b)
{
	Planet::flagCustomGrsSettings=b;
	// automatic saving of the setting
	conf->setValue("astro/flag_grs_custom", b);
	emit flagCustomGrsSettingsChanged(b);
}

bool SolarSystem::getFlagCustomGrsSettings() const
{
	return Planet::flagCustomGrsSettings;
}

void SolarSystem::setCustomGrsLongitude(int longitude)
{
	Planet::customGrsLongitude = longitude;
	// automatic saving of the setting
	conf->setValue("astro/grs_longitude", longitude);
	emit customGrsLongitudeChanged(longitude);
}

int SolarSystem::getCustomGrsLongitude() const
{
	return Planet::customGrsLongitude;
}

void SolarSystem::setCustomGrsDrift(double drift)
{
	Planet::customGrsDrift = drift;
	// automatic saving of the setting
	conf->setValue("astro/grs_drift", drift);
	emit customGrsDriftChanged(drift);
}

double SolarSystem::getCustomGrsDrift() const
{
	return Planet::customGrsDrift;
}

void SolarSystem::setCustomGrsJD(double JD)
{
	Planet::customGrsJD = JD;
	// automatic saving of the setting
	conf->setValue("astro/grs_jd", JD);
	emit customGrsJDChanged(JD);
}

double SolarSystem::getCustomGrsJD()
{
	return Planet::customGrsJD;
}

void SolarSystem::setOrbitColorStyle(QString style)
{
	if (style.toLower()=="groups")
		Planet::orbitColorStyle = Planet::ocsGroups;
	else if (style.toLower()=="major_planets")
		Planet::orbitColorStyle = Planet::ocsMajorPlanets;
	else
		Planet::orbitColorStyle = Planet::ocsOneColor;
}

QString SolarSystem::getOrbitColorStyle() const
{
	QString r = "one_color";
	switch (Planet::orbitColorStyle)
	{
		case Planet::ocsOneColor:
			r = "one_color";
			break;
		case Planet::ocsGroups:
			r = "groups";
			break;
		case Planet::ocsMajorPlanets:
			r = "major_planets";
			break;
	}
	return r;
}

double SolarSystem::getEclipseFactor(const StelCore* core) const
{
	Vec3d Lp = getLightTimeSunPosition();  //sun->getEclipticPos();
	Vec3d P3 = core->getObserverHeliocentricEclipticPos();
	const double RS = sun->getEquatorialRadius();

	double final_illumination = 1.0;

	for (const auto& planet : systemPlanets)
	{
		if(planet == sun || planet == core->getCurrentPlanet())
			continue;

		Mat4d trans;
		planet->computeModelMatrix(trans);

		const Vec3d C = trans * Vec3d(0., 0., 0.);
		const double radius = planet->getEquatorialRadius();

		Vec3d v1 = Lp - P3;
		Vec3d v2 = C - P3;

		const double L = v1.length();
		const double l = v2.length();

		v1 = v1 / L;
		v2 = v2 / l;

		const double R = RS / L;
		const double r = radius / l;
		const double d = ( v1 - v2 ).length();
		double illumination;

		if(d >= R + r) // distance too far
		{
			illumination = 1.0;
		}
		else if(d <= r - R) // umbra
		{
			illumination = 0.0;
		}
		else if(d <= R - r) // penumbra completely inside
		{
			illumination = 1.0 - r * r / (R * R);
		}
		else // penumbra partially inside
		{
			const double x = (R * R + d * d - r * r) / (2.0 * d);

			const double alpha = std::acos(x / R);
			const double beta = std::acos((d - x) / r);

			const double AR = R * R * (alpha - 0.5 * std::sin(2.0 * alpha));
			const double Ar = r * r * (beta - 0.5 * std::sin(2.0 * beta));
			const double AS = R * R * 2.0 * std::asin(1.0);

			illumination = 1.0 - (AR + Ar) / AS;
		}

		if(illumination < final_illumination)
			final_illumination = illumination;
	}

	return final_illumination;
}

bool SolarSystem::removeMinorPlanet(QString name)
{
	PlanetP candidate = searchMinorPlanetByEnglishName(name);
	if (!candidate)
	{
		qWarning() << "Cannot remove planet " << name << ": Not found.";
		return false;
	}
	Orbit* orbPtr=(Orbit*) candidate->orbitPtr;
	if (orbPtr)
		orbits.removeOne(orbPtr);
	systemPlanets.removeOne(candidate);
	systemMinorBodies.removeOne(candidate);
	candidate.clear();
	return true;
}

void SolarSystem::onNewSurvey(HipsSurveyP survey)
{
	// For the moment we only consider the survey url to decide if we
	// assign it to a planet.  It would be better to use some property
	// for that.
	QString planetName = QUrl(survey->getUrl()).fileName();
	PlanetP pl = searchByEnglishName(planetName);
	if (!pl || pl->survey)
		return;
	pl->survey = survey;
	survey->setProperty("planet", pl->getCommonEnglishName());
	// Not visible by default for the moment.
	survey->setProperty("visible", false);
}
