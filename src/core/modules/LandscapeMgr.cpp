/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
 * Copyright (C) 2010 Bogdan Marinov (add/remove landscapes feature)
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2012 Timothy Reaves
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

#include <QDebug>
#include <QSettings>
#include <QString>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>

#include <stdexcept>

#include "LandscapeMgr.hpp"
#include "Landscape.hpp"
#include "Atmosphere.hpp"
#include "StelApp.hpp"
#include "SolarSystem.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "Planet.hpp"
#include "StelIniParser.hpp"
#include "StelSkyDrawer.hpp"
#include "renderer/StelRenderer.hpp"
#include "karchive.h"
#include "kzip.h"

// Class which manages the cardinal points displaying
class Cardinals
{
public:
	Cardinals(float _radius = 1.);
	virtual ~Cardinals();
	void draw(const StelCore* core, StelRenderer* renderer, double latitude) const;
	void setColor(const Vec3f& c) {color = c;}
	Vec3f get_color() {return color;}
	void updateI18n();
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setFlagShow(bool b){fader = b;}
	bool getFlagShow() const {return fader;}
private:
	float radius;
	QFont font;
	Vec3f color;
	QString sNorth, sSouth, sEast, sWest;
	LinearFader fader;
};


Cardinals::Cardinals(float _radius) : radius(_radius), color(0.6,0.2,0.2)
{
	font.setPixelSize(30);
	// Default labels - if sky locale specified, loaded later
	// Improvement for gettext translation
	sNorth = "N";
	sSouth = "S";
	sEast = "E";
	sWest = "W";
}

Cardinals::~Cardinals()
{
}

// Draw the cardinals points : N S E W
// handles special cases at poles
void Cardinals::draw(const StelCore* core, StelRenderer* renderer, double latitude) const
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	renderer->setFont(font);

	if (!fader.getInterstate()) return;

	// direction text
	QString d[4];

	d[0] = sNorth;
	d[1] = sSouth;
	d[2] = sEast;
	d[3] = sWest;

	// fun polar special cases
	if (latitude ==  90.0 ) d[0] = d[1] = d[2] = d[3] = sSouth;
	if (latitude == -90.0 ) d[0] = d[1] = d[2] = d[3] = sNorth;

	renderer->setGlobalColor(color[0], color[1], color[2], fader.getInterstate());
	renderer->setBlendMode(BlendMode_Alpha);

	Vec3f pos;
	Vec3f xy;

	float shift = QFontMetrics(font).width(sNorth)/2;
	if (core->getProjection(StelCore::FrameJ2000)->getMaskType() == StelProjector::MaskDisk)
		shift = 0;

	// N for North
	pos.set(-1.f, 0.f, 0.f);
	if (prj->project(pos, xy)) 
	{
		renderer->drawText(TextParams(xy[0], xy[1], d[0])
		                  .shift(-shift, -shift).useGravity());
	}

	// S for South
	pos.set(1.f, 0.f, 0.f);
	if (prj->project(pos, xy)) 
	{
		renderer->drawText(TextParams(xy[0], xy[1], d[1])
		                  .shift(-shift, -shift).useGravity());
	}

	// E for East
	pos.set(0.f, 1.f, 0.f);
	if (prj->project(pos, xy)) 
	{
		renderer->drawText(TextParams(xy[0], xy[1], d[2])
		                  .shift(-shift, -shift).useGravity());
	}

	// W for West
	pos.set(0.f, -1.f, 0.f);
	if (prj->project(pos, xy)) 
	{
		renderer->drawText(TextParams(xy[0], xy[1], d[3])
		                  .shift(-shift, -shift).useGravity());
	}
}

// Translate cardinal labels with gettext to current sky language and update font for the language
void Cardinals::updateI18n()
{
	StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getAppStelTranslator();
	sNorth = trans.qtranslate("N");
	sSouth = trans.qtranslate("S");
	sEast = trans.qtranslate("E");
	sWest = trans.qtranslate("W");
}


LandscapeMgr::LandscapeMgr() : atmosphere(NULL), cardinalsPoints(NULL), landscape(NULL), flagLandscapeSetsLocation(false)
{
	setObjectName("LandscapeMgr");

	//TODO: Find a way to obtain this list automatically.
	//Note: The first entry in the list is used as the default 'default landscape' in removeLandscape().
	packagedLandscapeIDs = (QStringList() << "guereins" << "trees" << "moon" << "hurricane" << "ocean" << "garching" << "mars" << "saturn");
}

LandscapeMgr::~LandscapeMgr()
{
	delete atmosphere;
	delete cardinalsPoints;
	delete landscape;
	landscape = NULL;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double LandscapeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName)+20;
	if (actionName==StelModule::ActionUpdate)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+10;
	return 0;
}

void LandscapeMgr::update(double deltaTime)
{
	atmosphere->update(deltaTime);
	landscape->update(deltaTime);
	cardinalsPoints->update(deltaTime);

	// Compute the atmosphere color and intensity
	// Compute the sun position in local coordinate
	SolarSystem* ssystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");

	StelCore* core = StelApp::getInstance().getCore();
	Vec3d sunPos = ssystem->getSun()->getAltAzPosApparent(core);
	// Compute the moon position in local coordinate
	Vec3d moonPos = ssystem->getMoon()->getAltAzPosApparent(core);
	atmosphere->computeColor(core->getJDay(), sunPos, moonPos,
		ssystem->getMoon()->getPhaseAngle(ssystem->getEarth()->getHeliocentricEclipticPos()),
		core, ssystem->getEclipseFactor(core), core->getCurrentLocation().latitude, core->getCurrentLocation().altitude,
		15.f, 40.f);	// Temperature = 15c, relative humidity = 40%

	core->getSkyDrawer()->reportLuminanceInFov(3.75+atmosphere->getAverageLuminance()*3.5, true);

	// Compute the ground luminance based on every planets around
//	float groundLuminance = 0;
//	const vector<Planet*>& allPlanets = ssystem->getAllPlanets();
//	for (vector<Planet*>::const_iterator i=allPlanets.begin();i!=allPlanets.end();++i)
//	{
//		Vec3d pos = (*i)->getAltAzPos(core);
//		pos.normalize();
//		if (pos[2] <= 0)
//		{
//			// No need to take this body into the landscape illumination computation
//			// because it is under the horizon
//		}
//		else
//		{
//			// Compute the Illuminance E of the ground caused by the planet in lux = lumen/m^2
//			float E = pow10(((*i)->get_mag(core)+13.988)/-2.5);
//			//qDebug() << "mag=" << (*i)->get_mag(core) << " illum=" << E;
//			// Luminance in cd/m^2
//			groundLuminance += E/0.44*pos[2]*pos[2]; // 1m^2 from 1.5 m above the ground is 0.44 sr.
//		}
//	}
//	groundLuminance*=atmosphere->getFadeIntensity();
//	groundLuminance=atmosphere->getAverageLuminance()/50;
//	qDebug() << "Atmosphere lum=" << atmosphere->getAverageLuminance() << " ground lum=" <<  groundLuminance;
//	qDebug() << "Adapted Atmosphere lum=" << eye->adaptLuminance(atmosphere->getAverageLuminance()) << " Adapted ground lum=" << eye->adaptLuminance(groundLuminance);

	// compute global ground brightness in a simplistic way, directly in RGB
	float landscapeBrightness = 0;
	sunPos.normalize();
	moonPos.normalize();

	// We define the brigthness zero when the sun is 8 degrees below the horizon.
	float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPos[2])+8.*M_PI/180.));

	if(sinSunAngleRad < -0.1/1.5 )
		landscapeBrightness = 0.01;
	else
		landscapeBrightness = (0.01 + 1.5*(sinSunAngleRad+0.1/1.5));
	if (moonPos[2] > -0.1/1.5)
		landscapeBrightness += qMax(0.2/-12.*ssystem->getMoon()->getVMagnitude(core, true),0.)*moonPos[2];

	// TODO make this more generic for non-atmosphere planets
	if(atmosphere->getFadeIntensity() == 1)
	{
		// If the atmosphere is on, a solar eclipse might darken the sky
		// otherwise we just use the sun position calculation above
		landscapeBrightness *= (atmosphere->getRealDisplayIntensityFactor()+0.1);
	}

	// TODO: should calculate dimming with solar eclipse even without atmosphere on
	landscape->setBrightness(landscapeBrightness+0.05);
}

void LandscapeMgr::draw(StelCore* core, class StelRenderer* renderer)
{
	// Draw the atmosphere
	atmosphere->draw(core, renderer);

	// Draw the landscape
	landscape->draw(core, renderer);

	// Draw the cardinal points
	cardinalsPoints->draw(core, renderer, StelApp::getInstance().getCore()->getCurrentLocation().latitude);
}

void LandscapeMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	atmosphere = new Atmosphere();
	landscape = new LandscapeOldStyle();
	defaultLandscapeID = conf->value("init_location/landscape_name").toString();
	setCurrentLandscapeID(defaultLandscapeID);
	setFlagLandscape(conf->value("landscape/flag_landscape", conf->value("landscape/flag_ground", true).toBool()).toBool());
	setFlagFog(conf->value("landscape/flag_fog",true).toBool());
	setFlagAtmosphere(conf->value("landscape/flag_atmosphere", true).toBool());
	setAtmosphereFadeDuration(conf->value("landscape/atmosphere_fade_duration",0.5).toFloat());
	setAtmosphereLightPollutionLuminance(conf->value("viewing/light_pollution_luminance",0.0).toFloat());
	cardinalsPoints = new Cardinals();
	cardinalsPoints->setFlagShow(conf->value("viewing/flag_cardinal_points",true).toBool());
	setFlagLandscapeSetsLocation(conf->value("landscape/flag_landscape_sets_location",false).toBool());

	bool ok =true;
	setAtmosphereBortleLightPollution(conf->value("landscape/init_bortle_scale",3).toInt(&ok));
	if (!ok)
	{
		conf->setValue("landscape/init_bortle_scale",3);
		setAtmosphereBortleLightPollution(3);
		ok = true;
	}
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
}

void LandscapeMgr::setStelStyle(const QString& section)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();

	QString defaultColor = conf->value(section+"/default_color").toString();
	setColorCardinalPoints(StelUtils::strToVec3f(conf->value(section+"/cardinal_color", defaultColor).toString()));
}

bool LandscapeMgr::setCurrentLandscapeID(const QString& id)
{
	if (id.isEmpty())
		return false;

	// We want to lookup the landscape ID (dir) from the name.
	Landscape* newLandscape = NULL;
	try
	{
		newLandscape = createFromFile(StelFileMgr::findFile("landscapes/" + id + "/landscape.ini"), id);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading default landscape " << "landscapes/" + id + "/landscape.ini" << ", (" << e.what() << ")";
	}

	if (!newLandscape)
		return false;

	if (landscape)
	{
		// Copy display parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		delete landscape;
		landscape = newLandscape;
	}
	currentLandscapeID = id;

	if (getFlagLandscapeSetsLocation())
	{
		StelApp::getInstance().getCore()->moveObserverTo(landscape->getLocation());
		// GZ Patch: allow change in fog, extinction, refraction parameters and light pollution
		//QSettings* conf = StelApp::getInstance().getSettings();
		//Q_ASSERT(conf);
		StelSkyDrawer* drawer=StelApp::getInstance().getCore()->getSkyDrawer();

		if (landscape->getDefaultFogSetting() >-1)
		  {
			setFlagFog((bool) landscape->getDefaultFogSetting());
			landscape->setFlagShowFog((bool) landscape->getDefaultFogSetting());
		  }
		if (landscape->getDefaultBortleIndex() > 0)
		  {
		    setAtmosphereBortleLightPollution(landscape->getDefaultBortleIndex());
		    // TODO: HOWTO make the GUI aware of the new value? 
		    // conf->setValue("landscape/init_bortle_scale", landscape->getDefaultBortleIndex());
		  }
		if (landscape->getDefaultAtmosphericExtinction() >= 0.0)
		  {
		    drawer->setExtinctionCoefficient(landscape->getDefaultAtmosphericExtinction());
		  }
		if (landscape->getDefaultAtmosphericTemperature() > -273.15)
		  {
		    drawer->setAtmosphereTemperature(landscape->getDefaultAtmosphericTemperature());
		  }
		if (landscape->getDefaultAtmosphericPressure() >= 0.0)
		  {
		    drawer->setAtmospherePressure(landscape->getDefaultAtmosphericPressure());
		  }
		else if (landscape->getDefaultAtmosphericPressure() == -1.0)
		  {
		    // compute standard pressure for standard atmosphere in given altitude if landscape.ini coded as atmospheric_pressure=-1
		    // International altitude formula found in Wikipedia.
		    double alt=landscape->getLocation().altitude;
		    double p=1013.25*std::pow(1-(0.0065*alt)/288.15, 5.255);
		    drawer->setAtmospherePressure(p);
		  }
	}
	return true;
}

bool LandscapeMgr::setCurrentLandscapeName(const QString& name)
{
	if (name.isEmpty())
		return false;
	
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
		return setCurrentLandscapeID(nameToDirMap[name]);
	}
	else
	{
		qWarning() << "Can't find a landscape with name=" << name << endl;
		return false;
	}
}

// Change the default landscape to the landscape with the ID specified.
bool LandscapeMgr::setDefaultLandscapeID(const QString& id)
{
	if (id.isEmpty())
		return false;
	defaultLandscapeID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("init_location/landscape_name", id);
	return true;
}

void LandscapeMgr::updateI18n()
{
	// Translate all labels with the new language
	if (cardinalsPoints) cardinalsPoints->updateI18n();
}

void LandscapeMgr::setFlagLandscape(const bool displayed)
{
	if(landscape->getFlagShow() != displayed) {
		landscape->setFlagShow(displayed);
		emit landscapeDisplayedChanged(displayed);
	}
}

bool LandscapeMgr::getFlagLandscape() const
{
	return landscape->getFlagShow();
}

void LandscapeMgr::setFlagFog(const bool displayed)
{
	if (landscape->getFlagShowFog() != displayed) {
		landscape->setFlagShowFog(displayed);
		emit fogDisplayedChanged(displayed);
	}
}

bool LandscapeMgr::getFlagFog() const
{
	return landscape->getFlagShowFog();
}

/*********************************************************************
 Retrieve list of the names of all the available landscapes
 *********************************************************************/
QStringList LandscapeMgr::getAllLandscapeNames() const
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the keys
	foreach (QString i, nameToDirMap.keys())
	{
		result += i;
	}
	return result;
}

QStringList LandscapeMgr::getAllLandscapeIDs() const
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the keys
	foreach (QString i, nameToDirMap.values())
	{
		result += i;
	}
	return result;
}

QStringList LandscapeMgr::getUserLandscapeIDs() const
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	QStringList result;
	foreach (QString id, nameToDirMap.values())
	{
		if(!packagedLandscapeIDs.contains(id))
		{
			result += id;
		}
	}
	return result;
}

QString LandscapeMgr::getCurrentLandscapeName() const
{
	return landscape->getName();
}

QString LandscapeMgr::getCurrentLandscapeHtmlDescription() const
{
	QString desc = getDescription();
	desc+="<p>";
	desc+="<b>"+q_("Author: ")+"</b>";
	desc+=landscape->getAuthorName();
	desc+="<br>";
	desc+="<b>"+q_("Location: ")+"</b>";
	if (landscape->getLocation().longitude>-500.0 && landscape->getLocation().latitude>-500.0)
	{
		desc += StelUtils::radToDmsStrAdapt(landscape->getLocation().longitude * M_PI/180.);
		desc += "/" + StelUtils::radToDmsStrAdapt(landscape->getLocation().latitude *M_PI/180.);
		desc += QString(q_(", %1 m")).arg(landscape->getLocation().altitude);
		QString planetName = landscape->getLocation().planetName;
		if (!planetName.isEmpty())
		{
			desc += "<br><b>"+q_("Planet: ")+"</b>"+ q_(planetName);
		}
		desc += "<br><br>";
	}
	return desc;
}

//! Set flag for displaying Cardinals Points
void LandscapeMgr::setFlagCardinalsPoints(const bool displayed)
{
	if (cardinalsPoints->getFlagShow() != displayed) {
		cardinalsPoints->setFlagShow(displayed);
		emit cardinalsPointsDisplayedChanged(displayed);
	}
}

//! Get flag for displaying Cardinals Points
bool LandscapeMgr::getFlagCardinalsPoints() const
{
	return cardinalsPoints->getFlagShow();
}

//! Set Cardinals Points color
void LandscapeMgr::setColorCardinalPoints(const Vec3f& v)
{
	cardinalsPoints->setColor(v);
}

//! Get Cardinals Points color
Vec3f LandscapeMgr::getColorCardinalPoints() const
{
	return cardinalsPoints->get_color();
}

///////////////////////////////////////////////////////////////////////////////////////
// Atmosphere
//! Set flag for displaying Atmosphere
void LandscapeMgr::setFlagAtmosphere(const bool displayed)
{
	if (atmosphere->getFlagShow() != displayed) {
		atmosphere->setFlagShow(displayed);
		StelApp::getInstance().getCore()->getSkyDrawer()->setFlagHasAtmosphere(displayed);
		emit atmosphereDisplayedChanged(displayed);
	}
}

//! Get flag for displaying Atmosphere
bool LandscapeMgr::getFlagAtmosphere() const
{
	return atmosphere->getFlagShow();
}

//! Set atmosphere fade duration in s
void LandscapeMgr::setAtmosphereFadeDuration(float f)
{
	atmosphere->setFadeDuration(f);
}

//! Get atmosphere fade duration in s
float LandscapeMgr::getAtmosphereFadeDuration() const
{
	return atmosphere->getFadeDuration();
}

//! Set light pollution luminance level
void LandscapeMgr::setAtmosphereLightPollutionLuminance(float f)
{
	atmosphere->setLightPollutionLuminance(f);
}

//! Get light pollution luminance level
float LandscapeMgr::getAtmosphereLightPollutionLuminance() const
{
	return atmosphere->getLightPollutionLuminance();
}

//! Set the light pollution following the Bortle Scale
void LandscapeMgr::setAtmosphereBortleLightPollution(int bIndex)
{
	// This is an empirical formula
	setAtmosphereLightPollutionLuminance(qMax(0.,0.0020*std::pow(bIndex-1, 2.1)));
}

//! Get the light pollution following the Bortle Scale
int LandscapeMgr::getAtmosphereBortleLightPollution()
{
	return (int)std::pow(getAtmosphereLightPollutionLuminance()/0.0020, 1./2.1) + 1;
}

void LandscapeMgr::setZRotation(float d)
{
	if (landscape)
		landscape->setZRotation(d);
}

float LandscapeMgr::getLuminance()
{
	return atmosphere->getRealDisplayIntensityFactor();
}

Landscape* LandscapeMgr::createFromFile(const QString& landscapeFile, const QString& landscapeId)
{
	QSettings landscapeIni(landscapeFile, StelIniFormat);
	QString s;
	if (landscapeIni.status() != QSettings::NoError)
	{
		qWarning() << "ERROR parsing landscape.ini file: " << landscapeFile;
		s = "";
	}
	else
		s = landscapeIni.value("landscape/type").toString();

	Landscape* ldscp = NULL;
	if (s=="old_style")
		ldscp = new LandscapeOldStyle();
	else if (s=="spherical")
		ldscp = new LandscapeSpherical();
	else if (s=="fisheye")
		ldscp = new LandscapeFisheye();
	else
	{
		qDebug() << "Unknown landscape type: \"" << s << "\"";

		// to avoid making this a fatal error, will load as a fisheye
		// if this fails, it just won't draw
		ldscp = new LandscapeFisheye();
	}

	ldscp->load(landscapeIni, landscapeId);
	return ldscp;
}


QString LandscapeMgr::nameToID(const QString& name)
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();

	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
		Q_ASSERT(0);
		return "error";
	}
	else
	{
		return nameToDirMap[name];
	}
}

/****************************************************************************
 get a map of landscape name (from landscape.ini name field) to ID (dir name)
 ****************************************************************************/
QMap<QString,QString> LandscapeMgr::getNameToDirMap() const
{
	QSet<QString> landscapeDirs;
	QMap<QString,QString> result;
	try
	{
		landscapeDirs = StelFileMgr::listContents("landscapes",StelFileMgr::Directory);
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "ERROR while trying list landscapes:" << e.what();
	}

	foreach (const QString& dir, landscapeDirs)
	{
		try
		{
			QSettings landscapeIni(StelFileMgr::findFile("landscapes/" + dir + "/landscape.ini"), StelIniFormat);
			QString k = landscapeIni.value("landscape/name").toString();
			result[k] = dir;
		}
		catch (std::runtime_error& e)
		{
			//qDebug << "WARNING: unable to successfully read landscape.ini file from landscape " << dir;
		}
	}
	return result;
}


QString LandscapeMgr::installLandscapeFromArchive(QString sourceFilePath, bool display, bool toMainDirectory)
{
	Q_UNUSED(toMainDirectory);
	if (!QFile::exists(sourceFilePath))
	{
		qDebug() << "LandscapeMgr: File does not exist:" << sourceFilePath;
		emit errorUnableToOpen(sourceFilePath);
		return QString();
	}

	QDir parentDestinationDir;
	//TODO: Fix the "for all users" option
	parentDestinationDir.setPath(StelFileMgr::getUserDir());

	if (!parentDestinationDir.exists("landscapes"))
	{
		//qDebug() << "LandscapeMgr: No 'landscapes' subdirectory exists in" << parentDestinationDir.absolutePath();
		if (!parentDestinationDir.mkdir("landscapes"))
		{
			qWarning() << "LandscapeMgr: Unable to install landscape: Unable to create sub-directory 'landscapes' in" << parentDestinationDir.absolutePath();
			emit errorUnableToOpen(QDir::cleanPath(parentDestinationDir.filePath("landscapes")));//parentDestinationDir.absolutePath()
			return QString();
		}
	}
	QDir destinationDir (parentDestinationDir.absoluteFilePath("landscapes"));

	KZip sourceArchive(sourceFilePath);
	if(!sourceArchive.open(QIODevice::ReadOnly))
	{
		qWarning() << "LandscapeMgr: Unable to open as a ZIP archive:" << sourceFilePath;
		emit errorNotArchive();
		return QString();
	}

	//Detect top directory
	const KArchiveDirectory * archiveTopDirectory = NULL;
	QStringList topLevelContents = sourceArchive.directory()->entries();
	if(topLevelContents.contains("landscape.ini"))
	{
		//If the landscape archive has no top level directory...
		//(test case is "tulipfield" from the Stellarium Wiki)
		archiveTopDirectory = sourceArchive.directory();
	}
	else
	{
		foreach (QString entryPath, topLevelContents)
		{
			if (sourceArchive.directory()->entry(entryPath)->isDirectory())
			{
				if((dynamic_cast<const KArchiveDirectory*>(sourceArchive.directory()->entry(entryPath)))->entries().contains("landscape.ini"))
				{
					archiveTopDirectory = dynamic_cast<const KArchiveDirectory*>(sourceArchive.directory()->entry(entryPath));
					break;
				}
			}
		}
	}
	if (archiveTopDirectory == NULL)
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. There is no directory that contains a 'landscape.ini' file in the source archive.";
		emit errorNotArchive();
		return QString();
	}

	/*
	qDebug() << "LandscapeMgr: Contents of the source archive:" << endl
			 << "- top level direcotory:" << archiveTopDirectory->name() << endl
			 << "- contents:" << archiveTopDirectory->entries();
	*/

	//Check if the top directory name is unique
	//TODO: Prompt rename? Rename silently?
	/*
	if (destinationDir.exists(archiveTopDirectory->name()))
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. A directory named" << archiveTopDirectory->name() << "already exists in" << destinationDir.absolutePath();
		return QString();
	}
	*/
	//Determine the landscape's identifier
	QString landscapeID = archiveTopDirectory->name();
	if (landscapeID.length() < 2)
	{
		//If the archive has no top level directory (landscapeID is "/"),
		//use the first 65 characters of its file name for an identifier
		QFileInfo sourceFileInfo(sourceFilePath);
		landscapeID = sourceFileInfo.baseName().left(65);
	}

	//Check for duplicate IDs
	if (getAllLandscapeIDs().contains(landscapeID))
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. A landscape with the ID" << landscapeID << "already exists.";
		emit errorNotUnique(landscapeID);
		return QString();
	}

	//Read the .ini file and check if the landscape name is unique
	QTemporaryFile tempLandscapeIni("landscapeXXXXXX.ini");
	if (tempLandscapeIni.open())
	{
		const KZipFileEntry * archLandscapeIni = static_cast<const KZipFileEntry*>(archiveTopDirectory->entry("landscape.ini"));
		tempLandscapeIni.write(archLandscapeIni->createDevice()->readAll());
		tempLandscapeIni.close();

		QSettings confLandscapeIni(tempLandscapeIni.fileName(), StelIniFormat);
		QString landscapeName = confLandscapeIni.value("landscape/name").toString();
		if (getAllLandscapeNames().contains(landscapeName))
		{
			qWarning() << "LandscapeMgr: Unable to install landscape. There is already a landscape named" << landscapeName;
			emit errorNotUnique(landscapeName);
			return QString();
		}
	}

	//Copy the landscape directory to the target
	//sourceArchive.directory()->copyTo(destinationDir.absolutePath());

	//This case already has been handled - and commented out - above. :)
	if(destinationDir.exists(landscapeID))
	{
		qWarning() << "LandscapeMgr: A subdirectory" << landscapeID << "already exists in" << destinationDir.absolutePath() << "Its contents may be overwritten.";
	}
	else if(!destinationDir.mkdir(landscapeID))
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. Unable to create" << landscapeID << "directory in" << destinationDir.absolutePath();
		emit errorUnableToOpen(QDir::cleanPath(destinationDir.filePath(landscapeID)));
		return QString();
	}
	destinationDir.cd(landscapeID);
	QString destinationDirPath = destinationDir.absolutePath();
	QStringList landscapeFileEntries = archiveTopDirectory->entries();
	foreach (QString entry, landscapeFileEntries)
	{
		const KArchiveEntry * archEntry = archiveTopDirectory->entry(entry);
		if(archEntry->isFile())
		{
			static_cast<const KZipFileEntry*>(archEntry)->copyTo(destinationDirPath);
		}
	}

	sourceArchive.close();

	//If necessary, make the new landscape the current landscape
	if (display)
	{
		setCurrentLandscapeID(landscapeID);
	}

	//Make sure that everyone knows that the list of available landscapes has changed
	emit landscapesChanged();

	qDebug() << "LandscapeMgr: Successfully installed landscape directory" << landscapeID << "to" << destinationDir.absolutePath();
	return landscapeID;
}

bool LandscapeMgr::removeLandscape(QString landscapeID)
{
	if (landscapeID.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! No landscape ID passed to removeLandscape().";
		return false;
	}

	if (packagedLandscapeIDs.contains(landscapeID))
	{
		qWarning() << "LandscapeMgr: Landscapes that are part of the default installation cannot be removed.";
		return false;
	}

	qDebug() << "LandscapeMgr: Trying to remove landscape" << landscapeID;

	QString landscapePath = getLandscapePath(landscapeID);
	if (landscapePath.isEmpty())
		return false;

	QDir landscapeDir(landscapePath);
	foreach (QString fileName, landscapeDir.entryList(QDir::Files | QDir::NoDotAndDotDot))
	{
		if(!landscapeDir.remove(fileName))
		{
			qWarning() << "LandscapeMgr: Unable to remove" << fileName;
			emit errorRemoveManually(landscapeDir.absolutePath());
			return false;
		}
	}
	landscapeDir.cdUp();
	if(!landscapeDir.rmdir(landscapeID))
	{
		qWarning() << "LandscapeMgr: Error! Landscape" << landscapeID
				   << "could not be removed. "
				   << "Some files were deleted, but not all."
				   << endl
				   << "LandscapeMgr: You can delete manually" << QDir::cleanPath(landscapeDir.filePath(landscapeID));
		emit errorRemoveManually(QDir::cleanPath(landscapeDir.filePath(landscapeID)));
		return false;
	}

	qDebug() << "LandscapeMgr: Successfully removed" << landscapePath;

	//If the landscape has been selected, revert to the default one
	//TODO: Make this optional?
	if (getCurrentLandscapeID() == landscapeID)
	{
		if(getDefaultLandscapeID() == landscapeID)
		{
			setDefaultLandscapeID(packagedLandscapeIDs.first());
			//TODO: Find what happens if a missing landscape is specified in the configuration file
		}

		setCurrentLandscapeID(getDefaultLandscapeID());
	}

	//Make sure that everyone knows that the list of available landscapes has changed
	emit landscapesChanged();

	return true;
}

QString LandscapeMgr::getLandscapePath(QString landscapeID)
{
	QString result;
	//Is this necessary? This function is private.
	if (landscapeID.isEmpty())
		return result;

	try
	{
		result = StelFileMgr::findFile("landscapes/" + landscapeID, StelFileMgr::Directory);
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "LandscapeMgr: Error! Unable to find" << landscapeID << ":" << e.what();
		return result;
	}

	return result;
}

QString LandscapeMgr::loadLandscapeName(QString landscapeID)
{
	QString landscapeName;
	if (landscapeID.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! No landscape ID passed to loadLandscapeName().";
		return landscapeName;
	}

	QString landscapePath = getLandscapePath(landscapeID);
	if (landscapePath.isEmpty())
		return landscapeName;

	QDir landscapeDir(landscapePath);
	if (landscapeDir.exists("landscape.ini"))
	{
		QString landscapeSettingsPath = landscapeDir.filePath("landscape.ini");
		QSettings landscapeSettings(landscapeSettingsPath, StelIniFormat);
		landscapeName = landscapeSettings.value("landscape/name").toString();
	}
	else
	{
		qWarning() << "LandscapeMgr: Error! Landscape directory" << landscapePath << "does not contain a 'landscape.ini' file";
	}

	return landscapeName;
}

quint64 LandscapeMgr::loadLandscapeSize(QString landscapeID)
{
	quint64 landscapeSize = 0;
	if (landscapeID.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! No landscape ID passed to loadLandscapeSize().";
		return landscapeSize;
	}

	QString landscapePath = getLandscapePath(landscapeID);
	if (landscapePath.isEmpty())
		return landscapeSize;

	QDir landscapeDir(landscapePath);
	foreach (QFileInfo file, landscapeDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
	{
		//qDebug() << "name:" << file.baseName() << "size:" << file.size();
		landscapeSize += file.size();
	}

	return landscapeSize;
}

QString LandscapeMgr::getDescription() const
{
        QString lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
        if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
        {
                lang = lang.split("_").at(0);
        }
	QString descriptionFile = StelFileMgr::findFile("landscapes/" + getCurrentLandscapeID(), StelFileMgr::Directory) + "/description." + lang + ".utf8";
	QString desc;

	if(QFileInfo(descriptionFile).exists())
	{
		QFile file(descriptionFile);
		file.open(QIODevice::ReadOnly | QIODevice::Text);
		QTextStream in(&file);
		in.setCodec("UTF-8");
		desc = in.readAll();
		file.close();
	}
	else
	{
		desc = QString("<h2>%1</h2>").arg(q_(landscape->getName()));
		desc += landscape->getDescription();
	}

	return desc;
}
